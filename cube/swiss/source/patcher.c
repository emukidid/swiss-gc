/* DOL Patching code by emu_kidid */


#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "swiss.h"
#include "main.h"
#include "patcher.h"
#include "TDPATCH.H"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"

static unsigned int base_addr = LO_RESERVE;

/*** externs ***/
extern GXRModeObj *vmode;		/*** Graphics Mode Object ***/
extern u32 *xfb[2];			   	/*** Framebuffers ***/
extern int SDHCCard;
extern int GC_SD_CHANNEL;

extern void animateBox(int x1,int y1, int x2, int y2, int color,char *msg);

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
	0x7C, 0x08, 0x02, 0xA6,  // mflr        r0              
	0x90, 0x01, 0x00, 0x04,  // stw         r0, 4 (sp)      
	0x38, 0x00, 0x00, 0x01,  // li          r0, 1           
	0x94, 0x21, 0xFF, 0xD8,  // stwu        sp, -0x0028 (sp)
	0x93, 0xE1, 0x00, 0x24,  // stw         r31, 36 (sp)    
	0x93, 0xC1, 0x00, 0x20,  // stw         r30, 32 (sp)    
	0x3B, 0xC5, 0x00, 0x00,  // addi        r30, r5, 0      
	0x93, 0xA1, 0x00, 0x1C,  // stw         r29, 28 (sp)    
	0x3B, 0xA4, 0x00, 0x00,  // addi        r29, r4, 0      
	0x93, 0x81, 0x00, 0x18,  // stw         r28, 24 (sp)    
	0x3B, 0x83, 0x00, 0x00,  // addi        r28, r3, 0      
	0x90, 0xCD               // stw r6 to someplace in (sd1) - differs on different sdk's
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

u32 _OSRestoreInterrupts_original[9] = {
	0x2c030000,	// cmpwi       r3, 0
	0x7c8000a6,	// mfmsr       r4
	0x4182000c,	// beq-        +c
	0x60858000,	// ori         r5, r4, 0x8000
	0x48000008,	// b           +8
	0x5485045e,	// rlwinm      r5, r4, 0, 17, 15
	0x7ca00124,	// mtmsr       r5
	0x54848ffe,	// rlwinm      r4, r4, 17, 31, 31
	0x4e800020	// blr
};

u32 _OSRestoreInterrupts_original_v2[9] = {
	0x2c030000,	// cmpwi       r3, 0
	0x7c8000a6,	// mfmsr       r4
	0x4182000c,	// beq-        +c
	0x60858000,	// ori         r5, r4, 0x8000
	0x48000008,	// b           +8
	0x5485045e,	// rlwinm      r5, r4, 0, 17, 15
	0x7ca00124,	// mtmsr       r5
	0x54838ffe,	// rlwinm      r3, r4, 17, 31, 31
	0x4e800020	// blr
};

void set_base_addr(int useHi) {
	base_addr = useHi ? HI_RESERVE : LO_RESERVE;
}

u32 get_base_addr() {
	return base_addr;
}

// BROKEN THIS DOES NOT ENSURE ANYTHING ANYMORE!
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
	DCFlushRange((void*)base_addr,0x1800);
	ICInvalidateRange((void*)base_addr,0x1800);
	
	// IDE-EXI
  	if(deviceHandler_initial == &initial_IDE0 || deviceHandler_initial == &initial_IDE1) {
	  	memcpy((void*)base_addr,hdd_bin,hdd_bin_size);
  	}
	// SD Gecko
	else if(deviceHandler_initial == &initial_SD0 || deviceHandler_initial == &initial_SD1) {
		memcpy((void*)base_addr,sd_bin,sd_bin_size);
	}
	// DVD 2 disc code
	else if((deviceHandler_initial == &initial_DVD) && (drive_status == DEBUG_MODE)) {
		memcpy((void*)0x80001800,TDPatch,TDPATCH_LEN);
	}
}

