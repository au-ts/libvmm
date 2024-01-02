#include <stdio.h>
#include <alsa/asoundlib.h>
#include <sys/mman.h>
#include "uio.h"
// #include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "sddf_snd_shared_ringbuffer.h"
#include <assert.h>

#define PAGE_SIZE_4K 0x1000
#define RING_BYTES 0x200000

static void signal_ready_to_vmm()
{
    printf("UIO SND|INFO: faulting on VMM buffer\n");
    char command_str[64] = {0};
    sprintf(command_str, "devmem %d", UIO_SND_FAULT_ADDRESS);
    int res = system(command_str);
    assert(res == 0);
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

static void *map_uio(int index, const char *size_str, int uio_fd)
{
    size_t size;
    int res;

    res = get_uio_map_size(size_str, &size);
    if (res) return MAP_FAILED;

    printf("map index %d of size %lu \n", index, size);

    // "To map the memory of mapping N, you have to use N times the page size as your offset"
    // https://www.kernel.org/doc/html/v4.13/driver-api/uio-howto.html
    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, index * PAGE_SIZE_4K);
}

void print_bytes(void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}

int main(void)
{
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

    void *shared_state = map_uio(0, UIO_SIZE("0"), uio_fd);
    if (shared_state == MAP_FAILED) { perror("Error mapping shared_state"); }

    void *ring_buffers = map_uio(1, UIO_SIZE("1"), uio_fd);
    if (ring_buffers == MAP_FAILED) { perror("Error mapping ring_buffers"); }

    void *tx_data      = map_uio(2, UIO_SIZE("2"), uio_fd);
    if (tx_data == MAP_FAILED) { perror("Error mapping tx_data"); }

    void *rx_data      = map_uio(3, UIO_SIZE("3"), uio_fd);
    if (rx_data == MAP_FAILED) { perror("Error mapping rx_data"); }

    void *rx_free =  (sddf_snd_pcm_data_ring_t *)ring_buffers;
    void *rx_used =  (sddf_snd_pcm_data_ring_t *)(ring_buffers + RING_BYTES * 1);
    // void *tx_free =  (sddf_snd_pcm_data_ring_t *)(ring_buffers + diff * 2);
    // void *tx_used =  (sddf_snd_pcm_data_ring_t *)(ring_buffers + diff * 3);
    // void *commands = (sddf_snd_cmd_ring_t *)(ring_buffers + diff * 4);

    printf("shared_state: %p\n", shared_state);
    print_bytes(shared_state);

    printf("ring_buffers: %p\n", ring_buffers);
    print_bytes(ring_buffers);

    printf("tx_data: %p\n", tx_data);
    print_bytes(tx_data);

    printf("rx_data: %p\n", rx_data);
    print_bytes(rx_data);

    printf("Dequeue\n");
    sddf_snd_pcm_data_t pcm;
    sddf_snd_dequeue_pcm_data(rx_free, &pcm);

    sddf_snd_enqueue_pcm_data(rx_used, pcm.stream_id, pcm.addr, pcm.len);

    printf("Faulting to snd vmm\n");
    signal_ready_to_vmm();

    #undef UIO_MAP
    #undef UIO_ADDR
    #undef UIO_SIZE
    #undef UIO_OFFSET
}
