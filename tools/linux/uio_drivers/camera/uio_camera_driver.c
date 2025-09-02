#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>

#include <linux/videodev2.h>

#define PRINT_FORMATS                                   false
#define IMX708_FORMATS                                  true

#define CAMERA_DEV_PATH                                 "/dev/video0"  
#define UIO_PATH_SDDF_FRAME_BUFFER                      "/dev/uio0"
#define UIO_PATH_SDDF_FRAME_READY_FAULT_TO_VMM          "/dev/uio1"
#define UIO_PATH_SDDF_CONTROL_BUFFER                    "/dev/uio2"
#define UIO_PATH_SDDR_CONTROL_INTERRUPT                 "/dev/uio3"

#define NUM_REQUESTED_BUFFERS                           2
#define PAGE_SIZE_4K                                    0x1000
#define SIZE_BUFFER_16M                                 0x1000000

// different nums to indicate driver state
#define DRIVER_NOT_READY                                0
#define DRIVER_READY                                    1
#define DRIVER_READY_STREAMING                          2
#define DRIVER_FRAME_READY                              4
#define DRIVER_ERROR                                    5

static char             *dev_name = CAMERA_DEV_PATH;
static int              cam_dev_fd = -1;
static bool             streaming = false;

struct buffer {
        void   *start;
};

#define CONFIGURE_CAMERA                                1
#define CONFIGURE_CAMERA_DEFAULT                        2
#define START_STREAM                                    3
#define STOP_STREAM                                     4
#define CLIENT_ACK_FRAME                                5

#define INTFCE_TYPE_FFMT                                2

struct buffer           *buffers;
static unsigned int     n_buffers;
size_t                  buf_size = 0;

int                     epoll_fd = -1;
#define MAX_EVENTS      3                       // events: control region changes, camera indicating frame ready, should never reach max
struct epoll_event      events[MAX_EVENTS];

int                     uio_frame_buffer_fd = -1;
int                     uio_frame_ready_fd = -1;
int                     uio_control_buffer_fd = -1;
int                     uio_control_notify_irq_fd = -1;
char                    *sddf_notify_client_irq_fault_vaddr;
char                    *sddf_frame_buffer_vaddr;
char                    *sddf_control_buffer_vaddr;

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CLIENT_NOTIFY_FAULT() (*sddf_notify_client_irq_fault_vaddr = 0)

#define CAMERA_CONTROL_FRAME_INTERFACE_SIZE (1024 - 12)

// format driver expects in the control region
struct camera_control_frame {
        uint32_t state;
        uint32_t client_req;
        uint32_t typeof_interface;
        uint8_t  interface[CAMERA_CONTROL_FRAME_INTERFACE_SIZE]; // basically the rest of the page
};

/**
  * struct v4l2_capability - Describes V4L2 device caps returned by VIDIOC_QUERYCAP
  *
  * @driver:       name of the driver module (e.g. "bttv")
  * @card:         name of the card (e.g. "Hauppauge WinTV")
  * @bus_info:     name of the bus (e.g. "PCI:" + pci_name(pci_dev) )
  * @version:      KERNEL_VERSION
  * @capabilities: capabilities of the physical device as a whole
  * @device_caps:  capabilities accessed via this particular device (node)
  * @reserved:     reserved fields for future extensions
  */
// struct v4l2_capability {
//         __u8    driver[16];
//         __u8    card[32];
//         __u8    bus_info[32];
//         __u32   version;
//         __u32   capabilities;
//         __u32   device_caps;
//         __u32   reserved[3];
// };


// taken from linux kernel v4l2 header files
/*
 *      V I D E O   I M A G E   F O R M A T
 */
// struct v4l2_pix_format {
//         uint32_t                   width;
//         uint32_t                   height;
//         uint32_t                   pixelformat;
//         uint32_t                   field;          /* enum v4l2_field */
//         uint32_t                   bytesperline;   /* for padding, zero if unused */
//         uint32_t                   sizeimage;
//         uint32_t                   colorspace;     /* enum v4l2_colorspace */
//         uint32_t                   priv;           /* private data, depends on pixelformat */
//         uint32_t                   flags;          /* format flags (V4L2_PIX_FMT_FLAG_*) */
//         union {
//                 /* enum v4l2_ycbcr_encoding */
//                 uint32_t                   ycbcr_enc;
//                 /* enum v4l2_hsv_encoding */
//                 uint32_t                   hsv_enc;
//         };
//         uint32_t                   quantization;   /* enum v4l2_quantization */
//         uint32_t                   xfer_func;      /* enum v4l2_xfer_func */
// };

