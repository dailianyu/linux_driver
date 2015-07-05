#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <plat/regs-watchdog.h>

#define WATCHDOG_MAGIC 'k'  
#define FEED_DOG _IO(WATCHDOG_MAGIC,1)

#define DEVICE_NAME "s3c2410_watchdog"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fengyong");
MODULE_DESCRIPTION("s3c2410 Watchdog");


static int watchdog_major = 0;

static struct cdev watchdog_cdev;
struct class *watchdog_class;

static int watchdog_open(struct inode *inode ,struct file *file)	
{
	__raw_writel(0, S3C2410_WTCON); //关闭看门狗
	__raw_writel(0x8000, S3C2410_WTCNT);//设置计数寄存器，如果里面内容递减为0时，软件层还没有数据送来，则系统重新启动。
	__raw_writel(0x8000, S3C2410_WTDAT);//数据寄存器主要是把数据装在到计数寄存器里面去。

	__raw_writel(S3C2410_WTCON_ENABLE|S3C2410_WTCON_DIV16|S3C2410_WTCON_RSTEN |
		     S3C2410_WTCON_PRESCALE(0x80), S3C2410_WTCON);//设置控制寄存器的属性，具体参看datasheet，主要是得到t_watchdog时钟周期。

	printk(KERN_NOTICE"open the watchdog now!\n");
	return 0;
}


static int watchdog_release(struct inode *inode,struct file *file)
{
	return 0;
}
//喂狗
static int watchdog_ioctl(struct inode *inode,struct file *file,unsigned int cmd,unsigned long arg)
{	
	switch(cmd)	{
	case FEED_DOG:
		__raw_writel(0x8000,S3C2410_WTCNT); //从软件层向计数寄存器里写值，当其内容减为0时，系统重新启动。
		break;
	default:
		printk(KERN_INFO"undefine cmd\n");
	}
	return 0;	
}

//将设备注册到系统之中
static void watchdog_setup_dev(struct cdev *dev,int minor,struct file_operations *fops)
{
	int err;
	int devno=MKDEV(watchdog_major,minor);
	cdev_init(dev,fops); 
	dev->owner=THIS_MODULE;
	dev->ops=fops; 
	err=cdev_add(dev,devno,1);
	if(err)
	printk(KERN_INFO"Error %d adding watchdog %d\n",err,minor);
}

static struct file_operations watchdog_remap_ops={
	.owner=THIS_MODULE,
	.open=watchdog_open, 
	.release=watchdog_release, 
	.ioctl=watchdog_ioctl,
};

//注册设备驱动程序，主要完成主设备号的注册
static int __init s3c2410_watchdog_init(void)
{
	int result;
	
	dev_t dev = MKDEV(watchdog_major,0);
	
	if(watchdog_major)
		result = register_chrdev_region(dev,1,DEVICE_NAME);
	else
	{	
		result = alloc_chrdev_region(&dev,0,1,DEVICE_NAME);
		watchdog_major = MAJOR(dev);
	}
	
	if(result<0)
	{
		 printk(KERN_WARNING"watchdog:unable to get major %d\n",watchdog_major);		
	  	 return result;
	}
	if(watchdog_major == 0)
		watchdog_major = result;
	printk(KERN_NOTICE"[DEBUG] watchdog device major is %d\n",watchdog_major);
	watchdog_setup_dev(&watchdog_cdev,0,&watchdog_remap_ops);
	
	watchdog_class =class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(watchdog_class)) {
		printk("Err: failed in creating class.\n");
		return PTR_ERR(watchdog_class);
	}

	/* register your own device in sysfs, and this will cause udevd to create corresponding device node */
	device_create(watchdog_class,NULL, dev, NULL,DEVICE_NAME);
  
	return 0;
}

//驱动模块卸载
static void s3c2410_watchdog_exit(void)
{
	device_destroy(watchdog_class, MKDEV(watchdog_major, 0));
	class_destroy(watchdog_class);
	
	cdev_del(&watchdog_cdev);
	unregister_chrdev_region(MKDEV(watchdog_major,0),1);
	printk("watchdog device uninstalled\n");
}

module_init(s3c2410_watchdog_init);
module_exit(s3c2410_watchdog_exit);
