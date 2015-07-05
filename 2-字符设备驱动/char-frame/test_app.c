
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
//#include <linux/ioctl.h>
#include <error.h>


int
main(int argc, char **argv)
{
	int fd ,ret = -1;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("failed to open file ");
		return -1;
	}

	char buf[512];

	printf("buf = %p sizeof(buf) = %d \n", buf, sizeof(buf));
	
	ret = read(fd, buf, sizeof(buf) );

	ret = write(fd, buf, sizeof(buf));
	
	close(fd);
	return 0;

}
