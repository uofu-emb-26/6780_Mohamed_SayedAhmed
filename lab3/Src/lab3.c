#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx.h"

/* ---------- TEST: PC6/PC7 as normal GPIO outputs ---------- */

static void gpio_init_pc6_pc7_af_tim3(void);
/* ---------- GPIO init for PC8/PC9 outputs (blink) ---------- */
static void gpio_init_pc8_pc9(void)
{
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

  // PC8, PC9 output mode (01)
  GPIOC->MODER &= ~((3u<<(8*2)) | (3u<<(9*2)));
  GPIOC->MODER |=  ((1u<<(8*2)) | (1u<<(9*2)));

  // Start with one LED ON for the alternating pattern :contentReference[oaicite:4]{index=4}
  GPIOC->ODR |=  (1u<<9);   // green ON
  GPIOC->ODR &= ~(1u<<8);   // orange OFF
}

/* ---------- TIM2: 4 Hz update interrupt ---------- */
static void tim2_init_4hz_irq(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // Example approach: make timer tick = 1 kHz using PSC=7999 (8MHz/8000 = 1kHz) :contentReference[oaicite:5]{index=5}
  TIM2->PSC = 7999;
  // 4 Hz => period 0.25s = 250 ms => ARR = 250 ticks at 1ms/tick
  TIM2->ARR = 250;

  TIM2->EGR = TIM_EGR_UG;            // load PSC/ARR
  TIM2->DIER |= TIM_DIER_UIE;        // enable update interrupt :contentReference[oaicite:6]{index=6}

  NVIC_EnableIRQ(TIM2_IRQn);         // enable in NVIC :contentReference[oaicite:7]{index=7}

  TIM2->CR1 |= TIM_CR1_CEN;          // start timer :contentReference[oaicite:8]{index=8}
}

/* ---------- TIM2 IRQ Handler: toggle PC8/PC9 ---------- */
void TIM2_IRQHandler(void)
{
  if (TIM2->SR & TIM_SR_UIF)
  {
    TIM2->SR &= ~TIM_SR_UIF; // clear pending flag :contentReference[oaicite:9]{index=9}

    // Toggle between PC8 and PC9 :contentReference[oaicite:10]{index=10}
    GPIOC->ODR ^= (1u<<8);
    GPIOC->ODR ^= (1u<<9);
  }
}

/* ---------- TIM3 PWM: 800 Hz on CH1 & CH2, 20% duty ---------- */
static void tim3_init_pwm_800hz(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  TIM3->CR1 |= TIM_CR1_ARPE;
  // Choose a reasonable prescaler (not too granular) :contentReference[oaicite:11]{index=11}
  // Example: 8MHz/(PSC+1) = 1MHz => PSC = 7
  TIM3->PSC = 7;

  // 800 Hz => period = 1/800 s = 1.25 ms
  // At 1 MHz tick -> ARR = 1250
  TIM3->ARR = 1250;

  // 20% duty cycle => CCR = 0.2 * ARR :contentReference[oaicite:12]{index=12}
  TIM3->CCR1 = 1000;     // ~4%  (very dim)
  TIM3->CCR2 = 50;   // ~80% (bright)

  // CCMR1: CH1 output, PWM mode 2; CH2 output, PWM mode 1; preload enable :contentReference[oaicite:13]{index=13}
  TIM3->CCMR1 = 0;
  TIM3->CCMR1 |= (7u << 4);    // OC1M = 111 (PWM mode 2)
  TIM3->CCMR1 |= (1u << 3);    // OC1PE preload enable
  TIM3->CCMR1 |= (6u << 12);   // OC2M = 110 (PWM mode 1)
  TIM3->CCMR1 |= (1u << 11);   // OC2PE preload enable

  // CCER: enable CH1 & CH2 outputs :contentReference[oaicite:14]{index=14}
  TIM3->CCER |= (1u << 0);     // CC1E
  TIM3->CCER |= (1u << 4);     // CC2E

  TIM3->EGR = TIM_EGR_UG;      // update registers

  // Start TIM3 (lab says “do not use interrupts”; starting is fine)
  TIM3->CR1 |= TIM_CR1_CEN;
}

int main(void)
{
  HAL_Init();

  gpio_init_pc8_pc9();
  tim2_init_4hz_irq();

  gpio_init_pc6_pc7_af_tim3();  // AF mode for PWM pins
  tim3_init_pwm_800hz();         // start PWM

  while (1)
  {
    // nothing needed
  }
}
  static void gpio_init_pc6_pc7_af_tim3(void)
  {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // PC6, PC7 = Alternate Function mode (10)
    GPIOC->MODER &= ~((3u<<(6*2)) | (3u<<(7*2)));
    GPIOC->MODER |=  ((2u<<(6*2)) | (2u<<(7*2)));

    // Optional: high speed helps edges look cleaner on scope
    GPIOC->OSPEEDR |= ((3u<<(6*2)) | (3u<<(7*2)));

    // Select AF# for TIM3 on PC6/PC7:
    // NOTE: AF number depends on datasheet table (you must choose the one that matches TIM3_CH1 / TIM3_CH2 for PC6/PC7).
    // Commonly AF1 for TIM3 on many pins, but VERIFY from your STM32F072 datasheet tables as lab instructs. :contentReference[oaicite:11]{index=11}

    const uint32_t AF_TIM3 = 0u; // 

    GPIOC->AFR[0] &= ~((0xFu<<(6*4)) | (0xFu<<(7*4)));
    GPIOC->AFR[0] |=  ((AF_TIM3<<(6*4)) | (AF_TIM3<<(7*4)));
  }