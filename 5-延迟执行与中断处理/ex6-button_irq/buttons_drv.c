/*
 * 
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/gpio.h>

static int button_major = 0;
MODULE_AUTHOR("farsight");
MODULE_LICENSE("Dual BSD/GPL");

static int flag = 0;
static int g_irq = 0;
static struct cdev button_devs;
struct class *key_class;

struct tasklet_struct key_taslet;

void button_tasklet_fn(unsigned long arg)
{
	/*  deferrable part. 中断的下半部*/
	printk("in tasklet\n");
}



static irqreturn_t key_handler(int irq, void *dev_id)
{
	printk("Debug ++ %s ++ irq = %d\n", __func__, irq);

	/*....   do something urgent , 中断的上半部*/
	//execute urgent part .... 
    
	tasklet_schedule(&key_taslet);

	return IRQ_HANDLED;
}

void button_gpio_init(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPF(0), S3C2410_GPF0_EINT0);
	s3c2410_gpio_cfgpin(S3C2410_GPF(1), S3C2410_GPF1_EINT1);
	s3c2410_gpio_cfgpin(S3C2410_GPF(2), S3C2410_GPF2_EINT2);
	s3c2410_gpio_cfgpin(S3C2410_GPF(4), S3C2410_GPF4_EINT4);
	
	set_irq_type(IRQ_EINT0, IRQ_TYPE_EDGE_FALLING);
	set_irq_type(IRQ_EINT1, IRQ_TYPE_EDGE_FALLING);
	set_irq_type(IRQ_EINT2, IRQ_TYPE_EDGE_FALLING);
	set_irq_type(IRQ_EINT4, IRQ_TYPE_EDGE_FALLING);
	
}

/*
 * Module housekeeping.
 */
static int button_init(void)
{
	int result;


	button_gpio_init();	
	result = request_irq (IRQ_EINT0, key_handler,IRQF_SAMPLE_RANDOM, "key", NULL);	
	if (result) {
		printk(KERN_INFO "button: can't get assigned irq\n");
	}

	result = request_irq (IRQ_EINT1, key_handler,IRQF_SAMPLE_RANDOM, "key", NULL);	
	if (result) {
		printk(KERN_INFO "button: can't get assigned irq\n");
	}

	result = request_irq (IRQ_EINT2, key_handler,IRQF_SAMPLE_RANDOM, "key", NULL);	
	if (result) {
		printk(KERN_INFO "button: can't get assigned irq\n");
	}

	result = request_irq (IRQ_EINT4, key_handler,IRQF_SAMPLE_RANDOM, "key", NULL);	
	if (result) {
		printk(KERN_INFO "button: can't get assigned irq\n");
	}	

	tasklet_init(&key_taslet, button_tasklet_fn, (unsigned long)g_irq);
	
	return 0;

}


static void button_cleanup(void)
{
	printk("simple device uninstalled\n");

	free_irq(IRQ_EINT0,NULL);
	free_irq(IRQ_EINT1,NULL);
	free_irq(IRQ_EINT2,NULL);
	free_irq(IRQ_EINT4,NULL);
}

module_init(button_init);
module_exit(button_cleanup);
