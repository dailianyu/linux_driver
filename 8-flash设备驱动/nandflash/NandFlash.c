//#include "../inc/def.h"
//#include "../inc/config.h"
//#include "../inc/utils.h"
//#include "../inc/board.h"
#include "def.h"
#include "2410addr.h"
#include "2410lib.h"

#ifdef	NAND_FLASH_SUPPORT

struct NFChipInfo {
	U32 id;
	U32 size;
}

static NandFlashChip[] = {
	{0xec73, SIZE_16M},
	{0xec75, SIZE_32M},
	{0xec76, SIZE_64M},
	{0xec79, SIZE_128M},
	{0, 0},
};

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

/**********************************************************/

/*
struct Partition oNandPartition[] = {
	{0x00000000, 0x00030000, "boot"},
	{0x00030000, 0x001d0000, "kernel"},
	{0x00200000, 0x00600000, "rootfs"},
	{0x00800000, 0x00800000, "ext-fs1"},
	{0x01000000, 0x01000000, "ext-fs2"},
	{0x00000000, 0x00000000, 0}
};*/

extern struct Partition *pNandPart;

U32 NandFlashSize;

static U16 NandAddr;
static U16 support;

#define	NAND_DAT	0x02000000
#define	NAND_ALE	0x02000004
#define	NAND_CLE	0x02000002

#define	READCMD0	0
#define	READCMD1	1
#define	READCMD2	0x50
#define	ERASECMD0	0x60
#define	ERASECMD1	0xd0
#define	PROGCMD0	0x80
#define	PROGCMD1	0x10
#define	QUERYCMD	0x70
#define	READIDCMD	0x90

void NFChipSel(U32);
int  NFIsReady(void);
void NFWrCmd(int);
void NFWrAddr(int);
void NFWrDat(int);
U8   NFRdDat(void);

#define	NFChipEn()	NFChipSel(1)
#define	NFChipDs()	NFChipSel(0)
#define	NFIsBusy()	(!NFIsReady())

//#define	NFWrCmd(cmd)	*(volatile U8 *)NAND_CLE = (cmd)
//#define	NFWrAddr(addr)	*(volatile U8 *)NAND_ALE = (addr)
//#define	NFWrDat(dat)	*(volatile U8 *)NAND_DAT = (dat)
//#define	NFRdDat()		*(volatile U8 *)NAND_DAT

//#define	WIAT_BUSY_HARD
//#define	ER_BAD_BLK_TEST
//#define	WR_BAD_BLK_TEST

#ifdef	WIAT_BUSY_HARD
#define	NFWaitBusy()	while(NFIsBusy())
#else
static U32 NFWaitBusy(void)
{
	U8 stat;
	
	NFWrCmd(QUERYCMD);
	do {
		stat = NFRdDat();
		//printf("%x\n", stat);
	}while(!(stat&0x40));
	NFWrCmd(READCMD0);
	return stat&1;
}
#endif

static U32 NFReadID(void)
{
	U32 id, loop = 0;

	NFChipEn();
	NFWrCmd(READIDCMD);
	NFWrAddr(0);
	while(NFIsBusy()&&(loop<10000)) loop++;
	if(loop>=10000)
		return 0;
	id  = NFRdDat()<<8;
	id |= NFRdDat();
	NFChipDs();
	
	return id;
}

static U16 NFReadStat(void)
{
	U16 stat;
	
	NFChipEn();	
	NFWrCmd(QUERYCMD);
	stat = NFRdDat();	
	NFChipDs();
	
	return stat;
}

static U32 NFEraseBlock(U32 addr)
{
	U8 stat;

	addr &= ~0x1f;
		
	NFChipEn();	
	NFWrCmd(ERASECMD0);		
	NFWrAddr(addr);
	NFWrAddr(addr>>8);
	if(NandAddr)
		NFWrAddr(addr>>16);
	NFWrCmd(ERASECMD1);		
	stat = NFWaitBusy();
	NFChipDs();
	
#ifdef	ER_BAD_BLK_TEST
	if(!((addr+0xe0)&0xff)) stat = 1;	//just for test bad block
#endif
	
//	printf("Erase block 0x%08x %s\n", addr, stat?"fail":"ok");
	putch('.');
	
	return stat;
}

