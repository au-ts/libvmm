// Test reading and writing into /dev/vda

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int read_test(int blk)
{
    // Read from the device file
    printf("VIRTIO BLK TEST|INFO: Reading from /dev/vda...\n");
    char read_buf[17];
    int bytes_read = read(blk, read_buf, 17);
    if (bytes_read < 0) {
        printf("VIRTIO BLK TEST|ERROR: Error reading from /dev/vda\n");
        return -1;
    }
    printf("VIRTIO BLK TEST|INFO: Read %d bytes from /dev/vda, containing the string %s\n", bytes_read, read_buf);
    return 0;
}

int write_test(int blk)
{
    // Write to the device file
    printf("VIRTIO BLK TEST|INFO: Writing to /dev/vda...\n");
    char write_buf[17] = "0123456789abcdef\0";
    int bytes_written = write(blk, write_buf, 16);
    if (bytes_written < 0) {
        printf("VIRTIO BLK TEST|ERROR: Error writing to /dev/vda\n");
        return -1;
    }
    printf("VIRTIO BLK TEST|INFO: Wrote %d bytes to /dev/vda, containing the string 0123456789abcdef\n", bytes_written);
    return 0;
}

int lseek_to(int blk, int pos)
{
    // Set lseek to the beginning of the device file
    int ret = lseek(blk, pos, SEEK_SET);
    if (ret < 0) {
        printf("VIRTIO BLK TEST|ERROR: Error setting lseek to the beginning of /dev/vda\n");
        return -1;
    }
    return 0;
}

int fsync_file(int blk)
{
    printf("VIRTIO BLK TEST|INFO: fsyncing /dev/vda...\n");
    // fsync the write to block device
    int ret = fsync(blk);
    if (ret < 0) {
        printf("VIRTIO BLK TEST|ERROR: Error fsyncing /dev/vda\n");
        return -1;
    }
    printf("VIRTIO BLK TEST|INFO: fsynced /dev/vda\n");
    return 0;
}

int main(void)
{
    printf("VIRTIO BLK TEST|INFO: Starting virtio_blk_test...\n");
    
    printf("VIRTIO BLK TEST|INFO: Opening /dev/vda...\n");
    // Open the device file
    int blk = open("/dev/vda", O_RDWR);
    if (blk < 0) {
        printf("Error opening /dev/vda\n");
        return -1;
    }

    for (int i=0; i<100; i++) {
        write_test(blk);
        fsync_file(blk);
    }
    // write_test(blk);
    // fsync_file(blk);
    // lseek_to(blk, 0);
    // read_test(blk);

    printf("VIRTIO BLK TEST|INFO: Closing /dev/vda...\n");
    // Close the device file
    int ret = close(blk);
    if (ret < 0) {
        printf("VIRTIO BLK TEST|ERROR: Error closing /dev/vda\n");
        return -1;
    }

    printf("VIRTIO BLK TEST|INFO: virtio_blk_test finished\n");
    return 0;
}