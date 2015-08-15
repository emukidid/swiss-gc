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
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"
#include "sidestep.h"
#include "elf.h"

static u32 top_addr = VAR_PATCHES_BASE;

/*** externs ***/
extern GXRModeObj *vmode;		/*** Graphics Mode Object ***/
extern u32 *xfb[2];			   	/*** Framebuffers ***/
extern int SDHCCard;
extern int GC_SD_CHANNEL;

extern void animateBox(int x1,int y1, int x2, int y2, int color,char *msg);
int savePatchDevice = -1;

// Read
FuncPattern ReadDebug = {0xDC, 23, 18, 3, 2, 4, 0, 0, "Read (Debug)", 0};
FuncPattern ReadCommon = {0x10C, 30, 18, 5, 2, 3, 0, 0, "Read (Common)", 0};
FuncPattern ReadUncommon = {0x104, 29, 17, 5, 2, 3, 0, 0, "Read (Uncommon)", 0};

// OSExceptionInit
FuncPattern OSExceptionInitSig = {0x27C, 39, 14, 14, 20, 7, 0, 0, "OSExceptionInit", 0};
FuncPattern OSExceptionInitSigDBG = {0x28C, 61, 6, 18, 14, 14, 0, 0, "OSExceptionInitDBG", 0};

// __DVDInterruptHandler
u16 _dvdinterrupthandler_part[3] = {
	0x6000, 0x002A, 0x0054
};

u32 _osdispatch_part_a[2] = {
	0x3C60CC00, 0x83E33000
};

int install_code()
{
	void *location = (void*)LO_RESERVE;
	u8 *patch = NULL; u32 patchSize = 0;
	
	DCFlushRange(location,0x80003100-LO_RESERVE);
	ICInvalidateRange(location,0x80003100-LO_RESERVE);
	
	
	// IDE-EXI
  	if(deviceHandler_initial == &initial_IDE0 || deviceHandler_initial == &initial_IDE1) {
		patch = &hdd_bin[0]; patchSize = hdd_bin_size;
		print_gecko("Installing Patch for IDE-EXI\r\n");
  	}
	// SD Gecko
	else if(deviceHandler_initial == &initial_SD0 || deviceHandler_initial == &initial_SD1) {
		patch = &sd_bin[0]; patchSize = sd_bin_size;
		print_gecko("Installing Patch for SD Gecko\r\n");
	}
	// DVD 2 disc code
	else if(deviceHandler_initial == &initial_DVD) {
		patch = &dvd_bin[0]; patchSize = dvd_bin_size;
		location = (void*)LO_RESERVE_DVD;
		print_gecko("Installing Patch for DVD\r\n");
	}
	// USB Gecko
	else if(deviceHandler_initial == &initial_USBGecko) {
		patch = &usbgecko_bin[0]; patchSize = usbgecko_bin_size;
		print_gecko("Installing Patch for USB Gecko\r\n");
	}
	// Wiikey Fusion
	else if(deviceHandler_initial == &initial_WKF) {
		patch = &wkf_bin[0]; patchSize = wkf_bin_size;
		print_gecko("Installing Patch for WKF\r\n");
	}
	print_gecko("Space for patch remaining: %i\r\n",top_addr - LO_RESERVE);
	print_gecko("Space taken by vars/video patches: %i\r\n",VAR_PATCHES_BASE-top_addr);
	if(top_addr - LO_RESERVE < patchSize)
		return 0;
	memcpy(location,patch,patchSize);
	return 1;
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
		if( (word & 0xFC000000) ==  0xC0000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x38000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x3C000000 )
			FunctionPattern->Loads++;
		
		if( (word & 0xFC000000) ==  0x90000000 )
			FunctionPattern->Stores++;
		if( (word & 0xFC000000) ==  0x94000000 )
			FunctionPattern->Stores++;
		if( (word & 0xFC000000) ==  0xD0000000 )
			FunctionPattern->Stores++;

		if( (word & 0xFC0007FE) ==  0xFC000090 )
			FunctionPattern->Moves++;
		if( (word & 0xFF000000) ==  0x7C000000 )
			FunctionPattern->Moves++;

		if( word == 0x4E800020 )
			break;
	}

	FunctionPattern->Length = i;
}

bool compare_pattern( FuncPattern *FPatA, FuncPattern *FPatB  )
{
	return memcmp( FPatA, FPatB, sizeof(u32) * 6 ) == 0;
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
		if( (word & 0xFC000000) ==  0xC0000000 )
			FP.Loads++;
		if( (word & 0xFF000000) ==  0x38000000 )
			FP.Loads++;
		if( (word & 0xFF000000) ==  0x3C000000 )
			FP.Loads++;
		
		if( (word & 0xFC000000) ==  0x90000000 )
			FP.Stores++;
		if( (word & 0xFC000000) ==  0x94000000 )
			FP.Stores++;
		if( (word & 0xFC000000) ==  0xD0000000 )
			FP.Stores++;

		if( (word & 0xFC0007FE) ==  0xFC000090 )
			FP.Moves++;
		if( (word & 0xFF000000) ==  0x7C000000 )
			FP.Moves++;

		if( word == 0x4E800020 )
			break;
	}

	FP.Length = i;

	if(!functionPattern) {
		print_gecko("Length: 0x%02X\r\n", FP.Length );
		print_gecko("Loads : %d\r\n", FP.Loads );
		print_gecko("Stores: %d\r\n", FP.Stores );
		print_gecko("FCalls: %d\r\n", FP.FCalls );
		print_gecko("Branch : %d\r\n", FP.Branch);
		print_gecko("Moves: %d\r\n", FP.Moves);
		return 0;
	}
	
	return memcmp( &FP, functionPattern, sizeof(u32) * 6 ) == 0;
}

u32 branch(u32 dst, u32 src)
{
	u32 newval = dst - src;
	newval &= 0x03FFFFFC;
	return newval | 0x48000000;
}

