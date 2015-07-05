
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <error.h>
#include <string.h>

#define GET_AVAIL_LEN _IOR('M',1, int)


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


	/*write*/
	ret = write(fd, "hello", strlen("hello"));
	if (ret > 0)
		printf(" write %d bytes\n",ret);
	else  {
		perror("failed to write ");
	}

	/* get the number of available bytes */
	int avail_len;
	ret = ioctl(fd, GET_AVAIL_LEN, &avail_len);
	printf("%d bytes available\n",avail_len);
	

	/* read */
	char buf[512];
	ret = read(fd, buf, sizeof(buf) );
	if (ret > 0) {
		buf[ret] = '\0';
		printf("read : %s\n", buf);
	}

	close(fd);

	return 0;

}
