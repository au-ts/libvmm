#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define BLOCK_SIZE 512 // Size of the block to read

int main() {
    const char *device = "/dev/mmcblk0p1"; // Replace with your block device
    int fd;
    char buf[BLOCK_SIZE];
    struct timespec start, end;
    long long elapsed_time;

    fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    // Measure the time taken for the read operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (read(fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000LL;
    elapsed_time += (end.tv_nsec - start.tv_nsec);

    printf("Time taken: %lld nanoseconds\n", elapsed_time);

    close(fd);
    return 0;
}
