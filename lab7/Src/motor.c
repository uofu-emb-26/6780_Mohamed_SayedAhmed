#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stm32f0xx.h>
#include <SEGGER_RTT.h>
#include "motor.h"

/* -------------------------------------------------------------------------------------------------------------
 *  Global Variable and Type Declarations
 *  ------------------------------------------------------------------------------------------------------------- */
volatile int16_t error_integral;    // Integrated error signal
volatile uint8_t duty_cycle;        // Output PWM duty cycle
volatile int16_t target_rpm;        // Desired speed target in RPM
volatile int16_t motor_speed;       // Measured motor speed in encoder counts/sample
volatile uint16_t adc_value;        // ADC measured motor current
volatile int16_t error;             // Speed error signal
volatile uint8_t Kp;                // Proportional gain
volatile uint8_t Ki;                // Integral gain

static uint8_t buf0[1024];
static uint8_t buf1[1024];
static uint8_t buf2[1024];

union byte_split {
    uint32_t uword;
    int32_t word;
    uint8_t bytes[4];
};

void log_init(void) {
    SEGGER_RTT_ConfigUpBuffer(0, "", buf0, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(1, "", buf1, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(2, "", buf2, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void log_data(void) {
    __disable_irq();
    uint32_t duty_cycle_copy = duty_cycle;
    int32_t target_rpm_copy = target_rpm;
    int32_t motor_speed_copy = motor_speed;
    __enable_irq();

    union byte_split data;
    data.uword = duty_cycle_copy;
    SEGGER_RTT_Write(0, &data.bytes, 4);

    data.word = target_rpm_copy;
    SEGGER_RTT_Write(1, &data.bytes, 4);

    data.word = motor_speed_copy;
    SEGGER_RTT_Write(2, &data.bytes, 4);
}

// Sets up the entire motor drive system
void motor_init(void) {
    log_init();
    pwm_init();
    encoder_init();
    ADC_init();

    error_integral = 0;
    duty_cycle = 0;
    target_rpm = 0;
    motor_speed = 0;
    adc_value = 0;
    error = 0;

    // Starting gains; tune later if needed
    Kp = 8;
    Ki = 2;
}

// Sets up the PWM and direction signals to drive the H-Bridge
void pwm_init(void) {
    // Enable GPIOA clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // Set up pin PA4 for H-bridge PWM output (TIMER 14 CH1)
    GPIOA->MODER |= (1 << 9);
    GPIOA->MODER &= ~(1 << 8);

    // Set PA4 to AF4
    GPIOA->AFR[0] &= 0xFFF0FFFF;
    GPIOA->AFR[0] |= (1 << 18);

    // Set up PA5, PA6 as GPIO output pins for motor direction control
    GPIOA->MODER &= 0xFFFFC3FF;
    GPIOA->MODER |= (1 << 10) | (1 << 12);

    // Initialize one direction pin high, the other low
    GPIOA->ODR |= (1 << 5);
    GPIOA->ODR &= ~(1 << 6);

    // Set up PWM timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    TIM14->CR1 = 0;
    TIM14->CCMR1 = 0;
    TIM14->CCER = 0;

    // Set output-compare CH1 to PWM1 mode and enable CCR1 preload buffer
    TIM14->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE);
    TIM14->CCER |= TIM_CCER_CC1E;

    TIM14->PSC = 1;      // Run timer at 24 MHz
    TIM14->ARR = 1200;   // PWM at 20 kHz
    TIM14->CCR1 = 0;     // Start PWM at 0% duty cycle

    TIM14->CR1 |= TIM_CR1_CEN;
}

// Set the duty cycle of the PWM, accepts (0-100)
void pwm_setDutyCycle(uint8_t duty) {
    if (duty <= 100) {
        TIM14->CCR1 = ((uint32_t)duty * TIM14->ARR) / 100;
    }
}

// Sets up encoder interface to read motor speed
void encoder_init(void) {
    // Enable GPIOB clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set up encoder input pins PB4/PB5 (TIM3 CH1 / CH2)
    GPIOB->MODER &= ~(GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0);
    GPIOB->MODER |=  (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);
    GPIOB->AFR[0] |= ((1 << 16) | (1 << 20));

    // Set up encoder interface (TIM3 encoder input mode)
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CCMR1 = 0;
    TIM3->CCER = 0;
    TIM3->SMCR = 0;
    TIM3->CR1 = 0;

    TIM3->CCMR1 |= (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);
    TIM3->SMCR  |= (TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0);
    TIM3->ARR = 0xFFFF;
    TIM3->CNT = 0x7FFF;
    TIM3->CR1 |= TIM_CR1_CEN;

    // Configure TIM6 to periodically update speed and PI control
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 11;
    TIM6->ARR = 30000;

    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
}

// Encoder interrupt to calculate motor speed, also manages PI controller
void TIM6_DAC_IRQHandler(void) {
    motor_speed = (TIM3->CNT - 0x7FFF);
    TIM3->CNT = 0x7FFF;

    PI_update();
    log_data();

    TIM6->SR &= ~TIM_SR_UIF;
}

void ADC_init(void) {
    // Enable GPIOA clock already done in pwm_init, but harmless if repeated
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // Configure PA1 for ADC input
    GPIOA->MODER |= (GPIO_MODER_MODER1_0 | GPIO_MODER_MODER1_1);

    // Configure ADC
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    ADC1->CFGR1 = 0;                       // 12-bit resolution by default
    ADC1->CFGR1 |= ADC_CFGR1_CONT;         // Continuous mode
    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;     // Channel 1

    ADC1->CR = 0;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

    ADC1->CR |= ADC_CR_ADSTART;
}

/* Run PI control loop */
void PI_update(void) {
    __disable_irq();

    int16_t target_counts = (target_rpm * 4) / 5;

    error = target_counts - motor_speed;

    error_integral += (Ki * error);

    if (error_integral < 0) {
        error_integral = 0;
    }
    if (error_integral > 3200) {
        error_integral = 3200;
    }

    int16_t output = (Kp * error) + error_integral;

    output = output >> 5;

    if (output < 0) {
        output = 0;
    }
    if (output > 100) {
        output = 100;
    }

    if (target_rpm > 0 && output < 45) {
        output = 45;
    }

    pwm_setDutyCycle(output);
    duty_cycle = output;

    if (ADC1->ISR & ADC_ISR_EOC) {
        adc_value = ADC1->DR;
    }

    __enable_irq();
}