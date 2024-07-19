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

	printf("VSOCK RECV|INFO: starting\n");

	if (argc != 2) {
		printf("./vsock_recv.elf <CID>\n");
		return 1;
	}
	uint32_t cid = atoi(argv[1]);
	printf("VSOCK RECV|INFO: creating socket to wait on CID %d\n", cid);

	int s = socket(AF_VSOCK, SOCK_STREAM, 0);

	struct sockaddr_vm addr;
	memset(&addr, 0, sizeof(struct sockaddr_vm));
	addr.svm_family = AF_VSOCK;
	addr.svm_port = 9999;
	addr.svm_cid = cid;

	int r = bind(s, &addr, sizeof(struct sockaddr_vm));
	if (r) {
		printf("VSOCK RECV|ERROR: bind failed with '%s'\n", strerror(errno));
		return 1;
	}

	r = listen(s, 0);
	if (r) {
		printf("VSOCK RECV|ERROR: listen failed with '%s'\n", strerror(errno));
		return 1;
	}

	struct sockaddr_vm peer_addr;
	socklen_t peer_addr_size = sizeof(struct sockaddr_vm);
	int peer_fd = accept(s, &peer_addr, &peer_addr_size);

	char buf[64];
	size_t msg_len;
	while ((msg_len = recv(peer_fd, &buf, 64, 0)) > 0) {
		printf("Received %lu bytes: %.*s\n", msg_len, msg_len, buf);
	}

	return 0;
}
