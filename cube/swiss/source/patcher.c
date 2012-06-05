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
u8 _Read_original_3[46] = {	//0x801E2E24
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
	// USB Gecko
	else if(deviceHandler_initial == &initial_USBGecko) {
		memcpy((void*)base_addr,usbgecko_bin,usbgecko_bin_size);
	}
}

void make_pattern( u8 *Data, u32 Length, FuncPattern *FunctionPattern )
{
	u32 i;

	memset( FunctionPattern, 0, sizeof(FuncPattern) );

	for( i = 0; i < Length; i+=4 )
	{
		u32 word = *(u32*)(Data + i) ;
		
		if( (word & 0xFC000003) ==  0x48000001 )
			FunctionPattern->FCalls++;

		if( (word & 0xFC000003) ==  0x48000000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x40800000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x41800000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x40810000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x41820000 )
			FunctionPattern->Branch++;
		
		if( (word & 0xFC000000) ==  0x80000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x38000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x3C000000 )
			FunctionPattern->Loads++;
		
		if( (word & 0xFC000000) ==  0x90000000 )
			FunctionPattern->Stores++;
		if( (word & 0xFC000000) ==  0x94000000 )
			FunctionPattern->Stores++;

		if( (word & 0xFF000000) ==  0x7C000000 )
			FunctionPattern->Moves++;

		if( word == 0x4E800020 )
			break;
	}

	FunctionPattern->Length = i;
}
bool compare_pattern( FuncPattern *FPatA, FuncPattern *FPatB  )
{
	if( memcmp( FPatA, FPatB, sizeof(u32) * 6 ) == 0 )
		return true;
	else
		return false;
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
		{ 0xBC, 20, 3, 3, 4, 7, dis_int ? DVDReadAsync:DVDReadAsyncInt,  dis_int ? DVDReadAsync_length:DVDReadAsyncInt_length, "DVDReadAsync", 0 },
		{ 0x114, 23, 2, 6, 9, 8, dis_int ? DVDRead:DVDReadInt,   dis_int ? DVDRead_length:DVDReadInt_length,  "DVDRead", 0 }
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;
			
		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
			
		for( j=0; j < sizeof(DVDReadSigs); j++ )
		{
			if( !DVDReadSigs[j].offsetFoundAt && compare_pattern( &fp, &(DVDReadSigs[j]) ) ) {
				print_gecko("Found [%s] @ 0x%08X len %i\n", DVDReadSigs[j].Name, (u32)data + i, DVDReadSigs[j].Length);
				
				print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
						DVDReadSigs[j].Name, (u32)DVDReadSigs[j].Patch, (u32)data + i, DVDReadSigs[j].PatchLength);
							
				memcpy( (u8*)(data+i), &DVDReadSigs[j].Patch[0], DVDReadSigs[j].PatchLength );
				DCFlushRange((u8*)(data+i), DVDReadSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), DVDReadSigs[j].Length);
				count++;
				DVDReadSigs[j].offsetFoundAt = (u32)data+i;
			}
		}

		if( count == 2 )
			break;
	}
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

/** SDK VIConfigure patch to force 480p/576p mode */
u8 video_timing_480p[] = {
	0x0C,0x00,0x01,0xE0,
	0x00,0x30,0x00,0x30,
	0x00,0x06,0x00,0x06,
	0x18,0x18,0x18,0x18,
	0x04,0x0E,0x04,0x0E,
	0x04,0x0E,0x04,0x0E,
	0x04,0x1A,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75
};

u8 video_timing_576p[] = {
	0x0A,0x00,0x02,0x40,
	0x00,0x3E,0x00,0x3E,
	0x00,0x06,0x00,0x06,
	0x14,0x14,0x14,0x14,
	0x04,0xD8,0x04,0xD8,
	0x04,0xD8,0x04,0xD8,
	0x04,0xE2,0x01,0xB0,
	0x40,0x4B,0x6A,0xAC,
	0x01,0x7C
};

void Patch_ProgTiming(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	while(addr_start<addr_end) 
	{
		if(	memcmp(addr_start,video_timing_480p,sizeof(video_timing_480p))==0)
		{
			memcpy(addr_start, video_timing_576p, sizeof(video_timing_576p));
			print_gecko("Patched Progressive timing to 576p @ %08X\r\n",(u32)addr_start);
			break;
		}
		addr_start += 4;
	}
}

