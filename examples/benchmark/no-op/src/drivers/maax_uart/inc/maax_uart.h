#define UART_BASE 0x30860000
#define STAT 0x98
#define TRANSMIT 0x40
#define STAT_TDRE (1 << 14)
#define UART_REG(x) ((volatile uint32_t *)(UART_BASE + (x)))

#include <stdint.h>

void maax_putc(uint8_t ch);