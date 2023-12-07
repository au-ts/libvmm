#include <uart.h>
#include <nxp_uart.h>

volatile struct lpuart * const uart = (void*)0xff000000;

void uart_init(){
   nxp_uart_init(uart);
}

void uart_putc(char c){
    nxp_uart_putc(uart, c);
}

char uart_getchar(){
    return nxp_uart_getchar(uart);
}

void uart_enable_rxirq(){
    nxp_uart_enable_rxirq(uart);
}

void uart_clear_rxirq(){
    nxp_uart_clear_rxirq(uart);
}