u32 branchAndLink(u32 dst, u32 src)
{
	u32 newval = dst - src;
	newval &= 0x03FFFFFC;
	newval |= 0x48000001;
	return newval;
}

// Redirects 0xCC0060xx reads to VAR_DI_REGS
void PatchDVDInterface( u8 *dst, u32 Length, int dataType )
{
	u32 DIPatched = 0;
	int i;

#define REG_0xCC00 0
#define REG_OFFSET 1
#define REG_USED   2
	
	u32 regs[32][3];
	memset(regs, 0, 32*4*3);
	
	for( i=0; i < Length; i+=4 )
	{
		u32 op = *(u32*)(dst + i);
			
		// lis rX, 0xCC00
		if( (op & 0xFC1FFFFF) == 0x3C00CC00 ) {
			u32 dstR = (op >> 21) & 0x1F;
			if(regs[dstR][REG_USED]) {
				u32 lisOffset = regs[dstR][REG_OFFSET];
				*(u32*)lisOffset = (((*(u32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(u32*)regs[dstR][REG_OFFSET], dstR, (*(u32*)lisOffset) &0xFFFF);
				DIPatched++;
				regs[dstR][REG_USED] = 0;	// This might not eventuate to a 0xCC006000 write
			}
			regs[dstR][REG_0xCC00]	=	1;	// this reg is now 0xCC00
			regs[dstR][REG_OFFSET]	=	(u32)dst + i;
			continue;
		}

		// li rX, x or lis rX, x
		if( (op & 0xFC1F0000) == 0x38000000 || (op & 0xFC1F0000) == 0x3C000000 ) {
			u32 dstR = (op >> 21) & 0x1F;
			if (regs[dstR][REG_0xCC00]) {
				if(regs[dstR][REG_USED]) {
					u32 lisOffset = regs[dstR][REG_OFFSET];
					*(u32*)lisOffset = (((*(u32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(u32*)regs[dstR][REG_OFFSET], dstR, (*(u32*)lisOffset) &0xFFFF);
					DIPatched++;
				}
				regs[dstR][REG_0xCC00] = 0;	// reg is no longer 0xCC00
			}
			continue;
		}

		// addi rX, rY, 0x6000 (di)
		if( (op & 0xFC000000) == 0x38000000 ) {
			u32 src = (op >> 16) & 0x1F;
			if( regs[src][REG_0xCC00] )	{	// The source register is 0xCC00, patch this addi
				// Hack to fix a few games
				// If the next instruction is a lhz
				u32 nextOp = *(u32*)(dst + i+4);
				if(( (nextOp & 0xF8000000 ) == 0xA0000000 )) {
					u32 nextOpSrc = (nextOp >> 16) & 0x1F;
					if(nextOpSrc == src) {
						regs[src][REG_0xCC00] = 0;	// No 0xCC0060xx code uses load half op
						continue;
					}
				}
				
				// else just patch.
				if( (op & 0xFC00FF00) == 0x38006000 ) {
					*(u32*)(dst + i) = op - (0x6000 - (VAR_DI_REGS&0xFFFF));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: addi r%u, %04X\r\n", properAddress, *(u32*)(dst + i), src, *(u32*)(dst + i) &0xFFFF);
					regs[src][REG_USED]=1;	// was used in a 0xCC006000 addi
					DIPatched++;
				}
			}
			continue;
		}
		// ori rX, rY, 0x6000 (di)
		else if ((op & 0xFC00FF00) == 0x60006000) 
		{
			u32 src = (op >> 16) & 0x1F;
			if( regs[src][REG_0xCC00] )		// The source register is 0xCC00, patch this ori
			{
				*(u32*)(dst + i) -= (0x6000 - (VAR_DI_REGS&0xFFFF));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: ori r%u, %04X\r\n", properAddress, *(u32*)(dst + i), src, *(u32*)(dst + i) &0xFFFF);
				regs[src][REG_USED]=1;	// was used in a 0xCC006000 ori
				DIPatched++;
			}
			continue;
		}
		// lwz and lwzu, stw and stwu
		if (((op & 0xF8000000) == 0x80000000) || ( (op & 0xF8000000 ) == 0x90000000 ) || ( (op & 0xF8000000 ) == 0xA0000000 ) )
		{
			u32 src = (op >> 16) & 0x1F;
			//u32 dstR = (op >> 21) & 0x1F;
			u32 val = op & 0xFFFF;

			if( regs[src][REG_0xCC00] && ((val & 0xFF00) == 0x6000)) // case with 0x60XY(rZ) (di)
			{
				*(u32*)(dst + i) -= (0x6000 - (VAR_DI_REGS&0xFFFF));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: mem r%u, %04X\r\n", properAddress, *(u32*)(dst + i), src, *(u32*)(dst + i) &0xFFFF);
				regs[src][REG_USED]=1;	// was used in a 0xCC006000 load/store
				DIPatched++;
			}
			continue;
		}
		// blr, flush out and reset
		if(op == 0x4E800020) {
			int x = 0;
			for (x = 0; x < 32; x++) {
				if(regs[x][REG_0xCC00] && regs[x][REG_USED]) {
					u32 lisOffset = regs[x][REG_OFFSET];
					*(u32*)lisOffset = (((*(u32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(u32*)regs[x][REG_OFFSET], x, (*(u32*)lisOffset) &0xFFFF);
					DIPatched++;
				}
			}
			memset(regs, 0, 32*4*3);
		}
	}

	print_gecko("Patch:[DI] applied %u times\r\n", DIPatched);
}

u32 Patch_DVDLowLevelReadForWKF(void *addr, u32 length, int dataType) {
	int i = 0;

	for(i = 0; i < length; i+=4) {
		// Patch Read to adjust the offset for fragmented files
		if( *(u32*)(addr+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadCommon.Name, properAddress - 0x84);
			u32 newval = (u32)(ADJUST_LBA_OFFSET - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x84) = newval;
			return 1;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadDebug.Name, properAddress - 0x88);
			u32 newval = (u32)(ADJUST_LBA_OFFSET - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x88) = newval;
			return 1;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadUncommon.Name, properAddress - 0x7C);
			u32 newval = (u32)(ADJUST_LBA_OFFSET - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x7C) = newval;
			return 1;
		}
	}
	return 0;
}

/** Used for Multi-DOL games that require patches to be stored on SD */
u32 Patch_DVDLowLevelReadForDVD(void *addr, u32 length, int dataType) {
	int i = 0;
	
	// Where the DVD_DMA | DVD_START are about to be written to the DI reg,
	// overwrite it with a jump to our handler which will either allow the read to happen
	// or mark it as a 0xE000 command with DVD_START only (no DMA) and read a fragment from SD.
			
	// This fragment that is read will only be a patched bit of PPC code, 
	// it will be small so a blocking read will be used.

	for(i = 0; i < length; i+=4) {
		// Patch Read (called from DVDLowLevelRead) to read data from SD if it has been patched.
		if( *(u32*)(addr+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadCommon.Name, properAddress - 0x84);
			u32 newval = (u32)(READ_REAL_OR_PATCHED - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x84) = newval;
			return 1;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadDebug.Name, properAddress - 0x88);
			u32 newval = (u32)(READ_REAL_OR_PATCHED - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x88) = newval;
			return 1;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadUncommon.Name, properAddress - 0x7C);
			u32 newval = (u32)(READ_REAL_OR_PATCHED - properAddress);
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr + i + 0x7C) = newval;
			return 1;
		}
	}
	return 0;
}

u32 Patch_DVDLowLevelRead(void *addr, u32 length, int dataType) {
	void *addr_start = addr;
	void *addr_end = addr+length;
	int patched = 0;
	FuncPattern DSPHandler = {0x420,103,23,34,32, 9, 0, 0, "__DSPHandler", 0};
	while(addr_start<addr_end) {
		// Patch the memcpy call in OSExceptionInit to copy our code to 0x80000500 instead of anything else.
		if(*(u32*)(addr_start) == 0x7C0802A6)
		{
			if( find_pattern( (u8*)(addr_start), length, &OSExceptionInitSig ) )
			{
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 472)-(u32)(addr));
				if(properAddress) {
					print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
					*(u32*)(addr_start + 472) = branchAndLink(PATCHED_MEMCPY, properAddress);
					patched |= 0x100;
				}
			}
			// Debug version of the above
			else if( find_pattern( (u8*)(addr_start), length, &OSExceptionInitSigDBG ) )
			{
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 512)-(u32)(addr));
				if(properAddress) {
					print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
					*(u32*)(addr_start + 512) = branchAndLink(PATCHED_MEMCPY_DBG, properAddress);
					patched |= 0x100;
				}
			}
			// Audio Streaming Hook
			else if( find_pattern( (u8*)(addr_start), length, &DSPHandler ) )
			{
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+0xF8)-(u32)(addr));
				print_gecko("Found:[__DSPHandler] @ %08X\r\n", properAddress);
				*(u32*)(addr_start+0xF8) = branchAndLink(DSP_HANDLER_HOOK, properAddress);
			}
			// Read variations
			FuncPattern fp;
			make_pattern( (u8*)(addr_start), length, &fp );
			if(compare_pattern(&fp, &ReadCommon) 			// Common Read function
				|| compare_pattern(&fp, &ReadDebug)			// Debug Read function
				|| compare_pattern(&fp, &ReadUncommon)) 	// Uncommon Read function
			{	
				u32 iEnd = 4;
				while(*(u32*)(addr_start + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + iEnd)-(u32)(addr));
				print_gecko("Found:[Read] @ %08X\r\n", properAddress-iEnd);
				*(u32*)(addr_start + iEnd) = branch(READ_TRIGGER_INTERRUPT, properAddress);
				patched |= 0x10;
			}
		}
		// __DVDInterruptHandler
		else if( ((*(u32*)(addr_start + 0 )) & 0xFFFF) == _dvdinterrupthandler_part[0]
			&& ((*(u32*)(addr_start + 4 )) & 0xFFFF) == _dvdinterrupthandler_part[1]
			&& ((*(u32*)(addr_start + 8 )) & 0xFFFF) == _dvdinterrupthandler_part[2] ) 
		{
			u32 iEnd = 12;
			while(*(u32*)(addr_start + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+iEnd)-(u32)(addr));
			print_gecko("Found:[__DVDInterruptHandler] end @ %08X\r\n", properAddress );
			*(u32*)(addr_start+iEnd) = branch(STOP_DI_IRQ, properAddress);
			patched |= 0x1;
		}
		addr_start += 4;
	}
	if(patched != READ_PATCHED_ALL) {
		print_gecko("Failed to find all required patches\r\n");
	}
	// Replace all 0xCC0060xx references to VAR_AREA references
	PatchDVDInterface(addr, length, dataType);
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
	0x01,0x75,0x7A,0x00,
	0x01,0x9C
};

