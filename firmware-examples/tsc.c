/*
 * Acquire two channels from the touch sense controller repeatedly, and output
 * the counts onto a four-digit 7-segment LED display controlled via I2C.
 */
#include <init.h>
#include <rcc.h>
#include <gpio.h>
#include <tsc.h>
#include <i2c.h>
#include <misc.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Configure the display.
 */
static void
display_init(void)
{
    /* Enable clock to GPIO port B */
    RCC->iopenr |= RCC_IOPENR_IOPBEN_MASK;

    /* Configure GPIO port B pins 6 and 7 for I2C1 with ext. 10K pull-ups */
    GPIO_B->moder = (GPIO_B->moder & ~(GPIO_MODER_MODE6_MASK |
                                       GPIO_MODER_MODE7_MASK)) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE6_LSB) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE7_LSB);
    GPIO_B->otyper = (GPIO_B->otyper & ~(GPIO_OTYPER_OT6_MASK |
                                         GPIO_OTYPER_OT7_MASK)) |
                     (GPIO_OTYPE_OPEN_DRAIN << GPIO_OTYPER_OT6_LSB) |
                     (GPIO_OTYPE_OPEN_DRAIN << GPIO_OTYPER_OT7_LSB);
    GPIO_B->ospeedr = (GPIO_B->ospeedr & ~(GPIO_OSPEEDR_OSPEED6_MASK |
                                           GPIO_OSPEEDR_OSPEED7_MASK)) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED6_LSB) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED7_LSB);
    GPIO_B->afrl = (GPIO_B->afrl &
                    ~(GPIO_AFRL_AFSEL6_MASK | GPIO_AFRL_AFSEL7_MASK)) |
                   (1 << GPIO_AFRL_AFSEL6_LSB) |
                   (1 << GPIO_AFRL_AFSEL7_LSB);

    /* Enable clock to I2C1 */
    RCC->apb1enr |= RCC_APB1ENR_I2C1EN_MASK;

    /* Configure I2C1 at 100KHz SCL, based on 32MHz PCLK */
    I2C1->timingr = (I2C1->timingr & ~(I2C_TIMINGR_PRESC_MASK |
                                       I2C_TIMINGR_SCLL_MASK |
                                       I2C_TIMINGR_SCLH_MASK |
                                       I2C_TIMINGR_SDADEL_MASK |
                                       I2C_TIMINGR_SCLDEL_MASK)) |
                    (0x07 << I2C_TIMINGR_PRESC_LSB) |
                    (0x13 << I2C_TIMINGR_SCLL_LSB) |
                    (0x0f << I2C_TIMINGR_SCLH_LSB) |
                    (0x02 << I2C_TIMINGR_SDADEL_LSB) |
                    (0x04 << I2C_TIMINGR_SCLDEL_LSB);

    /* Enable I2C1 */
    I2C1->cr1 |= I2C_CR1_PE_MASK;
}

/**
 * Set display digit bitmaps.
 *
 * @param b0    First (left-most) digit's bitmap.
 * @param b1    Second digit's bitmap.
 * @param b2    Third digit's bitmap.
 * @param b3    Fourth (right-most) digit's bitmap.
 *
 * Digit bits:
 *
 * .-2-.   |   .-04-.
 * 7   0   |   80  01
 * :-6-:  O R  :-40-:
 * 4   3   |   10  08
 * '-5-'   |   '-20-'
 *
 * Dot in the middle: d1, bit 1 (02)
 * Red LED on the side: d2, bit 1 (02)
 */
static void
display_set_digits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    uint8_t data[] = {
        /* Start writing with control register sub-address (0x00) */
        0x00,
        /*
         * Dynamic mode (alternating display of digits 1+3, 2+4),
         * No digits blanked, almost maximum current.
         */
        0x27,
        /* Digit data, right to left */
        d3,
        d2,
        d1,
        d0,
    };
    uint8_t *p;

    /* Enable sending STOP condition after all bytes transferred */
    I2C1->cr2 |= I2C_CR2_AUTOEND_MASK;

    /* Set the slave address (0x70) */
    I2C1->cr2 = (I2C1->cr2 & ~I2C_CR2_SADD_MASK) |
                (0x70 << I2C_CR2_SADD_LSB);

    /* Set number of bytes to transfer */
    I2C1->cr2 = (I2C1->cr2 & ~I2C_CR2_NBYTES_MASK) |
                (sizeof(data) << I2C_CR2_NBYTES_LSB);

    /* Start the transmission */
    I2C1->cr2 |= I2C_CR2_START_MASK;

    /* Transmit the command data */
    for (p = data; p < data + ARRAY_SIZE(data); p++) {
        /* Wait for transmit data register to empty */
        while (!(I2C1->isr & I2C_ISR_TXE_MASK));
        /* Write the next byte to transmit data register */
        I2C1->txdr = *p;
    }
}

