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
#include <linux/timer.h> /*����timer.hͷ�ļ�*/
#include <asm/atomic.h> 
#include <linux/device.h>

MODULE_AUTHOR("longer zhou");
MODULE_LICENSE("Dual BSD/GPL");


uint devno_major = 0;
uint devno_minor = 0;
module_param(devno_major, uint, S_IRUGO);



static atomic_t c;/* һ�������˶����룿*/
struct timer_list s_timer; /*�豸Ҫʹ�õĶ�ʱ��*/
struct class *test_class; /*�豸��*/
struct device *test_device;/*�豸�ڵ�*/
struct cdev test_cdev;


/*��ʱ��������*/
static void second_timer_handle(unsigned long arg)
{
  printk(KERN_NOTICE "current jiffies is %lu,%d\n", jiffies,HZ);
  mod_timer(&s_timer,jiffies + HZ);
  atomic_inc(&c);
  
  
}

/*�ļ��򿪺���*/
int test_open(struct inode *inode, struct file *filp)
{
  /*��ʼ����ʱ��*/
  init_timer(&s_timer);
  s_timer.function = &second_timer_handle;
  s_timer.expires = jiffies + HZ;
  printk(KERN_NOTICE "second_open jiffies is %lu\n", jiffies);
  add_timer(&s_timer); /*��ӣ�ע�ᣩ��ʱ��*/
  
  atomic_set(&c,0); //������0

  return 0;
}
/*�ļ��ͷź���*/
int test_release(struct inode *inode, struct file *filp)
{
  del_timer(&s_timer);
  
  return 0;
}

/*globalfifo������*/
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

/*�ļ������ṹ��*/
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


	//step 1: �����豸��
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
	
	//step 2: ע���ַ��豸

	ret = register_cdev(devno);
	if (ret < 0) {
		printk(KERN_INFO "can't register char device\n");
		return -1;
	}

	//step 3 : �����豸�ڵ㡣
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
