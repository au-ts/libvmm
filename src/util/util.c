#include "../util.h"

void halt(int cond) {
    if (cond == 0) {
        printf("Halting VMM\n");
        while (1) {}
    }
}

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
    sel4cp_dbg_putc(character);
}
