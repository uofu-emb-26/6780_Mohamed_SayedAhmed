#include <stdio.h>
#include <stdlib.h>
#include "stm32f0xx.h"
#include "motor.h"
#include "SEGGER_RTT.h"

extern volatile uint8_t demo_state;
volatile uint8_t last_button_state = 0;

/* ------------------------------------------------------------------------------------------------------------- */
void LED_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
    GPIOC->OSPEEDR &= ~((GPIO_OSPEEDR_OSPEEDR8) | (GPIO_OSPEEDR_OSPEEDR9));
    GPIOC->PUPDR &= ~((GPIO_PUPDR_PUPDR8) | (GPIO_PUPDR_PUPDR9));

    GPIOC->ODR &= ~(GPIO_ODR_8 | GPIO_ODR_9);
}

/* ------------------------------------------------------------------------------------------------------------- */
void button_init(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // PA0 input
    GPIOA->MODER &= ~GPIO_MODER_MODER0;

    // NO pull-up, NO pull-down
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0;
}

/* ------------------------------------------------------------------------------------------------------------- */
void Lab7_Systick_Callback(void) {
    
    static uint8_t last_button_state = 0;
    uint8_t current_button_state = (GPIOA->IDR & (1 << 0)) ? 1 : 0;

    if (current_button_state && !last_button_state) {
      GPIOC->ODR ^= GPIO_ODR_8;
#if CHECKOFF_5_FINAL_DEMO
        demo_state++;
        if (demo_state > 3) demo_state = 0;
        set_target_from_demo_state();
#endif
    }

    last_button_state = current_button_state;
}

/* ------------------------------------------------------------------------------------------------------------- */
int main(void) {

    HAL_Init();
    LED_init();
    button_init();
    motor_init();
motor_init();

#if CHECKOFF_5_FINAL_DEMO
    set_target_from_demo_state();   // starts at demo_state = 0 -> target_rpm = 0
#endif


    while (1) {
    if (GPIOA->IDR & (1 << 0)) {
        GPIOC->ODR |= GPIO_ODR_8;   // orange ON
    } else {
        GPIOC->ODR &= ~GPIO_ODR_8;  // orange OFF
    }
}
}