#include "stm32f0xx.h"
#include <stdint.h>

#define GYRO_ADDR           0x69    
#define REG_WHO_AM_I        0x0F
#define WHO_AM_I_EXPECTED   0xD3

#define REG_CTRL1           0x20
#define REG_OUT_X_L         0x28
#define REG_OUT_Y_L         0x2A

#define GYRO_READ_AUTO_INC  0x80

#define THRESHOLD 4000
#define AXIS_MARGIN 1500
#define HOLD_COUNT   20
static void delay(volatile uint32_t t)
{
    while (t--) {
        __NOP();
    }
}

static void leds_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // PC6 PC7 PC8 PC9 = LEDs
    GPIOC->MODER &= ~((3u << (6 * 2)) |
                      (3u << (7 * 2)) |
                      (3u << (8 * 2)) |
                      (3u << (9 * 2)));

    GPIOC->MODER |=  ((1u << (6 * 2)) |
                      (1u << (7 * 2)) |
                      (1u << (8 * 2)) |
                      (1u << (9 * 2)));
}

static inline void led_red_on(void)    { GPIOC->BSRR = (1u << 6); }
static inline void led_red_off(void)   { GPIOC->BRR  = (1u << 6); }

static inline void led_orange_on(void) { GPIOC->BSRR = (1u << 8); }
static inline void led_orange_off(void){ GPIOC->BRR  = (1u << 8); }

static inline void led_green_on(void)  { GPIOC->BSRR = (1u << 9); }
static inline void led_green_off(void) { GPIOC->BRR  = (1u << 9); }

static inline void led_blue_on(void)   { GPIOC->BSRR = (1u << 7); }
static inline void led_blue_off(void)  { GPIOC->BRR  = (1u << 7); }

static void leds_all_off(void)
{
    led_red_off();
    led_green_off();
    led_orange_off();
    led_blue_off();
}

static void gpio_i2c_setup(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

    // PB11 = I2C2_SDA, PB13 = I2C2_SCL
    GPIOB->MODER &= ~((3u << (11 * 2)) | (3u << (13 * 2)));
    GPIOB->MODER |=  ((2u << (11 * 2)) | (2u << (13 * 2)));   // AF mode

    GPIOB->OTYPER |= (1u << 11) | (1u << 13);                 // open-drain

    
    GPIOB->AFR[1] &= ~((0xF << ((11-8)*4)) | (0xF << ((13-8)*4)));

GPIOB->AFR[1] |=  ((1u << ((11-8)*4)) |   // PB11 = AF1
                   (5u << ((13-8)*4)));   // PB13 = AF5
    // PB14 = GPIO output, set HIGH (SA0 high -> address 0x69)
    GPIOB->MODER &= ~(3u << (14 * 2));
    GPIOB->MODER |=  (1u << (14 * 2));
    GPIOB->OTYPER &= ~(1u << 14);
    GPIOB->BSRR = (1u << 14);

    // PC0 = GPIO output, set HIGH (I2C mode select)
    GPIOC->MODER &= ~(3u << (0 * 2));
    GPIOC->MODER |=  (1u << (0 * 2));
    GPIOC->OTYPER &= ~(1u << 0);
    GPIOC->BSRR = (1u << 0);

    // PB15 stays input (default is input)
}

static void i2c2_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

    I2C2->CR1 &= ~I2C_CR1_PE;

    // 100 kHz @ 8 MHz:
    // PRESC=1, SCLDEL=4, SDADEL=2, SCLH=0x0F, SCLL=0x13
    I2C2->TIMINGR =
        (1u    << 28) |
        (4u    << 20) |
        (2u    << 16) |
        (0x0Fu << 8)  |
        (0x13u << 0);

    I2C2->CR1 |= I2C_CR1_PE;
}

static void i2c2_clear_nack(void)
{
    I2C2->ICR = I2C_ICR_NACKCF;
}

static void i2c2_stop(void)
{
    I2C2->CR2 |= I2C_CR2_STOP;
    while (!(I2C2->ISR & I2C_ISR_STOPF)) {}
    I2C2->ICR = I2C_ICR_STOPCF;
}

static int i2c2_wait_txis_or_nack(void)
{
    while (1) {
        uint32_t isr = I2C2->ISR;
        if (isr & I2C_ISR_NACKF) return -1;
        if (isr & I2C_ISR_TXIS)  return 0;
    }
}

static int i2c2_wait_rxne_or_nack(void)
{
    while (1) {
        uint32_t isr = I2C2->ISR;
        if (isr & I2C_ISR_NACKF) return -1;
        if (isr & I2C_ISR_RXNE)  return 0;
    }
}

static int i2c2_wait_tc_or_nack(void)
{
    while (1) {
        uint32_t isr = I2C2->ISR;
        if (isr & I2C_ISR_NACKF) return -1;
        if (isr & I2C_ISR_TC)    return 0;
    }
}

static void i2c2_start_write(uint8_t addr7, uint8_t nbytes)
{
    I2C2->CR2 &= ~((0x3FFu << 0) |
                   (0xFFu  << 16) |
                   I2C_CR2_RD_WRN |
                   I2C_CR2_START  |
                   I2C_CR2_STOP   |
                   I2C_CR2_AUTOEND);

    I2C2->CR2 |= ((uint32_t)(addr7 << 1)) | ((uint32_t)nbytes << 16);
    I2C2->CR2 &= ~I2C_CR2_RD_WRN;   // write
    I2C2->CR2 |= I2C_CR2_START;
}

