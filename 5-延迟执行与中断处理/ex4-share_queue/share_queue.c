#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <asm/atomic.h>

MODULE_AUTHOR("longer zhou<longer.zhou@gmail.com>");
MODULE_LICENSE("GPL");

struct timer_data {
        struct timer_list timer;
        //struct workqueue_struct *work_queue;
        unsigned long prev_jiffies;
        unsigned int loops;
};

struct timer_data test_data;

void do_work(void *);
DECLARE_WORK(test_work, do_work);

//static struct workqueue_struct *test_workqueue;
atomic_t wq_run_times;

unsigned int failed_cnt = 0;

void test_timer_fn(unsigned long arg)
{
        struct timer_data *data = (struct timer_data *)arg;

        unsigned long j = jiffies;
        data->timer.expires += 2 * HZ;
        data->prev_jiffies = j;
        add_timer(&data->timer);

        if (schedule_work(&test_work) == 0) {
                printk("Timer (0) add work queue failed\n");
                (*(&failed_cnt))++;
        }
        data->loops++;
        printk("timer-0 loops: %u\n", data->loops);
}


void do_work(void *arg)
{
        atomic_inc(&wq_run_times);
        printk("====work queue run times: %u====\n", atomic_read(&wq_run_times));
        printk("====failed count: %u====\n", *(&failed_cnt));
}

int wq_init(void)
{
        unsigned long j = jiffies;

        printk("jiffies: %lu\n", jiffies);
        atomic_set(&wq_run_times, 0);

        init_timer(&test_data.timer);
        test_data.loops = 0;
        test_data.prev_jiffies = j;
        test_data.timer.function = test_timer_fn;
        test_data.timer.data = (unsigned long)(&test_data);
        test_data.timer.expires = j + 2 * HZ;
        add_timer(&test_data.timer);

        //test_workqueue = create_singlethread_workqueue("test-wq");
        printk("test-wq init success\n");
        printk("jiffies: %lu\n", jiffies);
        return 0;
}

void wq_exit(void)
{
        del_timer(&test_data.timer);
        //destroy_workqueue(test_workqueue);
        printk("wq exit success\n");
}

module_init(wq_init);
module_exit(wq_exit);

