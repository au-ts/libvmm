#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "../uio.h"

int main() {
    /*****************************************************************************/
    // Get address of map0 from UIO device
    size_t addr_fp = open("/sys/class/uio/uio0/maps/map0/addr", O_RDONLY);
    if (addr_fp == -1) {
        perror("Error opening file");
        return 1;
    }

    char addr_str[64]; // Buffer to hold the string representation of the address
    int addr_str_values_read = read(addr_fp, addr_str, sizeof(addr_str));

    if (addr_str_values_read <= 0) {
        perror("Error reading from file");
        close(addr_fp);
        return 2;
    }

    if (addr_str_values_read == sizeof(addr_str)) {
        fprintf(stderr, "Address string is too long to fit in address_str buffer\n");
        close(addr_fp);
        return 3;
    }

    addr_str[addr_str_values_read] = '\0'; // Null-terminate the string to be safe
    unsigned long long addr_val; // Use this to hold the actual address value
    sscanf(addr_str, "%llx", &addr_val);
    void *addr = (void *)addr_val;

    close(addr_fp);
    /*****************************************************************************/
    // Get size of map0 from UIO device
    size_t size_fp = open("/sys/class/uio/uio0/maps/map0/size", O_RDONLY);
    if (size_fp == -1) {
        perror("Error opening file");
        return 4;
    }

    char size_str[64]; // Buffer to hold the string representation of the size
    int size_str_values_read = read(size_fp, size_str, sizeof(size_str));

    if (size_str_values_read <= 0) {
        perror("Error reading from file");
        close(size_fp);
        return 5;
    }

    if (size_str_values_read == sizeof(size_str)) {
        fprintf(stderr, "Size string is too long to fit in size_str buffer\n");
        close(size_fp);
        return 6;
    }

    unsigned long long size;
    size_str[size_str_values_read] = '\0'; // Null-terminate the string to be safe
    sscanf(size_str, "%llx", &size);

    close(size_fp);
    /*****************************************************************************/
    // Get offset of map0 from UIO device
    unsigned long long offset;
    size_t offset_fp = open("/sys/class/uio/uio0/maps/map0/offset", O_RDONLY);
    if (offset_fp == -1) {
        perror("Error opening file");
        return 7;
    }

    char offset_str[64]; // Buffer to hold the string representation of the offset
    int offset_str_values_read = read(offset_fp, offset_str, sizeof(offset_str));

    if (offset_str_values_read <= 0) {
        perror("Error reading from file");
        close(offset_fp);
        return 8;
    }

    if (offset_str_values_read == sizeof(offset_str)) {
        fprintf(stderr, "Offset string is too long to fit in offset_str buffer\n");
        close(offset_fp);
        return 9;
    }

    offset_str[offset_str_values_read] = '\0'; // Null-terminate the string to be safe
    sscanf(offset_str, "%llx", &offset);

    close(offset_fp);
    /*****************************************************************************/
    printf("addr: %p\n", addr);
    printf("size: 0x%llx\n", size);
    printf("offset: 0x%llx\n", offset);
    /*****************************************************************************/ 

    // Open /dev/fb0 device to allow mmap
    int fb_fp = open("/dev/fb0", O_RDWR);
    if (fb_fp == -1) {
        perror("Error opening file");
        return 10;
    }
    printf("UIO FB|INFO: opened /dev/fb0\n");

    void *fbmap = mmap(NULL, FB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fp, 0);
    if (fbmap == MAP_FAILED) {
        printf("UIO FB|ERROR: failed to mmap frame buffer: %s\n", strerror(errno));
        return -1;
    }
    printf("UIO FB|INFO: mmaped /dev/fb0\n");

    // iocrtl(fb_fp, ,)

    close(fb_fp);

    // Open UIO device to allow read write and mmap
    int uio_fp = open("/dev/uio0", O_RDWR);
    if (uio_fp == -1) {
        perror("Error opening uio0");
        return 11;
    }
    printf("UIO FB|INFO: opened /dev/ui0\n");

    void *map0 = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fp, offset);

    if (map0 == MAP_FAILED) {
        perror("Error mapping uio_map0 memory");
        return 12;
    }

    printf("map0: %p\n", map0);

    uint32_t enable_uio_value = 1; // 4-byte integer value to write to file
    // Enable UIO interrupts first, incase it is already disabled
    if (write(uio_fp, &enable_uio_value, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("Error writing to uio0");
        close(uio_fp);
        return 13;
    } 

    printf("UIO FB|INFO: ready to receive data, faulting on VMM buffer\n");
    char command_str[64] = {0};
    sprintf(command_str, "devmem %d", UIO_INIT_ADDRESS);
    system(command_str);

    // Read from device, this blocks until interrupt
    int32_t read_value;
    int32_t irq_count = 0;
    while (read(uio_fp, &read_value, sizeof(uint32_t)) == sizeof(uint32_t)) {
        if (read_value >= irq_count) {
            // Copy contents of map0 into fbmap
            printf("UIO FB|INFO: Copying from map0 to fbmap\n");
            memcpy(fbmap, map0, FB_SIZE);
            printf("UIO FB|INFO: finished copying\n");
        }

        irq_count = read_value;

        // Enable interrupts again
        if (write(uio_fp, &enable_uio_value, sizeof(uint32_t)) != sizeof(uint32_t)) {
            perror("UIO FB|ERROR: could not write to uio_fp\n");
            close(uio_fp);
            return 14;
        }
    }

    close(uio_fp);
}
