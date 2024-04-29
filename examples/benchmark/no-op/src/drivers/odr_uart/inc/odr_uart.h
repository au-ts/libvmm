#define UART_BASE 0xff803000
#define UART_WFIFO 0x0
#define UART_STATUS 0xC
#define UART_TX_FULL (1 << 21)

#define UART_REG(x) ((volatile uint32_t *)(UART_BASE + (x)))

#include <stdint.h>

void odroid_putc(uint8_t ch);