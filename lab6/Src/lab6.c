#include "stm32f0xx.h"
#include <stdint.h>

#define EXERCISE 2   

static void delay(volatile uint32_t t)
{
    while (t--) __NOP();
}

#if EXERCISE == 1



static void leds_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOC->MODER &= ~((3u << (6 * 2)) |
                      (3u << (7 * 2)) |
                      (3u << (8 * 2)) |
                      (3u << (9 * 2)));

    GPIOC->MODER |=  ((1u << (6 * 2)) |
                      (1u << (7 * 2)) |
                      (1u << (8 * 2)) |
                      (1u << (9 * 2)));

    /* active-low LEDs: 1 = off, 0 = on */
    GPIOC->ODR |= (1u << 6) | (1u << 7) | (1u << 8) | (1u << 9);
}

static inline void led_red_on(void)    { GPIOC->ODR &= ~(1u << 6); }
static inline void led_red_off(void)   { GPIOC->ODR |=  (1u << 6); }

static inline void led_blue_on(void)   { GPIOC->ODR &= ~(1u << 7); }
static inline void led_blue_off(void)  { GPIOC->ODR |=  (1u << 7); }

static inline void led_orange_on(void) { GPIOC->ODR &= ~(1u << 8); }
static inline void led_orange_off(void){ GPIOC->ODR |=  (1u << 8); }

static inline void led_green_on(void)  { GPIOC->ODR &= ~(1u << 9); }
static inline void led_green_off(void) { GPIOC->ODR |=  (1u << 9); }

static void all_leds_off(void)
{
    led_red_off();
    led_blue_off();
    led_orange_off();
    led_green_off();
}

static void adc_init_pc0(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* PC0 = analog mode, no pull-up/pull-down */
    GPIOC->MODER &= ~(3u << (0 * 2));
    GPIOC->MODER |=  (3u << (0 * 2));
    GPIOC->PUPDR &= ~(3u << (0 * 2));

    
    if (ADC1->CR & ADC_CR_ADEN)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) { }
    }

    ADC1->CFGR1 = 0;
    ADC1->CFGR1 |= ADC_CFGR1_CONT;     /* continuous mode */
    ADC1->CFGR1 |= ADC_CFGR1_RES_0;    /* 8-bit resolution */

    ADC1->CHSELR = ADC_CHSELR_CHSEL10; /* PC0 = ADC_IN10 */

    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }

    ADC1->CR |= ADC_CR_ADSTART;
}

static uint8_t adc_read_8bit(void)
{
    return (uint8_t)(ADC1->DR & 0xFF);
}

int main(void)
{
    uint8_t adc_val;

    leds_init();
    adc_init_pc0();

    while (1)
    {
        adc_val = adc_read_8bit();
        all_leds_off();

        
        if (adc_val < 64) {
            led_red_on();
        } else if (adc_val < 128) {
            led_blue_on();
        } else if (adc_val < 192) {
            led_orange_on();
        } else {
            led_green_on();
        }

        delay(20000);
    }
}

#elif EXERCISE == 2



const uint8_t triangle_table[32] = {
    0,15,31,47,63,79,95,111,
    127,142,158,174,190,206,222,238,
    254,238,222,206,190,174,158,142,
    127,111,95,79,63,47,31,15
};

static void dac_init_pa4(void)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* PA4 -> analog mode */
    GPIOA->MODER &= ~(3u << (4 * 2));
    GPIOA->MODER |=  (3u << (4 * 2));

    /* Enable DAC clock */
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    /* Enable DAC channel 1 */
    DAC->CR |= DAC_CR_EN1;
}

int main(void)
{
    uint32_t i = 0;

    dac_init_pa4();

    while (1)
    {
        /* 8-bit right-aligned DAC data */
        DAC->DHR8R1 = triangle_table[i];

        i++;
        if (i >= 32) {
            i = 0;
        }

        /* about 1 ms delay */
        delay(48000);
    }
}

#endif