/** from linux kernel v4l2 header files
 * struct v4l2_format - stream data format
 * @type:               enum v4l2_buf_type; type of the data stream
 * @fmt.pix:            definition of an image format
 * @fmt.pix_mp:         definition of a multiplanar image format
 * @fmt.win:            definition of an overlaid image
 * @fmt.vbi:            raw VBI capture or output parameters
 * @fmt.sliced:         sliced VBI capture or output parameters
 * @fmt.raw_data:       placeholder for future extensions and custom formats
 * @fmt:                union of @pix, @pix_mp, @win, @vbi, @sliced, @sdr,
 *                      @meta and @raw_data
 */
// struct v4l2_format {
//         uint32_t    type;
//         union {
//                 struct v4l2_pix_format          pix;     /* V4L2_BUF_TYPE_VIDEO_CAPTURE */
//                 // struct v4l2_pix_format_mplane   pix_mp;  /* V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
//                 // struct v4l2_window              win;     /* V4L2_BUF_TYPE_VIDEO_OVERLAY */
//                 // struct v4l2_vbi_format          vbi;     /* V4L2_BUF_TYPE_VBI_CAPTURE */
//                 // struct v4l2_sliced_vbi_format   sliced;  /* V4L2_BUF_TYPE_SLICED_VBI_CAPTURE */
//                 // struct v4l2_sdr_format          sdr;     /* V4L2_BUF_TYPE_SDR_CAPTURE */
//                 // struct v4l2_meta_format         meta;    /* V4L2_BUF_TYPE_META_CAPTURE */
//                 // __u8    raw_data[200];                   /* user-defined */
//         } fmt;
// };

static void errno_exit(const char *s) {
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg) {
        int r;
        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

int open_uio(const char *abs_path) {
    int fd = open(abs_path, O_RDWR);
    if (fd == -1) {
        perror("open_uio(): open()");
        fprintf(stderr, "Failed to open UIO device @ %s\n", abs_path);
        exit(EXIT_FAILURE);
    }
    return fd;
}

char *map_uio(uint64_t length, int uiofd) {
    void *base = (char *) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, uiofd, 0);
    if (base == MAP_FAILED) {
        perror("map_uio(): mmap()");
        fprintf(stderr, "Failed to mmap UIO device @ %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
    return (char *) base;
}

int create_epoll(void) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("create_epoll(): epoll_create1()");
        fprintf(stderr, "can't create the epoll fd.\n");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

void bind_fd_to_epoll(int fd, int epollfd) {
    struct epoll_event sock_event;
    sock_event.events = EPOLLIN;
    sock_event.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &sock_event) == -1) {
        perror("bind_fd_to_epoll(): epoll_ctl()");
        fprintf(stderr, "can't register fd %d to epoll fd %d.\n", fd, epollfd);
        exit(EXIT_FAILURE);
    }
}

void uio_interrupt_ack(int uiofd) {
    uint32_t enable = 1;
    if (write(uiofd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("uio_interrupt_ack(): write()");
        fprintf(stderr, "Failed to write enable/ack interrupts on uio fd %d\n", uiofd);
        exit(EXIT_FAILURE);
    }
}

static void uninit_device(void) {
        unsigned int i;

        for (i = 0; i < n_buffers; ++i)
                if (-1 == munmap(buffers[i].start, buf_size))
                        errno_exit("munmap");
        free(buffers);
}

static void stop_streaming(void) {
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(cam_dev_fd, VIDIOC_STREAMOFF, &type))
                errno_exit("VIDIOC_STREAMOFF");

        //uninit_device();
}

