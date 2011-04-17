/* DOL Patching code by emu_kidid */


#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "main.h"
#include "gui/FrameBufferMagic.h"

/* the patches */
extern u8 slot_a_hdd[];
extern u32 slot_a_hdd_size;
extern u8 slot_b_hdd[];
extern u32 slot_b_hdd_size;
extern u8 slot_a_sdhc[];
extern u32 slot_a_sdhc_size;
extern u8 slot_a_sd[];
extern u32 slot_a_sd_size;
extern u8 slot_b_sdhc[];
extern u32 slot_b_sdhc_size;
extern u8 slot_b_sd[];
extern u32 slot_b_sd_size;

#define LO_RESERVE 0x80001800
#define HI_RESERVE 0x817F8000

static unsigned int base_addr = LO_RESERVE;

/* Function jump locations */
#define READ_JUMP_OFFSET (base_addr)
#define ID_JUMP_OFFSET (base_addr | 0x04)
#define VI_JUMP_OFFSET (base_addr | 0x1200)
#define SEEK_JUMP_OFFSET (base_addr | 0x0C)


/*** externs ***/
extern GXRModeObj *vmode;		/*** Graphics Mode Object ***/
extern u32 *xfb[2];			   	/*** Framebuffers ***/
extern int SDHCCard;
extern int jitterfix;
extern int gamefixA;
extern int twoDisc;
extern int GC_SD_CHANNEL;

extern void animateBox(int x1,int y1, int x2, int y2, int color,char *msg);

#define NTSC  0
#define PAL50 1
#define PAL60 2
#define AUTO  3
#define P480  4  

static const u32 _dvdlowreset_org[12] = {
	0x7C0802A6,0x3C80CC00,0x90010004,0x38000002,0x9421FFE0,0xBF410008,0x3BE43000,0x90046004,
	0x83C43024,0x57C007B8,0x60000001,0x941F0024
};

static const u32 _dvdlowreset_new[8] = {
	0x3FE08000,0x63FF1800,0x7FE803A6,0x4E800021,0x4800006C,0x60000000,0x60000000,0x60000000
};

/* Original Read bytes (for finding and patching) */
u8 _Read_original[46] = {
	0x7C, 0x08, 0x02, 0xA6,  //	mflr        r0
	0x90, 0x01, 0x00, 0x04,  //	stw         r0, 4 (sp)
	0x38, 0x00, 0x00, 0x00,  //	li          r0, 0
	0x94, 0x21, 0xFF, 0xD8,  //	stwu        sp, -0x0028 (sp)
	0x93, 0xE1, 0x00, 0x24,  //	stw         r31, 36 (sp)
	0x93, 0xC1, 0x00, 0x20,  //	stw         r30, 32 (sp)
	0x3B, 0xC5, 0x00, 0x00,  //	addi        r30, r5, 0
	0x93, 0xA1, 0x00, 0x1C,  //	stw         r29, 28 (sp)
	0x3B, 0xA4, 0x00, 0x00,  //	addi        r29, r4, 0
	0x93, 0x81, 0x00, 0x18,  //	stw         r28, 24 (sp)
	0x3B, 0x83, 0x00, 0x00,  //	addi        r28, r3, 0
	0x90, 0x0D               // stw r0 to someplace in (sd1) - differs on different sdk's
};

u8 _Read_original_2[38] = {
	0x7C, 0x08, 0x02, 0xA6,  	//	mflr        r0               
	0x90, 0x01, 0x00, 0x04,  	//	stw         r0, 4 (sp)       
	0x94, 0x21, 0xFF, 0xE0,  	//	stwu        sp, -0x0020 (sp) 
	0x93, 0xE1, 0x00, 0x1C,  	//	stw         r31, 28 (sp)     
	0x90, 0x61, 0x00, 0x08,  	//	stw         r3, 8 (sp)       
	0x7C, 0x9F, 0x23, 0x78,  	//	mr          r31, r4          
	0x90, 0xA1, 0x00, 0x10,  	//	stw         r5, 16 (sp)      
	0x90, 0xC1, 0x00, 0x14,  	//	stw         r6, 20 (sp)      
	0x80, 0x01, 0x00, 0x14,  	//	lwz         r0, 20 (sp)      
	0x90, 0x0D					//	stw r0 to someplace in (sd1) - differs on different sdk's
};

