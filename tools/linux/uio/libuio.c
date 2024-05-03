/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <dirent.h>
#include <regex.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <sddf/blk/queue.h>

#include <uio/libuio.h>
#include <uio/blk.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_UIO

#if defined(DEBUG_UIO)
#define LOG_UIO(...) do{ printf("UIO_DRIVER"); printf(": "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_UIO(...) do{}while(0)
#endif

#define LOG_UIO_ERR(...) do{ printf("UIO_DRIVER"); printf("|ERROR: "); printf(__VA_ARGS__); }while(0)

#define UIO_NUM 0

#define MAX_PATHNAME 64
#define UIO_MAX_MAPS 32

static struct pollfd pfd;
static void *maps[UIO_MAX_MAPS];
static uintptr_t maps_phys[UIO_MAX_MAPS];
static int num_maps;

/*
 * Just happily abort if the user can't be bother to provide these functions
 */
__attribute__((weak)) int driver_init(void **maps, uintptr_t *maps_phys, int num_maps, int argc, char **argv)
{
    assert(!"UIO driver did not implement driver_init");
}

__attribute__((weak)) void driver_notified()
{
    assert(!"UIO driver did not implement driver_notified");
}

void uio_notify()
{
    // writing 1 to the uio device re-enables/acks the IRQ
    int32_t one = 1;
    int ret = write(pfd.fd, &one, 4);
    if (ret < 0) {
        LOG_UIO_ERR("writing 1 to device failed with ret val: %d, errno: %d\n", ret, errno);
    }
    fsync(pfd.fd);
}

static int uio_num_maps()
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    regex_t regex;
    int count = 0;

    char path[MAX_PATHNAME];
    int len = snprintf(path, sizeof(path), "/sys/class/uio/uio%d/maps", UIO_NUM);
    if (len < 0 || len >= sizeof(path)) {
        LOG_UIO_ERR("Failed to create maps path string\n");
        return -1;
    }

    // Compile regex
    if (regcomp(&regex, "^map[0-9]+$", REG_EXTENDED) != 0) {
        LOG_UIO_ERR("Could not compile regex\n");
        return -1;
    }

    // Open directory
    dir = opendir(path);
    if (dir == NULL) {
        LOG_UIO_ERR("Failed to open uio maps directory\n");
        return -1;
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        char fullPath[MAX_PATHNAME];

        int len = snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        if (len < 0 || len >= sizeof(fullPath)) {
            LOG_UIO_ERR("Failed to create full uio maps path\n");
            return -1;
        };

        // Check if entry is a directory
        if (stat(fullPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            // skip over . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Check if directory name matches regex
            if (regexec(&regex, entry->d_name, 0, NULL, 0) == 0) {
                count++;
                LOG_UIO("Match found: %s\n", entry->d_name);
            }
        }
    }

    LOG_UIO("Total directories matching 'map[0-9]+': %d\n", count);

    // Cleanup
    closedir(dir);
    regfree(&regex);

    return count;
}

static int uio_map_size(int map_num)
{
    char path[MAX_PATHNAME];
    char buf[MAX_PATHNAME];

    int len = snprintf(path, sizeof(path), "/sys/class/uio/uio%d/maps/map%d/size", UIO_NUM, map_num);
    if (len < 0 || len >= sizeof(path)) {
        LOG_UIO_ERR("Failed to create uio%d map%d size path string\n", UIO_NUM, map_num);
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_UIO_ERR("Failed to open %s\n", path);
        return -1;
    }
    ssize_t ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        LOG_UIO_ERR("Failed to read map%d size\n", map_num);
        return -1;
    }
    close(fd);

    int size = strtoul(buf, NULL, 0);
    if (size == 0 || size == ULONG_MAX) {
        LOG_UIO_ERR("Failed to convert map%d size to integer\n", map_num);
        return -1;
    }

    return size;
}

