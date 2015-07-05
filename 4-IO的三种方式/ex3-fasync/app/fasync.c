
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


volatile int readable = 0;
void
sig_handler(int signo)
{
	switch (signo) {
	case SIGIO:
		printf("received SIGIO\n");
		readable = 1;
		break;
	default:
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
	//sigaction(SIGTERM, &sa, NULL);
	//sigaction(SIGINT, &sa, NULL);
	//sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGIO, &sa, NULL);


	fcntl(fd, F_SETOWN, getpid());
	unsigned long oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oflags | FASYNC);

		
	while (!readable) {
		sleep(1);
	}
	
	char buf[2048];
	len = read(fd, buf, sizeof(buf));
	if (len > 0) {
		buf[len] = '\0';
		printf("read %d bytes, and data is :%s\n",len, buf);
	} else if (len < 0) 
		perror("read failed:");
	

	close(fd);
	return 0;
}

