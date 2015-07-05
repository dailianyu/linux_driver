#include "def.h"
#include "2410addr.h"
#include "2410lib.h"


#define	BLK_IDXL	8
#define	BLK_IDXH	9
#define	FMT_TAG		15

char format_tags[] = "Formatted For NAND FLASH Driver";	//must be less than 32
/***********************************************************/
//nand ecc utils
typedef	unsigned char u_char;
static u_char eccpos[6] = {0, 1, 2, 3, 6, 7};

/*
 * Pre-calculated 256-way 1 byte column parity
 */
static const u_char nand_ecc_precalc_table[] = {
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
	0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
	0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
	0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
	0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
	0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
	0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
	0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00
};


/**
 * nand_trans_result - [GENERIC] create non-inverted ECC
 * @reg2:	line parity reg 2
 * @reg3:	line parity reg 3
 * @ecc_code:	ecc 
 *
 * Creates non-inverted ECC code from line parity
 */
static void nand_trans_result(u_char reg2, u_char reg3,
	u_char *ecc_code)
{
	u_char a, b, i, tmp1, tmp2;
	
	/* Initialize variables */
	a = b = 0x80;
	tmp1 = tmp2 = 0;
	
	/* Calculate first ECC byte */
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP15,13,11,9 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP14,12,10,8 --> ecc_code[0] */
			tmp1 |= b;
		b >>= 1;
		a >>= 1;
	}
	
	/* Calculate second ECC byte */
	b = 0x80;
	for (i = 0; i < 4; i++) {
		if (reg3 & a)		/* LP7,5,3,1 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		if (reg2 & a)		/* LP6,4,2,0 --> ecc_code[1] */
			tmp2 |= b;
		b >>= 1;
		a >>= 1;
	}
	
	/* Store two of the ECC bytes */
	ecc_code[0] = tmp1;
	ecc_code[1] = tmp2;
}

/**
 * nand_calculate_ecc - [NAND Interface] Calculate 3 byte ECC code for 256 byte block
 * @dat:	raw data
 * @ecc_code:	buffer for ECC
 */
int nand_calculate_ecc(const u_char *dat, u_char *ecc_code)
{
	u_char idx, reg1, reg2, reg3;
	int j;
	
	/* Initialize variables */
	reg1 = reg2 = reg3 = 0;
	ecc_code[0] = ecc_code[1] = ecc_code[2] = 0;
	
	/* Build up column parity */ 
	for(j = 0; j < 256; j++) {
		
		/* Get CP0 - CP5 from table */
		idx = nand_ecc_precalc_table[dat[j]];
		reg1 ^= (idx & 0x3f);
		
		/* All bit XOR = 1 ? */
		if (idx & 0x40) {
			reg3 ^= (u_char) j;
			reg2 ^= ~((u_char) j);
		}
	}
	
	/* Create non-inverted ECC code from line parity */
	nand_trans_result(reg2, reg3, ecc_code);
	
	/* Calculate final ECC code */
	ecc_code[0] = ~ecc_code[0];
	ecc_code[1] = ~ecc_code[1];
	ecc_code[2] = ((~reg1) << 2) | 0x03;
	return 0;
}