static int uio_map_addr(int map_num, uintptr_t *addr)
{
    char path[MAX_PATHNAME];
    char buf[MAX_PATHNAME];

    int len = snprintf(path, sizeof(path), "/sys/class/uio/uio%d/maps/map%d/addr", UIO_NUM, map_num);
    if (len < 0 || len >= sizeof(path)) {
        LOG_UIO_ERR("Failed to create uio%d map%d addr path string\n", UIO_NUM, map_num);
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_UIO_ERR("Failed to open %s\n", path);
        return -1;
    }
    ssize_t ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        LOG_UIO_ERR("Failed to read map%d addr\n", map_num);
        return -1;
    }
    close(fd);

    uintptr_t ret_addr = strtoul(buf, NULL, 0);
    if (ret_addr == 0 || ret_addr == ULONG_MAX) {
        LOG_UIO_ERR("Failed to convert map%d addr to integer\n", map_num);
        return -1;
    }

    *addr = ret_addr;

    return 0;
}

static int uio_map_init(int fd)
{
    num_maps = uio_num_maps();
    if (num_maps < 0) {
        LOG_UIO_ERR("Failed to get number of maps\n");
        return -1;
    }
    if (num_maps == 0) {
        LOG_UIO_ERR("No maps found\n");
        return -1;
    }
    if (num_maps > UIO_MAX_MAPS) {
        LOG_UIO_ERR("too many maps, num_maps: %d, maximum is 16\n", num_maps);
        close(fd);
        return -1;
    }

    for (int i = 0; i < num_maps; i++) {
        int size = uio_map_size(i);
        if (size < 0) {
            LOG_UIO_ERR("Failed to get size of map%d\n", i);
            close(fd);
            return -1;
        }

        if (uio_map_addr(i, &maps_phys[i]) != 0) {
            LOG_UIO_ERR("Failed to get addr of map%d\n", i);
            close(fd);
            return -1;
        }

        if ((maps[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, i * getpagesize())) == NULL) {
            LOG_UIO_ERR("mmap failed, errno: %d\n", errno);
            close(fd);
            return -1;
        }
        LOG_UIO("mmaped map%d with 0x%x bytes at %p\n", i, size, maps[i]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <uio_device_number> [driver_args...]\n", argv[0]);
        return 1;
    }

    // append UIO_NUM to "/dev/uio" to get the full path of the uio device, e.g. "/dev/uio0"
    char uio_device_name[MAX_PATHNAME];
    int len = snprintf(uio_device_name, sizeof(uio_device_name), "/dev/uio%d", UIO_NUM);
    if (len < 0 || len >= sizeof(uio_device_name)) {
        LOG_UIO_ERR("Failed to create uio device name\n");
        printf("Usage: %s <uio_device_number> [driver_args...]\n", argv[0]);
        return 1;
    }

    // get the file descriptor for polling
    pfd.fd = open(uio_device_name, O_RDWR);
    if (pfd.fd < 0) {
        LOG_UIO_ERR("Failed to open %s\n", uio_device_name);
        printf("Usage: %s <uio_device_number> [driver_args...]\n", argv[0]);
        return 1;
    }

    // the event we are polling for
    pfd.events = POLLIN;

    /* Initialise UIO device mappings */
    if (uio_map_init(pfd.fd) != 0) {
        LOG_UIO_ERR("Failed to initialise UIO device mappings\n");
        close(pfd.fd);
        return 1;
    }

    // Enable the uio interrupt
    uio_notify();

    /* Initialise driver */
    // Here we pass the UIO device mappings to the driver, skipping the first one which only contains UIO's irq status
    if (driver_init(maps + 1, maps_phys + 1, num_maps - 1, argc - 1, argv + 1) != 0) {
        LOG_UIO_ERR("Failed to initialise driver\n");
        close(pfd.fd);
        return 1;
    }

    while (true) {
        // poll() returns when there is something to read, in our case, when there is an IRQ occur.
        // poll() doesn't do anything with the IRQ.
        int num_victims = poll(&pfd, 1, -1);

        // TODO(@jade): handle this gracefully
        assert(num_victims != 0);
        assert(num_victims != -1);
        assert(pfd.revents == POLLIN);

        // actually ACK the IRQ by performing a read()
        int irq_count;
        int read_ret = read(pfd.fd, &irq_count, sizeof(irq_count));
        assert(read_ret >= 0);
        LOG_UIO("received irq, count: %d\n", irq_count);

        /* clear the return event(s). */
        pfd.revents = 0;

        /* wake the guest driver up to do some real works */
        driver_notified();
    }

    return 0;
}