u8 video_timing_576p[] = {
	0x0A,0x00,0x02,0x40,
	0x00,0x44,0x00,0x44,
	0x00,0x00,0x00,0x00,
	0x14,0x14,0x14,0x14,
	0x04,0xD8,0x04,0xD8,
	0x04,0xD8,0x04,0xD8,
	0x04,0xE2,0x01,0xB0,
	0x40,0x4B,0x6A,0xAC,
	0x01,0x7C,0x85,0x00,
	0x01,0xA4
};

void Patch_VidTiming(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;
	
	while(addr_start<addr_end) {
		if(memcmp(addr_start,video_timing_480p,sizeof(video_timing_480p))==0)
		{
			print_gecko("Patched PAL Progressive timing\n");
			memcpy(addr_start, video_timing_576p, sizeof(video_timing_576p));
			break;
		}
		addr_start += 4;
	}
}

u8 vertical_filters[][7] = {
	{0, 0, 21, 22, 21, 0, 0},
	{4, 8, 12, 16, 12, 8, 4},
	{8, 8, 10, 12, 10, 8, 8}
};

u8 vertical_reduction[][7] = {
	{ 0,  0,  0, 16, 16, 16, 16},	// GX_COPY_INTLC_EVEN
	{16, 16, 16, 16,  0,  0,  0}	// GX_COPY_INTLC_ODD
};

