

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>   
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int 
main(int argc, char **argv)   
{  
	int fbfd = 0;   
	struct fb_var_screeninfo vinfo;   
	struct fb_fix_screeninfo finfo;   
	long int screensize = 0;   

	/*���豸�ļ�*/  
	fbfd = open("/dev/fb0", O_RDWR);   
	if (fbfd < 0) {
		perror("open /dev/fb0 failed");
		return -1;
	}
  

	/*ȡ����Ļ��ز���*/  
	ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);  
	ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);   


	/*������Ļ��������С*/  
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;   

	/*ӳ����Ļ���������û���ַ�ռ�*/  
	char *fbp=(char*)mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED, fbfd, 0);  
	if (fbp == NULL) {
		perror("mmap failed");
		close(fbfd);
		return -1;
	}

	void *porg = malloc(screensize);
	memcpy(porg, fbp, screensize);
 
	 /*�����ͨ�� fbpָ���д������*/  
	memset(fbp, 0 , screensize);

	 sleep(4);

	memcpy(fbp, porg, screensize);

	/*�ͷŻ��������ر��豸*/ 
   	munmap(fbp, screensize);  
 	close(fbfd);  
} 

