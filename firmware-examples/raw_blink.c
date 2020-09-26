int
main(void)
{
    volatile unsigned int *rcc_iopenr = (unsigned int *)(0x40021000 + 0x2c);
    volatile unsigned int *gpioc_moder = (unsigned int *)(0x50000800 + 0x00);
    volatile unsigned int *gpioc_otyper = (unsigned int *)(0x50000800 + 0x04);
    volatile unsigned int *gpioc_odr = (unsigned int *)(0x50000800 + 0x14);
    volatile unsigned int i;

    /* Enable clock to GPIO port C */
    *rcc_iopenr |= 1 << 2;

    /* Configure the PC13 pin for open-drain output, 2MHz max speed */
    *gpioc_moder = (*gpioc_moder & ~ (0x3 << (13 * 2))) |
                   (1 << (13 * 2));
    *gpioc_otyper |= 1 << 13;

    /* Forever */
    while (1) {
        /* Wait */
        for (i = 0; i < 100000; i++);
        /* Toggle PC13 */
        *gpioc_odr ^= 1 << 13;
    }
}