//addr = page address
static void NFReadPage(U32 addr, U8 *buf)
{
	U16 i;
	
	NFChipEn();
	NFWrCmd(READCMD0);
	NFWrAddr(0);
	NFWrAddr(addr);
	NFWrAddr(addr>>8);
	if(NandAddr)
		NFWrAddr(addr>>16);
//	InitEcc();
	NFWaitBusy();		
	for(i=0; i<512; i++)
		buf[i] = NFRdDat();		
	NFChipDs();
}

//addr = page address
static U32 NFWritePage(U32 addr, U8 *buf, U16 blk_idx)
{
	U16 i, stat;
//	U8 tmp[3];
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
	NFWrCmd(PROGCMD0);
	NFWrAddr(0);
	NFWrAddr(addr);
	NFWrAddr(addr>>8);
	if(NandAddr)
		NFWrAddr(addr>>16);
//	InitEcc();	
	for(i=0; i<512; i++)
		NFWrDat(buf[i]);
		
	if(!addr) {
		NFWrDat('b');
		NFWrDat('o');
		NFWrDat('o');
		NFWrDat('t');		
	} else {		
		for(i=0; i<sizeof(oob_info); i++)
			NFWrDat(oob_info[i]);
	}
		
/*	tmp[0] = rNFECC0;
    tmp[1] = rNFECC1;
    tmp[2] = rNFECC2;
    	
	NFWrDat(tmp[0]);
    NFWrDat(tmp[1]);
    NFWrDat(tmp[2]);*/
    	
	NFWrCmd(PROGCMD1);
	stat = NFWaitBusy();
	NFChipDs();
	
#ifdef	WR_BAD_BLK_TEST
	if((addr&0xff)==0x17) stat = 1;	//just for test bad block
#endif
	
	if(stat)
		printf("Write nand flash 0x%x fail\n", addr);
	else {	
		U8 RdDat[512];
		
		NFReadPage(addr, RdDat);
		for(i=0; i<512; i++)
			if(RdDat[i]!=buf[i]) {
				printf("Check data at page 0x%x, offset 0x%x fail\n", addr, i);
				stat = 1;
				break;
			}
	}
		
	return stat;	
}

static void NFMarkBadBlk(U32 addr)
{
	int i;
	addr &= ~0x1f;
	
	NFChipEn();
	
	NFWrCmd(READCMD2);	//point to area c
	
	NFWrCmd(PROGCMD0);
	NFWrAddr(0);		//mark offset 0~15
	NFWrAddr(addr);
	NFWrAddr(addr>>8);
	if(NandAddr)
		NFWrAddr(addr>>16);
	for(i=0; i<16; i++)
		NFWrDat(0);		//mark with 0	
	NFWrCmd(PROGCMD1);
	NFWaitBusy();		//needn't check return status
	
	NFWrCmd(READCMD0);	//point to area a
		
	NFChipDs();
}

static int NFChkBadBlk(U32 addr)
{
	U8 dat;
	
	addr &= ~0x1f;
	
	NFChipEn();
	
	NFWrCmd(READCMD2);	//point to area c
	NFWrAddr(5);		//mark offset 5
	NFWrAddr(addr);
	NFWrAddr(addr>>8);
	if(NandAddr)
		NFWrAddr(addr>>16);
	NFWaitBusy();
	dat = NFRdDat();
	
	NFWrCmd(READCMD0);	//point to area a
	
	NFChipDs();

	return (dat!=0xff);
}



static struct Partition *NFSelPart(char *info)
{
	int i, max_sel;

	printf("Please select Nand flash region to %s, Esc to abort\n", info);
	
	for(i=0; pNandPart[i].size; i++)
		printf("%d : start 0x%08x, size 0x%08x\t[Part%d]\n", i, pNandPart[i].offset, pNandPart[i].size, i/*pNandPart[i].name*/ );

	max_sel = i;	
	
	while(1) {
		char c = getch();
		
		if(c==0x1b)
			return 0;
		if((c>='0')&&(c<(max_sel+'0')))
			return &(pNandPart[c-'0']);
	}	
}

static void NFReadPart(struct Partition *part, U8 *buf)
{
	U32 i, start_page;
	U8 *ram_addr;
	int size;
	
	start_page = part->offset>>9;
	size = part->size;
	ram_addr = buf;
	
	for(i=0; size>0; ) {
		if(!(i&0x1f)) {
			if(NFChkBadBlk(i+start_page)) {
				printf("Skipped bad block at 0x%08x\n", i+start_page);
				i += 32;
				size -= 32<<9;
				continue;
			}
		}
		NFReadPage((i+start_page), ram_addr);
//			printf("Read page %d\n", i);
		i++;
		size -= 512;
		ram_addr += 512;
	}
}

