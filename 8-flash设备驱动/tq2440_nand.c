/*   nandflash driver for tq2440, longer */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include <plat/regs-nand.h>
#include <plat/nand.h>

#define PA_NAND       0x4E000000
#define PA_NAND_SIZE  0x40

#define NFCONF_OFFSET    0x0
#define NFCONT_OFFSET    0x04
#define NFCMD_OFFSET     0x08
#define NFADDR_OFFSET    0x0C
#define NFDATA_OFFSET    0x10
#define NFECCD0_OFFSET   0x14
#define NFECCD1_OFFSET   0x18
#define NFECCD_OFFSET    0x1C
#define NFSTAT_OFFSET    0x20
#define NFESTAT0_OFFSET  0x24
#define NFESTAT1_OFFSET  0x28
#define NFMECC0_OFFSET   0x2C
#define NFMECC1_OFFSET   0x30
#define NFSECC_OFFSET    0x34
#define NFSBLK_OFFSET    0x38
#define NFEBLK_OFFSET    0x3C

static int hardware_ecc = 0;
struct nand_flash {
    struct mtd_info mtd ;
    struct nand_chip chip;
    void __iomem *baseaddr;
    struct resource *res;
    struct clk *clk;


    /* */
	/* timing information for controller, all times in nanoseconds */

	int	tacls;	/* time for active CLE/ALE to nWE/nOE */
	int	twrph0;	/* active time for nWE/nOE */
	int	twrph1;	/* time for release CLE/ALE from nWE/nOE inactive */
};

static struct nand_flash *nand = NULL;

static struct mtd_partition smdk_default_nand_part[] = {
        [0] = {
                .name   = "BootLoader",
                .size   = 0x00080000,   /* 256KB + 128KB = 384KB */
                .offset = 0,
        },
        [1] = {
                .name   = "Kernel",
                .size   = 0x00400000,   /* 4MB */
                .offset = 0x00080000,
        },
        [2] = {
                .name   = "Rootfs",
                .size   = 0x0FB80000,   /* 251.5 MB */
                //.size   = 0x03B80000,   /* 59.5MB */
                .offset = 0x00480000,
        }
};


/**
 * s3c2440_nand_select_chip - select the given nand chip
 * @mtd: The MTD instance for this chip.
 * @chip: The chip number.
 *
 * This is called by the MTD layer to either select a given chip for the
 * @mtd instance, or to indicate that the access has finished and the
 * chip can be de-selected.
 *
 * The routine ensures that the nFCE line is correctly setup, and any
 * platform specific selection code is called to route nFCE to the specific
 * chip.
 */
static void 
s3c2440_nand_select_chip(struct mtd_info *mtd, int chip)
{
    struct nand_flash *nandflash = NULL;
    unsigned long cur;
    
    nandflash = container_of(mtd, struct nand_flash, mtd);

    cur = readl(nandflash->baseaddr + NFCONT_OFFSET);
    
    if (chip == -1) 
        cur |= (1 << 1);
    else 
        cur &= ~(1 << 1);

    writel(cur, nandflash->baseaddr + NFCONT_OFFSET);
}

static void 
s3c2440_nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{

	struct nand_flash *nandflash = container_of(mtd, struct nand_flash, mtd);

	readsl(nandflash->baseaddr + NFDATA_OFFSET, buf, len >> 2);

	/* cleanup if we've got less than a word to do */
	if (len & 3) {
		buf += len & ~3;

		for (; len & 3; len--)
			*buf++ = readb(nandflash->baseaddr + NFDATA_OFFSET);
	}
}

static void 
s3c2440_nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	struct nand_flash *nandflash = container_of(mtd, struct nand_flash, mtd);

	writesl(nandflash->baseaddr + NFDATA_OFFSET, buf, len >> 2);

	/* cleanup any fractional write */
	if (len & 3) {
		buf += len & ~3;

		for (; len & 3; len--, buf++)
			writeb(*buf, nandflash->baseaddr + NFDATA_OFFSET);
	}
}

/* command and control functions */

static void 
s3c2440_nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{

    struct nand_flash *nandflash = container_of(mtd, struct nand_flash, mtd);

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		writeb(cmd, nandflash->baseaddr + NFCMD_OFFSET);
	else
		writeb(cmd, nandflash->baseaddr + NFADDR_OFFSET);
}

/* s3c2440_nand_devready()
 *
 * returns 0 if the nand is busy, 1 if it is ready
*/
static int s3c2440_nand_devready(struct mtd_info *mtd)
{
	struct nand_flash *nandflash = container_of(mtd, struct nand_flash, mtd);
	return readb(nandflash->baseaddr + NFSTAT_OFFSET) & (1 << 0);
}