int find_pattern( u8 *data, u32 length, FuncPattern *functionPattern )
{
	u32 i;
	FuncPattern FP;

	memset( &FP, 0, sizeof(FP) );

	for( i = 0; i < length; i+=4 )
	{
		u32 word =  *(u32*)(data + i);
		
		if( (word & 0xFC000003) ==  0x48000001 )
			FP.FCalls++;

		if( (word & 0xFC000003) ==  0x48000000 )
			FP.Branch++;
		if( (word & 0xFFFF0000) ==  0x40800000 )
			FP.Branch++;
		if( (word & 0xFFFF0000) ==  0x41800000 )
			FP.Branch++;
		if( (word & 0xFFFF0000) ==  0x40810000 )
			FP.Branch++;
		if( (word & 0xFFFF0000) ==  0x41820000 )
			FP.Branch++;
		
		if( (word & 0xFC000000) ==  0x80000000 )
			FP.Loads++;
		if( (word & 0xFF000000) ==  0x38000000 )
			FP.Loads++;
		if( (word & 0xFF000000) ==  0x3C000000 )
			FP.Loads++;
		
		if( (word & 0xFC000000) ==  0x90000000 )
			FP.Stores++;
		if( (word & 0xFC000000) ==  0x94000000 )
			FP.Stores++;

		if( (word & 0xFF000000) ==  0x7C000000 )
			FP.Moves++;

		if( word == 0x4E800020 )
			break;
	}

	FP.Length = i;

	if(!functionPattern) {
		print_gecko("Length: 0x%02X\r\n", FP.Length );
		print_gecko("FCalls: %d\r\n", FP.FCalls );
		print_gecko("Loads : %d\r\n", FP.Loads );
		print_gecko("Stores: %d\r\n", FP.Stores );
		print_gecko("Branch : %d\r\n", FP.Branch);
		print_gecko("Moves: %d\r\n", FP.Moves);
		return 0;
	}
	

	if( memcmp( &FP, functionPattern, sizeof(u32) * 6 ) == 0 )
		return 1;
	else
		return 0;
}

// Not working:
// HANGS: Metroid Prime: << Dolphin SDK - DVD    release build: Oct 29 2002 09:56:49 (0x2301) >>
// Smash Bros. Melee / Kirby Air Ride - r13 issue

// Working: 
// Simpsons Hit & Run: << Dolphin SDK - DVD    release build: Sep  5 2002 05:34:06 (0x2301) >>
// Burnout 2: << Dolphin SDK - DVD    release build: Oct 29 2002 09:56:49 (0x2301) >>
// GB Player Disc: << Dolphin SDK - DVD    release build: Dec 26 2002 21:33:56 (0x2407) >>
// F-Zero GX: << Dolphin SDK - DVD    release build: Jul 23 2003 11:27:57 (0x2301) >>
// Zelda: 4 Swords: << Dolphin SDK - DVD    release build: Sep 16 2003 09:50:54 (0x2301) >>
// Dragonball Z Sagas: << Dolphin SDK - DVD.release build: Feb 12 2004 05:02:49 (0x2301) >>
// Zelda: Twilight Princess: << Dolphin SDK - DVD    release build: Apr  5 2004 04:14:51 (0x2301) >>
int Patch_DVDHighLevelRead(u8 *data, u32 length) {
	int i, j, count = 0, dis_int = swissSettings.disableInterrupts;
	FuncPattern DVDReadSigs[2] = {
	{ 	0xBC,   20,     3,      3,      4,      7, 
		dis_int ? DVDReadAsync:DVDReadAsyncInt,  dis_int ? DVDReadAsync_length:DVDReadAsyncInt_length, "DVDReadAsync" },
	{ 	0x114,        23,     2,      6,      9,      8, 
		dis_int ? DVDRead:DVDReadInt,   dis_int ? DVDRead_length:DVDReadInt_length,  "DVDRead" }};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		for( j=0; j < 2; j++ )
		{
			if( find_pattern( (u8*)(data+i), length, &(DVDReadSigs[j]) ) )
			{
				print_gecko("Found [%s] @ 0x%08X len %i\n", DVDReadSigs[j].Name, (u32)data + i, DVDReadSigs[j].Length);
				
				print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
						DVDReadSigs[j].Name, (u32)DVDReadSigs[j].Patch, (u32)data + i, DVDReadSigs[j].PatchLength);
							
				memcpy( (u8*)(data+i), &DVDReadSigs[j].Patch[0], DVDReadSigs[j].PatchLength );
				DCFlushRange((u8*)(data+i), DVDReadSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), DVDReadSigs[j].Length);
				count++;
			}
		}

		if( count == 2 )
			break;
	}
	Patch_OSRestoreInterrupts(data, length);
	return count;
}

