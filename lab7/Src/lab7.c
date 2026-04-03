#include "main.h"
#include <stdint.h>

/
#define CHECKOFF_1_MANUAL_SPIN     0
#define CHECKOFF_2_ENCODER_ONLY    0
#define CHECKOFF_3_OPEN_LOOP_PWM   0
#define CHECKOFF_4_P_CONTROL       0
#define CHECKOFF_5_PI_FINAL        1


// PWM settings
#define PWM_PERCENT_MIN     0
#define PWM_PERCENT_MAX     100


#define TARGET_0            0
#define TARGET_50           40
#define TARGET_80           64

// Open-loop test duties
#define DUTY_LOW            30
#define DUTY_HIGH           60
#define DUTY_MANUAL         45

// Controller gains
#define KP_DEFAULT          2
#define KI_DEFAULT          1

// Integral clamp
#define INTEGRAL_MAX        2000
#define INTEGRAL_MIN        0

/* =========================================================
   EXTERNAL TIMERS
   
   ========================================================= */
extern TIM_HandleTypeDef htim14;   // PWM timer
extern TIM_HandleTypeDef htim3;    // Encoder timer
extern TIM_HandleTypeDef htim6;    // Control loop timer

/* =========================================================
   GLOBAL VARIABLES
   ========================================================= */
volatile int16_t motor_speed = 0;      // raw encoder counts/sample
volatile int16_t target_speed = 0;     // same units as motor_speed
volatile int16_t error_val = 0;
volatile int16_t error_integral = 0;
volatile uint8_t duty_cycle = 0;

volatile uint8_t Kp = KP_DEFAULT;
volatile uint8_t Ki = KI_DEFAULT;

volatile uint8_t demo_state = 0;
volatile uint32_t button_debouncer = 0;
volatile uint32_t open_loop_counter = 0;



static void Motor_SetForward(void)
{
    // H-bridge direction pins: PA5 = 1, PA6 = 0
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
}

static void Motor_StopPWM(void)
{
    __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
    duty_cycle = 0;
}

static void PWM_SetDutyPercent(uint8_t percent)
{
    if (percent > 100) percent = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim14);
    uint32_t ccr = (arr * percent) / 100;

    __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ccr);
    duty_cycle = percent;
}

static void Demo_UpdateTarget(void)
{
#if CHECKOFF_5_PI_FINAL
    switch (demo_state)
    {
        case 0: target_speed = TARGET_0;  break;
        case 1: target_speed = TARGET_80; break;
        case 2: target_speed = TARGET_50; break;
        case 3: target_speed = TARGET_80; break;
        default:
            target_speed = TARGET_0;
            demo_state = 0;
            break;
    }
#elif CHECKOFF_4_P_CONTROL
    target_speed = TARGET_80;
#else
    target_speed = TARGET_0;
#endif
}

static void PI_Update(void)
{
    error_val = target_speed - motor_speed;

    error_integral += (Ki * error_val);

    if (error_integral > INTEGRAL_MAX) error_integral = INTEGRAL_MAX;
    if (error_integral < INTEGRAL_MIN) error_integral = INTEGRAL_MIN;

    int16_t output = (Kp * error_val) + error_integral;

    // crude scaling to duty percentage
    output = output >> 5;

    if (output < 0) output = 0;
    if (output > 100) output = 100;

    PWM_SetDutyPercent((uint8_t)output);
}

static void P_Update(void)
{
    error_val = target_speed - motor_speed;

    int16_t output = (Kp * error_val);

    output = output >> 1;

    if (output < 0) output = 0;
    if (output > 100) output = 100;

    PWM_SetDutyPercent((uint8_t)output);
}

/* =========================================================
   INIT FUNCTIONS
   ========================================================= */

static void Lab7_ButtonInit(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;        // USER button
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void Lab7_DirectionPinsInit(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    Motor_SetForward();
}

static void Lab7_PWMStart(void)
{
    HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
    PWM_SetDutyPercent(0);
}

static void Lab7_EncoderStart(void)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    __HAL_TIM_SET_COUNTER(&htim3, 0x7FFF);
}

static void Lab7_ControlTimerStart(void)
{
    HAL_TIM_Base_Start_IT(&htim6);
}

/* =========================================================
   BUTTON CALLBACK THROUGH SYSTICK
   ========================================================= */

void Lab7_Systick_Callback(void)
{
    button_debouncer = (button_debouncer << 1);

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
        button_debouncer |= 1;
    }

    // stable press detect
    if (button_debouncer == 0x7FFFFFFF)
    {
#if CHECKOFF_5_PI_FINAL
        demo_state++;
        if (demo_state > 3) demo_state = 0;
        Demo_UpdateTarget();
#elif CHECKOFF_4_P_CONTROL
        if (target_speed == TARGET_0) target_speed = TARGET_80;
        else target_speed = TARGET_0;
#endif
    }
}

/* =========================================================
   CONTROL LOOP
 

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        // Read encoder delta each sample
        motor_speed = (int16_t)(__HAL_TIM_GET_COUNTER(&htim3) - 0x7FFF);
        __HAL_TIM_SET_COUNTER(&htim3, 0x7FFF);

#if CHECKOFF_1_MANUAL_SPIN

        Motor_SetForward();
        PWM_SetDutyPercent(DUTY_MANUAL);

#elif CHECKOFF_2_ENCODER_ONLY

        Motor_SetForward();
        PWM_SetDutyPercent(DUTY_MANUAL);
        // watch motor_speed in debugger

#elif CHECKOFF_3_OPEN_LOOP_PWM

        Motor_SetForward();

        open_loop_counter++;
        if (open_loop_counter < 70)
        {
            PWM_SetDutyPercent(DUTY_LOW);
        }
        else if (open_loop_counter < 140)
        {
            PWM_SetDutyPercent(DUTY_HIGH);
        }
        else
        {
            open_loop_counter = 0;
        }

#elif CHECKOFF_4_P_CONTROL

        Motor_SetForward();
        P_Update();

#elif CHECKOFF_5_PI_FINAL

        Motor_SetForward();
        PI_Update();

#else

        Motor_StopPWM();

#endif
    }
}

/* =========================================================
   MAIN
   ========================================================= */

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    
    MX_GPIO_Init();
    MX_TIM14_Init();
    MX_TIM3_Init();
    MX_TIM6_Init();

    Lab7_ButtonInit();
    Lab7_DirectionPinsInit();
    Lab7_PWMStart();
    Lab7_EncoderStart();
    Lab7_ControlTimerStart();

    demo_state = 0;
    error_integral = 0;
    Demo_UpdateTarget();

    while (1)
    {
        
        HAL_Delay(50);
    }
}