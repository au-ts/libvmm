#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    for (int i = 0;; i++) {
        for (int j = 0; j < 16; j++) {
            printf(i % 2 == 0 ? "Tick" : "Tock");
        }
        putchar('\n');
        usleep(100000);
    }
}