void Patch_FrameBuf(u8 *data, u32 length, int dataType) {
	int i,j;
	u8 *vfilter = vertical_filters[swissSettings.softProgressive];
	FuncPattern VIConfigurePanSig = 
		{0x390, 40, 11, 4, 25, 35, 0, 0, "VIConfigurePan", 0};
	FuncPattern GXGetYScaleFactorSigs[2] = {
		{0x234, 16, 14, 3, 18, 27, 0, 0, "GXGetYScaleFactor A", 0},
		{0x228, 20, 19, 3,  9, 14, 0, 0, "GXGetYScaleFactor B", 0}	// SN Systems ProDG
	};
	FuncPattern GXInitGXSigs[6] = {
		{0xF3C, 454, 81, 119, 43, 36, 0, 0, "__GXInitGX A", 0},
		{0x844, 307, 35, 107, 18, 10, 0, 0, "__GXInitGX B", 0},
		{0x880, 310, 35, 108, 24, 11, 0, 0, "__GXInitGX C", 0},
		{0x8C0, 313, 36, 110, 28, 11, 0, 0, "__GXInitGX D", 0},
		{0x880, 293, 36, 110,  7,  9, 0, 0, "__GXInitGX E", 0},		// SN Systems ProDG
		{0x934, 333, 34, 119, 28, 11, 0, 0, "__GXInitGX F", 0}
	};
	FuncPattern GXSetCopyFilterSigs[3] = {
		{0x224, 15,  7, 0, 4,  5, 0, 0, "GXSetCopyFilter A", 0},
		{0x288, 19, 23, 0, 3, 14, 0, 0, "GXSetCopyFilter B", 0},	// SN Systems ProDG
		{0x204, 25,  7, 0, 4,  0, 0, 0, "GXSetCopyFilter C", 0}
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &VIConfigurePanSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", VIConfigurePanSig.Name, properAddress);
				top_addr -= VIConfigurePanHook_length;
				memcpy((void*)top_addr, VIConfigurePanHook, VIConfigurePanHook_length);
				*(u32*)(top_addr+ 0) = *(u32*)(data+i+36);
				*(u32*)(top_addr+24) = branch(properAddress+40, top_addr+24);
				
				if((swissSettings.gameVMode == 2) || (swissSettings.gameVMode == 5)) {
					*(u32*)(data+i+24) = 0x5499F87E;	// srwi		25, 4, 1
					*(u32*)(data+i+32) = 0x54D7F87E;	// srwi		23, 6, 1
				}
				*(u32*)(data+i+36) = branch(top_addr, properAddress+36);
				break;
			}
		}
	}
	if((swissSettings.gameVMode >= 1) && (swissSettings.gameVMode <= 3)) {
		for( i=0; i < length; i+=4 )
		{
			if( *(u32*)(data+i) != 0x7C0802A6 )
				continue;
			
			FuncPattern fp;
			make_pattern( (u8*)(data+i), length, &fp );
			
			for( j=0; j < sizeof(GXGetYScaleFactorSigs)/sizeof(FuncPattern); j++ )
			{
				if( compare_pattern( &fp, &(GXGetYScaleFactorSigs[j]) ) )
				{
					u32 properAddress = Calc_ProperAddress(data, dataType, i);
					if(properAddress) {
						print_gecko("Found:[%s] @ %08X\n", GXGetYScaleFactorSigs[j].Name, properAddress);
						top_addr -= GXGetYScaleFactorHook_length;
						memcpy((void*)top_addr, GXGetYScaleFactorHook, GXGetYScaleFactorHook_length);
						*(u32*)(top_addr+16) = branch(properAddress+4, top_addr+16);
						*(u32*)(data+i) = branch(top_addr, properAddress);
						break;
					}
 				}
			}
		}
	}
	if((swissSettings.gameVMode == 2) || (swissSettings.gameVMode == 5)) {
		for( i=0; i < length; i+=4 )
		{
			if( *(u32*)(data+i) != 0x7C0802A6 )
				continue;
			
			FuncPattern fp;
			make_pattern( (u8*)(data+i), length, &fp );
			
			for( j=0; j < sizeof(GXInitGXSigs)/sizeof(FuncPattern); j++ )
			{
				if( compare_pattern( &fp, &(GXInitGXSigs[j]) ) )
				{
					u32 properAddress = Calc_ProperAddress(data, dataType, i);
					if(properAddress) {
						print_gecko("Found:[%s] @ %08X\n", GXInitGXSigs[j].Name, properAddress);
						switch(j) {
							case 0: *(u32*)(data+i+3776) = 0x38600003; break;
							case 1: *(u32*)(data+i+1976) = 0x38600003; break;
							case 2: *(u32*)(data+i+2036) = 0x38600003; break;
							case 3: *(u32*)(data+i+2096) = 0x38600003; break;
							case 4: *(u32*)(data+i+2004) = 0x38600003; break;
							case 5: *(u32*)(data+i+2212) = 0x38600003; break;
						}
						vfilter = vertical_reduction[1];
						break;
					}
				}
			}
		}
	}
	if((swissSettings.gameVMode != 1) && (swissSettings.gameVMode != 4)) {
		for( i=0; i < length; i+=4 )
		{
			if( *(u32*)(data+i) != 0x2C030000 && *(u32*)(data+i+4) != 0x5460063F )
				continue;
			
			FuncPattern fp;
			make_pattern( (u8*)(data+i), length, &fp );
			
			for( j=0; j < sizeof(GXSetCopyFilterSigs)/sizeof(FuncPattern); j++ )
			{
				if( compare_pattern( &fp, &(GXSetCopyFilterSigs[j]) ) )
				{
					u32 properAddress = Calc_ProperAddress(data, dataType, i);
					if(properAddress) {
						print_gecko("Found:[%s] @ %08X\n", GXSetCopyFilterSigs[j].Name, properAddress);
						switch(j) {
							case 0:
								*(u32*)(data+i+388) = 0x38000000 | vfilter[0];
								*(u32*)(data+i+392) = 0x38600000 | vfilter[1];
								*(u32*)(data+i+400) = 0x38000000 | vfilter[4];
								*(u32*)(data+i+404) = 0x38800000 | vfilter[2];
								*(u32*)(data+i+416) = 0x38600000 | vfilter[5];
								*(u32*)(data+i+428) = 0x38A00000 | vfilter[3];
								*(u32*)(data+i+432) = 0x38000000 | vfilter[6];
								break;
							case 1:
								*(u32*)(data+i+492) = 0x38000000 | vfilter[0];
								*(u32*)(data+i+496) = 0x39200000 | vfilter[1];
								*(u32*)(data+i+504) = 0x39400000 | vfilter[4];
								*(u32*)(data+i+508) = 0x39600000 | vfilter[2];
								*(u32*)(data+i+516) = 0x39000000 | vfilter[5];
								*(u32*)(data+i+536) = 0x39400000 | vfilter[6];
								*(u32*)(data+i+540) = 0x39200000 | vfilter[3];
								break;
							case 2:
								*(u32*)(data+i+372) = 0x38800000 | vfilter[0];
								*(u32*)(data+i+376) = 0x38600000 | vfilter[4];
								*(u32*)(data+i+384) = 0x38800000 | vfilter[1];
								*(u32*)(data+i+392) = 0x38E00000 | vfilter[2];
								*(u32*)(data+i+400) = 0x38800000 | vfilter[5];
								*(u32*)(data+i+404) = 0x38A00000 | vfilter[3];
								*(u32*)(data+i+412) = 0x38600000 | vfilter[6];
								break;
						}
						break;
					}
				}
			}
		}
	}
}

