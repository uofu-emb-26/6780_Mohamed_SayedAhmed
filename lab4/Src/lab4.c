#include "stm32f0xx.h"
#include <stdint.h>

/* -------------------- LED CONTROL (EDIT THESE) --------------------
   Replace these with YOUR LED GPIO pins / functions from earlier labs.
   Quick example placeholders below assume:
   - Red   = PC6
   - Green = PC7
   - Blue  = PC8
   If your LEDs differ, swap the pins.
-------------------------------------------------------------------*/
static void leds_init(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

  // PC6/PC7/PC8/PC9 output
  GPIOC->MODER &= ~((3u<<(6*2))|(3u<<(7*2))|(3u<<(8*2))|(3u<<(9*2)));
  GPIOC->MODER |=  ((1u<<(6*2))|(1u<<(7*2))|(1u<<(8*2))|(1u<<(9*2)));
}

static inline void led_red_on(void)    { GPIOC->BSRR = (1u<<6); }
static inline void led_red_off(void)   { GPIOC->BRR  = (1u<<6); }
static inline void led_red_toggle(void){ GPIOC->ODR ^= (1u<<6); }

static inline void led_green_on(void)    { GPIOC->BSRR = (1u<<7); }
static inline void led_green_off(void)   { GPIOC->BRR  = (1u<<7); }
static inline void led_green_toggle(void){ GPIOC->ODR ^= (1u<<7); }

static inline void led_blue_on(void)    { GPIOC->BSRR = (1u<<8); }
static inline void led_blue_off(void)   { GPIOC->BRR  = (1u<<8); }
static inline void led_blue_toggle(void){ GPIOC->ODR ^= (1u<<8); }

/* -------------------- USART3 (PB10/PB11 AF4) -------------------- */
#define UART_BAUD 115200u

static void usart3_gpio_init_pb10_pb11(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

  // PB10, PB11 -> Alternate Function mode (10)
  GPIOB->MODER &= ~((3u<<(10*2)) | (3u<<(11*2)));
  GPIOB->MODER |=  ((2u<<(10*2)) | (2u<<(11*2)));

  // Optional: push-pull default OK; set speed medium/high if you want
  // Pull-up on RX can help if line floats
  GPIOB->PUPDR &= ~((3u<<(10*2)) | (3u<<(11*2)));
  GPIOB->PUPDR |=  (1u<<(11*2)); // PB11 RX pull-up

  // AF4 for PB10/PB11 => AFRH bits (pins 8..15)
  // AFRH index: pin-8, so PB10 => (10-8)=2, PB11 =>3
  GPIOB->AFR[1] &= ~((0xFu<<(2*4)) | (0xFu<<(3*4)));
  GPIOB->AFR[1] |=  ((4u<<(2*4)) | (4u<<(3*4)));
}

static void usart3_init_115200(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

  USART3->CR1 &= ~USART_CR1_UE;

  SystemCoreClockUpdate();                 
  uint32_t fck = SystemCoreClock;          
  USART3->BRR = (uint16_t)(fck / UART_BAUD);

  USART3->CR1 |= USART_CR1_TE | USART_CR1_RE;
  USART3->CR1 |= USART_CR1_UE;
}

/* -------------------- Blocking TX -------------------- */
static void uart_write_char(char c)
{
  while (!(USART3->ISR & USART_ISR_TXE)) { }
  USART3->TDR = (uint8_t)c;
}

static void uart_write_str(const char *s)
{
  while (*s) uart_write_char(*s++);
}

/* -------------------- Blocking RX (polling) -------------------- */
static char uart_read_char_blocking(void)
{
  while (!(USART3->ISR & USART_ISR_RXNE)) { }
  return (char)(USART3->RDR & 0xFF);
}

/* -------------------- IRQ RX -------------------- */
static volatile uint8_t g_rx_ready = 0;
static volatile char    g_rx_char  = 0;

void USART3_4_IRQHandler(void)
{
  if (USART3->ISR & USART_ISR_RXNE)
  {
    g_rx_char  = (char)(USART3->RDR & 0xFF); // reading clears RXNE
    g_rx_ready = 1;
    GPIOC->ODR ^= (1u<<9);   // toggle GREEN LED for debug
  }
}

