#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f0xx.h"
#include <stdint.h>

// =====================================================
// CHECK-OFF MODE SELECT
// Set only ONE of these to 1 at a time
// =====================================================
#define CHECKOFF_1_HBRIDGE_MANUAL   0
#define CHECKOFF_2_ENCODER_ONLY     0
#define CHECKOFF_3_OPEN_LOOP_PWM    0
#define CHECKOFF_4_PI_SINGLE_SPEED  0
#define CHECKOFF_5_FINAL_DEMO       1

// =====================================================
// PI / scaling settings
// =====================================================
#define INTEGRAL_MAX        3200
#define INTEGRAL_MIN        0

// The timer interrupt measures raw encoder counts each sample.
// Your template stores motor_speed in raw encoder counts, not RPM.
// These targets are chosen to match that scheme.
//
// With TIM6 PSC=11 and ARR=30000 in the template, the sampling period
// is about 15 ms. At 3200 counts/output rev:
// 80 RPM  -> about 64 counts/sample
// 50 RPM  -> about 40 counts/sample
// 0 RPM   -> 0 counts/sample
#define TARGET_RAW_0_RPM    0
#define TARGET_RAW_50_RPM   40
#define TARGET_RAW_80_RPM   64

// Open-loop demo values
#define OPEN_LOOP_LOW_DUTY  30
#define OPEN_LOOP_HIGH_DUTY 60

// =====================================================
// Global variables required by template / debugger
// =====================================================
extern volatile int16_t error_integral;
extern volatile uint8_t duty_cycle;
extern volatile int16_t target_rpm;
extern volatile int16_t motor_speed;
extern volatile int8_t adc_value;
extern volatile int16_t error;
extern volatile uint8_t Kp;
extern volatile uint8_t Ki;

// =====================================================
// Function prototypes
// =====================================================
void log_init(void);
void log_data(void);
void set_target_from_demo_state(void);
void motor_init(void);
void pwm_init(void);
void pwm_setDutyCycle(uint8_t duty);
void encoder_init(void);
void ADC_init(void);
void PI_update(void);



#endif