#include "stm32f0xx.h"
#include <stdint.h>


static void leds_init(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

  
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

static inline void led_orange_on(void)    { GPIOC->BSRR = (1u<<9); }
static inline void led_orange_off(void)   { GPIOC->BRR  = (1u<<9); }
static inline void led_orange_toggle(void){ GPIOC->ODR ^= (1u<<9); }

#define UART_BAUD 115200u

static void usart3_gpio_init_pb10_pb11(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

  
  GPIOB->MODER &= ~((3u<<(10*2)) | (3u<<(11*2)));
  GPIOB->MODER |=  ((2u<<(10*2)) | (2u<<(11*2)));

  
  GPIOB->PUPDR &= ~((3u<<(10*2)) | (3u<<(11*2)));
  GPIOB->PUPDR |=  (1u<<(11*2)); // 

  
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
    
  }
}

static void uart_enable_rx_irq(void)
{
  g_rx_ready = 0;
  USART3->CR1 |= USART_CR1_RXNEIE;
  NVIC_SetPriority(USART3_4_IRQn, 1);
  NVIC_EnableIRQ(USART3_4_IRQn);
}


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
    
    case 'o':
      if (action=='0') { led_orange_off(); }
      else if (action=='1') { led_orange_on(); }
      else if (action=='2') { led_orange_toggle(); }
      else return 0;
      return 1;

    default:
      return 0;
  }
}


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
  uart_enable_rx_irq();
uart_write_str("IRQ mode:\r\n");
uart_write_str("  Single-letter: r/g/b/o then Enter = toggle\r\n");
uart_write_str("  Two-char: <led><0/1/2> (0=off,1=on,2=toggle) e.g., r2 g1 b0\r\n");
uart_write_str("CMD? ");

    char pending_led = 0;
  uint8_t state = 0; // 0 = idle, 1 = have pending LED

  while (1)
  {
    if (!g_rx_ready) continue;
    g_rx_ready = 0;

    char c = g_rx_char;

    // ignore line endings
    if (c == '\r' || c == '\n')
    {
      // If user typed only a single LED letter then hit Enter => toggle it.
      if (state == 1 && pending_led)
      {
        apply_cmd(pending_led, '2');  // single-letter means toggle
        uart_write_str("\r\nOK: ");
        uart_write_char(pending_led);
        uart_write_str(" (toggle)\r\nCMD? ");
        pending_led = 0;
        state = 0;
      }
      continue;
    }

    if (state == 0)
    {
      if (c=='r' || c=='g' || c=='b' || c=='o')
      {
        pending_led = c;
        state = 1;
      }
      else
      {
        uart_write_str("\r\nERR: LED must be r/g/b/o\r\nCMD? ");
      }
    }
    else // state == 1, we have pending_led
    {
      if (c=='0' || c=='1' || c=='2')
      {
        if (apply_cmd(pending_led, c))
        {
          uart_write_str("\r\nOK: ");
          uart_write_char(pending_led);
          uart_write_char(c);
          uart_write_str("\r\nCMD? ");
        }
        else
        {
          uart_write_str("\r\nERR\r\nCMD? ");
        }
        pending_led = 0;
        state = 0;
      }
      else
      {
        // Not an action => interpret pending_led as single-letter toggle,
        // then "replay" this char as a fresh start.
        apply_cmd(pending_led, '2');  // toggle
        uart_write_str("\r\nOK: ");
        uart_write_char(pending_led);
        uart_write_str(" (toggle)\r\n");

        pending_led = 0;
        state = 0;

        // replay current char
        if (c=='r' || c=='g' || c=='b' || c=='o')
        {
          pending_led = c;
          state = 1;
        }
        else
        {
          uart_write_str("ERR: action must be 0/1/2 OR LED r/g/b/o\r\nCMD? ");
        }
      }
    }
  }
}