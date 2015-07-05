
/*longer zhou(longer.zhou@gmail.com)*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define USE_UDEV  1

#ifdef USE_UDEV
struct class *test_class = NULL;
struct device *test_device = NULL;
#endif

struct cdev test_cdev;
uint devno_major = 0;
uint devno_minor = 0;
module_param(devno_major, uint, S_IRUGO);

MODULE_LICENSE("GPL");

static int test_open (struct inode *inode, struct file *file)
{
	printk (KERN_INFO "Hey! device opened\n");
	return 0;
}

static int test_release (struct inode *inode, struct file *file)
{
	printk (KERN_INFO "Hmmm! device closed\n");
	return 0;
}

ssize_t test_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO" test_write (buf = %p, count = %d)\n", buf, count);
	return 0;
}

ssize_t test_read (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO"test_read (buf = %p, count = %d)\n", buf, count);
	return 0;
}

struct file_operations test_fops = {
  .owner = THIS_MODULE,
  .open  = test_open,
  .release = test_release,
  .write = test_write,
  .read = test_read
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
char_init(void)
{
	dev_t devno;
	int ret;

	//step 1: 申请设备号
	if (devno_major > 0) {
		devno = MKDEV(devno_major, devno_minor);
		ret = register_chrdev_region(devno , 1, "char_test");
	} else {
		ret = alloc_chrdev_region(&devno, 0, 1, "char_test");
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

	//step 3 : 创建设备节点。有两种方法，可以用mknod
	/*如果用mknod命令手动创建节点，则此处不需要加代码*/
	//也可以用udev(用udev的方法，需要调用class_create创建一个设备类，
	// 然后在这个设备类的基础上调用device_create创建一个设备节点)
	#ifdef USE_UDEV
	test_class = class_create(THIS_MODULE, "test_class");
	if (IS_ERR(test_class)) {
		printk(KERN_ERR "failed to create device class \n");
		cdev_del(&test_cdev);
		unregister_chrdev_region(devno , 1);
		return -1;
	}

	test_device = device_create(test_class,NULL,devno,NULL,"test_dev");
	#endif

	return 0;
}

static void __exit
char_exit(void)
{
	dev_t devno = MKDEV(devno_major, devno_minor);

	cdev_del(&test_cdev);
	unregister_chrdev_region(devno , 1);

	#ifdef USE_UDEV
	device_destroy(test_class, devno);
	class_destroy(test_class);
	#endif
}

module_init(char_init);
module_exit(char_exit);

