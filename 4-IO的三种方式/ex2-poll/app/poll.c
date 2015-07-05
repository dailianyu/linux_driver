#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <string.h>
#include <poll.h>

/*
	int poll(struct pollfd *fds, nfds_t nfds, int timeout);

	struct pollfd {
		int fd ; //  file descriptor 
		short events ; // requested events 
		short revents; //  returned events 
	};

	nfds   the number of array 'fds'
	timeout   an upper limit the time which poll() will block in milliseconds
*/


volatile int terminate = 0;

void
sig_handler(int signo)
{
	switch (signo) {
	case SIGINT:
	case SIGTERM:
	case SIGTSTP:
		terminate = 1;
		break;
	case SIGIO:
		printf("received SIGIO\n");
		break;
	}
}

int
main(int argc, char **argv)
{
	struct sigaction sa;
	int fd,len;
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("open file failed:");
		return -1;
	}

	memset(&sa, 0 , sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sig_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGIO, &sa, NULL);


	struct pollfd pollfds[1];

	pollfds[0].fd = fd ;
	pollfds[0].events = POLLIN;

	while (!terminate) {

		if (poll(pollfds, 1, 100) <= 0)
			continue;

		if ((pollfds[0].revents & POLLIN) == POLLIN) {
			char buf[2048];
			len = read(pollfds[0].fd, buf, sizeof(buf));
			if (len > 0) {
				buf[len] = '\0';
				printf("read %d bytes, and data is :%s\n",len, buf);
			} else if (len < 0) 
				perror("read failed:");
		}
	}
	close(fd);
	return 0;
}