int Patch_ProgVideo(u8 *data, u32 length) {
	int i,j;
	FuncPattern VIConfigureSigs[6] = {
		{0x68C,  87, 41,  6, 31, 60, 0, 0, "VIConfigure_v1", 0},
		{0x6AC,  90, 43,  6, 32, 60, 0, 0, "VIConfigure_v2", 0},
		{0x73C, 100, 43, 13, 34, 61, 0, 0, "VIConfigure_v3", 0},
		{0x798, 105, 44, 12, 38, 63, 0, 0, "VIConfigure_v4", 0},
		{0x824, 111, 44, 13, 53, 64, 0, 0, "VIConfigure_v5", 0},
		{0x804, 110, 44, 13, 49, 63, 0, 0, "VIConfigure_v6", 0}
	};
	if((swissSettings.gameVMode != 2) && (swissSettings.gameVMode != 4)) {
		return 0;
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;
			
		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
			
		for( j=0; j < sizeof(VIConfigureSigs); j++ )
		{
			if( !VIConfigureSigs[j].offsetFoundAt && compare_pattern( &fp, &(VIConfigureSigs[j]) ) ) {
				print_gecko("Found [%s] @ 0x%08X len %i\n", VIConfigureSigs[j].Name, (u32)data + i, VIConfigureSigs[j].Length);			
				print_gecko("Writing Jump for [%s] at 0x%08X len %i\n", 
						VIConfigureSigs[j].Name, (u32)data + i, VIConfigureSigs[j].PatchLength);
				if(swissSettings.gameVMode == 2) {
					memcpy((void*)VAR_PROG_MODE,&ForceProgressive[0],ForceProgressive_length);	// Copy our patch (480p)
					print_gecko("Patched 480p Progressive mode \r\n");
				}
				else {
					memcpy((void*)VAR_PROG_MODE,&ForceProgressive576p[0],ForceProgressive576p_length);	// Copy our patch (576p)
					print_gecko("Patched 576p Progressive mode \r\n");
				}
				memcpy((void*)(VAR_PROG_MODE+40),(void*)(data+i+12),16);	// Copy what we'll overwrite here
				DCFlushRange((void*)VAR_PROG_MODE, ForceProgressive_length);
				ICInvalidateRange((void*)VAR_PROG_MODE, ForceProgressive_length);
				*(unsigned int*)(data +i+ 12) = 0x3C000000 | ((VAR_PROG_MODE+40) >> 16); 		// lis		0, 0x8000 (example)   
				*(unsigned int*)(data +i+ 16) = 0x60000000 | ((VAR_PROG_MODE+40) & 0xFFFF); 	// ori		0, 0, 0x1800 (example)
				*(unsigned int*)(data +i+ 20) = 0x7C0903A6; // mtctr	0
				*(unsigned int*)(data +i+ 24) = 0x4E800421; // bctrl, the function we call will blr
				if(swissSettings.gameVMode == 4) {
					switch (j) {
						case 0: *(unsigned int*)(data+i+ 808) = 0x38A00001; break;	// li		5, 1
						case 1: *(unsigned int*)(data+i+ 840) = 0x38A00001; break;	// li		5, 1
						case 2: *(unsigned int*)(data+i+ 956) = 0x38000001; break;	// li		0, 1
						case 3: *(unsigned int*)(data+i+1032) = 0x38C00001; break;	// li		6, 1
						case 4: *(unsigned int*)(data+i+1160) = 0x38C00001; break;	// li		6, 1
						case 5: *(unsigned int*)(data+i+1180) = 0x38E00001; break;	// li		7, 1
					}
					Patch_ProgTiming(data, length);	// Patch timing to 576p
				}
				VIConfigureSigs[j].offsetFoundAt = (u32)data+i;
				return 1;
			}
		}
	}
	return 0;
}

