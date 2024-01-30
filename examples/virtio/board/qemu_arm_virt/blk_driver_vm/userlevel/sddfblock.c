#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "sddf/blk/shared_queue.h"
#define BLKSIZE 4096

struct {
    blk_storage_t info;
    blk_req_queue_t requests;
    blk_req_queue_t reponses;
} *blk_p;

static char *blk_buffers;
static blk_queue_handle_t handle;
static const blk_queue_handle_t *hp = &handle;
static int fd;

int notified(channel)
{
    blk_request_code_t request;
    uintptr_t addr;
    uint32_t blk;
    uint16_t count;
    uint32_t id;
    uint32_t blksize = blk_p->info.blocksize;

    while (!blk_dequeue_req(hp, *request,
                            &addr,
                            &blk,
                            &count,
                            &id))
    {
        if (blk + count > blk_p->info.size || blk < 0) {
            blk_enqueue_resp(hp, SEEK_ERROR, addr, count, 0, id);
            break;
        }
                
        switch (code) {
        case READ_BLOCKS:
            lseek(fd, blk * blksize, SEEK_SET);
            donecount = read(fd, (void *)addr, count * blksize);
            if (donecount < 0) {
                blk_enqueue_resp(hp, errno, addr, count, 0, id);
            } else {
                blk_enqueue_resp(hp, SUCCESS, addr, count, donecount/blksize, id);
            }
            break;
        case WRITE_BLOCKS:
            lseek(fd, blk * blksize, SEEK_SET);
            donecount = write(fd, (void *)addr, count * blksize);
            if (donecount < 0) {
                blk_enqueue_resp(hp, errno, addr, count, 0, id);
            } else {
                blk_enqueue_resp(hp, SUCCESS, addr, count, donecount/blksize, id);
            }
            break;
        case FLUSH: case BARRIER:
            fsync(fd);
            break;
        }
    }
}


static void Usage()
{
    fprintf(stderr, "Usage: sddfblock uionum filename\n");
    exit(1);
}

int
main(int ac, char **av)
{
    int uionum;
    char uioname[16];
    char irqnum[8];
    char *p;
    int uiofd;
    size_t sz;

    if (ac != 2) {
        Usage();
    }
    
    uionum = strtol(av[1], &p, 10);
    if (*p) {
        Usage();
    }
    p = av[2];
    if ((fd = open(p, O_RDWR, 0)) == -1) {
        strerror(p);
        Usage();
    }
    
    snprintf(uioname, 16, "/dev/uio%d", uionum);
    if ((uiofd = open(uioname, O_RDWR, 0)) == -1) {
        strerror(uioname);
        Usage();
    }

    /*
     * Assume a single mapping including request, response and buffers
     */
    sz = BLKSIZE * (BLK_REQ_QUEUE_SIZE + BLK_RESP_QUEUE_SIZE + 1);
    blk_p = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED, uiofd, 0);
    if (blk_p == MAP_FAILED) {
        perror(uioname);
        Usage();
    }

    initialise_info(fd, blk_p->info);
    for (;;) {
        read(uiofd, &irqnum, sizeof irqnum);
        notified(1);
    }
    return 0;
}
