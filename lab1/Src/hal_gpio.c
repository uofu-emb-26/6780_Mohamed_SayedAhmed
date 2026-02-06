#include <stdint.h>
#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_gpio.h>

void My_HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    // 1) Enable clock for the port we are using
    if (GPIOx == GPIOC)
    {
        RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

        // Configure PC6 and PC7 as outputs (RED = PC6, BLUE = PC7)

        // MODER: clear bits for pins 6 and 7 (2 bits per pin)
        GPIOC->MODER &= ~((3U << (6 * 2)) | (3U << (7 * 2)));
        // MODER: set to 01 (general purpose output)
        GPIOC->MODER |=  ((1U << (6 * 2)) | (1U << (7 * 2)));

        // OTYPER: push-pull (0)
        GPIOC->OTYPER &= ~((1U << 6) | (1U << 7));

        // OSPEEDR: low speed (00)
        GPIOC->OSPEEDR &= ~((3U << (6 * 2)) | (3U << (7 * 2)));

        // PUPDR: no pull-up/pull-down (00)
        GPIOC->PUPDR &= ~((3U << (6 * 2)) | (3U << (7 * 2)));
    }
    else if (GPIOx == GPIOA)
    {
        RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

        // Configure PA0 as input with pull-down (USER button)

        // MODER: input (00)
        GPIOA->MODER &= ~(3U << (0 * 2));

        // OSPEEDR: low speed (00)
        GPIOA->OSPEEDR &= ~(3U << (0 * 2));

        // PUPDR: pull-down (10)
        GPIOA->PUPDR &= ~(3U << (0 * 2));
        GPIOA->PUPDR |=  (2U << (0 * 2));
    }

    (void)GPIO_Init; // ignore struct for this lab step (register-based config)

}


/*
void My_HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin)
{
}
*/

/*
GPIO_PinState My_HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    return -1;
}
*/

/*
void My_HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
}
*/

/*
void My_HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
}
*/