/* Luigis Mansion NTSC (not Players Choice) */
u8 _Read_original_3[46] = {
	0x7C, 0x08, 0x02, 0xA6,  //	mflr        r0
	0x90, 0x01, 0x00, 0x04,  //	stw         r0, 4 (sp)
	0x38, 0x00, 0x00, 0x00,  //	li          r0, 0
	0x94, 0x21, 0xFF, 0xD8,  //	stwu        sp, -0x0028 (sp)
	0x93, 0xE1, 0x00, 0x24,  //	stw         r31, 36 (sp)
	0x93, 0xC1, 0x00, 0x20,  //	stw         r30, 32 (sp)
	0x3B, 0xC5, 0x00, 0x00,  //	addi        r30, r5, 0
	0x93, 0xA1, 0x00, 0x1C,  //	stw         r29, 28 (sp)
	0x3B, 0xA4, 0x00, 0x00,  //	addi        r29, r4, 0
	0x93, 0x81, 0x00, 0x18,  //	stw         r28, 24 (sp)
	0x3B, 0x83, 0x00, 0x00,  //	addi        r28, r3, 0
	0x90, 0xCD               // stw r6 to someplace in (sd1) - differs on different sdk's
};

u32 _DVDLowReadDiskID_original[8] = {
  0x7C0802A6,  // mflr        r0
  0x39000000,  // li          r8, 0
  0x90010004,  // stw         r0, 4 (sp)
  0x3CA0A800,  // lis         r5, 0xA800
  0x38050040,  // addi        r0, r5, 64
  0x9421FFE8,  // stwu        sp, -0x0018 (sp)
  0x38C00020,  // li          r6, 32
  0x3CA08000   // lis         r5, 0x8000
};

u32 _DVDLowSeek_original[9] = {
  0x7C0802A6,  // mflr        r0
  0x90010004,  // stw         r0, 4 (sp)
  0x38000000,  // li          r0, 0
  0x9421FFE8,  // stwu        sp, -0x0018 (sp)
  0x93E10014,  // stw         r31, 20 (sp)
  0x93C10010,  // stw         r30, 16 (sp)
  0x908D3738,  // stw         r4, 0x3738 (sd1)
  0x3C80CC00,  // lis         r4, 0xCC00
  0x38846000   // addi        r4, r4, 24576
};

//aka AISetStreamPlayState
u32 _AIResetStreamSampleCount_original[9] = {
  0x3C60CC00,  // lis         r3, 0xCC00
  0x80036C00,  // lwz         r0, 0x6C00 (r3)
  0x5400003C,  // rlwinm      r0, r0, 0, 0, 30
  0x7C00EB78,  // or          r0, r0, r29
  0x90036C00,  // stw         r0, 0x6C00 (r3)
  0x80010024,  // lwz         r0, 36 (sp)
  0x83E1001C,  // lwz         r31, 28 (sp)
  0x83C10018,  // lwz         r30, 24 (sp)
  0x7C0803A6   // mtlr        r0
};

u32 sig_fwrite[8] = {
  0x9421FFD0,
  0x7C0802A6,
  0x90010034,
  0xBF210014,
  0x7C992378,
  0x7CDA3378,
  0x7C7B1B78,
  0x7CBC2B78
};

u32 sig_fwrite_alt[8] = {
  0x7C0802A6,
  0x90010004,
  0x9421FFB8,
  0xBF21002C,
  0x3B440000,
  0x3B660000,
  0x3B830000,
  0x3B250000
};

u32 sig_fwrite_alt2[8] = {
  0x7C0802A6,
  0x90010004,
  0x9421FFD0,
  0xBF410018,
  0x3BC30000,
  0x3BE40000,
  0x80ADB430,
  0x3C055A01 
};