/**
 * Set display number.
 *
 * @param number    The number to display.
 * @param dot       If true, turn on the dot between digits 2 and 3.
 * @param red       If true, turn on the red LED.
 */
static void
display_set_number(uint16_t number, bool dot, bool red)
{
    /*
     * Hexadecimal digit bitmaps
     */
    const uint8_t hexdigits[16] = {
        /* 0 */ 0x04 | 0x01 | 0x08 | 0x20 | 0x10 | 0x80,
        /* 1 */ 0x01 | 0x08,
        /* 2 */ 0x04 | 0x01 | 0x40 | 0x10 | 0x20,
        /* 3 */ 0x04 | 0x01 | 0x40 | 0x08 | 0x20,
        /* 4 */ 0x80 | 0x40 | 0x01 | 0x08,
        /* 5 */ 0x04 | 0x80 | 0x40 | 0x08 | 0x20,
        /* 6 */ 0x04 | 0x80 | 0x40 | 0x08 | 0x20 | 0x10,
        /* 7 */ 0x04 | 0x01 | 0x08,
        /* 8 */ 0x04 | 0x01 | 0x40 | 0x10 | 0x20 | 0x08 | 0x80,
        /* 9 */ 0x40 | 0x80 | 0x04 | 0x01 | 0x08 | 0x20,
        /* A */ 0x10 | 0x80 | 0x04 | 0x01 | 0x08 | 0x40,
        /* b */ 0x80 | 0x10 | 0x20 | 0x08 | 0x40,
        /* C */ 0x04 | 0x80 | 0x10 | 0x20,
        /* d */ 0x01 | 0x08 | 0x20 | 0x10 | 0x40,
        /* E */ 0x04 | 0x80 | 0x40 | 0x10 | 0x20,
        /* F */ 0x04 | 0x80 | 0x40 | 0x10,
    };
    display_set_digits(
        hexdigits[(number >> 12) & 0xf],
        hexdigits[(number >> 8) & 0xf] | (dot ? 2 : 0),
        hexdigits[(number >> 4) & 0xf] | (red ? 2 : 0),
        hexdigits[number & 0xf]
    );
}

/**
 * Configure the touch sense controller.
 * Use I/O Group 1 (PA0-PA3).
 * Use PA0 and PA1 as sensors and PA3 as the sampling cap.
 */