int Patch_DVDLowLevelRead(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	int patched = 0;
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,_Read_original,sizeof(_Read_original))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (READ_TYPE1_V1_OFFSET >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (READ_TYPE1_V1_OFFSET & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 92) = 0x3C60E000; // lis         r3, 0xE000 (make it a seek)
  			*(unsigned int*)(addr_start + 112) = 0x38000001;//  li          r0, 1 (IMM not DMA)
  			print_gecko("Read V1 patched\r\n");
			patched = 1;
		}		
		if(memcmp(addr_start,_Read_original_2,sizeof(_Read_original_2))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (READ_TYPE1_V2_OFFSET >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (READ_TYPE1_V2_OFFSET & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 68) = 0x3C00E000;	//  lis         r0, 0xE000 (make it a seek)
  			*(unsigned int*)(addr_start + 128) = 0x38000001;//  li          r0, 1 (IMM not DMA)
  			print_gecko("Read V2 patched\r\n");
			patched = 1;
		}
		if(memcmp(addr_start,_Read_original_3,sizeof(_Read_original_3))==0) 
		{
      		*(unsigned int*)(addr_start + 8) = 0x3C000000 | (READ_TYPE1_V3_OFFSET >> 16); // lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 12) = 0x60000000 | (READ_TYPE1_V3_OFFSET & 0xFFFF); // ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 16) = 0x7C0903A6; // mtctr	0          
  			*(unsigned int*)(addr_start + 20) = 0x4E800421; // bctrl  
  			*(unsigned int*)(addr_start + 84) = 0x3C60E000; // lis         r3, 0xE000 (make it a seek)
  			*(unsigned int*)(addr_start + 104) = 0x38000001;//  li          r0, 1 (IMM not DMA)
  			print_gecko("Read V3 patched\r\n");
			patched = 1;
		}
		addr_start += 4;
	}
	return patched;
}

int Patch_OSRestoreInterrupts(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	int patched = 0;
	while(addr_start<addr_end) 
	{
		if(	memcmp(addr_start,_OSRestoreInterrupts_original,sizeof(_OSRestoreInterrupts_original))==0  || 
			memcmp(addr_start,_OSRestoreInterrupts_original_v2,sizeof(_OSRestoreInterrupts_original_v2))==0)
		{
			*(unsigned int*)(addr_start + 0) = 0x3C000000 | (OS_RESTORE_INT_OFFSET >> 16); 		// lis		0, 0x8000 (example)   
  			*(unsigned int*)(addr_start + 4) = 0x60000000 | (OS_RESTORE_INT_OFFSET & 0xFFFF); 	// ori		0, 0, 0x1800 (example)
  			*(unsigned int*)(addr_start + 8) = 0x7C0903A6; // mtctr	0
  			*(unsigned int*)(addr_start + 12) = 0x4E800420; // bctr, the function we call will blr
			patched = 1;
			print_gecko("Patched OSRestoreInterrupts @ %08X\r\n",(u32)addr_start);
		}
		addr_start += 4;
	}
	return patched;
}

/** SDK VIConfigure patch to force 480p mode */

// Copy me to 0x80002808 - thx qoob
static const u32 patch_viconfigure_480p[22] = {
	0x00000002,
	0x028001E0,
	0x01E00028,
	0x00000280,
	0x01E00000,
	0x00000000,
	0x38800006,  		// li          r4, 6
	0x7C8903A6,  		// mtctr       r4
	0x3CC08000,  		// lis         r6, 0x8000
	0x60C42804,  		// ori         r4, r6, 0x2804
	0x80A40004,  		// lwz         r5, 4 (r4)
	0x7CA51670,  		// srawi       r5, r5, 2
	0x90A600CC,  		// stw         r5, 0x00CC (r6)
	0x3863FFFC,  		// subi        r3, r3, 4
	0x84A40004,  		// lwzu        r5, 4 (r4)
	0x94A30004,  		// stwu        r5, 4 (r3)
	0x4200FFF8,  		// bdnz+       0x80002840 ?
	0x7C6000A6,  		// mfmsr       r3
	0x5464045E,  		// rlwinm      r4, r3, 0, 17, 15
	0x7C800124,  		// mtmsr       r4
	0x54638FFE,  		// rlwinm      r3, r3, 17, 31, 31
	0x4E800020  		// blr
};

