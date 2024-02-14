#include <odr_uart.h>

void odroid_putc(uint8_t ch) {
	while ((*UART_REG(UART_STATUS) & UART_TX_FULL));
	*UART_REG(UART_WFIFO) = ch;
}