int Patch_VidMode(u8 *data, u32 length, int dataType) {
	int i,j;
	FuncPattern VIConfigureSigs[7] = {
		{0x6AC,  90, 43,  6, 32, 60, 0, 0, "VIConfigure A", 0},
		{0x68C,  87, 41,  6, 31, 60, 0, 0, "VIConfigure B", 0},
		{0x73C, 100, 43, 13, 34, 61, 0, 0, "VIConfigure C", 0},
		{0x798, 105, 44, 12, 38, 63, 0, 0, "VIConfigure D", 0},
		{0x824, 111, 44, 13, 53, 64, 0, 0, "VIConfigure E", 0},
		{0x8B4, 112, 43, 14, 53, 48, 0, 0, "VIConfigure F", 0},		// SN Systems ProDG
		{0x804, 110, 44, 13, 49, 63, 0, 0, "VIConfigure G", 0}
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
		
		for( j=0; j < sizeof(VIConfigureSigs)/sizeof(FuncPattern); j++ )
		{
			if( compare_pattern( &fp, &(VIConfigureSigs[j]) ) ) {
				u32 properAddress = Calc_ProperAddress(data, dataType, i);
				if(properAddress) {
					print_gecko("Found:[%s] @ %08X\n", VIConfigureSigs[j].Name, properAddress);
					switch(swissSettings.gameVMode) {
						case 1:
							print_gecko("Patched NTSC Interlaced mode\n");
							top_addr -= VIConfigure480i_length;
							memcpy((void*)top_addr, VIConfigure480i, VIConfigure480i_length);
							*(u32*)(top_addr+92) = branch(properAddress+4, top_addr+92);
							break;
						case 2:
							print_gecko("Patched NTSC Double-Strike mode\n");
							top_addr -= VIConfigure240p_length;
							memcpy((void*)top_addr, VIConfigure240p, VIConfigure240p_length);
							*(u32*)(top_addr+80) = branch(properAddress+4, top_addr+80);
							break;
						case 3:
							print_gecko("Patched NTSC Progressive mode\n");
							top_addr -= VIConfigure480p_length;
							memcpy((void*)top_addr, VIConfigure480p, VIConfigure480p_length);
							*(u32*)(top_addr+76) = branch(properAddress+4, top_addr+76);
							break;
						case 4:
							print_gecko("Patched PAL Interlaced mode\n");
							top_addr -= VIConfigure576i_length;
							memcpy((void*)top_addr, VIConfigure576i, VIConfigure576i_length);
							*(u32*)(top_addr+56) = branch(properAddress+4, top_addr+56);
							break;
						case 5:
							print_gecko("Patched PAL Double-Strike mode\n");
							top_addr -= VIConfigure288p_length;
							memcpy((void*)top_addr, VIConfigure288p, VIConfigure288p_length);
							*(u32*)(top_addr+44) = branch(properAddress+4, top_addr+44);
							break;
						case 6:
							print_gecko("Patched PAL Progressive mode\n");
							top_addr -= VIConfigure576p_length;
							memcpy((void*)top_addr, VIConfigure576p, VIConfigure576p_length);
							*(u32*)(top_addr+40) = branch(properAddress+4, top_addr+40);
							
							switch(j) {
								case 0:
									*(u32*)(data+i+  40) = 0x2C040006;	// cmpwi	4, 6
									*(u32*)(data+i+ 304) = 0x807B0000;	// lwz		3, 0 (27)
									*(u32*)(data+i+ 896) = 0x2C000006;	// cmpwi	0, 6
									break;
								case 1:
									*(u32*)(data+i+ 272) = 0x807C0000;	// lwz		3, 0 (28)
									*(u32*)(data+i+ 864) = 0x2C000006;	// cmpwi	0, 6
									break;
								case 2:
									*(u32*)(data+i+ 412) = 0x807C0000;	// lwz		3, 0 (28)
									*(u32*)(data+i+1040) = 0x2C000006;	// cmpwi	0, 6
									break;
								case 3:
									*(u32*)(data+i+ 476) = 0x38600000;	// li		3, 0
									*(u32*)(data+i+1128) = 0x2C000006;	// cmpwi	0, 6
									*(u32*)(data+i+1136) = 0x2C000007;	// cmpwi	0, 7
									break;
								case 4: 
									*(u32*)(data+i+ 604) = 0x38600000;	// li		3, 0
									*(u32*)(data+i+1260) = 0x2C000006;	// cmpwi	0, 6
									*(u32*)(data+i+1268) = 0x2C000007;	// cmpwi	0, 7
									break;
								case 5:
									*(u32*)(data+i+ 544) = 0x38000000;	// li		0, 0
									*(u32*)(data+i+1340) = 0x2C0A0006;	// cmpwi	10, 6
									*(u32*)(data+i+1368) = 0x2C0A0007;	// cmpwi	10, 7
									break;
								case 6:
									*(u32*)(data+i+ 604) = 0x38600000;	// li		3, 0
									break;
							}
							Patch_VidTiming(data, length);
							break;
						default:
							return 0;
					}
					switch(j) {
						case 0: *(u32*)(data+i+208) = 0xA07F0010; break;	// lhz		3, 16 (31)
						case 1: *(u32*)(data+i+176) = 0xA07F0010; break;	// lhz		3, 16 (31)
						case 2: *(u32*)(data+i+316) = 0xA07F0010; break;	// lhz		3, 16 (31)
						case 3: *(u32*)(data+i+328) = 0xA07F0010; break;	// lhz		3, 16 (31)
						case 4: *(u32*)(data+i+456) = 0xA07F0010; break;	// lhz		3, 16 (31)
						case 5: *(u32*)(data+i+436) = 0xA09B0010; break;	// lhz		4, 16 (27)
						case 6: *(u32*)(data+i+456) = 0xA0730010; break;	// lhz		3, 16 (19)
					}
					*(u32*)(data+i) = branch(top_addr, properAddress);
					Patch_FrameBuf(data, length, dataType);
					return 1;
				}
			}
		}
	}
	return 0;
}

