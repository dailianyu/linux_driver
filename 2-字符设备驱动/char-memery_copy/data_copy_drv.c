
/* longer, <longer.zhou@gmail.com >*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/string.h>

#define TEST_IOCTL_CMD1 _IO('C',1)
#define TEST_IOCTL_CMD2 _IOR('C',2, int)
#define TEST_IOCTL_CMD3 _IOW('C',3, int)

#define USE_UDEV 1

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
	printk(KERN_INFO" test_write\n");

	char data[256];
	size_t num = (count > 256) ? 256 : count;

	int ret  = copy_from_user(data, buf,  num);

	// print the data
	data[num - ret] = '\0';
	printk(KERN_INFO" user have written %d bytes, and the data is:%s\n", 
		num - ret, data);
	
	return num - ret;
}

ssize_t test_read (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO"test_read\n");
	int len = strlen("123456");

	int ret ;
	if (ret = copy_to_user(buf, "123456", len) )
		return len - ret;
	else 
		return len;
}

long test_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;

	/*
	   access_ok. Checks if a pointer to a block of memory in user space is valid.
	   Returns true (nonzero) if the memory block may be valid, false (zero) if it is definitely invalid.
	   Note that, depending on architecture, this function probably just checks that the pointer 
	   is in the user space range - after calling this function, memory access functions may still 
	   return -EFAULT
	*/

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
	case TEST_IOCTL_CMD1:
		printk(KERN_INFO " TEST_IOCTL_CMD1\n");
		break;
	case TEST_IOCTL_CMD2:
	{
		int * __user argp = (int *)arg;
		printk(KERN_INFO " TEST_IOCTL_CMD2\n");	
		//*((int *)arg) = 1024;/* it's ugly .longer */
		err = put_user(1024, argp);
		break;
	}
	case TEST_IOCTL_CMD3:
	{
		int* __user argp = (int *)arg;
		int data;
		printk(KERN_INFO " TEST_IOCTL_CMD3\n");

		err = get_user(data, argp);
		
		printk(KERN_INFO"read a int(%d) from user space\n",data);
		break;
	}
	default:
		printk(KERN_INFO" undefined ioctl cmd\n");
		break;
	}

	return err;
}

struct file_operations test_fops = {
  .owner = THIS_MODULE,
  .open  = test_open,
  .release = test_release,
  .write = test_write,
  .read = test_read,
  .unlocked_ioctl = test_ioctl
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
	test_class = class_create(THIS_MODULE, "data_class");
	if (IS_ERR(test_class)) {
		printk(KERN_ERR "failed to create device class \n");
		cdev_del(&test_cdev);
		unregister_chrdev_region(devno , 1);
		return -1;
	}

	test_device = device_create(test_class,NULL,devno,NULL,"data_dev");
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

