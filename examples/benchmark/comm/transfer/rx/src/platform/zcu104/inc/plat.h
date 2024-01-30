#ifndef PLAT_H
#define PLAT_H

#ifdef SEL4
#define PLAT_MEM_BASE 0x50000000
#else
#define PLAT_MEM_BASE 0x40000000
#endif
#define PLAT_MEM_SIZE 0x10000000

#define UART_IRQ_ID 54

#endif
