#include <rcc.h>
#include <gpio.h>

int
main(void)
{
    volatile unsigned int i;

    /* Enable clock to GPIO port C */
    RCC->iopenr |= 1 << 2;

    /* Configure the PC13 pin for open-drain output, 2MHz max speed */
    GPIO_C->moder = (GPIO_C->moder & ~ (0x3 << (13 * 2))) |
                   (1 << (13 * 2));
    GPIO_C->otyper |= 1 << 13;

    /* Forever */
    while (1) {
        /* Wait */
        for (i = 0; i < 100000; i++);
        /* Toggle PC13 */
        GPIO_C->odr ^= 1 << 13;
    }
}