/**
 * nand_correct_data - [NAND Interface] Detect and correct bit error(s)
 * @dat:	raw data read from the chip
 * @read_ecc:	ECC from the chip
 * @calc_ecc:	the ECC calculated from raw data
 *
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data(u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	u_char a, b, c, d1, d2, d3, add, bit, i;
	
	/* Do error detection */ 
	d1 = calc_ecc[0] ^ read_ecc[0];
	d2 = calc_ecc[1] ^ read_ecc[1];
	d3 = calc_ecc[2] ^ read_ecc[2];
	
	if ((d1 | d2 | d3) == 0) {
		/* No errors */
		return 0;
	}
	else {
		a = (d1 ^ (d1 >> 1)) & 0x55;
		b = (d2 ^ (d2 >> 1)) & 0x55;
		c = (d3 ^ (d3 >> 1)) & 0x54;
		
		/* Found and will correct single bit error in the data */
		if ((a == 0x55) && (b == 0x55) && (c == 0x54)) {
			c = 0x80;
			add = 0;
			a = 0x80;
			for (i=0; i<4; i++) {
				if (d1 & c)
					add |= a;
				c >>= 2;
				a >>= 1;
			}
			c = 0x80;
			for (i=0; i<4; i++) {
				if (d2 & c)
					add |= a;
				c >>= 2;
				a >>= 1;
			}
			bit = 0;
			b = 0x04;
			c = 0x80;
			for (i=0; i<3; i++) {
				if (d3 & c)
					bit |= b;
				c >>= 2;
				b >>= 1;
			}
			b = 0x01;
			a = dat[add];
			a ^= (b << bit);
			dat[add] = a;
			return 1;
		}
		else {
			i = 0;
			while (d1) {
				if (d1 & 0x01)
					++i;
				d1 >>= 1;
			}
			while (d2) {
				if (d2 & 0x01)
					++i;
				d2 >>= 1;
			}
			while (d3) {
				if (d3 & 0x01)
					++i;
				d3 >>= 1;
			}
			if (i == 1) {
				/* ECC Code Error Correction */
				read_ecc[0] = calc_ecc[0];
				read_ecc[1] = calc_ecc[1];
				read_ecc[2] = calc_ecc[2];
				return 2;
			}
			else {
				/* Uncorrectable Error */
				return -1;
			}
		}
	}
	
	/* Should never happen */
	return -1;
}


#define	EnNandFlash()	(rNFCONF |= 0x8000)
#define	DsNandFlash()	(rNFCONF &= ~0x8000)
#define	InitEcc()		(rNFCONF |= 0x1000)
#define	NoEcc()			(rNFCONF &= ~0x1000)
#define	NFChipEn()		(rNFCONF &= ~0x800)
#define	NFChipDs()		(rNFCONF |= 0x800)

#define	WrNFCmd(cmd)	(rNFCMD = (cmd))
#define	WrNFAddr(addr)	(rNFADDR = (addr))
#define	WrNFDat(dat)	(rNFDATA = (dat))
#define	RdNFDat()		(rNFDATA)
#define	RdNFStat()		(rNFSTAT)
#define	NFIsBusy()		(!(rNFSTAT&1))
#define	NFIsReady()		(rNFSTAT&1)

//#define	WIAT_BUSY_HARD	1
//#define	ER_BAD_BLK_TEST
//#define	WR_BAD_BLK_TEST

#define	READCMD0	0
#define	READCMD1	1
#define	READCMD2	0x50
#define	ERASECMD0	0x60
#define	ERASECMD1	0xd0
#define	PROGCMD0	0x80
#define	PROGCMD1	0x10
#define	QUERYCMD	0x70
#define	RdIDCMD		0x90

static U16 NandAddr;


static void InitNandCfg(void)
{
	//enable nand flash control, initilize ecc, chip disable,
	rNFCONF = (1<<15)|(1<<12)|(1<<11)|(7<<8)|(7<<4)|(7);	
}

#ifdef	WIAT_BUSY_HARD
#define	WaitNFBusy()	while(NFIsBusy())
#else
static U32 WaitNFBusy(void)	// R/B Î´½ÓºÃ?
{
	U8 stat;
	
	WrNFCmd(QUERYCMD);
	do {
		stat = RdNFDat();
		//printf("%x\n", stat);
	}while(!(stat&0x40));
	WrNFCmd(READCMD0);
	return stat&1;
}
#endif

static U32 ReadChipId(void)
{
	U32 id;
	
	NFChipEn();	
	WrNFCmd(RdIDCMD);
	WrNFAddr(0);
	while(NFIsBusy());	
	id  = RdNFDat()<<8;
	id |= RdNFDat();		
	NFChipDs();		
	
	return id;
}

static U16 ReadStatus(void)
{
	U16 stat;
	
	NFChipEn();	
	WrNFCmd(QUERYCMD);		
	stat = RdNFDat();	
	NFChipDs();
	
	return stat;
}

static U32 EraseBlock(U32 addr)
{
	U8 stat;

	addr &= ~0x1f;
		
	NFChipEn();	
	WrNFCmd(ERASECMD0);		
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFCmd(ERASECMD1);		
	stat = WaitNFBusy();
	NFChipDs();
	
#ifdef	ER_BAD_BLK_TEST
	if(!((addr+0xe0)&0xff)) stat = 1;	//just for test bad block
#endif
	
	//printf("Erase block 0x%x %s\n", addr, stat?"fail":"ok");
	putch('.');
	return stat;
}