/******************************************************/
int WrFileToNF(U32 src_addr, U32 w_size)
{
	struct Partition *part;
	U32 start_page, i, skip_blks;
	U8 *ram_addr;
	int size;	//must be int
	U32 start_addr;
	
	if(!support)
		return -1;
		
	part = NFSelPart("write");
	if(!part)
		return -1;
	
	if(w_size>part->size) {
		puts("File size is too long!\n");
		return -1;
	}

	start_page = part->offset>>9;
	
	printf("Are you sure to write nand flash from 0x%x with ram address 0x%x, size %d ?\n", part->offset, src_addr, w_size);
	if(!getyorn())
		return -1;
		
	skip_blks = 0;
	ram_addr = (U8 *)src_addr;
	start_addr = (U32)src_addr;
	size = w_size;
	printf("start address 0x%x\n", start_page);
	for(i=0; size>0; )	{
		if(!(i&0x1f)) {
			if(NFEraseBlock(i+start_page)) {
/*				part->size -= 32<<9;	//fail, partition available size - 1 block size
				if(FileSize>part->size) {
					puts("Program nand flash fail\n");
					return;
				}
				NFMarkBadBlk(i+start_page);
				skip_blks++;
				i += 32;			
				continue;*/
				goto WrFileToNFErr;
			}
		}
		if(NFWritePage(i+start_page, ram_addr, ((U32)ram_addr-start_addr)>>14)) {
			ram_addr -= (i&0x1f)<<9;
			size += (i&0x1f)<<9;
			i &= ~0x1f;
WrFileToNFErr:			
			part->size -= 32<<9;	//partition available size - 1 block size
			if(w_size>part->size) {
				puts("Program nand flash fail\n");
				return -1;
			}			
			NFMarkBadBlk(i+start_page);
			skip_blks++;			
			i += 32;		
			continue;
		}
		ram_addr += 512;
		size -= 512;
		i++;
	}
	printf("\nend address 0x%x\n", start_page+i);

	puts("Program nand flash partition success\n");
	if(skip_blks)
		printf("Skiped %d bad block(s)\n", skip_blks);
		
	return 0;	
}

int RdFileFrNF(U32 dst_addr, U32 load_part)
{
	U32 i;
	struct Partition *part;
	
	if(!support)
		return -1;

	for(i=0; pNandPart[i].size; i++);
	if(i>load_part)
		part = pNandPart+load_part;
	else {
		part = NFSelPart("read");
		if(!part)
			return -1;
	}
	
	puts("Loading...\n");
	
	NFReadPart(part, (U8 *)dst_addr);
	
	puts("end\n");
	
	return 0;
}

int EraseNandPart(void)
{
	struct Partition *part;
	U32 start_page, blk_cnt;
	int i, err = 0;	

	if(!support)
		return -1;
		
	part = NFSelPart("erase");
	if(!part)
		return -1;

	start_page = part->offset>>9;
	blk_cnt  = part->size>>14;

	printf("Are you sure to erase nand flash from page 0x%x, block count 0x%x ?", start_page, blk_cnt);
	if(!getyorn())
		return -1;
	
	printf("start address 0x%x\n", start_page);
	for(i=0; blk_cnt; blk_cnt--, i+=32) {
		if(NFEraseBlock(i+start_page)) {
			err ++;
			puts("Press any key to continue...\n");
			getch();
		}
	}
	printf("\nend address 0x%x\n", start_page+i-32);
	
	puts("Erase Nand partition completed ");
	if(err)
		printf("with %d bad block(s)\n", err);
	else
		puts("success\n");
	
	return 0;
}

#ifdef	SAVE_ENV_IN_NAND
U32 NFSaveParams(char *pEnv)
{
	char dat[512];
	U32 addr;
	
	if(support) {
		memcpy(dat, pEnv, sizeof(EnvParams));
		for(addr=SIZE_64K>>9; addr<(0x30000>>9); addr++) {
			NFEraseBlock(addr);
			if(!NFWritePage(addr, (U8 *)dat, 0))
				return 0;
		}
	}
	return -1;
}

U32 NFSearchParams(char *pEnv)
{
	char dat[512];
	U32 addr;
	
	if(support) {
		for(addr=SIZE_64K>>9; addr<(0x30000>>9); addr++) {
			NFReadPage(addr, (U8 *)dat);		
			if(!strncmp(dat, "params", 7)) {
				memcpy(pEnv, dat, sizeof(EnvParams));			
				return 0;
			}
		}
	}
	return -1;	
}
#endif