static const u32 geckoPatch[31] = {
  0x7C8521D7,
  0x40810070,
  0x3CE0CC00,
  0x38C00000,
  0x7C0618AE,
  0x5400A016,
  0x6408B000,
  0x380000D0,
  0x90076814,
  0x7C0006AC,
  0x91076824,
  0x7C0006AC,
  0x38000019,
  0x90076820,
  0x7C0006AC,
  0x80076820,
  0x7C0004AC,
  0x70090001,
  0x4082FFF4,
  0x80076824,
  0x7C0004AC,
  0x39200000,
  0x91276814,
  0x7C0006AC,
  0x74090400,
  0x4182FFB8,
  0x38C60001,
  0x7F862000,
  0x409EFFA0,
  0x7CA32B78,
  0x4E800020
};

u32 __viInit1[10] = {
	0x7C0802A6,
	0x3C808000,
	0x90010004,
	0x5460F0BE,
	0x9421FFD8,
	0x93E10024,
	0x547F07BE,
	0x93C10020,
	0x7C1E0378,
	0x900400CC	
};

u32 __viInit2[12] = {
	0x7C0802A6,
	0x90010004,
	0x9421FFD0,
	0x93E1002C,
	0x547FF0BE,
	0x93C10028,
	0x93A10024,
	0x3BA30000,
	0x3C608000,
	0x93E300CC,
	0x387D0000,
	0x57BE07BC
};

u32 _getTiming_v2[28] = {
	0x38650044,  // addi        r3, r5, 68	//VI_TVMODE_NTSC_INT
	0x4E800020,  // blr
	0x3865006A,  // addi        r3, r5, 106	//VI_TVMODE_NTSC_DS
	0x4E800020,  // blr
	0x38650090,  // addi        r3, r5, 144	//VI_TVMODE_PAL_INT
	0x4E800020,  // blr
	0x386500B6,  // addi        r3, r5, 182	//VI_TVMODE_PAL_DS
	0x4E800020,  // blr
	0x38650044,  // addi        r3, r5, 68	//VI_TVMODE_NTSC_INT
	0x4E800020,  // blr
	0x3865006A,  // addi        r3, r5, 106	//VI_TVMODE_NTSC_DS
	0x4E800020,  // blr
	0x386500DC,  // addi        r3, r5, 220	//VI_TVMODE_MPAL_INT
	0x4E800020,  // blr
	0x38650102,  // addi        r3, r5, 258	//VI_TVMODE_MPAL_DS
	0x4E800020,  // blr
	0x38650128,  // addi        r3, r5, 296	//VI_TVMODE_NTSC_PROG
	0x4E800020,  // blr
	0x3865014E,  // addi        r3, r5, 334	//VI_TVMODE_NTSC_PROG_DS
	0x4E800020,  // blr
	0x38650090,  // addi        r3, r5, 144	//VI_TVMODE_PAL_INT
	0x4E800020,  // blr
	0x386500B6,  // addi        r3, r5, 182	//VI_TVMODE_PAL_DS
	0x4E800020,  // blr
	0x38650174,  // addi        r3, r5, 372	//?
	0x4E800020,  // blr
	0x3865019A,  // addi        r3, r5, 410	//?
	0x4E800020,  // blr
	/*0x38600000,  // li          r3, 0		//fail(?)
	0x4E800020   // blr*/
};

u32 _getTiming_v1[25] = {
	0x7CA32B78,  // mr          r3, r5		//VI_TVMODE_NTSC_INT 
	0x4E800020,  // blr
	0x38650026,  // addi        r3, r5, 38	//VI_TVMODE_NTSC_DS
	0x4E800020,  // blr
	0x3865004C,  // addi        r3, r5, 76	//VI_TVMODE_PAL_INT
	0x4E800020,  // blr
	0x38650072,  // addi        r3, r5, 114	//VI_TVMODE_PAL_DS
	0x4E800020,  // blr
	0x7CA32B78,  // mr          r3, r5		//VI_TVMODE_NTSC_INT 
	0x4E800020,  // blr
	0x38650026,  // addi        r3, r5, 38	//VI_TVMODE_NTSC_DS
	0x4E800020,  // blr
	0x38650098,  // addi        r3, r5, 152	//VI_TVMODE_MPAL_INT
	0x4E800020,  // blr
	0x386500BE,  // addi        r3, r5, 190	//VI_TVMODE_MPAL_DS
	0x4E800020,  // blr
	0x386500E4,  // addi        r3, r5, 228	//VI_TVMODE_NTSC_PROG 
	0x4E800020,  // blr
	0x3865010A,  // addi        r3, r5, 266	//VI_TVMODE_NTSC_PROG_DS
	0x4E800020,  // blr
	0x3865004C,  // addi        r3, r5, 76	//VI_TVMODE_PAL_INT
	0x4E800020,  // blr
	0x38650072,  // addi        r3, r5, 114	//VI_TVMODE_PAL_DS
	0x4E800020,  // blr
	0x38600000   // li          r3, 0		//fail
};

