
/* 	
	
	longer <longer.zhou@gmail.com>
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define GET_AVAIL_READ_NUM  _IOR('M',1, int)
#define GET_AVAIL_WRITE_NUM  _IOR('M',2, int)


#define MAX_BUFFER_LEN   1024
struct my_pipe_buffer {
	unsigned int rpos; /*读位置*/
	unsigned int wpos;/*写位置*/
	unsigned int max_len; /*缓冲区的总长度*/
	unsigned int avail_len; /* 可读的长度*/
	unsigned char *data; /*缓冲区指针*/

	struct mutex lock;
	wait_queue_head_t r_wait;   
};

struct my_pipe_buffer *pipebuf = NULL;
struct class *my_pipe_class = NULL; /*设备类*/
struct device *my_pipe_device = NULL; /*设备节点*/

struct cdev test_cdev;
uint devno_major = 0;
uint devno_minor = 0;
module_param(devno_major, uint, S_IRUGO);

MODULE_LICENSE("GPL");



struct my_pipe_buffer*
pipe_buffer_init(size_t max_len)
{
	struct my_pipe_buffer *p = NULL;
	p = kmalloc(sizeof(struct my_pipe_buffer), GFP_KERNEL);
	p->data = kmalloc(max_len, GFP_KERNEL);
	p->rpos = p->wpos = p->avail_len = 0;
	p->max_len = max_len;

	mutex_init(&p->lock);
	init_waitqueue_head(&p->r_wait);
	return p;
}


static int my_pipe_open (struct inode *inode, struct file *file)
{
	printk (KERN_INFO "Hey! device opened\n");
	return 0;
}

static int my_pipe_release (struct inode *inode, struct file *file)
{
	printk (KERN_INFO "Hmmm! device closed\n");
	return 0;
}

ssize_t my_pipe_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO" pipe_write\n");
	int w, w1 = 0;
	struct my_pipe_buffer *pbuf = pipebuf;

	if (!pbuf)
		return -1;

	mutex_lock(&pbuf->lock);
	w = min(count, pbuf->max_len - pbuf->avail_len);

	if (pbuf->wpos < pbuf->rpos) {
		w1 = w - copy_from_user(pbuf->data + pbuf->wpos, buf, w);
		pbuf->wpos += w1;
		
	} else  {
		if (pbuf->max_len - pbuf->wpos > w) {
			w1 = w - copy_from_user(pbuf->data + pbuf->wpos,buf, w);
			pbuf->wpos += w1;
		} else {
			w1 = (pbuf->max_len - pbuf->wpos) -
				copy_from_user(pbuf->data + pbuf->wpos, buf, pbuf->max_len - pbuf->wpos);

			if (w1 != pbuf->max_len - pbuf->wpos) 
				pbuf->wpos += w1;
			else {				
				pbuf->wpos = 0;

				int remain = w - (pbuf->max_len - pbuf->wpos);
				if (remain) {
					int w2;
					w2 = remain - copy_from_user(pbuf->data, buf + w - remain, remain);
					pbuf->wpos += w2;
					w1 += w2;
				}
			}
		}
	}
	pbuf->avail_len += w1;

	if (pbuf->avail_len)
		wake_up_interruptible(&pbuf->r_wait); /* wake up 'r_wait' wait queue*/
	
	mutex_unlock(&pbuf->lock);
	return w1;
}