//addr = page address
static void ReadPage(U32 addr, U8 *buf)
{
	U16 i;
	
	NFChipEn();
	WrNFCmd(READCMD0);
	WrNFAddr(0);
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	InitEcc();
	WaitNFBusy();
	for(i=0; i<512; i++)
		buf[i] = RdNFDat();
	NFChipDs();
}

static U32 WritePage(U32 addr, U8 *buf, U16 blk_idx)
{
	U16 i;
	U8 stat;
	U8 ecc_code[3];
	U8 oob_info[16];
		
	for(i=0; i<sizeof(oob_info); i++)
		oob_info[i] = 0xff;
	
	nand_calculate_ecc(buf, ecc_code);
	oob_info[eccpos[0]] = ecc_code[0];
	oob_info[eccpos[1]] = ecc_code[1];
	oob_info[eccpos[2]] = ecc_code[2];
	nand_calculate_ecc(buf+256, ecc_code);
	oob_info[eccpos[3]] = ecc_code[0];
	oob_info[eccpos[4]] = ecc_code[1];
	oob_info[eccpos[5]] = ecc_code[2];
	oob_info[BLK_IDXL]  = blk_idx;
	oob_info[BLK_IDXH]  = blk_idx>>8;
	oob_info[FMT_TAG]   = format_tags[addr&0x1f];
	
	NFChipEn();
	WrNFCmd(PROGCMD0);
	WrNFAddr(0);
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
//	InitEcc();	
	for(i=0; i<512; i++)
		WrNFDat(buf[i]);
		
	if(!addr) {
		WrNFDat('b');
		WrNFDat('o');
		WrNFDat('o');
		WrNFDat('t');		
	} else {		
		for(i=0; i<sizeof(oob_info); i++)
			WrNFDat(oob_info[i]);
	}
/*	tmp[0] = rNFECC0;
    tmp[1] = rNFECC1;
    tmp[2] = rNFECC2;
    	
	WrNFDat(tmp[0]);
	WrNFDat(tmp[1]);
	WrNFDat(tmp[2]);
*/		
	WrNFCmd(PROGCMD1);
	stat = WaitNFBusy();
	NFChipDs();
	
#ifdef	WR_BAD_BLK_TEST
	if((addr&0xff)==0x17) stat = 1;	//just for test bad block
#endif
		
	if(stat)
		printf("Write nand flash 0x%x fail\n", addr);
	else {	
		U8 RdDat[512];
		
		ReadPage(addr, RdDat);		
		for(i=0; i<512; i++)
			if(RdDat[i]!=buf[i]) {
				printf("Check data at page 0x%x, offset 0x%x fail\n", addr, i);
				stat = 1;
				break;
			}
	}
		
	return stat;	
}

static void MarkBadBlk(U32 addr)
{
	addr &= ~0x1f;
	
	NFChipEn();
	
	WrNFCmd(READCMD2);	//point to area c
	
	WrNFCmd(PROGCMD0);
	WrNFAddr(4);		//mark offset 4,5,6,7
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFDat(0);			//mark with 0
	WrNFDat(0);
	WrNFDat(0);			//mark with 0
	WrNFDat(0);
	WrNFCmd(PROGCMD1);
	WaitNFBusy();		//needn't check return status
	
	WrNFCmd(READCMD0);	//point to area a
		
	NFChipDs();
}

static int CheckBadBlk(U32 addr)
{
	U8 dat;
	
	addr &= ~0x1f;
	
	NFChipEn();
	
	WrNFCmd(READCMD2);	//point to area c
	WrNFAddr(5);		//mark offset 4,5,6,7
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WaitNFBusy();
	dat = RdNFDat();
	
	WrNFCmd(READCMD0);	//point to area a
	
	NFChipDs();

	return (dat!=0xff);
}