/* init nand flash controller .longer */
int
nand_inithw(struct nand_flash *n )
{
    unsigned long clk_rate =  clk_get_rate(n->clk);
    int tacls, twrph0, twrph1;
    unsigned long cfg, mask, set;
    unsigned long flags;

    tacls = n->tacls * (clk_rate/1000) / 1000000;
    twrph0 = n->twrph0 *(clk_rate/1000) / 1000000 - 1;
    twrph1 = n->twrph1 *(clk_rate/1000) / 1000000 - 1;

    printk(KERN_INFO"%s: tacls:%d twrph0: %d twrph1: %d\n",
                    __func__, tacls, twrph0, twrph1);
    if (tacls > 3 || twrph0 > 7 || twrph1 >7)
        return -1;

    mask = (3 << 12) | (7 << 8) | (7 << 4);
    set = (tacls << 12) | (twrph0 << 8) | (twrph1 << 4);

	local_irq_save(flags);

	cfg = readl(n->baseaddr + NFCONF_OFFSET);
	cfg &= ~mask;
	cfg |= set;
	writel(cfg, n->baseaddr + NFCONF_OFFSET);

	local_irq_restore(flags);
    
    writel((1 << 0), n->baseaddr + NFCONT_OFFSET);

    return 0;
    
}

static int 
s3c2440_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	struct nand_flash *nandflash = container_of(mtd, struct nand_flash,mtd);
	unsigned long ecc = readl(nandflash->baseaddr + NFMECC0_OFFSET);

	ecc_code[0] = ecc;
	ecc_code[1] = ecc >> 8;
	ecc_code[2] = ecc >> 16;

	printk("%s: returning ecc %06lx\n", __func__, ecc & 0xffffff);

	return 0;
}
static void 
s3c2440_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_flash *nandflash = container_of(mtd, struct nand_flash,mtd);
	unsigned long ctrl;

	ctrl = readl(nandflash->baseaddr + NFCONT_OFFSET);
	writel(ctrl | (1 << 4), nandflash->baseaddr + NFCONT_OFFSET);
}
/* ECC handling functions */

static int 
s3c2440_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{

	unsigned int diff0, diff1, diff2;
	unsigned int bit, byte;

	printk("%s(%p,%p,%p,%p)\n", __func__, mtd, dat, read_ecc, calc_ecc);

	diff0 = read_ecc[0] ^ calc_ecc[0];
	diff1 = read_ecc[1] ^ calc_ecc[1];
	diff2 = read_ecc[2] ^ calc_ecc[2];

	printk("%s: rd %02x%02x%02x calc %02x%02x%02x diff %02x%02x%02x\n",
		 __func__,
		 read_ecc[0], read_ecc[1], read_ecc[2],
		 calc_ecc[0], calc_ecc[1], calc_ecc[2],
		 diff0, diff1, diff2);

	if (diff0 == 0 && diff1 == 0 && diff2 == 0)
		return 0;		/* ECC is ok */

	/* sometimes people do not think about using the ECC, so check
	 * to see if we have an 0xff,0xff,0xff read ECC and then ignore
	 * the error, on the assumption that this is an un-eccd page.
	 */
	/*if (read_ecc[0] == 0xff && read_ecc[1] == 0xff && read_ecc[2] == 0xff
	    && info->platform->ignore_unset_ecc)
		return 0;*/

	/* Can we correct this ECC (ie, one row and column change).
	 * Note, this is similar to the 256 error code on smartmedia */

	if (((diff0 ^ (diff0 >> 1)) & 0x55) == 0x55 &&
	    ((diff1 ^ (diff1 >> 1)) & 0x55) == 0x55 &&
	    ((diff2 ^ (diff2 >> 1)) & 0x55) == 0x55) {
		/* calculate the bit position of the error */

		bit  = ((diff2 >> 3) & 1) |
		       ((diff2 >> 4) & 2) |
		       ((diff2 >> 5) & 4);

		/* calculate the byte position of the error */

		byte = ((diff2 << 7) & 0x100) |
		       ((diff1 << 0) & 0x80)  |
		       ((diff1 << 1) & 0x40)  |
		       ((diff1 << 2) & 0x20)  |
		       ((diff1 << 3) & 0x10)  |
		       ((diff0 >> 4) & 0x08)  |
		       ((diff0 >> 3) & 0x04)  |
		       ((diff0 >> 2) & 0x02)  |
		       ((diff0 >> 1) & 0x01);

		printk( "correcting error bit %d, byte %d\n",
			bit, byte);

		dat[byte] ^= (1 << bit);
		return 1;
	}

	/* if there is only one bit difference in the ECC, then
	 * one of only a row or column parity has changed, which
	 * means the error is most probably in the ECC itself */

	diff0 |= (diff1 << 8);
	diff0 |= (diff2 << 16);

	if ((diff0 & ~(1<<fls(diff0))) == 0)
		return 1;

	return -1;
}
static int
nand_chip_init(struct mtd_info *mtd, struct nand_chip *chip)
{
    struct nand_flash *nandflash = container_of(mtd, struct nand_flash,mtd);
    
    chip->read_buf = s3c2440_nand_read_buf;
    chip->write_buf = s3c2440_nand_write_buf;
    chip->select_chip = s3c2440_nand_select_chip;
    chip->chip_delay = 50;
    chip->options	   = 0;

    //chip->controller = ;

    chip->IO_ADDR_W = nandflash->baseaddr + NFDATA_OFFSET;
    chip->IO_ADDR_R = chip->IO_ADDR_W;

    chip->cmd_ctrl  = s3c2440_nand_hwcontrol;
    chip->dev_ready = s3c2440_nand_devready;

    mtd->priv = chip;
    mtd->owner = THIS_MODULE;

    if (hardware_ecc) {
        chip->ecc.mode	    = NAND_ECC_HW;
		chip->ecc.correct   = s3c2440_nand_correct_data;
		chip->ecc.hwctl     = s3c2440_nand_enable_hwecc;
		chip->ecc.calculate = s3c2440_nand_calculate_ecc;
		
    } else 
        chip->ecc.mode	= NAND_ECC_SOFT;

    return 0;

}

