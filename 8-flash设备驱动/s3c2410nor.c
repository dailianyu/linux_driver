#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif


#define WINDOW_ADDR 0x0000000      		/* NOR FLASH物理地址 */
#define WINDOW_SIZE 0x0200000         /* NOR FLASH大小 */
#define BUSWIDTH    2
/* 探测的接口类型，可以是"cfi_probe", "jedec_probe", "map_rom", NULL }; */
//#define PROBETYPES { "cfi_probe", "jedec_probe","amd_flash",  NULL }
#define PROBETYPES { "amd_flash",  NULL }


#define MSG_PREFIX "S3C2410-NOR:"   /* prefix for our printk()'s */
#define MTDID      "s3c2410-nor"    /* for mtdparts= partitioning */

static struct mtd_info *mymtd;

struct map_info s3c2410nor_map =  // map_info
{
  .name = "NOR flash on S3C2410",
  .size = WINDOW_SIZE,
  .bankwidth = BUSWIDTH,
  .phys = WINDOW_ADDR,
};

#ifdef CONFIG_MTD_PARTITIONS
  /* MTD分区信息  */
  static struct mtd_partition static_partitions[] =
  {
    {
      .name = "BootLoader", .size = 0x040000, .offset = 0x0  //bootloader存放的区域
    } ,
    {
      .name = "jffs2", .size = 0x0100000, .offset = 0x40000 //内核映像存放的区域
    }   
  };
#endif

//static const char *part_probes[] __initdata = {"cmdlinepart", "RedBoot", NULL};
static int mtd_parts_nb = 0;
static struct mtd_partition *mtd_parts = 0;

int __init init_s3c2410nor(void)
{
  static const char *rom_probe_types[] = PROBETYPES;
  const char **type;
  const char *part_type = 0;

	printk(KERN_NOTICE MSG_PREFIX "0x%08x at 0x%08x\n", WINDOW_SIZE, WINDOW_ADDR);
  s3c2410nor_map.virt = (unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE);//物理->虚拟地址
  //printk("vir=%x\n",s3c2410nor_map.virt);
  
  if (!s3c2410nor_map.virt)
  {
    printk(MSG_PREFIX "failed to ioremap\n");
    return  - EIO;
  }

  simple_map_init(&s3c2410nor_map);

  mymtd = 0;
  type = rom_probe_types;
  for (; !mymtd &&  *type; type++)
  {         
    mymtd = do_map_probe(*type, &s3c2410nor_map);//探测NOR FLASH
  }
  
  if (mymtd)
  {
      mymtd->owner = THIS_MODULE; 

      mtd_parts = static_partitions;
      mtd_parts_nb = ARRAY_SIZE(static_partitions);
      part_type = "static";

	    add_mtd_device(mymtd);
  	  if (mtd_parts_nb == 0)
    	  printk(KERN_NOTICE MSG_PREFIX "no partition info available\n");
    	else
    	{
      	printk(KERN_NOTICE MSG_PREFIX "using %s partition definition\n",part_type);
      	add_mtd_partitions(mymtd, mtd_parts, mtd_parts_nb);//添加分区信息
    	}
    	return 0;
  }

  iounmap((void*)s3c2410nor_map.virt);
  return  - ENXIO;
}

static void __exit cleanup_s3c2410nor(void)
{
  if (mymtd)
  {
    del_mtd_partitions(mymtd);  //删除分区
    del_mtd_device(mymtd);   //删除设备
    map_destroy(mymtd);     
  }
  if (s3c2410nor_map.virt)
  {
    iounmap((void*)s3c2410nor_map.virt);
    s3c2410nor_map.virt = 0;
  }
}


module_init(init_s3c2410nor);
module_exit(cleanup_s3c2410nor);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("farsihgt");
MODULE_DESCRIPTION("Generic configurable MTD map driver");