int Patch_480pVideo(u8 *data, u32 length) {
	int i,j;
	FuncPattern VIConfigureSigs[2] = {	
		{0x824, 111, 44, 13, 53, 64, 0, 0, "VIConfigure_v1" },
		{0x798, 105, 44, 12, 38, 63, 0, 0, "VIConfigure_v2"}
	};
	if(swissSettings.gameVMode != 2) {
		return 0;
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;
		for(j = 0; j < 2; j++) {
			if( find_pattern( (u8*)(data+i), length, &VIConfigureSigs[j] ) )
			{
				sprintf(txtbuffer, "Found [%s] @ 0x%08X len %i\n", VIConfigureSigs[j].Name, (u32)data + i, VIConfigureSigs[j].Length);
				print_gecko(txtbuffer);
				
				sprintf(txtbuffer, "Writing Jump for [%s] at 0x%08X len %i\n", 
						VIConfigureSigs[j].Name, (u32)data + i, VIConfigureSigs[j].PatchLength);
				print_gecko(txtbuffer);

				writeBranchLink((u32)data+i+36,0x80002820);
				DCFlushRange((u8*)(data+i), VIConfigureSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), VIConfigureSigs[j].Length);
				memcpy((void*)0x80002808,patch_viconfigure_480p,sizeof(patch_viconfigure_480p));
				return 1;
			}
		}
	}
	return 0;
}

/** SDK DVD Audio NULL Driver Replacement
	- Allows streaming games to run with no streamed audio */

u32 __dvdLowAudioStatusNULL[17] = {
	// execute function(1); passed in on r4
	0x9421FFC0,	//  stwu        sp, -0x0040 (sp)
	0x7C0802A6,	//  mflr        r0
	0x90010000,	//  stw         r0, 0 (sp)
	0x7C8903A6,	//  mtctr       r4
	0x3C80CC00,	//  lis         r4, 0xCC00
	0x2E830000,	//  cmpwi       cr5, r3, 0
	0x4196000C,	//  beq-        cr5, +0xC ?
	0x38600001,	//  li          r3, 1
	0x48000008,	//  b           +0x8 ?
	0x38600000,	//  li          r3, 0
	0x90646020,	//  stw         r3, 0x6020 (r4)
	0x38600001,	//  li          r3, 1
	0x4E800421,	//  bctrl
	0x80010000,	//  lwz         r0, 0 (sp)
	0x7C0803A6,	//  mtlr        r0
	0x38210040,	//  addi        sp, sp, 64
	0x4E800020	//  blr
};

u32 __dvdLowAudioConfigNULL[10] = {
	// execute callback(1); passed in on r5 without actually touching the drive!
	0x9421FFC0,	//  stwu        sp, -0x0040 (sp)
	0x7C0802A6,	//  mflr        r0
	0x90010000,	//  stw         r0, 0 (sp)
	0x7CA903A6,	//  mtctr       r5
	0x38600001,	//  li          r3, 1
	0x4E800421,	//  bctrl
	0x80010000,	//  lwz         r0, 0 (sp)
	0x7C0803A6,	//  mtlr        r0
	0x38210040,	//  addi        sp, sp, 64
	0x4E800020	//  blr
};

u32 __dvdLowReadAudioNULL[] = {
	// execute callback(1); passed in on r6 without actually touching the drive!
	0x9421FFC0,	//  stwu        sp, -0x0040 (sp)
	0x7C0802A6,	//  mflr        r0
	0x90010000,	//  stw         r0, 0 (sp)
	0x7CC903A6,	//  mtctr       r6
	0x38600001,	//  li          r3, 1
	0x4E800421,	//  bctrl
	0x80010000,	//  lwz         r0, 0 (sp)
	0x7C0803A6,	//  mtlr        r0
	0x38210040,	//  addi        sp, sp, 64
	0x4E800020
};

