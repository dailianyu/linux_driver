


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/page.h>


#define PAGE_COUNT   16
#define SHM_SIZE   (PAGE_COUNT * PAGE_SIZE)
unsigned long shm_start = 0;
unsigned int order = 4;


struct class *test_class = NULL;
struct device *test_device = NULL;

struct cdev test_cdev;
uint devno_major = 0;
uint devno_minor = 0;
module_param(devno_major, uint, S_IRUGO);


MODULE_LICENSE("GPL");

static void 
vma_open(struct vm_area_struct * area)
{
	printk("vma_open\n");
}
static void 
vma_close(struct vm_area_struct * area)
{
	printk("vma_close\n");
}
static int 
vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	unsigned long page_num = vmf->pgoff + vma->vm_pgoff;
	unsigned long page_addr = page_num * PAGE_SIZE + shm_start;

	printk("page_addr = %p\n", page_addr);
	
	vmf->page = virt_to_page(page_addr);
	if (!vmf->page) {
		printk("error vma_fault \n");
		return -1;
	}
	get_page(vmf->page);/* 增加引用计数*/
	return 0;
}

static struct vm_operations_struct vma_ops = {
	.open = vma_open,
	.close = vma_close,
	.fault = vma_fault
};



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
	return 0;
}

ssize_t test_read (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO"test_read\n");
	return 0;
}

int test_mmap (struct file *filp, struct vm_area_struct *vma)
{
    printk(KERN_INFO "test_mmap\n");

	/* 先判断该vma范围是否合理。longer*/
	unsigned long want_size = vma->vm_end - vma->vm_start;
	if (vma->vm_pgoff * PAGE_SIZE + want_size > SHM_SIZE) {
		printk(" go beyond the mmap range(%d)\n", SHM_SIZE);
		return -1;
	}
	

	#if 0
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, 
                        vma->vm_end - vma->vm_start, 
                        vma->vm_page_prot)) {
        return -EAGAIN;
    }
	#else
	vma->vm_ops = &vma_ops;
	
	//vma->vm_flags &= ~VM_IO;	/* not I/O memory */
	//vma->vm_flags |= VM_RESERVED;	/* avoid to swap out this VMA */
	//vma_open(vma);

	

	#endif

    return 0;
}

struct file_operations test_fops = {
    .owner = THIS_MODULE,
    .open  = test_open,
    .release = test_release,
    .write = test_write,
    .read = test_read,
    .mmap = test_mmap
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


	shm_start = __get_free_pages(__GFP_ZERO, order);
	if (!shm_start) {
		/*some errors occured */
		return -1;
	}

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

    /* get the major and minor number of device for using in function 'char_exit'. longer */
	devno_major = MAJOR(devno);  
    devno_minor = MINOR(devno);
	
	//step 2: 注册字符设备

	ret = register_cdev(devno);
	if (ret < 0) {
		printk(KERN_INFO "can't register char device\n");
		return -1;
	}

    //step 3
    test_class = class_create(THIS_MODULE, "test");
    if (IS_ERR(test_class)) {
        printk(KERN_ERR "failed to create device class\n");
        cdev_del(&test_cdev);
        unregister_chrdev_region(devno, 1);
        return -1;
    }
    test_device = device_create(test_class, NULL, devno,NULL,"test");

	

	return 0;
}

static void __exit
char_exit(void)
{
	dev_t devno = MKDEV(devno_major, devno_minor);

	cdev_del(&test_cdev);
	unregister_chrdev_region(devno , 1);

    device_destroy(test_class, devno);
    class_destroy(test_class);

	if (shm_start)
		free_pages(shm_start, order);
}

module_init(char_init);
module_exit(char_exit);
