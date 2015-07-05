
#include <fcntl.h>
#include <stdio.h>
#include <error.h>
#include <unistd.h>


#ifndef HAVE_BOOL
#define HAVE_BOOL
typedef int bool;
#endif

#ifndef FALSE
#define FALSE 0
#define false FALSE
#endif

#ifndef TRUE
#define TRUE 1
#define true TRUE
#endif



bool
SetNonBlock(int fh, int nonblock)
{
	int flags = fcntl(fh, F_GETFL);
	if (nonblock) {
		if (flags & O_NONBLOCK) 
            return true;
		flags |= O_NONBLOCK;
	} else {
		if (!(flags & O_NONBLOCK))
            return true; 
		flags &= ~O_NONBLOCK;
	}

	return (fcntl(fh, F_SETFL, flags) >= 0);
}

int
main(int argc , char **argv)
{
	int fd , len;
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("open file failed :");
		return -1;
	}


	SetNonBlock(fd, true);
	
	char buf[2048];
	len = read(fd, buf, sizeof(buf) );
	if (len > 0) {
		buf[len] = '\0';
		printf("read %d bytes, and data is :%s\n",len, buf);
	} else if (len < 0)
		perror("read file failed :");

	close(fd);

	return 0;
	
}
