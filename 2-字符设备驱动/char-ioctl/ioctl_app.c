#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <error.h>

#define TEST_IOCTL_CMD1 _IO('C',1)
#define TEST_IOCTL_CMD2 _IOR('C',2, int)
#define TEST_IOCTL_CMD3 _IOW('C',3, int)

int
main(int argc, char **argv)
{
	int fd ;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("failed to open file ");
		return -1;
	}

	ioctl(fd,TEST_IOCTL_CMD1 );

	int r;
	ioctl(fd,TEST_IOCTL_CMD2, &r);
	printf("r = %d\n",r);

	r = 2048;

	ioctl(fd,TEST_IOCTL_CMD3 ,&r);

	return 0;
}

