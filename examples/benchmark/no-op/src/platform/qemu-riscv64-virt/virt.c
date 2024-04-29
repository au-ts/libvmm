#include <8250_uart.h>


#define VIRT_UART16550_INTERRUPT	10

#define VIRT_UART16550_ADDR		0x10000000
#define VIRT_UART_BAUDRATE		115200
#define VIRT_UART_SHIFTREG_ADDR		1843200


void uart_init(){
    uart8250_init(VIRT_UART16550_ADDR, VIRT_UART_SHIFTREG_ADDR,
			     VIRT_UART_BAUDRATE, 0, 1);
}

void uart_putc(char c)
{
    uart8250_putc(c);
}

char uart_getchar(void)
{
    return uart8250_getc();
}

void uart_enable_rxirq()
{
    uart8250_enable_rx_int();
}

void uart_clear_rxirq()
{
    uart8250_interrupt_handler(); 
}
