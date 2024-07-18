#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
		printf("./vsock_test.elf <DEST CID>\n");
		return 1;
	}

	printf("VSOCK TEST|INFO: starting\n");

	if (argc != 2) {
	uint32_t dest_cid = atoi(argv[1]);
	printf("VSOCK TEST|INFO: using destination CID %d\n", dest_cid);

	int s = socket(AF_VSOCK, SOCK_STREAM, 0);

	struct sockaddr_vm addr;
	memset(&addr, 0, sizeof(struct sockaddr_vm));
	addr.svm_family = AF_VSOCK;
	addr.svm_port = 9999;
	addr.svm_cid = dest_cid;

	int r = connect(s, &addr, sizeof(struct sockaddr_vm));
	if (r < 0) {
		printf("VSOCK TEST|ERROR: connect failed with '%s'\n", strerror(errno));
		return 1;
	}

	char *hello = "Hello, world!";
	r = send(s, hello, strlen(hello), 0);
	if (r < 0) {
		printf("VSOCK TEST|ERROR: send failed with '%s'\n", strerror(errno));
		return 1;
	}

	close(s);

	return 0;
}