static U32 nand_id;

void NandFlashInit(void)
{
	int i;	
	
	support = 0;
	nand_id = NFReadID();
	
	for(i=0; NandFlashChip[i].id!=0; i++)
		if(NandFlashChip[i].id==nand_id) {			
			nand_id = i;
			NandFlashSize = NandFlashChip[i].size;
			support  = 1;
			NandAddr = NandFlashSize>SIZE_32M;
			if(!pNandPart[0].size) {
				pNandPart[0].offset = 0;
				pNandPart[0].size   = NandFlashSize;
				pNandPart[1].size   = 0;				
			}			
			return;
		}
	
}

void NandFlashStatusRep(void)
{
	if(support) {
		printf("Nand Flash ID is 0x%x, Size = %dM, Status = 0x%x\n", NandFlashChip[nand_id].id, NandFlashChip[nand_id].size>>20, NFReadStat());
	} else {
		printf("No supported Nand Flash Found!\n");
	}
}

//void (*pNandFlashInit)(void) = NandFlashInit;

#endif	/* NAND_SUPPORT */

#ifdef	NAND_FLASH_SUPPORT

int NandProg(int argc, char *argv[])
{
	unsigned int size = ~0;
	unsigned int data_begin = ~0;

	data_begin = download_addr;
	size       = download_len;
	
	if(argc>1)
		data_begin = strtoul((char *)argv[1]);
	if(argc>2)	
		size       = strtoul((char *)argv[2]);	
	if((size==-1)||(data_begin==-1)) {
		printf("Arguments error\n");
		goto err_exit;
	}
	
	if(size==0) {
		printf("Write 0 Bytes!\n");
		return -1;
	}
	else
		return WrFileToNF(data_begin, size);
	
err_exit:
	printf("Usage:	nfprog addr size\naddr = data pointer to ram\nsize = program size(Bytes)\n");
	return -1;
}

int NandLoad(int argc, char *argv[])
{
	U32 load_part = -1;
	download_addr = DFT_DOWNLOAD_ADDR;
	
	if(argc>1) {
		download_addr = strtoul((char *)argv[1]);
		if(download_addr==-1)
			download_addr = DFT_DOWNLOAD_ADDR;
		if(argc>2)
			load_part = strtoul(argv[2]);
	}
	
	printf("Load data form nand flash to 0x%x\n", download_addr);
	return RdFileFrNF(download_addr, load_part);
}

int NandErase(int argc, char *argv[])
{
	return EraseNandPart();
}

int NandPart(int argc, char*argv[])
{
	unsigned long i, addr[8];
	
	if(!NandFlashSize)
		return -1;
	if((argc<=1)||(argc>9))
		goto err_exit;

	addr[0] = 0;
	for(i=1; i<argc; i++) {
		addr[i] = strtoul((char *)argv[i]);		
		if((addr[i]==-1)||(addr[i]<=addr[i-1])||(addr[i]&0x3fff)||(addr[i]>NandFlashSize))
			goto err_exit;
	}
	
	printf("Set %d partition(s) :\n", argc-1);
	for(i=0; i<argc-1; i++) {
		pNandPart[i].offset = addr[i];
		pNandPart[i].size   = addr[i+1]-addr[i];
		pNandPart[i].name   = " ";
		pNandPart[i+1].size = 0;
		printf("part%d : start address 0x%-8x, size 0x%-8x\n", i, pNandPart[i].offset, pNandPart[i].size);
	}

	return 0;
	
err_exit:
	puts("Usage : nfpart a1 a2 ... an\na1 = part1 end address, an = partn end address, n<=8, address must be 16KB align and don't excess the size of nand flash \n");
	return -1;
}

#endif


#define SAVE_ENV_IN_NAND
#ifdef	SAVE_ENV_IN_NAND

U32 NFSaveParams(char *pEnv)
{	
	char dat[512];
	U32 addr;
	InitNandFlash();
	if(1) {
		memcpy(dat, pEnv, sizeof(EnvParams));
		for(addr=SIZE_128K>>9; addr<(0x30000>>9); addr++) {
			//NFEraseBlock(addr);
			 EraseBlock(addr);
			//if(!NFWritePage(addr, (U8 *)dat, 0))
			 if(!WritePage(addr, (U8 *)dat))
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
	if(1) {
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

