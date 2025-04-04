#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <unistd.h>

#define NUMS_TO_SEND (16384)
/* 32k buffers of 2-bytes unsigneds. */
uint16_t nums[NUMS_TO_SEND];

int main(int argc, char *argv[])
{
	printf("VSOCK SEND|INFO: starting\n");

	if (argc != 2) {
		printf("./vsock_send.elf <CID>\n");
		return 1;
	}
	uint32_t cid = atoi(argv[1]);
	printf("VSOCK SEND|INFO: creating socket to send on CID %d\n", cid);

	int s = socket(AF_VSOCK, SOCK_STREAM, 0);

	struct sockaddr_vm addr;
	memset(&addr, 0, sizeof(struct sockaddr_vm));
	addr.svm_family = AF_VSOCK;
	addr.svm_port = 9999;
	addr.svm_cid = cid;

	int r;

	r = connect(s, &addr, sizeof(struct sockaddr_vm));
	if (r < 0) {
		printf("VSOCK SEND|ERROR: connect failed %d with '%s'\n", r, strerror(errno));
		return 1;
	}

	printf("VSOCK SEND|INFO: connected, preparing payload\n");
	for (uint16_t i = 0; i < NUMS_TO_SEND; i++) {
		nums[i] = i;
	}
	printf("VSOCK SEND|INFO: now sending %lu bytes!\n", sizeof(nums));

	r = send(s, nums, sizeof(nums), 0);
	if (r < 0) {
		printf("VSOCK SEND|ERROR: send failed with '%s'\n", strerror(errno));
		return 1;
	}

	close(s);

	return 0;
}