int Patch_DVDAudioStreaming(u8 *data, u32 length) {
	int i, j, count = 0;
	FuncPattern DVDAudioSigs[3] = {	
		{0x94, 18, 10, 2, 0, 2, (u8*)__dvdLowReadAudioNULL, sizeof(__dvdLowReadAudioNULL), "DVDLowReadAudio" },
		{0x88, 18, 8, 2, 0, 2, (u8*)__dvdLowAudioStatusNULL, sizeof(__dvdLowAudioStatusNULL), "DVDLowAudioStatus" },
		{0x98, 19, 8, 2, 1, 3, (u8*)__dvdLowAudioConfigNULL, sizeof(__dvdLowAudioConfigNULL), "DVDLowAudioConfig" }
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		for( j=0; j < 3; j++ )
		{
			if( find_pattern( (u8*)(data+i), length, &(DVDAudioSigs[j]) ) )
			{
				sprintf(txtbuffer, "Found [%s] @ 0x%08X len %i\n", DVDAudioSigs[j].Name, (u32)data + i, DVDAudioSigs[j].Length);
				print_gecko(txtbuffer);
				
				sprintf(txtbuffer, "Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
						DVDAudioSigs[j].Name, (u32)DVDAudioSigs[j].Patch, (u32)data + i, DVDAudioSigs[j].PatchLength);
				print_gecko(txtbuffer);
							
				memcpy( (u8*)(data+i), &DVDAudioSigs[j].Patch[0], DVDAudioSigs[j].PatchLength );
				DCFlushRange((u8*)(data+i), DVDAudioSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), DVDAudioSigs[j].Length);
				count++;
			}
		}
	}
	return count;
	// TODO see if Ikaruga needs this patch below?
	//else if(memcmp(addr_start,_AIResetStreamSampleCount_original,sizeof(_AIResetStreamSampleCount_original))==0) 
	//	*(u32*)(addr_start+12) = 0x60000000;  //NOP
}

/** SDK DVD Reset Replacement 
	- Allows debug spinup for backups */

static const u32 _dvdlowreset_org[12] = {
	0x7C0802A6,0x3C80CC00,0x90010004,0x38000002,0x9421FFE0,0xBF410008,0x3BE43000,0x90046004,
	0x83C43024,0x57C007B8,0x60000001,0x941F0024
};

static const u32 _dvdlowreset_new[8] = {
	0x3FE08000,0x63FF1800,0x7FE803A6,0x4E800021,0x4800006C,0x60000000,0x60000000,0x60000000
};
	
void Patch_DVDReset(void *addr,u32 length)
{
	void *copy_to,*cache_ptr;
	void *addr_start = addr;
	void *addr_end = addr+length;

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

/** SDK fwrite USB Gecko Slot B redirect */
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

void Patch_Fwrite(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	
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

/** SDK DVDCompareDiskId Patch
	- Make this function return true and also swap our file base for our read replacement to read from */

int Patch_DVDCompareDiskId(u8 *data, u32 length) {
	int i, patched = 0;
	FuncPattern DVDCompareDiskIdSig = 	
		{0xF4, 15, 4, 2, 16, 9, DVDCompareDiskId, DVDCompareDiskId_length, "DVDCompareDiskId" };
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &DVDCompareDiskIdSig ) )
		{
			print_gecko("Found [%s] @ 0x%08X len %i\n", DVDCompareDiskIdSig.Name, (u32)data + i, DVDCompareDiskIdSig.Length);		
			print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
					DVDCompareDiskIdSig.Name, (u32)DVDCompareDiskIdSig.Patch, (u32)data + i, DVDCompareDiskIdSig.PatchLength);

			memcpy( (u8*)(data+i), &DVDCompareDiskIdSig.Patch[0], DVDCompareDiskIdSig.PatchLength );
			DCFlushRange((u8*)(data+i), DVDCompareDiskIdSig.Length);
			ICInvalidateRange((u8*)(data+i), DVDCompareDiskIdSig.Length);
			patched++;
		}
	}
	return patched;
}

/** SDK GXSetVAT patch for Wii compatibility - specific for Zelda WW */

