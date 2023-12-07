#include <pl011_uart.h>

Pl011_Uart *uart  = (void*) 0xFF010000;

void uart_init(void)
{
    pl011_uart_init(uart);
    pl011_uart_enable(uart);

    return;
}

void uart_putc(char c)
{
    pl011_uart_putc(uart, c);
}

char uart_getchar(void)
{
    return pl011_uart_getc(uart);
}

void uart_enable_rxirq(){

}

void uart_clear_rxirq(){
    while(!(uart->flag & UART_FR_RXFE)) {
        volatile char c = uart->data;
    }
    uart->isr_clear = 0xffff;
}