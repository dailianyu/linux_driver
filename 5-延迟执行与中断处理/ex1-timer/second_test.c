
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

int 
main(int argc, char **argv)
{
  int fd;
  int counter = 0;
  int old_counter = 0;
  
  /*��/dev/second�豸�ļ�*/
  fd = open(argv[1], O_RDONLY);
  if (fd !=  - 1)
  {
    while (1)
    {
      read(fd,&counter, sizeof(unsigned int));//��Ŀǰ����������
      if(counter!=old_counter)
      {	
      	printf("seconds after open /dev/second :%d\n",counter);
      	old_counter = counter;
      }	
    }    
  }
  else
  {
    printf("Device open failure\n");
  }

  close(fd);
}