static void i2c2_start_read(uint8_t addr7, uint8_t nbytes)
{
    I2C2->CR2 &= ~((0x3FFu << 0) |
                   (0xFFu  << 16) |
                   I2C_CR2_RD_WRN |
                   I2C_CR2_START  |
                   I2C_CR2_STOP   |
                   I2C_CR2_AUTOEND);

    I2C2->CR2 |= ((uint32_t)(addr7 << 1)) | ((uint32_t)nbytes << 16);
    I2C2->CR2 |= I2C_CR2_RD_WRN;    // read
    I2C2->CR2 |= I2C_CR2_START;
}

static int gyro_write_reg(uint8_t reg, uint8_t value)
{
    i2c2_start_write(GYRO_ADDR, 2);

    if (i2c2_wait_txis_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -1; }
    I2C2->TXDR = reg;

    if (i2c2_wait_txis_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -2; }
    I2C2->TXDR = value;

    if (i2c2_wait_tc_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -3; }

    i2c2_stop();
    return 0;
}

static int gyro_read_reg(uint8_t reg, uint8_t *value)
{
    i2c2_start_write(GYRO_ADDR, 1);

    if (i2c2_wait_txis_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -1; }
    I2C2->TXDR = reg;

    if (i2c2_wait_tc_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -2; }

    i2c2_start_read(GYRO_ADDR, 1);

    if (i2c2_wait_rxne_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -3; }
    *value = (uint8_t)I2C2->RXDR;

    if (i2c2_wait_tc_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -4; }

    i2c2_stop();
    return 0;
}

static int gyro_read_regs(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    i2c2_start_write(GYRO_ADDR, 1);

    if (i2c2_wait_txis_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -1; }
    I2C2->TXDR = start_reg | GYRO_READ_AUTO_INC;

    if (i2c2_wait_tc_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -2; }

    i2c2_start_read(GYRO_ADDR, len);

    for (uint8_t i = 0; i < len; i++) {
        if (i2c2_wait_rxne_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -3; }
        buf[i] = (uint8_t)I2C2->RXDR;
    }

    if (i2c2_wait_tc_or_nack() < 0) { i2c2_clear_nack(); i2c2_stop(); return -4; }

    i2c2_stop();
    return 0;
}

static int16_t make_int16(uint8_t low, uint8_t high)
{
    return (int16_t)((high << 8) | low);
}

int main(void)
{
    leds_init();
    gpio_i2c_setup();
    i2c2_init();

    leds_all_off();

    // CHECK-OFF 1: WHO_AM_I
    uint8_t who = 0;
    int rc = gyro_read_reg(REG_WHO_AM_I, &who);

    if (rc != 0 || who != WHO_AM_I_EXPECTED) {
        // error: turn RED on forever
        led_red_on();
        while (1) {}
    }

    // success: blink green a few times
    for (int i = 0; i < 3; i++) {
        led_green_on();
        delay(300000);
        led_green_off();
        delay(300000);
    }

    // Enable gyro axes, normal mode
    // CTRL_REG1 bits:
    // PD=1, Zen=1, Yen=1, Xen=1 => 0x0F
    if (gyro_write_reg(REG_CTRL1, 0x0F) != 0) {
        led_red_on();
        while (1) {}
    }

    delay(500000);

    // opposite-pair mapping:
// +X -> Orange
// -X -> Green
// +Y -> Red
// -Y -> Blue
while (1) {
    static int hold_led = 0;      // 0 none, 1 orange, 2 green, 3 red, 4 blue
    static int hold_count = 0;

    uint8_t data[4];
    int read_ok = gyro_read_regs(REG_OUT_X_L, data, 4);

    if (read_ok != 0) {
        led_red_on();
        continue;
    }

    int16_t x = make_int16(data[0], data[1]);
    int16_t y = make_int16(data[2], data[3]);

    int16_t ax = (x >= 0) ? x : -x;
    int16_t ay = (y >= 0) ? y : -y;

    int detected_led = 0;

    if ((ax > THRESHOLD) && (ax > ay + AXIS_MARGIN)) {
        if (x < 0) {
            detected_led = 1;   // orange
        } else {
            detected_led = 2;   // green
        }
    } else if ((ay > THRESHOLD) && (ay > ax + AXIS_MARGIN)) {
        if (y < 0) {
            detected_led = 3;   // red
        } else {
            detected_led = 4;   // blue
        }
    }

    // If a direction is detected, refresh the hold timer
    if (detected_led != 0) {
        hold_led = detected_led;
        hold_count = HOLD_COUNT;
    } else if (hold_count > 0) {
        hold_count--;
    } else {
        hold_led = 0;
    }

    leds_all_off();

    switch (hold_led) {
        case 1: led_orange_on(); break;
        case 2: led_green_on();  break;
        case 3: led_red_on();    break;
        case 4: led_blue_on();   break;
        default: break;
    }

    delay(50000);
}
}