int Patch_WideAspect(u8 *data, u32 length) {
	int i;
	FuncPattern MTXPerspectiveSig =
		{0xCC, 3, 3, 1, 0, 3, 0, 0, "C_MTXPerspective", 0};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXPerspectiveSig ) )
		{
			print_gecko("Found [%s] @ 0x%08X len %i\n", MTXPerspectiveSig.Name, (u32)data + i, MTXPerspectiveSig.Length);
			*(volatile float *)VAR_ASPECT_FLOAT = 0.5625f;
			memmove((void*)(data+i+ 28),(void*)(data+i+ 36),44);
			memmove((void*)(data+i+188),(void*)(data+i+192),16);
			*(unsigned int*)(data+i+52) = 0x48000001 | ((*(unsigned int*)(data+i+52) & 0x3FFFFFC) + 8);
			*(unsigned int*)(data+i+72) = 0x3C600000 | (VAR_AREA >> 16);			// lis		3, 0x8180
			*(unsigned int*)(data+i+76) = 0xC0230000 | (VAR_ASPECT_FLOAT & 0xFFFF);	// lfs		1, -0x1C (3)
			*(unsigned int*)(data+i+80) = 0xEC240072; // fmuls	1, 4, 1
			return 1;
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
		{0x94, 18, 10, 2, 0, 2, (u8*)__dvdLowReadAudioNULL, sizeof(__dvdLowReadAudioNULL), "DVDLowReadAudio", 0 },
		{0x88, 18, 8, 2, 0, 2, (u8*)__dvdLowAudioStatusNULL, sizeof(__dvdLowAudioStatusNULL), "DVDLowAudioStatus", 0 },
		{0x98, 19, 8, 2, 1, 3, (u8*)__dvdLowAudioConfigNULL, sizeof(__dvdLowAudioConfigNULL), "DVDLowAudioConfig", 0 }
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
			
		for( j=0; j < sizeof(DVDAudioSigs); j++ )
		{
			if( !DVDAudioSigs[j].offsetFoundAt && compare_pattern( &fp, &(DVDAudioSigs[j]) ) )	{
				print_gecko("Found [%s] @ 0x%08X len %i\n", DVDAudioSigs[j].Name, (u32)data + i, DVDAudioSigs[j].Length);
				print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
						DVDAudioSigs[j].Name, (u32)DVDAudioSigs[j].Patch, (u32)data + i, DVDAudioSigs[j].PatchLength);

				memcpy( (u8*)(data+i), &DVDAudioSigs[j].Patch[0], DVDAudioSigs[j].PatchLength );
				DCFlushRange((u8*)(data+i), DVDAudioSigs[j].Length);
				ICInvalidateRange((u8*)(data+i), DVDAudioSigs[j].Length);
				DVDAudioSigs[j].offsetFoundAt = (u32)data+i;
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
		{0xF4, 15, 4, 2, 16, 9, DVDCompareDiskId, DVDCompareDiskId_length, "DVDCompareDiskId", 0 };
	
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
	int i, count = 0;
	FuncPattern DVDStatusSig =	
		{0xCC, 17, 10, 5, 3, 2, (u8*)__dvdLowInquiryNULL, sizeof(__dvdLowInquiryNULL), "DVDInquiryAsync", 0 };

	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		if( find_pattern( (u8*)(data+i), length, &(DVDStatusSig) ) )
		{
			print_gecko("Found [%s] @ 0x%08X len %i\n", DVDStatusSig.Name, (u32)data + i, DVDStatusSig.Length);		
			print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
					DVDStatusSig.Name, (u32)DVDStatusSig.Patch, (u32)data + i, DVDStatusSig.PatchLength);
			memcpy( (u8*)(data+i), &DVDStatusSig.Patch[0], DVDStatusSig.PatchLength );
			DCFlushRange((u8*)(data+i), DVDStatusSig.Length);
			ICInvalidateRange((u8*)(data+i), DVDStatusSig.Length);
			count++;
		}
	}
	return count;
}


