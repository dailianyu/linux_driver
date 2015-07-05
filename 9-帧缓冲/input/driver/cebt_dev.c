#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <asm/irq.h>
#include <asm/io.h>


#define DRIVER_VERSION "1.0"
#define DRIVER_NAME "cebt_dev"
#define BT_MOUSE_NAME "bt_mouse"
#define BT_KB_NAME "bt_key"

static struct input_dev *btmouse_dev, *btkb_dev;


static int btmouse_open(struct input_dev *dev)
{
        return 0;
}
static void btmouse_close(struct input_dev *dev)
{
	return;
}

static int btkb_open(struct input_dev *dev)
{
        return 0;
}
static void btkb_close(struct input_dev *dev)
{
	return;
}


static int __init bt_init(void)
{

	int error1,error2;
	int i;	
	
	btmouse_dev = input_allocate_device();
	//btmouse_dev->dev = btmouse_dev;
	btmouse_dev->id.bustype = BUS_USB;
	btmouse_dev->name = BT_MOUSE_NAME;   

	btmouse_dev->phys = "usb-0000:01:0d.1-1/input0";
	btmouse_dev->id.vendor = 0x0001;
	btmouse_dev->id.product = 0x0002;
	btmouse_dev->id.version = 0x0001;

	
	btmouse_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REL) |BIT(EV_SYN) |BIT(EV_MSC);
	btmouse_dev->relbit[0] = BIT(REL_X) | BIT(REL_Y) |BIT(REL_RX) | BIT(REL_RY) | BIT(REL_RZ);
	btmouse_dev->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_MIDDLE) | BIT_MASK(BTN_RIGHT)|BIT_MASK(MSC_SCAN);	
	/*set_bit(EV_KEY,btmouse_dev->evbit);
	set_bit(EV_REL,btmouse_dev->evbit);	
	set_bit(EV_SYN,btmouse_dev->evbit);
	set_bit(REL_X,btmouse_dev->relbit);
	set_bit(REL_Y,btmouse_dev->relbit);
	set_bit(BTN_LEFT,btmouse_dev->keybit);*/

	btmouse_dev->open = btmouse_open;
	btmouse_dev->close = btmouse_close;
	
	error1 = input_register_device(btmouse_dev);
	if (error1) { 
		printk("bt_mouse: Failed to register device\n");
		goto err_free_dev1; 
	}

	btkb_dev = input_allocate_device();
	btkb_dev->id.bustype = BUS_USB;
	btkb_dev->name = BT_KB_NAME;   

	btkb_dev->phys = "usb-0000:01:0d.1-1/input0";
	btkb_dev->id.vendor = 0x0001;
	btkb_dev->id.product = 0x0002;
	btkb_dev->id.version = 0x0001;


	btkb_dev->evbit[0] = BIT(EV_KEY) ;
	for (i = 0; i < 0xF6; i++)
		set_bit(i, btkb_dev->keybit);


	btkb_dev->open = btkb_open;
	btkb_dev->close = btkb_close;
	
	error2 = input_register_device(btkb_dev);
	if (error2) { 
		printk("bt_keyboard: Failed to register device\n");
		goto err_free_dev2; 
	}


	return 0;
err_free_dev1:
	input_free_device(btmouse_dev);
	return error1;

err_free_dev2:
	input_free_device(btkb_dev);
	return error2;

}

static void __exit bt_exit(void)
{
	input_unregister_device(btmouse_dev);
	input_unregister_device(btkb_dev);
	return ;	
}

module_init(bt_init);
module_exit(bt_exit);

MODULE_AUTHOR("zhuping");
MODULE_DESCRIPTION("bluetooth mouse or keyboard");
MODULE_LICENSE("GPL");