/************************************************************/
struct Partition{
	U32 offset;
	U32 size;
	char *name;
};
static struct Partition NandPart[] = {
	{0, 		 0x00040000, "bootloader"},		//256K
	{0x00040000, 0x001c0000, "zImage"},
	{0x00200000, 0x01e00000, "cramfs"},		//30M
	{0x02000000, 0x02000000, "WinCE"},	//30M
	{0,			 0         , 0}
};
/*
static void TestFunc(void)
{
	U32 i;
	U8 buf[512];
	
	if(EraseBlock(0x180))
		return;
	
	for(i=0; i<512; i++)
		buf[i] = i;
		
	WritePage(0x180, buf);	
	for(i=0; i<512; i++)
		buf[i] = 0;
	ReadPage(0x180, buf);
	
	for(i=0; i<512; i++)
		printf("%4x", buf[i]);
}
*/
static U32 StartPage, BlockCnt;
extern U32 downloadAddress; 
extern U32 downloadFileSize;


static int NandSelPart(char *info)
{
	U16 i, max_sel;
	struct Partition *ptr = NandPart;
	
	printf("Please select which region to %s : Esc to abort\n", info);
	
	for(i=0; ptr->size!=0; i++, ptr++)
		printf("%d : offset 0x%-8x, size 0x%-8x [%s]\n", i, ptr->offset, ptr->size, ptr->name);
		
	max_sel = i;
	
	while(1) {
		i = getch();
		if(i==0x1b)
			return -1;
		if((i>='0')&&(i<(max_sel+'0'))) {
			i -= '0';
			StartPage = NandPart[i].offset>>9;
			BlockCnt  = NandPart[i].size>>14;
			return i;
		}
	}	
}

static void WrFileToNF(void)
{
	int nf_part, i ,size, skip_blks;
	U32 ram_addr;
	U8 *ram_addr_ecc;
	
	nf_part = NandSelPart("write");
	if(nf_part<0)
		return;	
	
	if(downloadFileSize>NandPart[nf_part].size) {
		puts("Download file size is more large than selected partition size!!!\n");
//		return;
	}
	
	printf("Now write nand flash page 0x%x from ram address 0x%x, filesize = %d\n", StartPage, downloadAddress, downloadFileSize);
	puts("Are you sure? [y/n]\n");
	while(1) {
		char c = getch();
		if((c=='y')||(c=='Y'))
			break;
		if((c=='n')||(c=='N'))			
			return;
	}
	
	skip_blks = 0;
	ram_addr = downloadAddress;
	ram_addr_ecc = (U8 *)downloadAddress;
	size = downloadFileSize;
	for(i=0; size>0; )	{	
		if(!(i&0x1f)) {
			if(EraseBlock(i+StartPage)) {
			/*	NandPart[nf_part].size -= 32<<9;	//partition available size - 1 block size
				if(downloadFileSize>NandPart[nf_part].size) {
					puts("Program nand flash fail\n");
					return;
				}
				MarkBadBlk(i+StartPage);
				skip_blks++;				
				i += 32;				
				continue;*/
				goto WrFileToNFErr;
			}
		}
		if(WritePage(i+StartPage, (U8 *)ram_addr, ((U32)ram_addr_ecc-downloadAddress)>>14 )) {
			ram_addr -= (i&0x1f)<<9;
			size += (i&0x1f)<<9;
			i &= ~0x1f;
WrFileToNFErr:			
			NandPart[nf_part].size -= 32<<9;	//partition available size - 1 block size
			if(downloadFileSize>NandPart[nf_part].size) {
				puts("Program nand flash fail\n");
				return;
			}			
			MarkBadBlk(i+StartPage);
			skip_blks++;			
			i += 32;			
			continue;
		}
		ram_addr += 512;
		size -= 512;
		i++;
	}

	puts("Program nand flash partition success\n");
	if(skip_blks)
		printf("Skiped %d bad block(s)\n", skip_blks);
}

#define LINUX_PAGE_SHIFT	12
#define LINUX_PAGE_SIZE		(1<<LINUX_PAGE_SHIFT)
#define COMMAND_LINE_SIZE 	1024

