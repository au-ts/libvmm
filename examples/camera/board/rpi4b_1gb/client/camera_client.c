#include <microkit.h>
#include <stdint.h>
#include <stddef.h>
#include <libvmm/util/printf.h>

#define DRIVER_VMM_CH 1

// different nums to indicate driver state
#define DRIVER_NOT_READY                                0
#define DRIVER_READY                                    1
#define DRIVER_READY_STREAMING                          2
#define DRIVER_FRAME_READY                              4
#define DRIVER_ERROR                                    5

// driver requests
#define CONFIGURE_CAMERA                                1
#define CONFIGURE_CAMERA_DEFAULT                        2
#define START_STREAM                                    3
#define STOP_STREAM                                     4
#define CLIENT_ACK_FRAME                                5

uintptr_t frame_buffer_data_region;

uintptr_t camera_control_region;

struct camera_control_frame {
    uint32_t state;
    uint32_t client_req;
    uint32_t typeof_interface;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t pixel_format;
    uint32_t gain;
    uint32_t exposure;
    uint32_t defrect;
};

size_t frame_buffer_size = 3732480;

int loop_count = 0;

void alter_camera_control_region(uintptr_t control_region, uint32_t request) {
    camera_control_region = control_region;

    if (request > CLIENT_ACK_FRAME) {
        printf("CLIENT: UNKOWN REQUEST ID %d\n", request);
        return;
    }

    // Initialize the camera control region
    struct camera_control_frame *frame = (struct camera_control_frame *)camera_control_region;
    printf("CLIENT: REQUEST ID %d\n", request);
    frame->client_req = request;
    
    /* code for use when adding in camera configuration 
    switch (request) {
        case CONFIGURE_CAMERA:
            switch (frame->typeof_interface) {
                case XXXX:
                case XXXX:
                default:
            }
            
        case CONFIGURE_CAMERA_DEFAULT:
            // typeof interface will currently be ignored, 


        case START_STREAM:
            // dont do anything here yet, driver will just see start stream and begin
        
        case STOP_STREAM:
            // dont do anything here yet, driver will just see stop stream and begin
        case CLIENT_ACK_FRAME:
            // do nothing
        default:
    }*/

    // notify driver of the change
    microkit_notify(DRIVER_VMM_CH);
}

void notified(microkit_channel ch) {
    // if channel is from vmm, co_switch(worker)
    struct camera_control_frame *control = (struct camera_control_frame *)camera_control_region;
    printf("CAMERA CLIENT: NOTIIFIED\n");
    // Receive a message from the VMM
    switch (ch) {
        case DRIVER_VMM_CH:
            switch (control->state) {
                case DRIVER_NOT_READY:
                    // do nothing and wait for camera to notify it is ready
                    break;
                case DRIVER_READY:
                    // tell driver to start streaming (or whatever you want)
                    printf("NOTIFYING DRIVER VM TO BEGIN STREAMING\n");
                    alter_camera_control_region((uintptr_t)camera_control_region, START_STREAM);
                    break;
                case DRIVER_READY_STREAMING:
                    // wait for frame, do nothing
                    printf("CLIENT: WAITING FOR FRAMES\n");
                    break;
                case DRIVER_FRAME_READY:
                    // Handle VMM notifications if needed
                    microkit_dbg_puts("Received frame from VMM!\n");  
                    // Switch to the worker coroutine to process the frame
                    // wait for ntfn from VMM that frame is ready, this yields back to main coroutine
                    loop_count++;
                    // Process the frame data
                    if (frame_buffer_data_region) {
                        // process the frame data
                        microkit_dbg_puts("Processing frame data...\n");
                        uint64_t sum = 0;
                        uint8_t *buf = (uint8_t *)frame_buffer_data_region;

                        for (size_t i = 0; i < frame_buffer_size; i++) {
                            uint8_t val = *(uint8_t *)(buf + i);
                            sum += val;
                        }
                        printf("Frame processed, byte sum: %lu\n", sum);

                    } else {
                        microkit_dbg_puts("Frame buffer not initialized.\n");
                    }
                    
                    control->client_req= CLIENT_ACK_FRAME;
                    printf("NOTIFYING with %d : lc = %d\n", (loop_count % 3 == 0) ?  STOP_STREAM : CLIENT_ACK_FRAME, loop_count);

                    alter_camera_control_region((uintptr_t)camera_control_region, (loop_count == 3) ? STOP_STREAM : CLIENT_ACK_FRAME);

                    break;
                case DRIVER_ERROR:
                    printf("DRIVER ERROR\n");
                    break;
                default:
                    microkit_dbg_puts("Unknown State\n");
            }
            break;
        default:
            microkit_dbg_puts("Unknown channel\n");
            return;
    }
}

void init() {
    // setup camera data region
    printf("Camera Client Started\n");
}