void Patch_WideAspect(u8 *data, u32 length, int dataType) {
	int i,j;
	FuncPattern MTXFrustumSig =
		{0x98, 4, 16, 0, 0, 0, 0, 0, "C_MTXFrustum", 0};
	FuncPattern MTXLightFrustumSig =
		{0x90, 6, 13, 0, 0, 0, 0, 0, "C_MTXLightFrustum", 0};
	FuncPattern MTXPerspectiveSig =
		{0xCC, 8, 19, 1, 0, 6, 0, 0, "C_MTXPerspective", 0};
	FuncPattern MTXLightPerspectiveSig =
		{0xC8, 8, 15, 1, 0, 8, 0, 0, "C_MTXLightPerspective", 0};
	FuncPattern MTXOrthoSig =
		{0x94, 4, 16, 0, 0, 0, 0, 0, "C_MTXOrtho", 0};
	FuncPattern GXSetScissorSigs[3] = {
		{0xAC, 19, 6, 0, 0, 6, 0, 0, "GXSetScissor A", 0},
		{0x8C, 12, 6, 0, 0, 6, 0, 0, "GXSetScissor B", 0},
		{0x74, 13, 6, 0, 0, 1, 0, 0, "GXSetScissor C", 0}
	};
	FuncPattern GXSetProjectionSigs[3] = {
		{0xD0, 30, 17, 0, 1, 0, 0, 0, "GXSetProjection A", 0},
		{0xB0, 22, 17, 0, 1, 0, 0, 0, "GXSetProjection B", 0},
		{0xA0, 18, 11, 0, 1, 0, 0, 0, "GXSetProjection C", 0}
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i) != 0xED241828 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXFrustumSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXFrustumSig.Name, properAddress);
				top_addr -= MTXFrustumHook_length;
				memcpy((void*)top_addr, MTXFrustumHook, MTXFrustumHook_length);
				*(u32*)(top_addr+28) = branch(properAddress+4, top_addr+28);
				*(u32*)(data+i) = branch(top_addr, properAddress);
				*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
				MTXFrustumSig.offsetFoundAt = (u32)data+i;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i+8) != 0xED441828 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXLightFrustumSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightFrustumSig.Name, properAddress);
				top_addr -= MTXLightFrustumHook_length;
				memcpy((void*)top_addr, MTXLightFrustumHook, MTXLightFrustumHook_length);
				*(u32*)(top_addr+28) = branch(properAddress+12, top_addr+28);
				*(u32*)(data+i+8) = branch(top_addr, properAddress+8);
				*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXPerspectiveSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXPerspectiveSig.Name, properAddress);
				top_addr -= MTXPerspectiveHook_length;
				memcpy((void*)top_addr, MTXPerspectiveHook, MTXPerspectiveHook_length);
				*(u32*)(top_addr+20) = branch(properAddress+84, top_addr+20);
				*(u32*)(data+i+80) = branch(top_addr, properAddress+80);
				*(u32*)VAR_FLOAT9_16 = 0x3F100000;
				MTXPerspectiveSig.offsetFoundAt = (u32)data+i;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXLightPerspectiveSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightPerspectiveSig.Name, properAddress);
				top_addr -= MTXLightPerspectiveHook_length;
				memcpy((void*)top_addr, MTXLightPerspectiveHook, MTXLightPerspectiveHook_length);
				*(u32*)(top_addr+20) = branch(properAddress+100, top_addr+20);
				*(u32*)(data+i+96) = branch(top_addr, properAddress+96);
				*(u32*)VAR_FLOAT9_16 = 0x3F100000;
				break;
			}
		}
	}
	if(swissSettings.forceWidescreen == 2) {
		for( i=0; i < length; i+=4 )
		{
			if( *(u32*)(data+i) != 0xED041828 )
				continue;
			if( find_pattern( (u8*)(data+i), length, &MTXOrthoSig ) )
			{
				u32 properAddress = Calc_ProperAddress(data, dataType, i);
				if(properAddress) {
					print_gecko("Found:[%s] @ %08X\n", MTXOrthoSig.Name, properAddress);
					top_addr -= MTXOrthoHook_length;
					memcpy((void*)top_addr, MTXOrthoHook, MTXOrthoHook_length);
					*(u32*)(top_addr+128) = branch(properAddress+4, top_addr+128);
					*(u32*)(data+i) = branch(top_addr, properAddress);
					*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
					*(u32*)VAR_FLOATM_1 = 0xBF800000;
					break;
				}
			}
		}
		for( i=0; i < length; i+=4 )
		{
			if( (*(u32*)(data+i+4) & 0xFC00FFFF) != 0x38000156 )
				continue;
			
			FuncPattern fp;
			make_pattern( (u8*)(data+i), length, &fp );
			
			for( j=0; j < sizeof(GXSetScissorSigs)/sizeof(FuncPattern); j++ )
			{
				if( compare_pattern( &fp, &(GXSetScissorSigs[j]) ) ) {
					u32 properAddress = Calc_ProperAddress(data, dataType, i);
					if(properAddress) {
						print_gecko("Found:[%s] @ %08X\n", GXSetScissorSigs[j].Name, properAddress);
						top_addr -= GXSetScissorHook_length;
						memcpy((void*)top_addr, GXSetScissorHook, GXSetScissorHook_length);
						*(u32*)(top_addr+ 0) = *(u32*)(data+i);
						*(u32*)(top_addr+ 4) = j == 1 ? 0x800801E8:0x800701E8;
						*(u32*)(top_addr+64) = branch(properAddress+4, top_addr+64);
						*(u32*)(data+i) = branch(top_addr, properAddress);
						break;
					}
				}
			}
		}
	}
	if( !MTXFrustumSig.offsetFoundAt && !MTXPerspectiveSig.offsetFoundAt ) {
		for( i=0; i < length; i+=4 )
		{
			if( *(u32*)(data+i+4) != 0x2C040001 )
				continue;
			
			FuncPattern fp;
			make_pattern( (u8*)(data+i), length, &fp );
			
			for( j=0; j < sizeof(GXSetProjectionSigs)/sizeof(FuncPattern); j++ )
			{
				if( compare_pattern( &fp, &(GXSetProjectionSigs[j]) ) )
				{
					u32 properAddress = Calc_ProperAddress(data, dataType, i);
					if(properAddress) {
						print_gecko("Found:[%s] @ %08X\n", GXSetProjectionSigs[j].Name, properAddress);
						top_addr -= GXSetProjectionHook_length;
						memcpy((void*)top_addr, GXSetProjectionHook, GXSetProjectionHook_length);
						*(u32*)(top_addr+20) = branch(properAddress+16, top_addr+20);
						*(u32*)(data+i+12) = branch(top_addr, properAddress+12);
						*(u32*)VAR_FLOAT3_4 = 0x3F400000;
						break;
					}
				}
			}
		}
	}
}

