#if 0

#include <pl011_uart.h>

volatile Pl011_Uart *uart = 0x22000000;

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

#else

#include <zynq_uart.h>

Xil_Uart *uart  = (void*) 0xFF000000;

void uart_init(void)
{
    xil_uart_init(uart);
    xil_uart_enable(uart);

    return;
}

void uart_putc(char c)
{
    xil_uart_putc(uart, c);
}

char uart_getchar(void)
{
    return xil_uart_getc(uart);
}

void uart_enable_rxirq(){
    xil_uart_enable_irq(uart, UART_ISR_EN_RTRIG);
}

void uart_clear_rxirq(){
    xil_uart_clear_rxbuf(uart);
    xil_uart_clear_irq(uart, 0xFFFFFFFF);
}


#endif
