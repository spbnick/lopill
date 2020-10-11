/*
 * Output "COOL" onto a four-digit 7-segment LED display,
 * driven by Philips SAA1064T, accessed via I2C.
 */
#include <init.h>
#include <rcc.h>
#include <gpio.h>
#include <i2c.h>
#include <misc.h>
#include <stdint.h>

int
main(void)
{
    /* Basic init */
    init();

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

    /*
     * Output a command to the display
     */
    {
        /*
         * Digit bits:
         *
         * .-2-.   |   .-04-.
         * 7   0   |   80  01
         * :-6-:  O R  :-40-:
         * 4   3   |   10  08
         * '-5-'   |   '-20-'
         *
         * The rightmost digit is sent first.
         *
         * Red LED on the side: second digit, bit 1 (02)
         * Dot in the middle: third digit, bit 1 (02)
         */
        uint8_t data[] = {
            /* Start writing with control register sub-address (0x00) */
            0x00,
            /*
             * Dynamic mode (alternating display of digits 1+3, 2+4),
             * No digits blanked, almost maximum current.
             */
            0xe7,
            /* Digit data */
            0xb0,
            0xbd,
            0xbd,
            0xb4,
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
}