ssize_t my_pipe_read (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int r, r1 = 0;
	struct my_pipe_buffer *pbuf = pipebuf;

	printk(KERN_INFO"pipe_read\n");

	if (!pbuf)
		return -1;	

	mutex_lock(&pbuf->lock);
	r = min(count, pbuf->avail_len);

	if ((r == 0) && !(filp->f_flags & O_NONBLOCK)) {	
		/*have no data and do not set 'O_NONBLOCK' bit. it should be blocked. longer */
		DEFINE_WAIT(wait);

		add_wait_queue(&pbuf->r_wait, &wait);
		
		set_current_state(TASK_INTERRUPTIBLE);

		mutex_unlock(&pbuf->lock); /* unlock */
		
		schedule();

		remove_wait_queue(&pbuf->r_wait, &wait);
		
		if (signal_pending(current)) {
			/* interrupt by a signal */
			return -ERESTARTSYS;
		}

		mutex_lock(&pbuf->lock);
		r = min(count, pbuf->avail_len);
	
	}
	

	if (pbuf->rpos < pbuf->wpos) {
		r1 = r - copy_to_user(buf , pbuf->data + pbuf->rpos, r);
		pbuf->rpos += r1;
	} else if (pbuf->rpos > pbuf->wpos){

		if (pbuf->max_len - pbuf->rpos > r) {
			r1 = r - copy_to_user(buf , pbuf->data + pbuf->rpos, r);
			pbuf->rpos += r1;
		} else {
			r1 =  (pbuf->max_len - pbuf->rpos) - 
				copy_to_user(buf , pbuf->data + pbuf->rpos, pbuf->max_len - pbuf->rpos);
			if (r1 != pbuf->max_len - pbuf->rpos)
				pbuf->rpos = r1;
			else {
			pbuf->rpos = 0;

			int remain = r - (pbuf->max_len - pbuf->rpos);
			if (remain) {
					int r2;
					r2 = remain - copy_to_user(buf + r - remain , pbuf->data + pbuf->rpos, remain);
					pbuf->rpos += r2;
					r1 += r2;
				}
			}
			
		}		
	}

	pbuf->avail_len -= r1;
	
	mutex_unlock(&pbuf->lock);
	return r1;
}

long my_pipe_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
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
	case GET_AVAIL_READ_NUM :
	{
		printk(KERN_INFO " GET_AVAIL_LEN\n");	
		int * __user argp = (int *)arg;
		int avail_len = pipebuf->avail_len;
		
		err = put_user(avail_len, argp);
		
		break;
	}
	case GET_AVAIL_WRITE_NUM :
	{
		printk(KERN_INFO " GET_AVAIL_LEN\n");	
		int * __user argp = (int *)arg;
		int avail_len = pipebuf->max_len - pipebuf->avail_len;
		
		err = put_user(avail_len, argp);
		
		break;
	}
	default :
		break;
	}

	return err;
}
struct file_operations my_pipe_fops = {
  .owner = THIS_MODULE,
  .open  = my_pipe_open,
  .release = my_pipe_release,
  .write = my_pipe_write,
  .read = my_pipe_read,
  .unlocked_ioctl = my_pipe_ioctl
};

int
register_cdev(dev_t devno)
{
	int ret;
	
	cdev_init(&test_cdev, &my_pipe_fops);
	test_cdev.owner = THIS_MODULE;

	ret = cdev_add(&test_cdev, devno, 1);
	return ret;
}

static int __init 
my_pipe_init(void )
{
	dev_t devno;
	int ret;


	//step 1: 申请设备号
	if (devno_major > 0) {
		devno = MKDEV(devno_major, devno_minor);
		ret = register_chrdev_region(devno , 1, "mypipe41");
	} else {
		ret = alloc_chrdev_region(&devno, 0, 1, "mypipe41");
		
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

	//step 3:  创建设备节点
	my_pipe_class  = class_create(THIS_MODULE, "my_pipe_class41");
	if (IS_ERR(my_pipe_class)) {
		printk(KERN_ERR "failed to create device class \n");
		cdev_del(&test_cdev);
		unregister_chrdev_region(devno , 1);
		return -1;
	}
	my_pipe_device = device_create(my_pipe_class,NULL, devno, NULL,"mypipe41");


	pipebuf = pipe_buffer_init(MAX_BUFFER_LEN);

	return 0;
}

static void __exit
my_pipe_exit(void)
{
	dev_t devno = MKDEV(devno_major, devno_minor);

	cdev_del(&test_cdev);
	unregister_chrdev_region(devno , 1);


	device_destroy(my_pipe_class, devno);
	class_destroy(my_pipe_class);
	if (pipebuf) {
		if (pipebuf->data)
			kfree(pipebuf->data);
		kfree(pipebuf);
	}
}


module_init(my_pipe_init);
module_exit(my_pipe_exit);