u32 __viInitPatch[4] = {
	0x7C0802A6,  //	mflr	0						# this is what we patched over, so do it here.
	0x3C608000,  //	lis 	3,		0x8000                                          
	0x80632F40,  //	lwz 	3,		0x2F40(3)		# load our video mode to r3 ;)       
	0x4E800020   //	blr                                                      
};

u32 GXSETVAT_NTSC_orig[32] = {
    0x8142ce00, 0x39800000, 0x39600000, 0x3ce0cc01,
    0x48000070, 0x5589063e, 0x886a04f2, 0x38000001,
    0x7c004830, 0x7c600039, 0x41820050, 0x39000008,
    0x99078000, 0x61230070, 0x380b001c, 0x98678000,
    0x61250080, 0x388b003c, 0x7cca002e, 0x61230090,
    0x380b005c, 0x90c78000, 0x99078000, 0x98a78000,
    0x7c8a202e, 0x90878000, 0x99078000, 0x98678000,
    0x7c0a002e, 0x90078000, 0x396b0004, 0x398c0001
};

u32 GXSETVAT_NTSC_patched[32] = {
    0x8122ce00, 0x39400000, 0x896904f2, 0x7d284b78,
    0x556007ff, 0x41820050, 0x38e00008, 0x3cc0cc01,
    0x98e68000, 0x61400070, 0x61440080, 0x61430090,
    0x98068000, 0x38000000, 0x80a8001c, 0x90a68000,
    0x98e68000, 0x98868000, 0x8088003c, 0x90868000,
    0x98e68000, 0x98668000, 0x8068005c, 0x90668000,
    0x98068000, 0x556bf87f, 0x394a0001, 0x39080004,
    0x4082ffa0, 0x38000000, 0x980904f2, 0x4e800020
};

u32 GXSETVAT_PAL_orig[32] = {   
    0x8142cdd0, 0x39800000, 0x39600000, 0x3ce0cc01,
    0x48000070, 0x5589063e, 0x886a04f2, 0x38000001,
    0x7c004830, 0x7c600039, 0x41820050, 0x39000008,
    0x99078000, 0x61230070, 0x380b001c, 0x98678000,
    0x61250080, 0x388b003c, 0x7cca002e, 0x61230090,
    0x380b005c, 0x90c78000, 0x99078000, 0x98a78000,
    0x7c8a202e, 0x90878000, 0x99078000, 0x98678000,
    0x7c0a002e, 0x90078000, 0x396b0004, 0x398c0001
};
   
const u32 GXSETVAT_PAL_patched[32] = {
    0x8122cdd0, 0x39400000, 0x896904f2, 0x7d284b78,
    0x556007ff, 0x41820050, 0x38e00008, 0x3cc0cc01,
    0x98e68000, 0x61400070, 0x61440080, 0x61430090,
    0x98068000, 0x38000000, 0x80a8001c, 0x90a68000,
    0x98e68000, 0x98868000, 0x8088003c, 0x90868000,
    0x98e68000, 0x98668000, 0x8068005c, 0x90668000,
    0x98068000, 0x556bf87f, 0x394a0001, 0x39080004,
    0x4082ffa0, 0x38000000, 0x980904f2, 0x4e800020
};

void writeBranchLink(unsigned int sourceAddr,unsigned int destAddr) {
	unsigned int temp;
		
	//write the new instruction to the source
	temp = ((destAddr)-(sourceAddr));	//calc addr
	temp &= 0x03FFFFFF;					//extract only addr
	temp |= 0x4B000001;					//make it a bl opcode
	*(volatile unsigned int*)sourceAddr = temp;	//write it to the dest
}

