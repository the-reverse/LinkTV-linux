#include <linux/mtd/rtk_nand.h>

#if RTK_NAND_TEST
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/pm.h>
#include <asm/io.h>

/* Ken-Yu */
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <asm/r4kcache.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/mach-venus/platform.h>

#define DATA_SIZE 			(2048)
#define NFT_START_BLK_NUM	0//204//(7533)
#define NFT_END_BLK_NUM		8192//(7600)
#define OOB_SIZE 			(32)
#define NF_PAGE_PER_BLK			(64)
#define NF_BLK_TOTAL_NUM		(8192)

unsigned char  u8InDataBuffer[DATA_SIZE] __attribute__((__aligned__(16)));;
unsigned char  u8OutDataBuffer[DATA_SIZE] __attribute__((__aligned__(16)));;
unsigned char  u8InOobDataBuffer[OOB_SIZE] __attribute__((__aligned__(16)));;
unsigned char  u8OutOobDataBuffer[OOB_SIZE] __attribute__((__aligned__(16)));;
unsigned char u8Record[NF_BLK_TOTAL_NUM]={0};

static void RTK_NFT_GenData(void)
{
	//memset(&u8InDataBuffer,0,DATA_SIZE);
	memset(&u8OutDataBuffer,0,DATA_SIZE);
	memset(&u8InOobDataBuffer,'ee',OOB_SIZE);
	memset(&u8OutOobDataBuffer,'ff',OOB_SIZE);
	int i=0;
	for(i=0;i<DATA_SIZE;i++)
	{
		if(i%2)
			u8InDataBuffer[i]=0x55;
		else
			u8InDataBuffer[i]=0xAA;
	}

	//for(i=0;i<OOB_SIZE;i++)
	//{
	//	u8InOobDataBuffer[i]=(unsigned char)i;
	//}
	
}

static int RTK_NFT_CompData(unsigned char *pu8InData, unsigned char *pu8OutData,unsigned int len)
{
	int i=0;

	for(i=0;i<len;i++)
	{
		if(pu8InData[i]!=pu8OutData[i])
		{
			//printk("[%d]InData 0x%x, OutData 0x%x\n",i,pu8InData[i],pu8OutData[i]);
			return -1;
		}
	}
	return 0;		
}

static int RTK_NFT_EraseAllBlock(struct mtd_info *mtd,unsigned int u32StartBlk, unsigned int u32EndBlk)
{
	int b =0 ;
	for(b=u32StartBlk;b<u32EndBlk;b++)
	{
		rtk_erase_block(mtd, 0, NF_PAGE_PER_BLK*b);
	}
printk("Erase all block is done...(BBT block included)\n");
}
static int RTK_NFT_simpleTest(struct mtd_info *mtd)
{
	int b = 0;
	int p = 0;
	int i = 0;
	int r = 0;
	RTK_NFT_GenData();
    
	for(b=NFT_START_BLK_NUM;b<NFT_END_BLK_NUM;b++)//Erase block
		{
			rtk_erase_block(mtd, 0, NF_PAGE_PER_BLK*b);	
			for(p=0;p<NF_PAGE_PER_BLK;p++)
			{
				j_rtk_write_ecc_page (mtd,0, p+b*NF_PAGE_PER_BLK,&u8InDataBuffer, &u8InOobDataBuffer,1);
				
				j_rtk_read_ecc_page (mtd, 0, p+b*NF_PAGE_PER_BLK, &u8OutDataBuffer, &u8OutOobDataBuffer);

				if(RTK_NFT_CompData(u8InDataBuffer, u8OutDataBuffer,DATA_SIZE)!=0)
					{
						//printk("Blk %d, page %d DataError!\n",b,p);
						if(u8Record[b]==0)
						{
							u8Record[b]=1;
							r++;
						}
						continue;
					}

				#if 0
				j_rtk_write_oob(mtd,0, p+b*NF_PAGE_PER_BLK,OOB_SIZE,&u8InOobDataBuffer);
				j_rtk_read_oob(mtd,0, p+b*NF_PAGE_PER_BLK,OOB_SIZE,&u8OutOobDataBuffer);
				if(RTK_NFT_CompData(u8InOobDataBuffer, u8OutOobDataBuffer,DATA_SIZE)!=0)
					{
						printk("\n Blk %d, page %d OobError!\n",b,p);
						continue;
					}

				#endif
				
			}
			printk(".");
		}

    printk("\nHas %d back blocks\n",r);
	for(i=0;i<NF_BLK_TOTAL_NUM;i++)
	{
		if(u8Record[i])
			printk("block [%d] has error \n",i);
	}
	printk("NAND simple test is done...\n");
}



int RTK_NF_Test_Main(struct mtd_info *mtd)
{
	RTK_NFT_EraseAllBlock(mtd,0,8192);
	
	//Step 1. simple test
	//RTK_NFT_simpleTest(mtd);
	

}


#endif


