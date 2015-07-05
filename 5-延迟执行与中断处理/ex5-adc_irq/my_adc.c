/*************************************

NAME:ad driver
COPYRIGHT:www.embedhq.org

*************************************/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/regs-clock.h>
#include <plat/regs-timer.h>
	 
#include <plat/regs-adc.h>
#include <mach/regs-gpio.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>

static unsigned int ch = 2;
static unsigned int pvalue = 255;


/*s3c2440 0x58000000*/
#define S3C2440_ADC_BASE   0x58000000

struct adcdev_t {
	struct clk *adc_clk;
	void __iomem *base_addr;
	volatile unsigned long *adccon;
	volatile unsigned long *adctsc;
	volatile unsigned long *adcdata0;
};

struct adcdev_t adcdev;


void START_ADC()
{
	*(adcdev.adccon) = 0x00;
	*(adcdev.adccon) = (1 << 14) |( pvalue << 6)| (ch << 3) ;
	*(adcdev.adccon) = *(adcdev.adccon)| (1 << 0);

}

static irqreturn_t adcdone_int_handler(int irq, void *dev_id)
{
	struct adcdev_t *adcd = (struct adcdev_t *)dev_id; 
	unsigned int c = (*(adcd->adccon) >> 3) & 0x07;
	unsigned int adcdata;

	//printk(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);
	/*共享中断，先判断是不是我们的设备产生了中断*/
	if ((c != ch) ||(*(adcd->adcdata0) & (1 << 14)) )
		return IRQ_NONE;
	//printk(KERN_INFO"%s %d\n",__FUNCTION__,__LINE__);

	/*AD convertion data, handling it*/
	
	adcdata = *(adcd->adcdata0) & 0x3f;
	printk(KERN_INFO"adcdata %d\n", adcdata);

	START_ADC();
	

	return IRQ_HANDLED;
}




static int __init my_adc_init(void)
{
	int ret;

	adcdev.base_addr = ioremap(S3C2440_ADC_BASE,0x20);
	if (adcdev.base_addr == NULL)
	{
		printk(KERN_ERR "failed to remap register block\n");
		return -ENOMEM;
	}
	adcdev.adccon = (volatile unsigned long *)(adcdev.base_addr);
	adcdev.adctsc = (volatile unsigned long *)(adcdev.base_addr + 4);
	adcdev.adcdata0 = (volatile unsigned long *)(adcdev.base_addr + 12);
	

	adcdev.adc_clk = clk_get(NULL, "adc");
	if (!adcdev.adc_clk)
	{
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}
	clk_enable(adcdev.adc_clk);

	/* enable AD convertion */
	*(adcdev.adctsc) = 0x00;
	START_ADC();
	
	ret = request_irq(IRQ_ADC, adcdone_int_handler, IRQF_SHARED, "adc", &adcdev);
	if (ret)
	{
		iounmap(adcdev.base_addr);
		return ret;
	}

	
	printk ("adc  initialized\n");
	return ret;
}

static void __exit my_adc_exit(void)
{
	free_irq(IRQ_ADC, &adcdev);
	iounmap(adcdev.base_addr);

	if (adcdev.adc_clk)
	{
		clk_disable(adcdev.adc_clk);
		clk_put(adcdev.adc_clk);
		adcdev.adc_clk = NULL;
	}

}

module_init(my_adc_init);
module_exit(my_adc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("www.embedhq.org");
MODULE_DESCRIPTION("ADC Drivers for TQ2440 Board and support touch");

