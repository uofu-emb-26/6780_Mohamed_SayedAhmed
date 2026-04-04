#include <stdio.h>
#include <stdlib.h>
#include "stm32f0xx.h"
#include "motor.h"


/* -------------------------------------------------------------------------------------------------------------
 *  Global Variables
 * ------------------------------------------------------------------------------------------------------------- */
volatile uint32_t debouncer;
volatile uint8_t target_state = 0;

/* -------------------------------------------------------------------------------------------------------------
 *  LED Initialization
 * ------------------------------------------------------------------------------------------------------------- */
void LED_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
    GPIOC->OSPEEDR &= ~((GPIO_OSPEEDR_OSPEEDR8) | (GPIO_OSPEEDR_OSPEEDR9));
    GPIOC->PUPDR &= ~((GPIO_PUPDR_PUPDR8) | (GPIO_PUPDR_PUPDR9));

    GPIOC->ODR &= ~(GPIO_ODR_8 | GPIO_ODR_9);
}

/* -------------------------------------------------------------------------------------------------------------
 *  Button Initialization (FIXED)
 * ------------------------------------------------------------------------------------------------------------- */
void button_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODER0_0 | GPIO_MODER_MODER0_1);
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEEDR0_0 | GPIO_OSPEEDR_OSPEEDR0_1);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0_0 | GPIO_PUPDR_PUPDR0_1);
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1;   // Pull-down
}

/* -------------------------------------------------------------------------------------------------------------
 *  SysTick Callback (Button Debounce + Target Switching)
 * ------------------------------------------------------------------------------------------------------------- */
void Lab7_Systick_Callback(void) {
    static uint8_t prev_pressed = 0;
    uint8_t pressed = (GPIOA->IDR & (1 << 0)) ? 1 : 0;

    // Detect rising edge only
    if (pressed && !prev_pressed) {
        __disable_irq();

        GPIOC->ODR ^= GPIO_ODR_8;   // debug LED toggle so you can SEE button press

        switch (target_state) {
            case 0:
                target_rpm = 80;
                target_state = 1;
                break;

            case 1:
                target_rpm = 50;
                target_state = 2;
                break;

            case 2:
                target_rpm = 80;
                target_state = 3;
                break;

            default:
                target_rpm = 0;
                target_state = 0;
                break;
        }

        __enable_irq();
    }

    prev_pressed = pressed;
}

/* -------------------------------------------------------------------------------------------------------------
 *  Main Program
 * ------------------------------------------------------------------------------------------------------------- */
int main(void) {

    debouncer = 0;

    HAL_Init();

    LED_init();
    button_init();
    motor_init();

    while (1) {
        GPIOC->ODR ^= GPIO_ODR_9;   // Heartbeat LED
        HAL_Delay(128);
    }
}