int __init
my_nand_init(void )
{
	int err = 0;
    struct nand_flash *nandflash = NULL;

	printk(KERN_INFO"##################%s########################\n",__func__);
	nandflash = kmalloc(sizeof(*nandflash), GFP_KERNEL);
	if (!nandflash) {
		printk(KERN_INFO"%s : kmalloc failed\n",__func__);
		err = -ENOMEM;
		goto out;
	}
	memset(nandflash, 0, sizeof(*nandflash));

	nand = nandflash;

    nandflash->clk = clk_get(NULL, "nand");
    if (IS_ERR(nandflash->clk)) {
        printk(KERN_INFO"%s: clk_get failed \n",__func__);
        err = -ENOENT;
        goto out;
    }
    clk_enable(nandflash->clk);
    

    nandflash->res = request_mem_region(PA_NAND, PA_NAND_SIZE, "s3c2440-nand");
    if (nandflash->res == NULL) {
        printk(KERN_INFO"%s: request_mem_region failed \n",__func__);
        err = -ENOENT;
        goto out;
    }

    nandflash->baseaddr = ioremap(PA_NAND, PA_NAND_SIZE);
    if (nandflash->baseaddr == NULL) {
        printk(KERN_INFO"%s: ioremap failed \n",__func__);
        err = -ENOENT;
        goto out;
    }

    /* */
    nandflash->tacls = 25;
    nandflash->twrph0 = 55;
    nandflash->twrph1 = 40;

    if (nand_inithw(nandflash)) {
        printk(KERN_INFO"%s: ioremap failed \n",__func__);
        err = -ENOENT;
        goto out;
    }


	if (nand_chip_init(&nandflash->mtd, &nandflash->chip)) {
        printk(KERN_INFO"%s: nand_chip_init failed \n",__func__);
        err = -ENOENT;
        goto out;
	}

    if (nand_scan(&nandflash->mtd, 1)) {
        printk(KERN_INFO"%s: nand_scan failed \n",__func__);
        err = -ENOENT;
        goto out;
    }

    err = add_mtd_partitions(&nandflash->mtd, 
                        smdk_default_nand_part, 
                        ARRAY_SIZE(smdk_default_nand_part));
	
out:
	printk(KERN_INFO"##################%s########################\n",__func__);
	return err;
	
}

void __exit
my_nand_exit(void)
{
    struct nand_flash *nandflash = nand;
    if (!nandflash)
        return ;
        
    nand_release(&nandflash->mtd);

    if (nandflash->baseaddr) {
        iounmap(nandflash->baseaddr);
        kfree(nandflash->baseaddr);
        nandflash->baseaddr = NULL;
    }

    if (nandflash->res) {
        release_resource(nandflash->res);
        kfree(nandflash->res);
        nandflash->res = NULL;
    }

    kfree(nandflash);
    nand = NULL;
}


module_init(my_nand_init);
module_exit(my_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("longer <longer.zhou@gmail.com>");
MODULE_DESCRIPTION("TQ2440 MTD NAND driver");

