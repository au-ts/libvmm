#include <stdio.h>
#include <alsa/asoundlib.h>
#include <sys/mman.h>
#include "uio.h"
// #include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "sddf_snd_shared_ringbuffer.h"

static void signal_ready_to_vmm()
{
    printf("UIO SND|INFO: faulting on VMM buffer\n");
    char command_str[64] = {0};
    sprintf(command_str, "devmem %d", UIO_INIT_ADDRESS);
    system(command_str);
}

static int get_uio_map_addr(const char *path, void **addr)
{
    // Get address of map0 from UIO device
    size_t addr_fp = open(path, O_RDONLY);
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
    unsigned long addr_val; // Use this to hold the actual address value
    sscanf(addr_str, "%lx", &addr_val);
    *addr = (void *)addr_val;

    close(addr_fp);
    return 0;
}

static int get_uio_map_size(const char *path, size_t *size)
{
    // Get size of map0 from UIO device
    size_t size_fp = open(path, O_RDONLY);
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

    size_str[size_str_values_read] = '\0'; // Null-terminate the string to be safe
    sscanf(size_str, "%lx", size);

    close(size_fp);
    return 0;
}

static int get_uio_map_offset(const char *path, size_t *offset)
{
    // Get offset of map0 from UIO device
    size_t offset_fp = open(path, O_RDONLY);
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
    sscanf(offset_str, "%lx", offset);

    close(offset_fp);
    return 0;
}


static void *map_uio(const char *addr_str, const char *size_str, const char *offset_str, int uio_fd)
{
    void *addr;
    size_t size, offset;
    int res;

    res = get_uio_map_addr(addr_str, &addr);
    if (res) return MAP_FAILED;
    
    res = get_uio_map_size(size_str, &size);
    if (res) return MAP_FAILED;

    res = get_uio_map_offset(offset_str, &offset);
    if (res) return MAP_FAILED;

    printf("map [%p, %p) (off %#lx) \n", addr, addr + size, offset);
    return mmap(addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, offset);
}

int main(void)
{
    void *addr;
    int res;

    #define UIO_MAP "/sys/class/uio/uio0/maps/map"
    #define UIO_ADDR(m) UIO_MAP m "/addr"
    #define UIO_SIZE(m) UIO_MAP m "/size"
    #define UIO_OFFSET(m) UIO_MAP m "/offset"
    
    int uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd == -1) {
        perror("Error opening /dev/uio0");
        return 11;
    }
    printf("UIO SND|INFO: opened /dev/uio0\n");

    res = get_uio_map_addr(UIO_ADDR("0"), &addr);
    assert(res == 0);

    // error when non-contiguous?

    // void *shared_state = map_uio(UIO_ADDR("0"), UIO_SIZE("0"), UIO_OFFSET("0"), uio_fd);
    // if (shared_state == MAP_FAILED) { perror("Error mapping shared_state"); return 12; }

    // void *ring_buffers = map_uio(UIO_ADDR("1"), UIO_SIZE("1"), UIO_OFFSET("1"), uio_fd);
    // if (ring_buffers == MAP_FAILED) { perror("Error mapping ring_buffers"); return 12; }

    void *tx_data      = map_uio(UIO_ADDR("1"), UIO_SIZE("1"), UIO_OFFSET("1"), uio_fd);
    if (tx_data == MAP_FAILED) { perror("Error mapping tx_data"); return 12; }

    // void *rx_data      = map_uio(UIO_ADDR("2"), UIO_SIZE("2"), UIO_OFFSET("2"), uio_fd);
    // if (rx_data == MAP_FAILED) { perror("Error mapping rx_data"); return 12; }

    // void *tx_data      = map_uio(UIO_ADDR("2"), UIO_SIZE("2"), UIO_OFFSET("2"), uio_fd);
    // if (tx_data == MAP_FAILED) { perror("Error mapping tx_data"); return 12; }

    // void *rx_data      = map_uio(UIO_ADDR("3"), UIO_SIZE("3"), UIO_OFFSET("3"), uio_fd);
    // if (rx_data == MAP_FAILED) { perror("Error mapping rx_data"); return 12; }

    // printf("shared_state: %p\n", shared_state);
    // printf("ring_buffers: %p\n", ring_buffers);
    printf("tx_data: %p\n", tx_data);
    // printf("rx_data: %p\n", rx_data);

    printf("success!\n");
}
