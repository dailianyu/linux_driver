#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>

#define WATCHDOG_MAGIC 'k' 
#define FEED_DOG _IO(WATCHDOG_MAGIC,1)

int main(int argc,char **argv)
{
	int fd;
	//打开看门狗
	fd = open("/dev/s3c2410_watchdog",O_RDWR);	 
	if(fd<0) {
		printf("cannot open the watchdog device\n");
		return -1;
	}
	
	//喂狗 ，否则系统重启
	while(1)
	{
		ioctl(fd,FEED_DOG);
        sleep(1);
		printf("feed the watchdog now!\n");
	}
	
	close(fd);
	return 0;

}
