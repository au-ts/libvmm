#include <stdio.h>
#include <stdlib.h>

int main() {
    system("cat /dev/urandom > /dev/fb0");
    return 0;
}