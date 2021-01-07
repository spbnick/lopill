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
    gpio_pin_conf_output(GPIO_C, 13, GPIO_OTYPE_OPEN_DRAIN,
                         GPIO_PUPD_NONE, GPIO_OSPEED_HIGH);

    /* Forever */
    while (1) {
        /* Wait */
        for (i = 0; i < 1600000; i++);
        /* Toggle PC13 */
        GPIO_C->odr ^= GPIO_ODR_OD13_MASK;
    }
}