void install_code()
{
	
  /* copy the right patchcode */
  	if(curDevice == IDEEXI)	{	
	  	// IDE-EXI in slot A
  		if((GC_SD_CHANNEL==0)) {
	  		memcpy((void*)base_addr,slot_a_hdd,slot_a_hdd_size);
  		}
  		else {
	  		// IDE-EXI in slot B
	  		memcpy((void*)base_addr,slot_b_hdd,slot_b_hdd_size);
  		}
  	}
	else if((SDHCCard == 1) && (GC_SD_CHANNEL==0))	{	//byte addressing
		memcpy((void*)base_addr,slot_a_sd,slot_a_sd_size);
	}
	else if((SDHCCard == 0) && (GC_SD_CHANNEL==0))	{	//sector addressing
		memcpy((void*)base_addr,slot_a_sdhc,slot_a_sdhc_size);
 	}
 	else if((SDHCCard == 1) && (GC_SD_CHANNEL==1))	{	//byte addressing
 		memcpy((void*)base_addr,slot_b_sd,slot_b_sd_size);
 	}
 	else if((SDHCCard == 0) && (GC_SD_CHANNEL==1))	{	//sector addressing
 		memcpy((void*)base_addr,slot_b_sdhc,slot_b_sdhc_size);
 	}
  	
}

void dvd_patchDVDRead(void *addr, u32 len) {
	void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,_Read_original,sizeof(_Read_original))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (base_addr >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (base_addr & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 92) = 0x3C60AB00; // lis         r3, 0xAB00 (make it a seek)
  			*(unsigned int*)(addr_start + 112) = 0x38000001;//  li          r0, 1 (IMM not DMA)
		}		
		if(memcmp(addr_start,_Read_original_2,sizeof(_Read_original_2))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (base_addr >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (base_addr & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 68) = 0x3C00AB00;	//  lis         r0, 0xAB00 (make it a seek)
  			*(unsigned int*)(addr_start + 128) = 0x38000001;//  li          r0, 1 (IMM not DMA)
		}
		if(memcmp(addr_start,_Read_original_3,sizeof(_Read_original_3))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (base_addr >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (base_addr & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 84) = 0x3C60AB00; // lis         r3, 0xAB00 (make it a seek)
  			*(unsigned int*)(addr_start + 104) = 0x38000001;//  li          r0, 1 (IMM not DMA)
		}
		addr_start += 4;
	}
}

void dvd_patchAISCount(void *addr, u32 len) {
  void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,_AIResetStreamSampleCount_original,sizeof(_AIResetStreamSampleCount_original))==0) 
		{
      *(u32*)(addr_start+12) = 0x60000000;  //NOP
		}
		addr_start += 4;
	}
}

void dvd_patchreset(void *addr,u32 len)
{
	void *copy_to,*cache_ptr;
	void *addr_start = addr;
	void *addr_end = addr+len;

	if(len<sizeof(_dvdlowreset_org)) return;

	while(addr_start<addr_end) {
		if(memcmp(addr_start,_dvdlowreset_org,sizeof(_dvdlowreset_org))==0) {
			// we found the DVDLowReset
			copy_to = (addr_start+0x18);
			cache_ptr = (void*)((u32)addr_start&~0x1f);

			ICInvalidateRange(cache_ptr,sizeof(_dvdlowreset_new)*2);
			memcpy(copy_to,_dvdlowreset_new,sizeof(_dvdlowreset_new));
		}
		addr_start += 4;
	}
}

void dvd_patchfwrite(void *addr, u32 len) {
	void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,&sig_fwrite[0],sizeof(sig_fwrite))==0) 
		{
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));	
		}
		if(memcmp(addr_start,&sig_fwrite_alt[0],sizeof(sig_fwrite_alt))==0) 
		{
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));	
		}
		if(memcmp(addr_start,&sig_fwrite_alt2[0],sizeof(sig_fwrite_alt2))==0) 
		{
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));	
		}
		addr_start += 4;
	}

}


void dvd_patchDVDReadID(void *addr, u32 len) {
  void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,_DVDLowReadDiskID_original,sizeof(_DVDLowReadDiskID_original))==0) 
		{
      writeBranchLink((u32)addr_start+8,ID_JUMP_OFFSET);
		}
		addr_start += 4;
	}
}