struct param_struct {
    union {
	struct {
	    unsigned long page_size;			/*  0 */
	    unsigned long nr_pages;				/*  4 */
	    unsigned long ramdisk_size;			/*  8 */
	    unsigned long flags;				/* 12 */
#define FLAG_READONLY	1
#define FLAG_RDLOAD		4
#define FLAG_RDPROMPT	8
	    unsigned long rootdev;				/* 16 */
	    unsigned long video_num_cols;		/* 20 */
	    unsigned long video_num_rows;		/* 24 */
	    unsigned long video_x;				/* 28 */
	    unsigned long video_y;				/* 32 */
	    unsigned long memc_control_reg;		/* 36 */
	    unsigned char sounddefault;			/* 40 */
	    unsigned char adfsdrives;			/* 41 */
	    unsigned char bytes_per_char_h;		/* 42 */
	    unsigned char bytes_per_char_v;		/* 43 */
	    unsigned long pages_in_bank[4];		/* 44 */
	    unsigned long pages_in_vram;		/* 60 */
	    unsigned long initrd_start;			/* 64 */
	    unsigned long initrd_size;			/* 68 */
	    unsigned long rd_start;				/* 72 */
	    unsigned long system_rev;			/* 76 */
	    unsigned long system_serial_low;	/* 80 */
	    unsigned long system_serial_high;	/* 84 */
	    unsigned long mem_fclk_21285;       /* 88 */
	} s;
	char unused[256];
    } u1;
    union {
	char paths[8][128];
	struct {
	    unsigned long magic;
	    char n[1024 - sizeof(unsigned long)];
	} s;
    } u2;
    char commandline[COMMAND_LINE_SIZE];
};

//extern char boot_params[];
extern void  call_linux(U32 a0, U32 a1, U32 a2);
void memcpy(char *s1, const char *s2, int n);
void LoadRun(int part_sel)
{
	U32 i, ram_addr, buf = 0x30200000;
	struct param_struct *params = (struct param_struct *)0x30000100;
	int size;

//#ifdef SDRAM_SIZE8M
//	char *linux_params = "root=/dev/mtdblock2 load_ramdisk=0 init=/linuxrc console=ttyS0 mem=8M";
//#elif defined SDRAM_SIZE16M
//	char *linux_params = "root=/dev/mtdblock2 lIZE32M
//	char *linux_params = "oad_ramdisk=0 init=/linuxrc console=ttyS0 mem=16M";
//#elif defined SDRAM_Sroot=/dev/mtdblock2 load_ramdisk=0 init=/linuxrc console=ttyS0 mem=32M";
//#elif defined SDRAM_SIZE64M
//	char *linux_params = "root=/dev/mtdblock2 load_ramdisk=0 init=/linuxrc console=ttyS0 mem=64M devfs=mount";
//#else
//	char *linux_params = "root=/dev/mtdblock2 load_ramdisk=0 init=/linuxrc console=ttyS0 mem=16M";

//	char *linux_params = "root=1f02 init=/linuxrc console=ttyS0,115200 devfs=mount display=sam240";//
	char *linux_params = "root=1f02 init=/linuxrc console=ttyS0,115200 devfs=mount display=shp240";//modify by czx
	NFSearchParams((char *)&Env);
	
	if(Env.boot_params[0]=='r')
		linux_params = Env.boot_params;

	StartPage = NandPart[part_sel].offset>>9;
	size = NandPart[part_sel].size;
	if(part_sel == 3)
		size = 0x01e00000;	//load wince ...;
	ram_addr = buf;	
	
	for(i=0; size>0; ) {
		if(!(i&0x1f)) {
			if(CheckBadBlk(i+StartPage)) {
				printf("Skipped bad block at 0x%x\n", i+StartPage);
				i += 32;
				size -= 32<<9;
				continue;
			}
		}
		ReadPage((i+StartPage), (U8 *)ram_addr);
		i++;
		size -= 512;
		ram_addr += 512;
	}
	DsNandFlash();

	for(i=0; i<(sizeof(struct param_struct)>>2); i++)
		((U32 *)params)[i] = 0;
	params->u1.s.page_size = LINUX_PAGE_SIZE;
	params->u1.s.nr_pages = (0x04000000 >> LINUX_PAGE_SHIFT);
	for(i=0; linux_params[i]; i++)
		params->commandline[i] = linux_params[i];
	
	//set nCS3 for cs8900,2006-08-25 modified by zhongzm
	rBWSCON |= 0x0000d000;
	rBWSCON &= ~0x00040000;

	puts("Set boot params = ");
	puts(linux_params);
	putch('\n');
	call_linux(0, 193, buf);
}

