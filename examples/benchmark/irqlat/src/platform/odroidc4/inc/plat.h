#ifndef PLAT_H
#define PLAT_H

#define PLAT_MEM_BASE 0x20000000
#define PLAT_MEM_SIZE 0x8000000

#define PLAT_GICD_BASE 0xffc01000
#define PLAT_GICC_BASE 0xffc02000

#define PLAT_TIMER_FREQ (24000000ull) // 24 MHz

#define UART_IRQ_ID 225

#endif