static void
tsc_init(void)
{
    /* Enable clock to GPIO port A */
    RCC->iopenr |= RCC_IOPENR_IOPAEN_MASK;

    /* Configure PA0 and PA1 as sensors and PA3 as sampling cap */
    GPIO_A->moder = (GPIO_A->moder & ~(GPIO_MODER_MODE0_MASK |
                                       GPIO_MODER_MODE1_MASK |
                                       GPIO_MODER_MODE3_MASK)) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE0_LSB) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE1_LSB) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE3_LSB);
    GPIO_A->otyper = (GPIO_A->otyper & ~(GPIO_OTYPER_OT0_MASK |
                                         GPIO_OTYPER_OT1_MASK |
                                         GPIO_OTYPER_OT3_MASK)) |
                     (GPIO_OTYPE_PUSH_PULL << GPIO_OTYPER_OT0_LSB) |
                     (GPIO_OTYPE_PUSH_PULL << GPIO_OTYPER_OT1_LSB) |
                     (GPIO_OTYPE_OPEN_DRAIN << GPIO_OTYPER_OT3_LSB);
    GPIO_A->ospeedr = (GPIO_A->ospeedr & ~(GPIO_OSPEEDR_OSPEED0_MASK |
                                           GPIO_OSPEEDR_OSPEED1_MASK |
                                           GPIO_OSPEEDR_OSPEED1_MASK)) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED0_LSB) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED1_LSB);
    GPIO_A->afrl = (GPIO_A->afrl & ~(GPIO_AFRL_AFSEL0_MASK |
                                     GPIO_AFRL_AFSEL1_MASK |
                                     GPIO_AFRL_AFSEL3_MASK)) |
                   (3 << GPIO_AFRL_AFSEL0_LSB) |
                   (3 << GPIO_AFRL_AFSEL1_LSB) |
                   (3 << GPIO_AFRL_AFSEL3_LSB);

    /* Enable clock to the TSC */
    RCC->ahbenr |= RCC_AHBENR_TSCEN_MASK;

    /* Configure and enable the touch sense controller */
    TSC->cr = (TSC->cr & ~(TSC_CR_PGPSC_MASK |
                           TSC_CR_CTPH_MASK |
                           TSC_CR_CTPL_MASK |
                           TSC_CR_MCV_MASK)) |
              /* Divide 32MHz AHB clock by 8 to get 4MHz PGCLK */
              (3 << TSC_CR_PGPSC_LSB) |
              /* Use four cycles of PGCLK to get 1us charge transfer high */
              /* TODO: Make sure this charges the sampling cap fully */
              (3 << TSC_CR_CTPH_LSB) |
              /* Use four cycles of PGCLK to get 1us charge transfer low */
              (3 << TSC_CR_CTPL_LSB) |
              /* Set max count value to 16383 */
              (TSC_CR_MCV_VAL_16383 << TSC_CR_MCV_LSB) |
              /* Enable the TSC */
              TSC_CR_TSCE_MASK;

    /* Disable hysteresis on all used I/Os */
    TSC->iohcr &= ~(TSC_IOHCR_G1_IO1_MASK |
                    TSC_IOHCR_G1_IO2_MASK |
                    TSC_IOHCR_G1_IO4_MASK);

    /* Enable PA3 as sampling channel */
    TSC->ioscr |= TSC_IOSCR_G1_IO4_MASK;

    /* Enable the analog I/O group */
    TSC->iogcsr |= TSC_IOGCSR_G1E_MASK;
}

/**
 * Acquire a single reading of two channels from the TSC.
 *
 * @param pcount1   Location for the acquired counter value for the first
 *                  channel. Could be NULL to not have the value output.
 * @param perror1   Location for the acquisition error flag for the first
 *                  channel. Could be NULL to not have the flag output.
 * @param pcount2   Location for the acquired counter value for the second
 *                  channel. Could be NULL to not have the value output.
 * @param perror2   Location for the acquisition error flag for the second
 *                  channel. Could be NULL to not have the flag output.
 *
 * @return The acquired counter value.
 */
static void
tsc_acquire(uint16_t *pcount1, bool *perror1,
            uint16_t *pcount2, bool *perror2)
{
    struct {
        unsigned int    mask;
        uint16_t       *pcount;
        bool           *perror;
    } channel_list[] = {
        {TSC_IOCCR_G1_IO1_MASK, pcount1, perror1},
        {TSC_IOCCR_G1_IO2_MASK, pcount2, perror2}
    };
    size_t i;
    unsigned int isr;

    for (i = 0; i < ARRAY_SIZE(channel_list); i++) {
        /* Enable the sensor channel */
        TSC->ioccr |= channel_list[i].mask;
        /* Start a new acquisition */
        TSC->cr |= TSC_CR_START_MASK;
        /* Wait for the acquisition to succeed or fail */
        do {
            isr = TSC->isr;
        } while (!(isr & (TSC_ISR_EOAF_MASK | TSC_ISR_MCEF_MASK)));
        /* Output the error flag, if requested */
        if (channel_list[i].perror) {
            *channel_list[i].perror = (isr & TSC_ISR_MCEF_MASK) != 0;
        }
        /* Output the acquired count, if requested */
        if (channel_list[i].pcount != NULL) {
            *channel_list[i].pcount = (uint16_t)TSC->iog1cr;
        }
        /* Reset error and acquisition flags */
        TSC->icr |= TSC_ICR_MCEIC_MASK | TSC_ICR_EOAIC_MASK;
        /* Disable the sensor channel */
        TSC->ioccr &= ~channel_list[i].mask;
    }
}

int
main(void)
{
    uint16_t count1 = 0, count2 = 0;
    bool error1 = false, error2 = false;

    init();
    display_init();
    tsc_init();

    while (true) {
        tsc_acquire(&count1, &error1, &count2, &error2);
        display_set_number(((count1 & 0xff0) << 4) | ((count2 & 0xff0) >> 4),
                           true, error1 || error2);
    }
}
