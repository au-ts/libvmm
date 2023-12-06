#ifndef PLAT_H
#define PLAT_H

#ifdef MEM_BASE
#define PLAT_MEM_BASE MEM_BASE
#else
#define PLAT_MEM_BASE 0x40000000
#endif
#define PLAT_MEM_SIZE 0x8000000

#define UART_IRQ_ID 54

#endif
