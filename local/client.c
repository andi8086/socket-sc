#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define UDS_FILE "/tmp/rt_test.sock"

char buffer[2] = {'A', '\0'};

int main(int argc, char **argv)
{
	int sock_fd, ret;
	struct sockaddr_un address;

	sock_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		printf("Error creating socket: %s\n", strerror(errno));
		return -1;
	}

	address.sun_family = AF_LOCAL;
	strcpy(address.sun_path, UDS_FILE);

	ret = connect(sock_fd, (struct sockaddr *)&address, sizeof(address));
	if (ret) {
		printf("Error, cannot connect to server socket: %s\n",
			strerror(errno));
		return -1;
	}
	printf("Connected to server...");

	while (true) {
		ret = send(sock_fd, buffer, strlen(buffer), 0);
		if (ret == -1) {
			printf("Error sending to server: %s\n", strerror(errno));
			close(sock_fd);
			return 1;
		}
	}
	close(sock_fd);
	return 0;
}