int Patch_TexFilt(u8 *data, u32 length, int dataType)
{
	int i,j;
	FuncPattern GXInitTexObjLODSigs[3] = {
		{0x190, 29, 11, 0, 11, 11, 0, 0, "GXInitTexObjLOD A", 0},
		{0x160, 16,  4, 0,  8,  6, 0, 0, "GXInitTexObjLOD B", 0},	// SN Systems ProDG
		{0x160, 30, 11, 0, 11,  6, 0, 0, "GXInitTexObjLOD C", 0}
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(u32*)(data+i+8) != 0xFC030040 && *(u32*)(data+i+16) != 0xFC030000 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
		
		for( j=0; j < sizeof(GXInitTexObjLODSigs)/sizeof(FuncPattern); j++ )
		{
			if( compare_pattern( &fp, &(GXInitTexObjLODSigs[j]) ) )
			{
				u32 properAddress = Calc_ProperAddress(data, dataType, i);
				if(properAddress) {
					print_gecko("Found:[%s] @ %08X\n", GXInitTexObjLODSigs[j].Name, properAddress);
					top_addr -= GXInitTexObjLODHook_length;
					memcpy((void*)top_addr, GXInitTexObjLODHook, GXInitTexObjLODHook_length);
					*(u32*)(top_addr+24) = *(u32*)(data+i);
					*(u32*)(top_addr+28) = branch(properAddress+4, top_addr+28);
					*(u32*)(data+i) = branch(top_addr, properAddress);
					return 1;
				}
			}
		}
	}
	return 0;
}

u32 _fontencode_part_a[3] = {
	0x3C608000, 0x800300CC, 0x2C000000
};

u32 _fontencode_part_b[3] = {
	0x3C60CC00, 0xA003206E, 0x540007BD
};

int Patch_FontEnc(void *addr, u32 length)
{
	void *addr_start = addr;
	void *addr_end = addr+length;
	int patched = 0;
	
	while(addr_start<addr_end) {
		if(memcmp(addr_start,_fontencode_part_a,sizeof(_fontencode_part_a))==0
			&& memcmp(addr_start+24,_fontencode_part_b,sizeof(_fontencode_part_b))==0)
		{
			switch(swissSettings.forceEncoding) {
				case 1:
					*(u32*)(addr_start+40) = 0x38000000;
					break;
				case 2:
					*(u32*)(addr_start+48) = 0x38000001;
					*(u32*)(addr_start+60) = 0x38000001;
					break;
			}
			patched++;
		}
		addr_start += 4;
	}
	return patched;
}


/** SDK DVD Reset Replacement 
	- Allows debug spinup for backups */

static const u32 _dvdlowreset_org[12] = {
	0x7C0802A6,0x3C80CC00,0x90010004,0x38000002,0x9421FFE0,0xBF410008,0x3BE43000,0x90046004,
	0x83C43024,0x57C007B8,0x60000001,0x941F0024
};

