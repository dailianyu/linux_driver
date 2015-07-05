/*longer zhou(longer.zhou@gmail.com)*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/timer.h> /*包括timer.h头文件*/
#include <asm/atomic.h> 
#include <linux/device.h>

MODULE_AUTHOR("longer zhou");
MODULE_LICENSE("Dual BSD/GPL");


uint devno_major = 0;
uint devno_minor = 0;
module_param(devno_major, uint, S_IRUGO);



static atomic_t c;/* 一共经历了多少秒？*/
struct timer_list s_timer; /*设备要使用的定时器*/
struct class *test_class; /*设备类*/
struct device *test_device;/*设备节点*/
struct cdev test_cdev;


/*定时器处理函数*/
static void second_timer_handle(unsigned long arg)
{
  printk(KERN_NOTICE "current jiffies is %lu,%d\n", jiffies,HZ);
  mod_timer(&s_timer,jiffies + HZ);
  atomic_inc(&c);
  
  
}

/*文件打开函数*/
int test_open(struct inode *inode, struct file *filp)
{
  /*初始化定时器*/
  init_timer(&s_timer);
  s_timer.function = &second_timer_handle;
  s_timer.expires = jiffies + HZ;
  printk(KERN_NOTICE "second_open jiffies is %lu\n", jiffies);
  add_timer(&s_timer); /*添加（注册）定时器*/
  
  atomic_set(&c,0); //计数清0

  return 0;
}
/*文件释放函数*/
int test_release(struct inode *inode, struct file *filp)
{
  del_timer(&s_timer);
  
  return 0;
}

/*globalfifo读函数*/
static ssize_t test_read(struct file *filp, char __user *buf, size_t count,
  loff_t *ppos)
{  
  int counter;
  
  counter = atomic_read(&c);
  if(put_user(counter, (int*)buf))
  	return - EFAULT;
  else
  	return sizeof(unsigned int);  
}

/*文件操作结构体*/
static const struct file_operations test_fops =
{
  .owner = THIS_MODULE, 
  .open = test_open, 
  .release = test_release,
  .read = test_read,
};

int
register_cdev(dev_t devno)
{
	int ret;
	
	cdev_init(&test_cdev, &test_fops);
	test_cdev.owner = THIS_MODULE;

	ret = cdev_add(&test_cdev, devno, 1);
	return ret;
}


static int __init 
timer_test_init(void)
{
	dev_t devno;
	int ret;


	//step 1: 申请设备号
	if (devno_major > 0) {
		devno = MKDEV(devno_major, devno_minor);
		ret = register_chrdev_region(devno , 1, "timer_test");
	} else {
		ret = alloc_chrdev_region(&devno, 0, 1, "timer_test");
		
	}

	if (ret < 0) {
		printk(KERN_INFO"can't register char device driver\n");
		return -1;
	}

	devno_major = MAJOR(devno); 
	devno_minor = MINOR(devno);
	
	//step 2: 注册字符设备

	ret = register_cdev(devno);
	if (ret < 0) {
		printk(KERN_INFO "can't register char device\n");
		return -1;
	}

	//step 3 : 创建设备节点。
	test_class = class_create(THIS_MODULE, "test_class");
	if (IS_ERR(test_class)) {
		printk(KERN_ERR "failed to create device class \n");
		cdev_del(&test_cdev);
		unregister_chrdev_region(devno , 1);
		return -1;
	}

	test_device = device_create(test_class,NULL,devno,NULL,"test_timer");


	return 0;
}

static void __exit
timer_test_exit(void)
{
	dev_t devno = MKDEV(devno_major, devno_minor);

	cdev_del(&test_cdev);
	unregister_chrdev_region(devno , 1);

	device_destroy(test_class, devno);
	class_destroy(test_class);

}


module_init(timer_test_init);
module_exit(timer_test_exit);