u32 GXSETVAT_PAL_orig[32] = {
    0x8142ce00, 0x39800000, 0x39600000, 0x3ce0cc01,
    0x48000070, 0x5589063e, 0x886a04f2, 0x38000001,
    0x7c004830, 0x7c600039, 0x41820050, 0x39000008,
    0x99078000, 0x61230070, 0x380b001c, 0x98678000,
    0x61250080, 0x388b003c, 0x7cca002e, 0x61230090,
    0x380b005c, 0x90c78000, 0x99078000, 0x98a78000,
    0x7c8a202e, 0x90878000, 0x99078000, 0x98678000,
    0x7c0a002e, 0x90078000, 0x396b0004, 0x398c0001
};

u32 GXSETVAT_PAL_patched[32] = {
    0x8122ce00, 0x39400000, 0x896904f2, 0x7d284b78,
    0x556007ff, 0x41820050, 0x38e00008, 0x3cc0cc01,
    0x98e68000, 0x61400070, 0x61440080, 0x61430090,
    0x98068000, 0x38000000, 0x80a8001c, 0x90a68000,
    0x98e68000, 0x98868000, 0x8088003c, 0x90868000,
    0x98e68000, 0x98668000, 0x8068005c, 0x90668000,
    0x98068000, 0x556bf87f, 0x394a0001, 0x39080004,
    0x4082ffa0, 0x38000000, 0x980904f2, 0x4e800020
};

u32 GXSETVAT_NTSC_orig[32] = {   
    0x8142cdd0, 0x39800000, 0x39600000, 0x3ce0cc01,
    0x48000070, 0x5589063e, 0x886a04f2, 0x38000001,
    0x7c004830, 0x7c600039, 0x41820050, 0x39000008,
    0x99078000, 0x61230070, 0x380b001c, 0x98678000,
    0x61250080, 0x388b003c, 0x7cca002e, 0x61230090,
    0x380b005c, 0x90c78000, 0x99078000, 0x98a78000,
    0x7c8a202e, 0x90878000, 0x99078000, 0x98678000,
    0x7c0a002e, 0x90078000, 0x396b0004, 0x398c0001
};
   
const u32 GXSETVAT_NTSC_patched[32] = {
    0x8122cdd0, 0x39400000, 0x896904f2, 0x7d284b78,
    0x556007ff, 0x41820050, 0x38e00008, 0x3cc0cc01,
    0x98e68000, 0x61400070, 0x61440080, 0x61430090,
    0x98068000, 0x38000000, 0x80a8001c, 0x90a68000,
    0x98e68000, 0x98868000, 0x8088003c, 0x90868000,
    0x98e68000, 0x98668000, 0x8068005c, 0x90668000,
    0x98068000, 0x556bf87f, 0x394a0001, 0x39080004,
    0x4082ffa0, 0x38000000, 0x980904f2, 0x4e800020
};

void Patch_GXSetVATZelda(void *addr, u32 length,int mode) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	
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

/** SDK DVD Info/Query Function patches */

u32 __dvdLowInquiryNULL[] = {
	0x38600001,	//  li          r3, 1
	0x4E800020	//	blr
};

int Patch_DVDStatusFunctions(u8 *data, u32 length) {
	int i, j, count = 0;
	FuncPattern DVDStatusSigs[1] = {	
		{0xCC, 17, 10, 5, 3, 2, (u8*)__dvdLowInquiryNULL, sizeof(__dvdLowInquiryNULL), "DVDInquiryAsync" }
	};

	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		for( j=0; j < 1; j++ )
		{
			if( find_pattern( (u8*)(data+i), length, &(DVDStatusSigs[j]) ) )
			{
				sprintf(txtbuffer, "Found [%s] @ 0x%08X len %i\n", DVDStatusSigs[j].Name, (u32)data + i, DVDStatusSigs[j].Length);
				print_gecko(txtbuffer);
				
				sprintf(txtbuffer, "Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
						DVDStatusSigs[j].Name, (u32)DVDStatusSigs[j].Patch, (u32)data + i, DVDStatusSigs[j].PatchLength);
				print_gecko(txtbuffer);
							
				memcpy( (u8*)(data+i), &DVDStatusSigs[j].Patch[0], DVDStatusSigs[j].PatchLength );
				DCFlushRange((u8*)(data+i), DVDStatusSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), DVDStatusSigs[j].Length);
				count++;
			}
		}
	}
	return count;
}
