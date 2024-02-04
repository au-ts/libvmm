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
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <sddf/blk/shared_queue.h>
#include "libuio.h"
#include "block.h"

/* Uncomment this to enable debug logging */
#define DEBUG_UIO

#if defined(DEBUG_UIO)
#define LOG_UIO(...) do{ printf("UIO_DRIVER: "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_UIO(...) do{}while(0)
#endif

#define LOG_UIO_ERR(...) do{ printf("UIO_DRIVER|ERROR: "); printf(__VA_ARGS__); }while(0)


#define UIO_DEV_NAME "/dev/uio0"
#define UIO_MAPS_DIR "/sys/class/uio/uio0/maps"
#define UIO_MAPS_MAX_NAME 64
#define UIO_MAX_MAPS 32

static struct pollfd pfd;
static void *maps[UIO_MAX_MAPS];
static int num_maps;

/*
 * Just happily abort if the user can't be bother to provide these functions
 */
__attribute__((weak)) int driver_init(void **maps, int num_maps)
{
    assert(!"should not be here!");
}

__attribute__((weak)) void driver_notified()
{
    assert(!"should not be here!");
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

static int uio_num_maps() {
    const char *path = UIO_MAPS_DIR;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    regex_t regex;
    int count = 0;

    // Compile regex
    if (regcomp(&regex, "^map[0-9]+$", REG_EXTENDED) != 0) {
        LOG_UIO_ERR("Could not compile regex\n");
        return -1;
    }

    // Open directory
    dir = opendir(path);
    if (dir == NULL) {
        LOG_UIO_ERR("Failed to open directory\n");
        return -1;
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        char fullPath[UIO_MAPS_MAX_NAME];
        
        int len = snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        if (len < 0 || len >= sizeof(fullPath)) {
            LOG_UIO_ERR("Failed to create full path\n");
            return -1;
        };

        // Check if entry is a directory
        if (stat(fullPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            // skip over . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

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

static int uio_map_size(int map_num) {
    char path[UIO_MAPS_MAX_NAME];
    char buf[UIO_MAPS_MAX_NAME];

    int len = snprintf(path, sizeof(path), "%s/map%d/size", UIO_MAPS_DIR, map_num);
    if (len < 0 || len >= sizeof(path)) {
        LOG_UIO_ERR("Failed to create map%d size path string\n", map_num);
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

    return strtoul(buf, NULL, 0);
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

    for (int i=0; i<num_maps; i++) {
        int size = uio_map_size(i);
        if (size < 0) {
            LOG_UIO_ERR("Failed to get size of map%d\n", i);
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

int main() {
    LOG_UIO("Starting\n");
    char *uio_device_name = UIO_DEV_NAME;

    // get the file descriptor for polling
    pfd.fd = open(uio_device_name, O_RDWR);
    assert(pfd.fd >= 0);

    // the event we are polling for
    pfd.events = POLLIN;

    /* Initialise UIO device mappings */
    if (uio_map_init(pfd.fd) != 0) {
        LOG_UIO_ERR("Failed to initialise UIO device mappings\n");
        close(pfd.fd);
        return 1;
    }
    
    /* Initialise driver */
    if (driver_init(maps, num_maps) != 0) {
        LOG_UIO_ERR("Failed to initialise driver\n");
        close(pfd.fd);
        return 1;
    }

    // Enable the uio interrupt
    uio_notify();

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

