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
#include <time.h>
#include <curses.h>
#include <math.h>

#define UDS_FILE "/tmp/rt_test.sock"

#define BUFF_SIZE 128

char buffer[BUFF_SIZE];
uint32_t histogram[150]; // store a histogram for delay times
WINDOW *mainwin;

void print_histogram() {
	int x, y;
	werase(mainwin);
	for (int i = 0; i < 150; i++) {
		wmove(mainwin, 40-log(histogram[i]+1), i);
		waddch(mainwin, 'X');
	}
	wrefresh(mainwin);
}


int main(int argc, char **argv)
{
	int sock_fd, sock_con, ret;
	socklen_t addrlen;
	struct sockaddr_un address;
	bool done;
	int size;
	struct timespec t1, t2;

	done = false;

	mainwin = initscr();
	if (!mainwin) {
		printf("Error initializing ncurses.\n");
		exit(1);
	}

	// Create a Unix domain socket
	sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		printf("Cannot create socket: %s\n", strerror(errno));
		return 1;
	}

	// Delete any still existing socket file
	unlink(UDS_FILE);

	address.sun_family = AF_LOCAL;
	strcpy(address.sun_path, UDS_FILE);

	ret = bind(sock_fd, (struct sockaddr *)&address, sizeof(address));
	if (ret) {
		printf("Cannot bind socket: %s\n", strerror(errno));
		close(sock_fd);
		return 1;
	}

	ret = listen(sock_fd, 5);
	if (ret) {
		printf("Cannot listen on socket: %s\n", strerror(errno));
		close(sock_fd);
		return 1;
	}
	addrlen = sizeof(struct sockaddr_in);

	ret = clock_gettime(CLOCK_MONOTONIC, &t1);
	if (ret) {
		printf("Cannot get time.\n", strerror(errno));
		close(sock_fd);
		return 1;
	}

	printf("Waiting for connection...\n");
	sock_con = accept(sock_fd, (struct sockaddr *)&address,
			(socklen_t * restrict)&addrlen);
	uint32_t counter = 1;
	bool no_measure = false;
	if (sock_con > 0) {
		printf("Client connected.\n");
		while(true) {
			if ((counter % 10) == 0) {
				print_histogram();
				counter++;
				no_measure = true;
				continue;
			}
			size = recv(sock_con, buffer, BUFF_SIZE-1, 0);
			if (size > 0) {
				//printf("%d bytes received.\n", size);
				counter++;
			} else {
				continue;
			}
			ret = clock_gettime(CLOCK_MONOTONIC, &t2);
			if (ret) {
				printf("Cannot get time.\n", strerror(errno));
				close(sock_fd);
				return 1;
			}
			time_t sec;
			long   nsec;
			uint64_t delta;
			sec = t2.tv_sec - t1.tv_sec;
			nsec = t2.tv_nsec - t1.tv_nsec;
			delta = sec * 1E9 + nsec;
			memcpy(&t1, &t2, sizeof(t2));
			if (no_measure) {
				no_measure = false;
				continue;
			}
			if ((delta / 500) < 150) {
				histogram[delta / 500]+=size;
			}
		}
		close(sock_con);
	}

	delwin(mainwin);
	endwin();
	refresh();

	close(sock_fd);
	unlink(UDS_FILE);
	return 0;
}