void dvd_patchDVDLowSeek(void *addr, u32 len) {
  void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,_DVDLowSeek_original,sizeof(_DVDLowSeek_original))==0) 
		{
      writeBranchLink((u32)addr_start+4,SEEK_JUMP_OFFSET);
		}
		addr_start += 4;
	}
}

void copyMode(int mode) {
	if(mode == NTSC) {
		*(volatile unsigned long*)0x80002F40 = 0;
		*(volatile unsigned long*)0x800000CC = 0;
	}
	else if(mode == PAL50) {
		*(volatile unsigned long*)0x80002F40 = 4;
		*(volatile unsigned long*)0x800000CC = 1;
	}
	else if(mode == PAL60) {
		*(volatile unsigned long*)0x80002F40 = 20;
		*(volatile unsigned long*)0x800000CC = 5;
	}
	else if(mode == P480) {
		*(volatile unsigned long*)0x80002F40 = 2;
		*(volatile unsigned long*)0x800000CC = 0;
	}
	memcpy((void*)VI_JUMP_OFFSET,__viInitPatch,16);
}

void dvd_patchVideoMode(void *addr, u32 len,int mode)
{
	if(mode == AUTO) return;
	
	void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,__viInit1,sizeof(__viInit1))==0) 
		{
			copyMode(mode);
			writeBranchLink((u32)addr_start,VI_JUMP_OFFSET);
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Patched VI Mode v1");
			DrawFrameFinish();
			sleep(2);
		}
		
		if(memcmp(addr_start,__viInit2,sizeof(__viInit2))==0) 
		{
			copyMode(mode);
			writeBranchLink((u32)addr_start,VI_JUMP_OFFSET);
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Patched VI Mode v2");
			DrawFrameFinish();
			sleep(2);
		}
		if(memcmp(addr_start,_getTiming_v1,sizeof(_getTiming_v1))==0) 
		{	
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Patched GetTiming v1");
			DrawFrameFinish();
			sleep(2);
			
			*(volatile unsigned int*)(addr_start-4) = 0x60000000;  //nop
			if(mode==P480) {
				*(volatile unsigned int*)(addr_start) = 0x386500E4;
			}
			if(mode == NTSC) {
				*(volatile unsigned int*)(addr_start) = 0x7CA32B78;
			}
			if(mode == PAL50) {
				*(volatile unsigned int*)(addr_start) = 0x38650090;
			}
		}
		if(memcmp(addr_start,_getTiming_v2,sizeof(_getTiming_v2))==0) 
		{	
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Patched GetTiming v2");
			DrawFrameFinish();
			sleep(2);
			
			*(volatile unsigned int*)(addr_start-4) = 0x60000000;  //nop
			if(mode==P480) {
				*(volatile unsigned int*)(addr_start) = 0x38650128;
			}
			if(mode == NTSC) {
				*(volatile unsigned int*)(addr_start) = 0x38650044;
			}
			if(mode == PAL50) {
				*(volatile unsigned int*)(addr_start) = 0x38650090;
			}
		}
		addr_start += 4;
	}
}

void set_base_addr(int useHi) {
	base_addr = useHi ? HI_RESERVE : LO_RESERVE;
}

u32 get_base_addr() {
	return base_addr;
}

void patchZeldaWW(void *addr, u32 len,int mode) {
	void *addr_start = addr;
	void *addr_end = addr+len;	
	
	while(addr_start<addr_end) 
	{
		if(mode == 1 && memcmp(addr_start,GXSETVAT_PAL_orig,sizeof(GXSETVAT_PAL_orig))==0) 
		{
			memcpy(addr_start, GXSETVAT_PAL_patched, sizeof(GXSETVAT_PAL_patched));
		}
		if(mode == 2 && memcmp(addr_start,GXSETVAT_NTSC_orig,sizeof(GXSETVAT_NTSC_orig))==0) 
		{
			memcpy(addr_start, GXSETVAT_NTSC_patched, sizeof(GXSETVAT_NTSC_patched));
		}
		addr_start += 4;
	}
}
