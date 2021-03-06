#include <init.h>
#include <rcc.h>
#include <gpio.h>

int
main(void)
{
    volatile unsigned int i;

    /* Basic init */
    init();

    /* Enable clock to GPIO port C */
    RCC->iopenr |= RCC_IOPENR_IOPCEN_MASK;

    /* Configure the PC13 pin for open-drain output */
    GPIO_C->otyper = (GPIO_C->otyper & ~GPIO_OTYPER_OT13_MASK) |
                     (GPIO_OTYPE_OPEN_DRAIN << GPIO_OTYPER_OT13_LSB);
    GPIO_C->moder = (GPIO_C->moder & ~GPIO_MODER_MODE13_MASK) |
                    (GPIO_MODE_OUTPUT << GPIO_MODER_MODE13_LSB);
    GPIO_C->ospeedr = (GPIO_C->ospeedr & ~GPIO_OSPEEDR_OSPEED13_MASK) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED13_LSB);

    /* Forever */
    while (1) {
        /* Wait */
        for (i = 0; i < 1600000; i++);
        /* Toggle PC13 */
        GPIO_C->odr ^= GPIO_ODR_OD13_MASK;
    }
}
