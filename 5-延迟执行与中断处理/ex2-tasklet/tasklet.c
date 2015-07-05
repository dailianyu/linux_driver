#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

static struct  tasklet_struct my_tasklet;
unsigned long j1 ;
unsigned long j2 ;

static void tasklet_handler (unsigned long data)
{
	j2 = jiffies;
	printk(KERN_ALERT"tasklet_handler is running(j1 = %lu, j2 = %lu).\n",j1, j2);
}

static int __init test_init(void)
{
	tasklet_init(&my_tasklet,tasklet_handler,0);
	tasklet_schedule(&my_tasklet);
	j1 = jiffies;
	return 0;
}

static  void __exit test_exit(void)
{
	tasklet_kill(&my_tasklet);
	printk(KERN_ALERT"test_exit\n");
}
MODULE_LICENSE("GPL");

module_init(test_init);
module_exit(test_exit);

