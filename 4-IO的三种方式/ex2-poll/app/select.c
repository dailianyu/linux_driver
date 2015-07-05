
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
	


	fd_set rfds;
	struct timeval timeout;
	int maxfd = fd + 1;

	while (!terminate) {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 1000000;

		if (select(maxfd, &rfds, NULL, NULL, &timeout) <= 0)
			continue;

		if (FD_ISSET(fd, &rfds)) {
			char buf[2048];
			len = read(fd, buf, sizeof(buf));
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