/************************************************************/
static int support=0;
static void InitNandFlash(void)
{	
	U32 i;
	
	InitNandCfg();
	i = ReadChipId();
//	printf("Read chip id = %x\n", i);	
	if((i==0x9873)||(i==0xec75))	
		NandAddr = 0;
	else if(i==0xec76)
	{	
		support=1;	//by chang
		NandAddr = 1;
	}
	else {	
		puts("Chip id error!!!\n");
		return;
	}
//	printf("Nand flash status = %x\n", ReadStatus());
}

void NandErase(void)
{
	int i, err = 0;
	
	InitNandFlash();
	
	i = NandSelPart("erase");
	if(i<0)
		return;	
	
	printf("Are you sure to erase nand flash from page 0x%x, block count 0x%x ? [y/n]\n", StartPage, BlockCnt);
	while(1) {
		char c;
		
		c = getch();
		if((c=='y')||(c=='Y'))
			break;
		if((c=='n')||(c=='N'))
			return;
	}	
	
	for(i=0; BlockCnt; BlockCnt--, i+=32) {
		if(EraseBlock(i+StartPage)) {
			err ++;
			puts("Press any key to continue...\n");
			getch();
		}
	}	

	DsNandFlash();		//disable nand flash interface
	puts("Erase Nand partition completed ");
	if(err)
		printf("with %d bad block(s)\n", err);
	else
		puts("success\n");
}

void NandWrite(void)
{
	InitNandFlash();
	WrFileToNF();
	DsNandFlash();		//disable nand flash interface
}

/*
void NandLoadRun(void)
{
	U8 key;
	
	while(1) {
		puts("Please select which OS to boot:\n1: Linux\n2: Wince\nEsc: exit\n");
		key = getch();
		if(key==ESC_KEY)
			return;
		if(key=='1'||key=='2')
			break;
	}
	
	InitNandFlash();
	
	printf("Now boot %s...\n", (key=='1')?"Linux":"Wince");
	LoadRun((key=='1')?1:5);
}
*/
void NandLoadRun(void)
{
/*	Beep( 2000, 500 ) ;
	Delay( 500 ) ;
	Beep( 2000, 500 ) ;
*/
	InitNandFlash();
	LoadRun(1);
}

void NandLoadRunW(void)
{
	printf("Now boot Wince\n");
	ClearMemory();
	InitNandFlash();
	LoadRun(3);
	
}

void NandRunSystem(void)
{
	NFSearchParams((char *)&Env);
	if(Env.Os_Auto_Flag==2)
		NandLoadRunW();
	else
		NandLoadRun();
}


void memcpy(char *s1, const char *s2, int n)
{
	int i;

	for (i = 0; i < n; i++)
		((s1))[i] = ((s2))[i];
}

int strncmp(const char *s1, const char *s2, int maxlen)
{
	int i;

	for(i = 0; i < maxlen; i++) {
		if(s1[i] != s2[i])
			return ((int) s1[i]) - ((int) s2[i]);
		if(s1[i] == 0)
			return 0;
	}

	return 0;
}

/********************add by chang ***********************************/
#define SAVE_ENV_IN_NAND
#ifdef	SAVE_ENV_IN_NAND

U32 NFSaveParams(char *pEnv)
{	
	char dat[512];
	U32 addr;
	InitNandFlash();
	if(support) {
		memcpy(dat, pEnv, sizeof(EnvParams));
		for(addr=SIZE_128K>>9; addr<(0x30000>>9); addr++) {
			//NFEraseBlock(addr);
			 EraseBlock(addr);
			//if(!NFWritePage(addr, (U8 *)dat, 0))
			 if(!WritePage(addr, (U8 *)dat, 0))
			 {
				//printf("wite succes\n");
				return 0;
			 }
		}
	}
	return -1;
	
}


U32 NFSearchParams(char *pEnv)
{
	char dat[512];
	U32 addr;
	InitNandFlash();
	if(support) {
		for(addr=SIZE_128K>>9; addr<(0x30000>>9); addr++) {
			ReadPage(addr, (U8 *)dat);		
			//if(!strncmp(dat, "params", 7)) {
			memcpy(pEnv, dat, sizeof(EnvParams));			
			return 0;	
		}
	}
	return -1;	
}

#endif
/********************** add by chang *********************************/