static const u32 _dvdlowreset_new[5] = {
	0x3FE08000,0x63FF0000,0x7FE803A6,0x4E800021,0x4800006C
};
	
int Patch_DVDReset(void *addr,u32 length)
{
	void *addr_start = addr;
	void *addr_end = addr+length;

	while(addr_start<addr_end) {
		if(memcmp(addr_start,_dvdlowreset_org,sizeof(_dvdlowreset_org))==0) {
			// we found the DVDLowReset
			memcpy((addr_start+0x18),_dvdlowreset_new,sizeof(_dvdlowreset_new));
			// Adjust the offset of where to jump to
			u32 *ptr = (addr_start+0x18);
			ptr[1] = _dvdlowreset_new[1] | (ENABLE_BACKUP_DISC&0xFFFF);
			print_gecko("Found:[DVDLowReset] @ 0x%08X\r\n", (u32)ptr);
			return 1;
		}
		addr_start += 4;
	}
	return 0;
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

int Patch_Fwrite(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	
	
	while(addr_start<addr_end) 
	{
		if(memcmp(addr_start,&sig_fwrite[0],sizeof(sig_fwrite))==0) 
		{
			print_gecko("Found:[fwrite]\r\n");
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));	
			return 1;
		}
		if(memcmp(addr_start,&sig_fwrite_alt[0],sizeof(sig_fwrite_alt))==0) 
		{
			print_gecko("Found:[fwrite1]\r\n");
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));	
			return 1;
		}
		if(memcmp(addr_start,&sig_fwrite_alt2[0],sizeof(sig_fwrite_alt2))==0) 
		{
			print_gecko("Found:[fwrite2]\r\n");
 			memcpy(addr_start,&geckoPatch[0],sizeof(geckoPatch));
			return 1;
		}
		addr_start += 4;
	}
	return 0;
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

u32 Calc_ProperAddress(u8 *data, u32 type, u32 offsetFoundAt) {
	if(type == PATCH_DOL) {
		int i;
		DOLHEADER *hdr = (DOLHEADER *) data;

		// Doesn't look valid
		if (hdr->textOffset[0] != DOLHDRLENGTH) {
			print_gecko("DOL Header doesn't look valid %08X\r\n",hdr->textOffset[0]);
			return 0;
		}

		// Inspect text sections to see if what we found lies in here
		for (i = 0; i < MAXTEXTSECTION; i++) {
			if (hdr->textAddress[i] && hdr->textLength[i]) {
				// Does what we found lie in this section?
				if((offsetFoundAt >= hdr->textOffset[i]) && offsetFoundAt <= (hdr->textOffset[i] + hdr->textLength[i])) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return offsetFoundAt+hdr->textAddress[i]-hdr->textOffset[i];
				}
			}
		}

		// Inspect data sections (shouldn't really need to unless someone was sneaky..)
		for (i = 0; i < MAXDATASECTION; i++) {
			if (hdr->dataAddress[i] && hdr->dataLength[i]) {
				// Does what we found lie in this section?
				if((offsetFoundAt >= hdr->dataOffset[i]) && offsetFoundAt <= (hdr->dataOffset[i] + hdr->dataLength[i])) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return offsetFoundAt+hdr->dataAddress[i]-hdr->dataOffset[i];
				}
			}
		}
	
	}
	else if(type == PATCH_ELF) {
		if(!valid_elf_image((u32)data))
			return 0;
			
		Elf32_Ehdr* ehdr;
		Elf32_Shdr* shdr;
		int i;

		ehdr = (Elf32_Ehdr *)data;

		// Search through each appropriate section
		for (i = 0; i < ehdr->e_shnum; ++i) {
			shdr = (Elf32_Shdr *)(data + ehdr->e_shoff + (i * sizeof(Elf32_Shdr)));

			if (!(shdr->sh_flags & SHF_ALLOC) || shdr->sh_addr == 0 || shdr->sh_size == 0)
				continue;

			shdr->sh_addr &= 0x3FFFFFFF;
			shdr->sh_addr |= 0x80000000;

			if (shdr->sh_type != SHT_NOBITS) {
				if((offsetFoundAt >= shdr->sh_offset) && offsetFoundAt <= (shdr->sh_offset + shdr->sh_size)) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return offsetFoundAt+shdr->sh_addr-shdr->sh_offset;
				}
			}
		}	
	}
	else if(type == PATCH_LOADER) {
		return offsetFoundAt+0x81300000;
	}
	print_gecko("No cases matched, returning 0 for proper address\r\n");
	return 0;
}

// Ocarina cheat engine hook - Patch OSSleepThread
int Patch_CheatsHook(u8 *data, u32 length, u32 type) {
	int i;
	
	for( i=0; i < length; i+=4 )
	{
		// Find OSSleepThread
		if(*(u32*)(data+i+0) == 0x3C808000 &&
			(*(u32*)(data+i+4) == 0x38000004 || *(u32*)(data+i+4) == 0x808400E4) &&
			(*(u32*)(data+i+8) == 0x38000004 || *(u32*)(data+i+8) == 0x808400E4)) 
		{
			
			// Find the end of the function and replace the blr with a relative branch to 0x800018A8
			int j = 12;
			while( *(u32*)(data+i+j) != 0x4E800020 )
				j+=4;
			// As the data we're looking at will not be in this exact memory location until it's placed there by our ARAM relocation stub,
			// we'll need to work out where it will end up when it does get placed in memory to write the relative branch.
			u32 properAddress = Calc_ProperAddress(data, type, i+j);
			if(properAddress) {
				print_gecko("Found:[Hook:OSSleepThread] @ %08X\n", properAddress );
				u32 newval = (u32)(0x800018A8 - properAddress);
				newval&= 0x03FFFFFC;
				newval|= 0x48000000;
				*(u32*)(data+i+j) = newval;
				return 1;
			}
		}
	}
	return 0;
}


