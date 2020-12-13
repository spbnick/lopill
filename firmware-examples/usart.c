/*
 * Transmit data on USART
 */
#include <init.h>
#include <rcc.h>
#include <gpio.h>
#include <usart.h>
#include <stdbool.h>

int
main(void)
{
    const char *message = "Hello, world!\r\n";
    const char *p;
    unsigned int c;

    init();

    /* Enable clock to I/O port A */
    RCC->iopenr |= RCC_IOPENR_IOPAEN_MASK;

    /* Configure TX (PA14) and RX (PA15) pins */
    GPIO_A->moder = (GPIO_A->moder & ~(GPIO_MODER_MODE14_MASK |
                                       GPIO_MODER_MODE15_MASK)) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE14_LSB) |
                    (GPIO_MODE_AF << GPIO_MODER_MODE15_LSB);
    GPIO_A->ospeedr = (GPIO_A->ospeedr & ~(GPIO_OSPEEDR_OSPEED14_MASK |
                                           GPIO_OSPEEDR_OSPEED15_MASK)) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED14_LSB) |
                      (GPIO_OSPEED_HIGH << GPIO_OSPEEDR_OSPEED15_LSB);
    GPIO_A->afrh = (GPIO_A->afrh &
                    ~(GPIO_AFRH_AFSEL14_MASK | GPIO_AFRH_AFSEL15_MASK)) |
                   (4 << GPIO_AFRH_AFSEL14_LSB) |
                   (4 << GPIO_AFRH_AFSEL15_LSB);

    /* Enable clock to USART2 */
    RCC->apb1enr |= RCC_APB1ENR_USART2EN_MASK;

    /* Set baud rate to ~115200, accounting for Fck = 32MHz */
    USART2->brr = (USART2->brr & (~USART_BRR_BRR_MASK)) | 0x116;

    /* Enable USART, leave the default mode of 8N1 */
    USART2->cr1 |= USART_CR1_UE_MASK;

    /* Enable receiver */
    USART2->cr1 |= USART_CR1_RE_MASK;

    /* Enable transmitter, sending an idle frame */
    USART2->cr1 |= USART_CR1_TE_MASK;

    /*
     * Transfer
     */
    while (1) {
        /* Wait for Enter to be pressed */
        do {
            /* Wait for receive register to fill */
            while (!(USART2->isr & USART_ISR_RXNE_MASK));
            /* Read the byte */
            c = USART2->rdr;
        } while (c != '\r');

        /* Output the message */
        for (p = message; *p != 0; p++) {
            /* Wait for transmit register to empty */
            while (!(USART2->isr & USART_ISR_TXE_MASK));
            /* Write the byte */
            USART2->tdr = *p;
        }
        /* Wait for transfer to complete, if any */
        if (p > message)
            while (!(USART2->isr & USART_ISR_TC_MASK));
    }
}
