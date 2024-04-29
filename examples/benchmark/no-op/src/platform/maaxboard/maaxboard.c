#include <maax_uart.h>

void uart_init(void)
{
    return;
}

void uart_putc(char c)
{
    maax_putc(c);
}

char uart_getchar(void)
{
    return '!';
}

void uart_enable_rxirq(){
    return;
}

void uart_clear_rxirq(){
    return;
}
