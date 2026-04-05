#include "stm32f0xx.h"
#include <stdint.h>
#include "SEGGER_RTT.h"
#include "motor.h"

/* ------------------------------------------------------------------------------------------------------------- */
volatile int16_t error_integral;
volatile uint8_t duty_cycle;
volatile int16_t target_rpm;
volatile int16_t motor_speed;
volatile int8_t adc_value;
volatile int16_t error;
volatile uint8_t Kp;
volatile uint8_t Ki;

volatile uint8_t demo_state = 0;
volatile uint32_t open_loop_divider = 0;

static uint8_t buf0[1024];
static uint8_t buf1[1024];
static uint8_t buf2[1024];

union byte_split {
    uint32_t uword;
    int32_t word;
    uint8_t bytes[4];
};

/* ------------------------------------------------------------------------------------------------------------- */
// RTT logging (needed for checkoff 2+)
void log_init(void) {
    SEGGER_RTT_ConfigUpBuffer(0, "", buf0, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(1, "", buf1, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_ConfigUpBuffer(2, "", buf2, 1024, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void log_data(void) {
    union byte_split data;

    data.word = motor_speed;
    SEGGER_RTT_Write(0, &data.bytes, 4);

    data.word = target_rpm;
    SEGGER_RTT_Write(1, &data.bytes, 4);

    data.word = duty_cycle;
    SEGGER_RTT_Write(2, &data.bytes, 4);
}

/* ------------------------------------------------------------------------------------------------------------- */
void motor_init(void) {

    log_init();        //  needed for checkoff 2
    pwm_init();
    encoder_init();
    ADC_init();

    error_integral = 0;
    duty_cycle     = 0;
    motor_speed    = 0;

    Kp = 8;
    Ki = 2;

   #if CHECKOFF_1_HBRIDGE_MANUAL
    target_rpm = 0;
#elif CHECKOFF_2_ENCODER_ONLY
    target_rpm = 0;
#elif CHECKOFF_3_OPEN_LOOP_PWM
    target_rpm = 0;
#elif CHECKOFF_4_PI_SINGLE_SPEED
    target_rpm = 80;
#elif CHECKOFF_5_FINAL_DEMO
    target_rpm = 0;
#else
    target_rpm = 0;
#endif

    pwm_setDutyCycle(80);   // strong start
}

/* ------------------------------------------------------------------------------------------------------------- */
void pwm_init(void) {

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // PA4 → AF
    GPIOA->MODER |= (1 << 9);
    GPIOA->MODER &= ~(1 << 8);
    GPIOA->AFR[0] |= (1 << 18);

    // PA5, PA6 output
    GPIOA->MODER |= (1 << 10) | (1 << 12);

    GPIOA->ODR |= (1 << 5);
    GPIOA->ODR &= ~(1 << 6);

    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;

    TIM14->CCMR1 |= (6 << 4);
    TIM14->CCER  |= 1;

    TIM14->PSC = 1;
    TIM14->ARR = 1200;
    TIM14->CCR1 = 1000;

    TIM14->CR1 |= 1;
}

void pwm_setDutyCycle(uint8_t duty) {
    TIM14->CCR1 = ((uint32_t)duty * 1200) / 100;
}

/* ------------------------------------------------------------------------------------------------------------- */
void encoder_init(void) {

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    GPIOB->MODER |= (2 << 8) | (2 << 10);
    GPIOB->AFR[0] |= (1 << 16) | (1 << 20);

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->SMCR |= 3;
    TIM3->ARR = 0xFFFF;
    TIM3->CNT = 0x7FFF;
    TIM3->CR1 |= 1;

    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC = 11;
    TIM6->ARR = 30000;
    TIM6->DIER |= 1;
    TIM6->CR1 |= 1;

    NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/* ------------------------------------------------------------------------------------------------------------- */
void ADC_init(void) {

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= (3 << 2);

    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    ADC1->CR |= ADC_CR_ADEN;
}

/* ------------------------------------------------------------------------------------------------------------- */
void set_target_from_demo_state(void) {
    switch(demo_state) {
        case 0: target_rpm = 0; break;
        case 1: target_rpm = 80; break;
        case 2: target_rpm = 50; break;
        case 3: target_rpm = 80; break;
        default:
            target_rpm = 0;
            demo_state = 0;
            break;
    }
}

/* ------------------------------------------------------------------------------------------------------------- */
void TIM6_DAC_IRQHandler(void) {

    motor_speed = (TIM3->CNT - 0x7FFF);
    TIM3->CNT   = 0x7FFF;

#if CHECKOFF_1_HBRIDGE_MANUAL

    pwm_setDutyCycle(80);
    duty_cycle = 80;

#elif CHECKOFF_2_ENCODER_ONLY

    pwm_setDutyCycle(70);
    duty_cycle = 70;

#elif CHECKOFF_3_OPEN_LOOP_PWM

    open_loop_divider++;
    if (open_loop_divider < 66) {
        pwm_setDutyCycle(30);
        duty_cycle = 30;
    } else if (open_loop_divider < 132) {
        pwm_setDutyCycle(80);
        duty_cycle = 80;
    } else {
        open_loop_divider = 0;
    }

#elif CHECKOFF_4_PI_SINGLE_SPEED

    PI_update();

#elif CHECKOFF_5_FINAL_DEMO

    PI_update();

#else

    pwm_setDutyCycle(0);
    duty_cycle = 0;

#endif

    log_data();   //  for checkoff 2+

    TIM6->SR &= ~TIM_SR_UIF;
}

/* ------------------------------------------------------------------------------------------------------------- */
void PI_update(void) {

    int16_t target_counts = 0;

    if (target_rpm >= 80) {
        target_counts = 64;
    } else if (target_rpm >= 50) {
        target_counts = 40;
    } else {
        target_counts = 0;
    }

    error = target_counts - motor_speed;

    error_integral += Ki * error;

    if (error_integral > 5000)  error_integral = 5000;
    if (error_integral < 0)     error_integral = 0;

    int16_t output = 45 + ((Kp * error + error_integral) >> 4);

    if (target_rpm == 0) {
        output = 0;
        error_integral = 0;
    }

    if (output < 0)   output = 0;
    if (output > 100) output = 100;

    pwm_setDutyCycle((uint8_t)output);
    duty_cycle = (uint8_t)output;
}