/** SDK CARD patches for memory card emulation - from DML, ported to GameCube by emu_kidid */
int Patch_CARDFunctions(u8 *data, u32 length) {
	FuncPattern CPatterns[] =
	{
		{ 0x14C,        28,     12,     7,      12,     4,	CARDFreeBlocks,		CARDFreeBlocks_length,	"CARDFreeBlocks A"		, 0},
		{ 0x11C,        24,     10,     7,      10,     4,	CARDFreeBlocks,		CARDFreeBlocks_length,	"CARDFreeBlocks B"		, 0},
		{ 0x94,			11,     6,      3,      5,      4,	__CARDSync,			__CARDSync_length,		"__CARDSync"			, 0},
		{ 0x50,			6,      3,      2,      2,      2,	CARDCheck,			CARDCheck_length,		"CARDCheck"				, 0},
	//	{ 0x24,			4,      2,      1,      0,      2,	CARDCheckAsync,		CARDCheckAsync_length,	"CARDCheckAsync"		, 0},	// Use me?
		{ 0x58C,        82,     11,     18,     41,     57,	CARDCheckExAsync,	CARDCheckExAsync_length,"CARDCheckExAsync"		, 0},
		{ 0x34,			4,      2,      1,      2,      2,	CARDProbe,			CARDProbe_length,		"CARDProbe"				, 0},
	//	{ 0x1C,			2,      2,      1,      0,      2,	CARDProbe,			CARDProbe_length,		"CARDProbe B"			, 0},	//This is causing more trouble than a hack...
		{ 0x178,        20,     6,      6,      20,     4,	CARDProbeEX,		CARDProbeEX_length,		"CARDProbeEx A"			, 0},
		{ 0x198,        22,     6,      5,      19,     4,	CARDProbeEX,		CARDProbeEX_length,		"CARDProbeEx B"			, 0},
		{ 0x160,        17,     6,      5,      18,     4,	CARDProbeEX,		CARDProbeEX_length,		"CARDProbeEx C"			, 0},
		{ 0x19C,        32,     14,     11,     12,     3,	CARDMountAsync,		CARDMountAsync_length,	"CARDMountAsync A"		, 0},
		{ 0x184,        30,     14,     11,     10,     3,	CARDMountAsync,		CARDMountAsync_length,	"CARDMountAsync B"		, 0},
		{ 0x174,        23,     6,      7,      14,     5,	CARDOpen,			CARDOpen_length,		"CARDOpen A"			, 0},
		{ 0x118,        14,     6,      6,      11,     4,	CARDOpen,			CARDOpen_length,		"CARDOpen B"			, 0},
		{ 0x170,        23,     6,      7,      14,     5,	CARDOpen,			CARDOpen_length,		"CARDOpen C"			, 0},
		{ 0x15C,        27,     6,      5,      15,     6,	CARDFastOpen,		CARDFastOpen_length,	"CARDFastOpen"			, 0},
		{ 0x50,			8,      4,      2,      2,      3,	CARDClose,			CARDClose_length,		"CARDClose"				, 0},	//CODE ME ?
		{ 0x21C,        44,     6,      13,     19,     12,	CARDCreateAsync,	CARDCreateAsync_length,	"CARDCreateAsync A"		, 0},
		{ 0x214,        42,     6,      13,     19,     12,	CARDCreateAsync,	CARDCreateAsync_length,	"CARDCreateAsync B"		, 0},
		{ 0x10C,        25,     6,      9,      9,      5,	CARDDeleteAsync,	CARDDeleteAsync_length,	"CARDDeleteAsync"		, 0},
		{ 0x144,        27,     3,      8,      10,     9,	CARDReadAsync,		CARDReadAsync_length,	"CARDReadAsync A"		, 0},
		{ 0x140,        30,     7,      7,      10,     10,	CARDReadAsync,		CARDReadAsync_length,	"CARDReadAsync B"		, 0},
		{ 0x140,        27,     3,      8,      10,     9,	CARDReadAsync,		CARDReadAsync_length,	"CARDReadAsync C"		, 0},
		{ 0x110,        24,     4,      8,      9,      6,	CARDWriteAsync,		CARDWriteAsync_length,	"CARDWriteAsync A"		, 0},
		{ 0x10C,        23,     4,      8,      9,      6,	CARDWriteAsync,		CARDWriteAsync_length,	"CARDWriteAsync B"		, 0},
		{ 0x128,        25,     9,      9,      6,      5,	CARDGetStatus,		CARDGetStatus_length,	"CARDGetStatus A"		, 0},
		{ 0x110,        25,     9,      8,      6,      5,	CARDGetStatus,		CARDGetStatus_length,	"CARDGetStatus B"		, 0},
		{ 0x124,        25,     9,      9,      6,      5,	CARDGetStatus,		CARDGetStatus_length,	"CARDGetStatus C"		, 0},
		{ 0x170,        29,     9,      9,      12,     5,	CARDSetStatusAsync,	CARDSetStatusAsync_length,	"CARDSetStatusAsync A"	, 0},
		{ 0x16C,        29,     9,      9,      12,     5,	CARDSetStatusAsync,	CARDSetStatusAsync_length,	"CARDSetStatusAsync B"	, 0},
		{ 0xC0,			22,     5,      2,      5,      10,	CARDGetSerialNo,	CARDGetSerialNo_length,	"CARDGetSerialNo"		, 0},
		{ 0x84,			12,     5,      3,      4,      2,	CARDGetEncoding,	CARDGetEncoding_length,	"CARDGetEncoding"		, 0},
		{ 0x80,			11,     5,      3,      4,      2,	CARDGetMemSize,		CARDGetMemSize_length,	"CARDGetMemSize"		, 0}
	};
	
	// TODO:
	// Implement CARDFastDelete
	// Implement CARDGetSectorSize
	// Implement CARDGetXferredBytes
	// Implement CARDRenameAsync
	// Implement CARDUnmount - CARD_RESULT_READY for slot 0 and CARD_RESULT_NOCARD for slot 1
	
	int i, j, k, count = 0, foundCardFuncStart = 0;
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data + i ) != 0x7C0802A6 )
			continue;

		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
			
		for( j=0; j < sizeof(CPatterns); j++ )
		{
			if( !CPatterns[j].offsetFoundAt && compare_pattern( &fp, &(CPatterns[j]) ) )	{
				if( CPatterns[j].Patch == CARDFreeBlocks ) {
					//Check for CARDGetResultCode which is always (when used) above CARDFreeBlocks
					if(*(u32*)(data + i - 0x30 ) == 0x2C030000) {
						print_gecko("Found [CARDGetResultCode] @ 0x%08X\n", (u32)data + i - 0x30 );
						memcpy( data + i - 0x30, CARDGetResultCode, CARDGetResultCode_length );
					}
					foundCardFuncStart = 1;
				}

				if(!foundCardFuncStart)
					continue;
			
				CPatterns[j].offsetFoundAt = (u32)data + i;
				print_gecko("Found [%s] @ 0x%08X len %i\n", CPatterns[j].Name, CPatterns[j].offsetFoundAt, CPatterns[j].Length);
				
				//If by now no CARDProbe is found it won't be so set it to found to prevent CARDProbe B false hits
				if( CPatterns[j].Patch == CARDReadAsync ) {
					for( k=0; k < sizeof(CPatterns)/sizeof(FuncPattern); ++k ) {
						if( CPatterns[k].Patch == CARDProbe ) {
							if( !CPatterns[k].offsetFoundAt )	//Don't overwrite the offset!
								CPatterns[k].offsetFoundAt = -1;
						}
					}
				}
				
				if( strstr( CPatterns[j].Name, "Async" ) != NULL ) {	// We've found an async function, try to find the sync version branching to this async ver
					u32 offset = (u32)data + i;
					int fail = 0;
					
					while(fail < 3)
					{
						if(((*(u32*) offset ) & 0xFC000003 ) == 0x48000001 ) {	// Is this instr a bl opcode?
							if(((((*(u32*)( offset ) & 0x03FFFFFC ) + offset) & 0x03FFFFFC) | 0x80000000) == (((u32)data)+i) ) // does the bl branch to this async function?
								break;
						}

						if(*(u32*)offset == 0x4E800020)
							fail++;

						offset+=4;
					}

					if( fail < 3 ) {
						print_gecko("Found function call to [%s] @ 0x%08X\n", CPatterns[j].Name, offset );
			
						//Forge a branch to +4 in the async function which will land on an instr that clears the Callback (skipped otherwise)
						u32 newval = ((u32)data + i + 4) - offset;
						newval&= 0x03FFFFFC;
						newval|= 0x48000001;
						*(u32*)offset = newval;
					} else {
						print_gecko("No sync function found!\n");
					}
				}
				
				//print_gecko("Writing Patch for [%s] from 0x%08X to 0x%08X len %i\n", 
				//		CPatterns[j].Name, (u32)CPatterns[j].Patch, (u32)data + i, CPatterns[j].PatchLength);
							
				memcpy( (u8*)(data+i), &CPatterns[j].Patch[0], CPatterns[j].PatchLength );
				CPatterns[j].offsetFoundAt = 1;
				count++;
			}
		}
	}
	return count;
}

// Ocarina cheat engine hook - Patch OSSleepThread
int Patch_CheatsHook(u8 *data, u32 length) {
	
}