static void uart_enable_rx_irq(void)
{
  g_rx_ready = 0;
  USART3->CR1 |= USART_CR1_RXNEIE;
  NVIC_SetPriority(USART3_4_IRQn, 1);
  NVIC_EnableIRQ(USART3_4_IRQn);
}

/* -------------------- Helpers: apply command -------------------- */
/* cmd format: <led><action>
   led: r/g/b
   action: '0'=off, '1'=on, '2'=toggle
*/
static int apply_cmd(char led, char action)
{
  switch (led)
  {
    case 'r':
      if (action=='0') { led_red_off(); }
      else if (action=='1') { led_red_on(); }
      else if (action=='2') { led_red_toggle(); }
      else return 0;
      return 1;

    case 'g':
      if (action=='0') { led_green_off(); }
      else if (action=='1') { led_green_on(); }
      else if (action=='2') { led_green_toggle(); }
      else return 0;
      return 1;

    case 'b':
      if (action=='0') { led_blue_off(); }
      else if (action=='1') { led_blue_on(); }
      else if (action=='2') { led_blue_toggle(); }
      else return 0;
      return 1;

    default:
      return 0;
  }
}

/* -------------------- MAIN (choose mode) -------------------- */
int main(void)
{
  SystemCoreClockUpdate();

  leds_init();
  GPIOC->ODR ^= (1u<<9);
for (volatile uint32_t d=0; d<300000; d++);
GPIOC->ODR ^= (1u<<9);
  usart3_gpio_init_pb10_pb11();
  usart3_init_115200();

  uart_write_str("\r\nLab4 UART ready @115200\r\n");

  // ---- PART A: polling demo (single-char toggle) ----
  // Uncomment if your lab asks for this first.
  /*
  uart_write_str("Polling mode: type r/g/b to toggle. Any other -> ERR\r\n");
  while (1) {
    char c = uart_read_char_blocking();
    if      (c=='r') { led_red_toggle();   uart_write_str("Toggled R\r\n"); }
    else if (c=='g') { led_green_toggle(); uart_write_str("Toggled G\r\n"); }
    else if (c=='b') { led_blue_toggle();  uart_write_str("Toggled B\r\n"); }
    else             { uart_write_str("ERR\r\n"); }
  }
  */

    // ---- PART B: IRQ mode + 2-char command parser ----
  uart_enable_rx_irq();

  uart_write_char('A');  // optional: loopback single byte
  uart_write_str("IRQ mode: CMD? <led><0/1/2>  (ex: r2, g1, b0)\r\n");
  uart_write_str("CMD? ");

  char led = 0;
  uint8_t state = 0; // 0=expect led, 1=expect action

  while (1)
  {
        // MAIN LOOP HEARTBEAT (PC8): proves main() is alive
    for (volatile uint32_t d=0; d<2000000; d++) { }   // delay
    GPIOC->ODR ^= (1u<<8);   // toggle PC8
    
    
    static uint32_t t = 0;
    if (++t >= 5) {   // slow it down a lot
      t = 0;
      uart_write_char('A');
    }
    

    if (!g_rx_ready) continue;
    g_rx_ready = 0;

    char c = g_rx_char;
    if (c == '\r' || c == '\n') continue;

    if (state == 0)
    {
      if (c=='r' || c=='g' || c=='b') { led = c; state = 1; }
      else { uart_write_str("\r\nERR: LED must be r/g/b\r\nCMD? "); }
    }
    else
    {
      if (c=='0' || c=='1' || c=='2')
      {
        if (apply_cmd(led, c)) {
          uart_write_str("\r\nOK: ");
          uart_write_char(led);
          uart_write_char(c);
          uart_write_str("\r\nCMD? ");
        } else {
          uart_write_str("\r\nERR\r\nCMD? ");
        }
      }
      else { uart_write_str("\r\nERR: action must be 0/1/2\r\nCMD? "); }
      state = 0;
    }
  }
}