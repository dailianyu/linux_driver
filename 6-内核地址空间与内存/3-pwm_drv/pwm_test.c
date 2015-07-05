
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BEEP_OFF _IO('P',1)
#define BEEP_ON_TIMES _IOW('P',2, unsigned int)

struct spihr
{
	unsigned short status;
	unsigned short dat;
};

int main(int argc, char **argv)
{
	int fd;
	unsigned int temp =0;
	int i;

	fd = open("/dev/pwm", 0);
	if (fd < 0) {
		perror("open device pwm");
		exit(1);
	}
	printf("Please enter the times number (0 is stop) :\n");

	for(i=0;;i++)
	{
		scanf("%d",&temp);
		printf("times = %d \n",temp);
		if(temp == 0)
		{
			ioctl(fd, BEEP_OFF);
			printf("Stop Control Beep!\n");
			break;
		}
		else
		{
			ioctl(fd, BEEP_ON_TIMES, &temp);
		}
	}
	close(fd);
	return 0;
}