static void start_streaming(void) {
    unsigned int i;
    enum v4l2_buf_type type;
        printf("number of bufs = %d\n", n_buffers);
    for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
        
            printf("buffer num: %d\n", i);
            if (-1 == xioctl(cam_dev_fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");

            printf("Buffer %d queued\n", i);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(cam_dev_fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
}

static void configure_camera_set(void) {
        struct camera_control_frame *frame = (struct camera_control_frame *)sddf_control_buffer_vaddr;
        // check which interface the client is trying to change
        switch (frame->typeof_interface) {
                case INTFCE_TYPE_FFMT:
                        printf("Configuring camera frame format set\n");
                break;
        }

        return;
}

static void configure_camera_default(void) {
        struct camera_control_frame *frame = (struct camera_control_frame *)sddf_control_buffer_vaddr;
        // check which interface the client is trying to change
        switch (frame->typeof_interface) {
                case INTFCE_TYPE_FFMT:
                        printf("Configuring camera frame format Default\n");
                break;
        }

        return;
}

// true if state change, false if no change
static bool handle_control_region_change(void) {
        // process the control regions data, setup device with the provided values
        struct camera_control_frame *frame = (struct camera_control_frame *)sddf_control_buffer_vaddr;

        switch (frame->client_req) {
                case CONFIGURE_CAMERA:
                        // config based on 
                        configure_camera_set();
                        break;
                case CONFIGURE_CAMERA_DEFAULT:
                        // default cam config
                        configure_camera_default();
                        break;
                case START_STREAM:
                        printf("DRIVER: Starting Stream\n");
                        if (streaming == true) {
                                // continue streaming
                                frame->state = DRIVER_READY_STREAMING;
                                return true;
                        }
                        // if start streaming is set, start streaming frames and notify client when a frame is ready 
                        printf("starting stream\n");
                        frame->state = DRIVER_READY_STREAMING;
                        streaming = true;
                        start_streaming();
                        return true;
                case STOP_STREAM:
                        printf("DRIVER: Stopping Stream\n");
                        if (streaming == false) {
                                // stopping stream when not streaming keeps state the same
                                frame->state = DRIVER_READY;
                                return true;
                        }
                        frame->state = DRIVER_READY;
                        streaming = false;
                        stop_streaming();
                        break;
                case CLIENT_ACK_FRAME:
                        frame->state = DRIVER_READY_STREAMING;
                        return true;
                default:
                        // maybe return an error here not sure
                        fprintf(stderr, "CAMERA DRIVER|ERROR: NO VALID CLIENT REQUEST\n");
                        frame->state = DRIVER_ERROR;
                        return false;
        }

        return false;
}

// copy frame into buffer
static void process_image(const void *p, size_t bytes_used, int buf_index) {
        if (p && buf_size && buf_index >= 0 && bytes_used) {
                //size_t offset = buf_index * buf_size;
                size_t offset = 0; // for now to test, we assume a single buffer

                fprintf(stdout, "Processing frame %d, bytes used: %zu, buffer size: %zu\n", buf_index, bytes_used, buf_size);
                char *frame_buffer = (char *)((uintptr_t)sddf_frame_buffer_vaddr + offset);

                uint64_t byte_sum = 0;

                for (size_t i = 0; i < bytes_used; i++) {
                        // Copy the frame data to the UIO mapped memory
                        frame_buffer[i] = ((const char *)p)[i];
                        // Calculate the sum of the bytes in the frame
                        byte_sum += (uint8_t)frame_buffer[i];
                }
                printf("Frame %d processed, byte sum: %lu\n", buf_index, byte_sum);
                volatile int i = 0;
                while (i < 1000000) {
                        i++;
                }
        } else {
                fprintf(stderr, "Invalid frame data: p=%p, bytes_used=%zu, buf_index=%d, buf_size=%zu\n",
                        p, bytes_used, buf_index, buf_size);
                exit(EXIT_FAILURE);
        }
}

static void handle_frame(void) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(cam_dev_fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                case EAGAIN:
                case EIO:
                        /* Could ignore EIO, see spec. */
                        /* fall through */
                default:
                        errno_exit("VIDIOC_DQBUF");
                }
        }

        process_image(buffers[buf.index].start, buf.bytesused, buf.index);

        if (-1 == xioctl(cam_dev_fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
}

static void init_uio(void) {
        uio_frame_buffer_fd = open_uio(UIO_PATH_SDDF_FRAME_BUFFER);
        if (uio_frame_buffer_fd < 0) {
                fprintf(stderr, "Failed to open UIO device @ %s\n", UIO_PATH_SDDF_FRAME_BUFFER);
                exit(EXIT_FAILURE);
        }
        fprintf(stderr, "UIO device opened @ %s with fd %d\n", UIO_PATH_SDDF_FRAME_BUFFER, uio_frame_buffer_fd);
        // Map the UIO device memory
        sddf_frame_buffer_vaddr = map_uio(SIZE_BUFFER_16M, uio_frame_buffer_fd);
        // Check if the mapped address is valid
        if (sddf_frame_buffer_vaddr == NULL) {
                fprintf(stderr, "Failed to map UIO device memory @ %s\n", UIO_PATH_SDDF_FRAME_BUFFER);
                exit(EXIT_FAILURE);
        }
        fflush(stderr);

        uio_control_buffer_fd = open_uio(UIO_PATH_SDDF_CONTROL_BUFFER);

        if (uio_control_buffer_fd < 0) {
                fprintf(stderr, "Failed to open UIO device @ %s\n", UIO_PATH_SDDF_CONTROL_BUFFER);
                exit(EXIT_FAILURE);
        }
        fprintf(stderr, "UIO device opened @ %s with fd %d\n", UIO_PATH_SDDF_CONTROL_BUFFER, uio_control_buffer_fd);
        fflush(stderr);

        sddf_control_buffer_vaddr = map_uio(PAGE_SIZE_4K, uio_control_buffer_fd);
        //Map the UIO control buffer memory
        if (sddf_control_buffer_vaddr == NULL) {
                fprintf(stderr, "Failed to map UIO device memory @ %s\n", UIO_PATH_SDDF_CONTROL_BUFFER);
                exit(EXIT_FAILURE);
        }

        uio_control_notify_irq_fd = open_uio(UIO_PATH_SDDR_CONTROL_INTERRUPT);
        if (uio_control_notify_irq_fd < 0) {
                fprintf(stderr, "Failed to open UIO device @ %s\n", UIO_PATH_SDDR_CONTROL_INTERRUPT);
                exit(EXIT_FAILURE);
        }
        fprintf(stderr, "UIO device opened @ %s with fd %d\n", UIO_PATH_SDDR_CONTROL_INTERRUPT, uio_control_notify_irq_fd);
        fflush(stderr);

        uio_frame_ready_fd = open_uio(UIO_PATH_SDDF_FRAME_READY_FAULT_TO_VMM);
        if (uio_frame_ready_fd < 0) {
                fprintf(stderr, "Failed to open UIO device @ %s\n", UIO_PATH_SDDF_FRAME_READY_FAULT_TO_VMM);
                exit(EXIT_FAILURE);
        }

        fprintf(stderr, "UIO device opened @ %s with fd %d\n", UIO_PATH_SDDF_FRAME_READY_FAULT_TO_VMM, uio_frame_ready_fd);
        fflush(stderr);

        // will prematurely notify client
        sddf_notify_client_irq_fault_vaddr = map_uio(PAGE_SIZE_4K, uio_frame_ready_fd);

        // acknowledge to start the interrupts for the control buffer
        uio_interrupt_ack(uio_control_notify_irq_fd);
}

static void init_mmap(void) {
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = NUM_REQUESTED_BUFFERS;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(cam_dev_fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mappingn", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s, needs > 1 frame buffers\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        printf("Requesting %d buffers\n", req.count);
        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(cam_dev_fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buf_size = buf.length; // Store the buffer size for later use

                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              cam_dev_fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
        
        printf("Mapped %d buffers\n", n_buffers);
        for (unsigned int i = 0; i < n_buffers; ++i) {
                printf("Buffer %u: start=%p\n", i, buffers[i].start);
        }
}

static void init_epoll(void) {
        epoll_fd = create_epoll();
        if (epoll_fd < 0) {
                fprintf(stderr, "Failed to create epoll instance\n");
                exit(EXIT_FAILURE);
        }

        // bind interrupts to epoll
        bind_fd_to_epoll(uio_control_notify_irq_fd, epoll_fd);
        bind_fd_to_epoll(cam_dev_fd, epoll_fd);
}

static void init_device(void) {
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(cam_dev_fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n",
                            dev_name);
                exit(EXIT_FAILURE);
        }

        /* Select video input, video standard and tune here. */

        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(cam_dev_fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(cam_dev_fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
            printf("Cropcap failed\n");                
        }

        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (IMX708_FORMATS) {
                printf("Forcing format to IMX708 standard settings\n");
                // from https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-006.html#c.v4l2_xfer_func and https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/pixfmt-002.html#c.v4l2_pix_format
                fmt.fmt.pix.width               = 2304;
                fmt.fmt.pix.height              = 1296;
                fmt.fmt.pix.pixelformat         = V4L2_PIX_FMT_SBGGR10P;
                fmt.fmt.pix.field               = V4L2_FIELD_NONE;
                fmt.fmt.pix.bytesperline        = 2880;
                fmt.fmt.pix.sizeimage           = 3732480;
                fmt.fmt.pix.colorspace          = V4L2_COLORSPACE_RAW;
                fmt.fmt.pix.xfer_func           = V4L2_XFER_FUNC_DEFAULT; // V4L2_XFER_FUNC_NONE
                fmt.fmt.pix.ycbcr_enc           = V4L2_YCBCR_ENC_DEFAULT; // V4L2_YCBCR_ENC_601
                fmt.fmt.pix.quantization        = V4L2_QUANTIZATION_DEFAULT; // V4L2_QUANTIZATION_FULL_RANGE

                if (-1 == xioctl(cam_dev_fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        } else {
                printf("Using default format\n");
                /* Preserve original settings as set by v4l2-ctl for example */
                if (-1 == xioctl(cam_dev_fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        init_mmap();
}

static void open_device(void) {
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no devicen", dev_name);
                exit(EXIT_FAILURE);
        }

        cam_dev_fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == cam_dev_fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char **argv) {
        open_device();

        init_uio();

        struct camera_control_frame *control = (struct camera_control_frame *)sddf_control_buffer_vaddr;
        control->state = DRIVER_NOT_READY;

        init_device();

        init_epoll();
        control->state = DRIVER_READY;
        int z = 0;
        while (z < 100000000) {
                z++;
                continue;
        }
        CLIENT_NOTIFY_FAULT();
        printf("finished notify\n");
        while (true) {
                // wait for client to notify of change in control region or cam interrupt for new frame ready
                int n_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
                if (n_events == -1) {
                perror("main(): epoll_wait():");
                fprintf(stderr, "epoll wait failed\n");
                exit(EXIT_FAILURE);
                }
                if (n_events == MAX_EVENTS) {
                fprintf(stderr, "epoll_wait() returned MAX_EVENTS, there maybe dropped events!\n");
                }

                for (int i = 0; i < n_events; i++) {
                        if (!(events[i].events & EPOLLIN)) {
                                //fprintf(stderr, "got non EPOLLIN event on fd %d\n", events[i].data.fd);
                                continue;
                        }
                        if (events[i].data.fd == uio_control_notify_irq_fd) {
                                // notified by client that control region has changed
                                printf("DRIVER: Enter control handler i=%d\n", i);
                                bool res = handle_control_region_change();
                                // acknowledge interrupt handled
                                uio_interrupt_ack(uio_control_notify_irq_fd);
                                printf("DRIVER: Finishing acking\n");
                                if (res) {
                                        printf("SIGNAL TO CLIENT OF STATE CHANGE\n");
                                        int k = 0;
                                        while (k < 10000000) {
                                                // needs this or else client notify happens too early
                                                k++;
                                                continue;
                                        }
                                        CLIENT_NOTIFY_FAULT();
                                }
                        } else if (events[i].data.fd == cam_dev_fd && streaming) {
                                // notified by camera that a frame is ready, only valid if we are streaming
                                handle_frame();
                                control->state = DRIVER_FRAME_READY;
                                CLIENT_NOTIFY_FAULT(); // Signal the VMM that a frame is ready to be processed
                        } else if (events[i].data.fd == cam_dev_fd && !streaming) {
                                // notified by camera but we are not streaming
                                fprintf(stderr, "CAMERA DRIVER: CAMERA SENT UNEXPECTED INTERRUPT");
                        } else {
                                fprintf(stderr, "CAMERA DRIVER: Shouldn't reach here (unless I forgot a condition)");
                        }
                }
        }
}