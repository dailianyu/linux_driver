
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <plat/regs-timer.h>
#include <mach/regs-irq.h>
#include <asm/mach/time.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/ioctl.h>

#define DEVICE_NAME     "pwm"

#define BEEP_OFF _IO('P',1)
#define BEEP_ON_TIMES _IOW('P',2, unsigned int)

static int pwm_major = 0; 
static int pwm_minor = 0;
static struct cdev pwm_dev; 
static struct class *pwm_class; 

static struct semaphore lock;

static int tq2440_pwm_open(struct inode *inode, struct file *file)
{
	if (!down_trylock(&lock))
		return 0;
	else
		return -EBUSY;
}

static int tq2440_pwm_close(struct inode *inode, struct file *file)
{
	up(&lock);
	return 0;
}

static int tq2440_pwm_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long tcfg0;
	unsigned long tcfg1;
	unsigned long tcntb;
	unsigned long tcmpb;
	unsigned long tcon;

	switch (cmd) {
	case BEEP_OFF:
		s3c2410_gpio_cfgpin(S3C2410_GPB(0), S3C2410_GPIO_OUTPUT);
		s3c2410_gpio_setpin(S3C2410_GPB(0), 0);
		break;
	case BEEP_ON_TIMES:
	{
		void __user *argp =  (void __user *)arg;
		unsigned int args = *((unsigned int *)argp);
		struct clk *clk_p;
		unsigned long pclk;

		//set GPB0 as tout0, pwm output
		s3c2410_gpio_cfgpin(S3C2410_GPB(0), S3C2410_GPB0_TOUT0);

		tcon = __raw_readl(S3C2410_TCON);
		tcfg1 = __raw_readl(S3C2410_TCFG1);
		tcfg0 = __raw_readl(S3C2410_TCFG0);

		//prescaler = 50
		tcfg0 &= ~S3C2410_TCFG_PRESCALER0_MASK;
		tcfg0 |= (50 - 1); 

		//mux = 1/16
		tcfg1 &= ~S3C2410_TCFG1_MUX0_MASK;
		tcfg1 |= S3C2410_TCFG1_MUX0_DIV16;

		__raw_writel(tcfg1, S3C2410_TCFG1);
		__raw_writel(tcfg0, S3C2410_TCFG0);

		clk_p = clk_get(NULL, "pclk");
		pclk  = clk_get_rate(clk_p);
		tcntb  = (pclk/128)/args;
		tcmpb = tcntb>>1;

		__raw_writel(tcntb, S3C2410_TCNTB(0));
		__raw_writel(tcmpb, S3C2410_TCMPB(0));
			
		tcon &= ~0x1f;
		tcon |= 0xb;		//start timer
		__raw_writel(tcon, S3C2410_TCON);

		tcon &= ~2;
		__raw_writel(tcon, S3C2410_TCON);
	}
		break;
	}

	
	return 0;
}


static struct file_operations pwm_fops = {
    .owner	=   THIS_MODULE,
    .open	=   tq2440_pwm_open,
    .release	=   tq2440_pwm_close, 
    .ioctl	=   tq2440_pwm_ioctl,
};

static void pwm_setup_cdev(struct cdev *dev,int minor,struct file_operations *fops)
{
	int err;
	int devno = MKDEV(pwm_major,pwm_minor);
	cdev_init(dev,fops);
	dev->owner = THIS_MODULE;
	
	err = cdev_add(dev,devno,1);
	if(err)
		printk(KERN_INFO"Error %d adding pwm drv %d\n",err,minor);
}

static int __init dev_init(void)
{
	int ret;
	dev_t dev_no = 0;
	
	init_MUTEX(&lock);
	dev_no = MKDEV(pwm_major, pwm_minor);
  	if (pwm_major)
  	{
 		ret = register_chrdev_region(dev_no, 1, DEVICE_NAME);
  	}
  	else
  	{
 		ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
 		pwm_major = MAJOR(dev_no);
  	}

  	if (ret < 0)
  	{
 		printk(KERN_WARNING"Led:unable to get major %d\n",pwm_major);
 		return ret;
  	}

  	pwm_setup_cdev(&pwm_dev, 0, &pwm_fops);

	pwm_class =class_create(THIS_MODULE, "pwm_class");
  	if(IS_ERR(pwm_class)) {
     		printk("Err: faipwm in creating class.\n");
     		return -ENOMEM;
  	}
  
  	device_create(pwm_class,NULL, dev_no, NULL,DEVICE_NAME);

	printk (DEVICE_NAME" initialized\n");
	return ret;
}

static void __exit dev_exit(void)
{
	device_destroy(pwm_class, MKDEV(pwm_major, pwm_minor));
	class_destroy(pwm_class); 
	
	cdev_del(&pwm_dev); 
	unregister_chrdev_region(MKDEV(pwm_major, pwm_minor), 1);
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PWM Drivers for TQ2440 Board");
