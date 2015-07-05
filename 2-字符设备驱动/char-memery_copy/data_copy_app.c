
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
	int fd ,ret;
	int r = -1;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("failed to open file ");
		return -1;
	}

	ret = ioctl(fd,TEST_IOCTL_CMD2, &r);
	printf("r = %d ,err = %d\n",r, ret);

	r = 2048;
	ret = ioctl(fd,TEST_IOCTL_CMD3 ,&r);
	printf("ret = %d\n",ret);

	/* read */
	char buf[512];
	ret = read(fd, buf, sizeof(buf) );
	if (ret > 0) {
		buf[ret] = '\0';
		printf("read : %s\n", buf);
	}

	/*write*/
	ret = write(fd, "hello", sizeof("hello"));
	printf(" write %d bytes\n",ret);

	return 0;
}

