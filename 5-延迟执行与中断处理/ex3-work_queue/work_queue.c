#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
//#include <asm/atomic.h>

MODULE_AUTHOR("longer zhou<longer.zhou@gmail.com>");
MODULE_LICENSE("GPL");


static struct timer_list timer;
struct work_struct test_work;
static struct workqueue_struct *test_workqueue;

unsigned long j1;
unsigned long j2;

void do_work(void *);


static void test_timer_fn(unsigned long arg)
{
	timer.expires += 2 * HZ;
	add_timer(&timer);

	if (queue_work(test_workqueue, &test_work)) {
		j1 = jiffies;
	}
	
}


void do_work(void *arg)
{
	j2 = jiffies;
	printk(KERN_INFO"do_work is running. (j1 = %lu, j2 = %lu)\n",j1, j2);
}

int wq_init(void)
{

    printk("jiffies: %lu\n", jiffies);
	
    init_timer(&timer);
 
	timer.function = test_timer_fn;
    timer.data = (unsigned long)(&timer);
    timer.expires = jiffies + 2 * HZ;
    add_timer(&timer);

    test_workqueue = create_singlethread_workqueue("test-wq");

	INIT_WORK(&test_work, do_work);
	
    printk("test-wq init success\n");

	return 0;
}

void wq_exit(void)
{
    del_timer(&timer);
    destroy_workqueue(test_workqueue);
    printk("wq exit \n");
}

module_init(wq_init);
module_exit(wq_exit);

