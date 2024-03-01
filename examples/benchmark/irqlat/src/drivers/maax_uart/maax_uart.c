#include <maax_uart.h>


void maax_putc(uint8_t ch) {
	while (!(*UART_REG(STAT) & STAT_TDRE))
	{
	}
	*UART_REG(TRANSMIT) = ch;
}