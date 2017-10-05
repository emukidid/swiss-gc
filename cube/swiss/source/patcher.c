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
#include "ata.h"
#include "cheats.h"

static u32 top_addr = VAR_PATCHES_BASE;

/*** externs ***/
extern GXRModeObj *vmode;		/*** Graphics Mode Object ***/
extern u32 *xfb[2];			   	/*** Framebuffers ***/
extern u32 SDHCCard;

extern void animateBox(int x1,int y1, int x2, int y2, int color,char *msg);

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
	
	// Pokemon XD / Colosseum tiny stub for memset testing
	if(!strncmp((char*)0x80000000, "GXX", 3) || !strncmp((char*)0x80000000, "GC6", 3)) 
	{
		print_gecko("Patch:[Pokemon memset] applied\r\n");
		// patch in test < 0x3000 function at an empty spot in RAM
		*(vu32*)0x80000088 = 0x3CC08000;
		*(vu32*)0x8000008C = 0x60C63000;
		*(vu32*)0x80000090 = 0x7C033000;
		*(vu32*)0x80000094 = 0x41800008;
		*(vu32*)0x80000098 = 0x480053A5;
		*(vu32*)0x8000009C = 0x48005388;
	}
	
	// IDE-EXI
  	if(devices[DEVICE_CUR] == &__device_ide_a || devices[DEVICE_CUR] == &__device_ide_b) {	
		patch = (!_ideexi_version)?&ideexi_v1_bin[0]:&ideexi_v2_bin[0]; 
		patchSize = (!_ideexi_version)?ideexi_v1_bin_size:ideexi_v2_bin_size;
		print_gecko("Installing Patch for IDE-EXI\r\n");
  	}
	// SD Gecko
	else if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b) {
		patch = &sd_bin[0]; patchSize = sd_bin_size;
		print_gecko("Installing Patch for SD Gecko\r\n");
	}
	// DVD 2 disc code
	else if(devices[DEVICE_CUR] == &__device_dvd) {
		patch = &dvd_bin[0]; patchSize = dvd_bin_size;
		location = (void*)LO_RESERVE_DVD;
		print_gecko("Installing Patch for DVD\r\n");
	}
	// USB Gecko
	else if(devices[DEVICE_CUR] == &__device_usbgecko) {
		patch = &usbgecko_bin[0]; patchSize = usbgecko_bin_size;
		print_gecko("Installing Patch for USB Gecko\r\n");
	}
	// Wiikey Fusion
	else if(devices[DEVICE_CUR] == &__device_wkf) {
		patch = &wkf_bin[0]; patchSize = wkf_bin_size;
		print_gecko("Installing Patch for WKF\r\n");
	}
	// Broadband Adapter
	else if(devices[DEVICE_CUR] == &__device_smb) {
		patch = &bba_bin[0]; patchSize = bba_bin_size;
		print_gecko("Installing Patch for Broadband Adapter\r\n");
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
		u32 word = *(vu32*)(Data + i) ;
		
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
		u32 word =  *(vu32*)(data + i);
		
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
	newval |= 0x48000000;
	return newval;
}

u32 branchAndLink(u32 dst, u32 src)
{
	u32 newval = dst - src;
	newval &= 0x03FFFFFC;
	newval |= 0x48000001;
	return newval;
}

u32 branchResolve(u8 *data, u32 offsetFoundAt)
{
	u32 newval = *(vu32*)(data + offsetFoundAt);
	newval = ((s32)((newval & 0x03FFFFFC) << 6) >> 6) + offsetFoundAt;
	return newval;
}

// Redirects 0xCC0060xx reads to VAR_DI_REGS
void PatchDVDInterface( u8 *dst, u32 Length, int dataType )
{
	PatchDetectLowMemUsage(dst, Length, dataType);
	u32 DIPatched = 0;
	int i;

#define REG_0xCC00 0
#define REG_OFFSET 1
#define REG_USED   2
	
	u32 regs[32][3];
	memset(regs, 0, 32*4*3);
	
	for( i=0; i < Length; i+=4 )
	{
		u32 op = *(vu32*)(dst + i);
			
		// lis rX, 0xCC00
		if( (op & 0xFC1FFFFF) == 0x3C00CC00 ) {
			u32 dstR = (op >> 21) & 0x1F;
			if(regs[dstR][REG_USED]) {
				u32 lisOffset = regs[dstR][REG_OFFSET];
				*(vu32*)lisOffset = (((*(vu32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(vu32*)regs[dstR][REG_OFFSET], dstR, (*(vu32*)lisOffset) &0xFFFF);
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
					*(vu32*)lisOffset = (((*(vu32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(vu32*)regs[dstR][REG_OFFSET], dstR, (*(vu32*)lisOffset) &0xFFFF);
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
				u32 nextOp = *(vu32*)(dst + i+4);
				if(( (nextOp & 0xF8000000 ) == 0xA0000000 )) {
					u32 nextOpSrc = (nextOp >> 16) & 0x1F;
					if(nextOpSrc == src) {
						regs[src][REG_0xCC00] = 0;	// No 0xCC0060xx code uses load half op
						continue;
					}
				}
				
				// else just patch.
				if( (op & 0xFC00FF00) == 0x38006000 ) {
					*(vu32*)(dst + i) = op - (0x6000 - (VAR_DI_REGS&0xFFFF));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: addi r%u, %04X\r\n", properAddress, *(vu32*)(dst + i), src, *(vu32*)(dst + i) &0xFFFF);
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
				*(vu32*)(dst + i) -= (0x6000 - (VAR_DI_REGS&0xFFFF));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: ori r%u, %04X\r\n", properAddress, *(vu32*)(dst + i), src, *(vu32*)(dst + i) &0xFFFF);
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
				*(vu32*)(dst + i) -= (0x6000 - (VAR_DI_REGS&0xFFFF));
				//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
				//print_gecko("DI:[%08X] %08X: mem r%u, %04X\r\n", properAddress, *(vu32*)(dst + i), src, *(vu32*)(dst + i) &0xFFFF);
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
					*(vu32*)lisOffset = (((*(vu32*)lisOffset) & 0xFFFF0000) | (VAR_DI_REGS>>16));
					//u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
					//print_gecko("DI:[%08X] %08X: lis r%u, %04X\r\n", properAddress, *(vu32*)regs[x][REG_OFFSET], x, (*(vu32*)lisOffset) &0xFFFF);
					DIPatched++;
				}
			}
			memset(regs, 0, 32*4*3);
		}
	}

	print_gecko("Patch:[DI] applied %u times\r\n", DIPatched);
}

int PatchDetectLowMemUsage( u8 *dst, u32 Length, int dataType )
{

#define REG_0x8000 0
#define REG_INUSE  1

	u32 LowMemPatched = 0;
	int i;

	u32 regs[32][2];
	memset(regs, 0, 32*4*2);
	
	for( i=0; i < Length; i+=4 )
	{
		u32 op = *(vu32*)(dst + i);
			
		// lis rX, 0x8000
		if( (op & 0xFC1FFFFF) == 0x3C008000 ) {
			u32 dstR = (op >> 21) & 0x1F;
			regs[dstR][REG_0x8000]	=	1;	// this reg is now 0x80000000
			continue;
		}

		// li rX, x or lis rX, x
		if( (op & 0xFC1F0000) == 0x38000000 || (op & 0xFC1F0000) == 0x3C000000 ) {
			u32 dstR = (op >> 21) & 0x1F;
			if (regs[dstR][REG_0x8000]) {
				regs[dstR][REG_0x8000] = 0;	// reg is no longer 0x80000000
			}
			continue;
		}

		// lwz and lwzu, stw and stwu
		if (((op & 0xF8000000) == 0x80000000) || ( (op & 0xF8000000 ) == 0x90000000 ) || ( (op & 0xF8000000 ) == 0xA0000000 ) )
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dstR = (op >> 21) & 0x1F;
			u32 val = op & 0xFFFF;
			if(dstR == src) {regs[src][REG_0x8000] = 0; continue; }
			if( regs[src][REG_0x8000] && (((val & 0xFFFF) >= 0x1000) && ((val & 0xFFFF) < 0x3000))) // case with load in our range(rZ)
			{
				u32 properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
				print_gecko("LowMem:[%08X] %08X: mem r%u, %04X\r\n", properAddress, *(vu32*)(dst + i), src, *(vu32*)(dst + i) &0xFFFF);
				*(vu32*)(dst + i) = 0x60000000;	// We could redirect ...
				regs[src][REG_INUSE]=1;	// was used in a 0x80001000->0x80003000 load/store
				LowMemPatched++;
			}
			continue;
		}
		// blr, flush out and reset
		if(op == 0x4E800020) {
			memset(regs, 0, 32*4*2);
		}
	}

	print_gecko("Patch:[LowMem] applied %u times\r\n", LowMemPatched);
	return LowMemPatched;
}


u32 Patch_DVDLowLevelReadForWKF(void *addr, u32 length, int dataType) {
	int i = 0;
	int patched = 0;
	patched = PatchDetectLowMemUsage(addr, length, dataType);
	for(i = 0; i < length; i+=4) {
		if(patched == 0x11) break;	// we're done

		// Patch Read to adjust the offset for fragmented files
		if( *(vu32*)(addr+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &OSExceptionInitSig))
		{
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 472)-(u32)(addr));
			print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 472) = branchAndLink(PATCHED_MEMCPY_WKF, properAddress);
			patched |= 0x10;
		}
		// Debug version of the above
		if(compare_pattern(&fp, &OSExceptionInitSigDBG))
		{
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 512)-(u32)(addr));
			print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 512) = branchAndLink(PATCHED_MEMCPY_DBG_WKF, properAddress);
			patched |= 0x10;
		}		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadUncommon.Name, properAddress - 0x7C);
			*(vu32*)(addr + i + 0x7C) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
	}
	return patched;
}

u32 Patch_DVDLowLevelReadForUSBGecko(void *addr, u32 length, int dataType) {
	int i = 0;
	int patched = 0;
	patched = PatchDetectLowMemUsage(addr, length, dataType);
	for(i = 0; i < length; i+=4) {
		if(patched == 0x11) break;	// we're done

		// Patch Read to adjust the offset for fragmented files
		if( *(vu32*)(addr+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &OSExceptionInitSig))
		{
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 472)-(u32)(addr));
			print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 472) = branchAndLink(PATCHED_MEMCPY_USB, properAddress);
			patched |= 0x10;
		}
		// Debug version of the above
		if(compare_pattern(&fp, &OSExceptionInitSigDBG))
		{
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 512)-(u32)(addr));
			print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 512) = branchAndLink(PATCHED_MEMCPY_DBG_USB, properAddress);
			patched |= 0x10;
		}		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(PERFORM_READ_USBGECKO, properAddress);
			*(vu32*)(addr + i + 0xB8) = 0x60000000;
			*(vu32*)(addr + i + 0xEC) = 0x60000000;
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(PERFORM_READ_USBGECKO, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadUncommon.Name, properAddress - 0x7C);
			*(vu32*)(addr + i + 0x7C) = branchAndLink(PERFORM_READ_USBGECKO, properAddress);
			patched |= 0x100;
		}
	}
	return patched;
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
		if( *(vu32*)(addr+i) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
			return 1;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
			return 1;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadUncommon.Name, properAddress - 0x7C);
			*(vu32*)(addr + i + 0x7C) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
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
		if(*(vu32*)(addr_start) == 0x7C0802A6)
		{
			if( find_pattern( (u8*)(addr_start), length, &OSExceptionInitSig ) )
			{
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 472)-(u32)(addr));
				print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 472) = branchAndLink(PATCHED_MEMCPY, properAddress);
				patched |= 0x100;
			}
			// Debug version of the above
			else if( find_pattern( (u8*)(addr_start), length, &OSExceptionInitSigDBG ) )
			{
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 512)-(u32)(addr));
				print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 512) = branchAndLink(PATCHED_MEMCPY_DBG, properAddress);
				patched |= 0x100;
			}
			// Audio Streaming Hook (only if required)
			else if(!swissSettings.muteAudioStreaming && find_pattern( (u8*)(addr_start), length, &DSPHandler ) )
			{	
				if(strncmp((const char*)0x80000000, "PZL", 3)) {	// ZeldaCE uses a special case for MM
					u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+0xF8)-(u32)(addr));
					print_gecko("Found:[__DSPHandler] @ %08X\r\n", properAddress);
					*(vu32*)(addr_start+0xF8) = branchAndLink(DSP_HANDLER_HOOK, properAddress);
				}
			}
			// Read variations
			FuncPattern fp;
			make_pattern( (u8*)(addr_start), length, &fp );
			if(compare_pattern(&fp, &ReadCommon) 			// Common Read function
				|| compare_pattern(&fp, &ReadDebug)			// Debug Read function
				|| compare_pattern(&fp, &ReadUncommon)) 	// Uncommon Read function
			{
				u32 iEnd = 4;
				while(*(vu32*)(addr_start + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
				u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + iEnd)-(u32)(addr));
				print_gecko("Found:[Read] @ %08X\r\n", properAddress-iEnd);
				*(vu32*)(addr_start + iEnd) = branch(READ_TRIGGER_INTERRUPT, properAddress);
				patched |= 0x10;
			}
		}
		// __DVDInterruptHandler
		else if( ((*(vu32*)(addr_start + 0 )) & 0xFFFF) == _dvdinterrupthandler_part[0]
			&& ((*(vu32*)(addr_start + 4 )) & 0xFFFF) == _dvdinterrupthandler_part[1]
			&& ((*(vu32*)(addr_start + 8 )) & 0xFFFF) == _dvdinterrupthandler_part[2] ) 
		{
			u32 iEnd = 12;
			while(*(vu32*)(addr_start + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
			u32 properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+iEnd)-(u32)(addr));
			print_gecko("Found:[__DVDInterruptHandler] end @ %08X\r\n", properAddress );
			*(vu32*)(addr_start+iEnd) = branch(STOP_DI_IRQ, properAddress);
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

u8 video_timing_480i[] = {
	0x06,0x00,0x00,0xF0,
	0x00,0x18,0x00,0x19,
	0x00,0x03,0x00,0x02,
	0x0C,0x0D,0x0C,0x0D,
	0x02,0x08,0x02,0x07,
	0x02,0x08,0x02,0x07,
	0x02,0x0D,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C
};

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

u8 video_timing_576i[] = {
	0x05,0x00,0x01,0x20,
	0x00,0x21,0x00,0x22,
	0x00,0x01,0x00,0x00,
	0x0D,0x0C,0x0B,0x0A,
	0x02,0x6B,0x02,0x6A,
	0x02,0x69,0x02,0x6C,
	0x02,0x71,0x01,0xB0,
	0x40,0x4B,0x6A,0xAC,
	0x01,0x7C,0x85,0x00,
	0x01,0xA4
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
		if(memcmp(addr_start,video_timing_480i,sizeof(video_timing_480i))==0)
		{
			print_gecko("Patched PAL Interlaced timing\n");
			memcpy(addr_start, video_timing_576i, sizeof(video_timing_576i));
			addr_start += sizeof(video_timing_576i);
			continue;
		}
		if(memcmp(addr_start,video_timing_480p,sizeof(video_timing_480p))==0)
		{
			print_gecko("Patched PAL Progressive timing\n");
			memcpy(addr_start, video_timing_576p, sizeof(video_timing_576p));
			addr_start += sizeof(video_timing_576p);
			break;
		}
		addr_start += 2;
	}
}

static u32 patch_locations[PATCHES_MAX];

void checkPatchAddr() {
	if(top_addr < 0x80001800) {
		print_gecko("Too many patches applied, top_addr has gone below the reserved area\r\n");
		// Display something on screen
		while(1);
	}
}

// Returns where the ASM patch has been copied to
u32 installPatch(int patchId) {
	u32 patchSize = 0;
	void* patchLocation = 0;
	switch(patchId) {
		case GX_GETYSCALEFACTORHOOK:
			patchSize = GXGetYScaleFactorHook_length; patchLocation = GXGetYScaleFactorHook; break;
		case GX_INITTEXOBJLODHOOK:
			patchSize = GXInitTexObjLODHook_length; patchLocation = GXInitTexObjLODHook; break;
		case GX_SETPROJECTIONHOOK:
			patchSize = GXSetProjectionHook_length; patchLocation = GXSetProjectionHook; break;
		case GX_SETSCISSORHOOK:
			patchSize = GXSetScissorHook_length; patchLocation = GXSetScissorHook; break;
		case MTX_FRUSTUMHOOK:
			patchSize = MTXFrustumHook_length; patchLocation = MTXFrustumHook; break;
		case MTX_LIGHTFRUSTUMHOOK:
			patchSize = MTXLightFrustumHook_length; patchLocation = MTXLightFrustumHook; break;
		case MTX_LIGHTPERSPECTIVEHOOK:
			patchSize = MTXLightPerspectiveHook_length; patchLocation = MTXLightPerspectiveHook; break;
		case MTX_ORTHOHOOK:
			patchSize = MTXOrthoHook_length; patchLocation = MTXOrthoHook; break;
		case MTX_PERSPECTIVEHOOK:
			patchSize = MTXPerspectiveHook_length; patchLocation = MTXPerspectiveHook; break;
		case SETFBBREGSHOOK:
			patchSize = setFbbRegsHook_length; patchLocation = setFbbRegsHook; break;
		case VI_CONFIGURE240P:
			patchSize = VIConfigure240p_length; patchLocation = VIConfigure240p; break;
		case VI_CONFIGURE288P:
			patchSize = VIConfigure288p_length; patchLocation = VIConfigure288p; break;
		case VI_CONFIGURE480I:
			patchSize = VIConfigure480i_length; patchLocation = VIConfigure480i; break;
		case VI_CONFIGURE480P:
			patchSize = VIConfigure480p_length; patchLocation = VIConfigure480p; break;
		case VI_CONFIGURE576I:
			patchSize = VIConfigure576i_length; patchLocation = VIConfigure576i; break;
		case VI_CONFIGURE576P:
			patchSize = VIConfigure576p_length; patchLocation = VIConfigure576p; break;
		case VI_CONFIGURE960I:
			patchSize = VIConfigure960i_length; patchLocation = VIConfigure960i; break;
		case VI_CONFIGURE1152I:
			patchSize = VIConfigure1152i_length; patchLocation = VIConfigure1152i; break;
		case VI_CONFIGUREPANHOOK:
			patchSize = VIConfigurePanHook_length; patchLocation = VIConfigurePanHook; break;
		case MAJORA_SAVEREGS:
			patchSize = MajoraSaveRegs_length; patchLocation = MajoraSaveRegs; break;
		case MAJORA_AUDIOSTREAM:
			patchSize = MajoraAudioStream_length; patchLocation = MajoraAudioStream; break;
		case MAJORA_LOADREGS:
			patchSize = MajoraLoadRegs_length; patchLocation = MajoraLoadRegs; break;
		default:
			break;
	}
	top_addr -= patchSize;
	checkPatchAddr();
	memcpy((void*)top_addr, patchLocation, patchSize);
	print_gecko("Installed patch %i to %08X\r\n", patchId, top_addr);
	return top_addr;
}

// See patchIds enum in patcher.h
u32 getPatchAddr(int patchId) {
	if(patchId > PATCHES_MAX || patchId < 0) {
		print_gecko("Invalid Patch location requested\r\n");
		return -1;
	}
	if(!patch_locations[patchId]) {
		patch_locations[patchId] = installPatch(patchId);
	}
	return patch_locations[patchId];
}

u8 vertical_filters[][7] = {
	{0, 0, 21, 22, 21, 0, 0},
	{0, 0, 21, 22, 21, 0, 0},
	{4, 8, 12, 16, 12, 8, 4},
	{8, 8, 10, 12, 10, 8, 8}
};

u8 vertical_reduction[][7] = {
	{ 0,  0,  0, 16, 16, 16, 16},
	{ 0,  0,  0, 16, 16, 16, 16},
	{16, 16, 16, 16,  0,  0,  0},
	{ 0,  0, 21, 22, 21,  0,  0}
};

void Patch_VidMode(u8 *data, u32 length, int dataType) {
	int i, j, k;
	u8 *vfilter = NULL;
	FuncPattern __VIRetraceHandlerSigs[6] = {
		{0x20C, 39,  7, 10, 15, 13, 0, 0, "__VIRetraceHandler A", 0},
		{0x220, 42,  8, 11, 15, 13, 0, 0, "__VIRetraceHandler B", 0},
		{0x224, 42,  9, 11, 15, 13, 0, 0, "__VIRetraceHandler C", 0},
		{0x22C, 43, 10, 11, 15, 13, 0, 0, "__VIRetraceHandler D", 0},
		{0x248, 46, 13, 10, 15, 15, 0, 0, "__VIRetraceHandler E", 0},	// SN Systems ProDG
		{0x270, 50, 10, 15, 16, 13, 0, 0, "__VIRetraceHandler F", 0}
	};
	FuncPattern VIWaitForRetraceSig = 
		{0x50, 7, 4, 3, 1, 4, 0, 0, "VIWaitForRetrace", 0};
	FuncPattern VIConfigureSigs[7] = {
		{0x6AC,  90, 43,  6, 32, 60, 0, 0, "VIConfigure A", 0},
		{0x68C,  87, 41,  6, 31, 60, 0, 0, "VIConfigure B", 0},
		{0x73C, 100, 43, 13, 34, 61, 0, 0, "VIConfigure C", 0},
		{0x798, 105, 44, 12, 38, 63, 0, 0, "VIConfigure D", 0},
		{0x824, 111, 44, 13, 53, 64, 0, 0, "VIConfigure E", 0},
		{0x8B8, 112, 44, 14, 53, 48, 0, 0, "VIConfigure F", 0},			// SN Systems ProDG
		{0x804, 110, 44, 13, 49, 63, 0, 0, "VIConfigure G", 0}
	};
	FuncPattern VIConfigurePanSig = 
		{0x390, 40, 11, 4, 25, 35, 0, 0, "VIConfigurePan", 0};
	FuncPattern __GXInitGXSigs[6] = {
		{0xF3C, 454, 81, 119, 43, 36, 0, 0, "__GXInitGX A", 0},
		{0x844, 307, 35, 107, 18, 10, 0, 0, "__GXInitGX B", 0},
		{0x880, 310, 35, 108, 24, 11, 0, 0, "__GXInitGX C", 0},
		{0x8C0, 313, 36, 110, 28, 11, 0, 0, "__GXInitGX D", 0},
		{0x884, 293, 37, 110,  7,  9, 0, 0, "__GXInitGX E", 0},			// SN Systems ProDG
		{0x934, 333, 34, 119, 28, 11, 0, 0, "__GXInitGX F", 0}
	};
	FuncPattern GXGetYScaleFactorSigs[2] = {
		{0x234, 16, 14, 3, 18, 27, 0, 0, "GXGetYScaleFactor A", 0},
		{0x228, 20, 19, 3,  9, 14, 0, 0, "GXGetYScaleFactor B", 0}		// SN Systems ProDG
	};
	FuncPattern GXSetCopyFilterSigs[3] = {
		{0x224, 15,  7, 0, 4,  5, 0, 0, "GXSetCopyFilter A", 0},
		{0x288, 19, 23, 0, 3, 14, 0, 0, "GXSetCopyFilter B", 0},		// SN Systems ProDG
		{0x204, 25,  7, 0, 4,  0, 0, 0, "GXSetCopyFilter C", 0}
	};
	FuncPattern GXSetViewportSigs[4] = {
		{0x20, 3, 2, 1, 0, 2, 0, 0, "GXSetViewport A", 0},
		{0x20, 3, 2, 1, 0, 2, 0, 0, "GXSetViewport B", 0},
		{0x38, 7, 6, 0, 1, 0, 0, 0, "GXSetViewport C", 0},				// SN Systems ProDG
		{0x44, 5, 8, 1, 0, 2, 0, 0, "GXSetViewport D", 0}
	};
	FuncPattern GXSetViewportJitterSigs[4] = {
		{0x118, 20, 15, 1, 1, 3, 0, 0, "GXSetViewportJitter A", 0},
		{0x100, 14, 15, 1, 1, 3, 0, 0, "GXSetViewportJitter B", 0},
		{0x078,  6, 10, 1, 0, 4, 0, 0, "GXSetViewportJitter C", 0},		// SN Systems ProDG
		{0x054,  6,  8, 1, 0, 2, 0, 0, "GXSetViewportJitter D", 0}
	};
	FuncPattern getCurrentFieldEvenOddSigs[2] = {
		{0xB8, 14, 2, 2, 4, 8, 0, 0, "getCurrentFieldEvenOdd A", 0},
		{0x5C,  7, 0, 0, 1, 5, 0, 0, "getCurrentFieldEvenOdd B", 0}		// SN Systems ProDG
	};
	FuncPattern setFbbRegsSigs[2] = {
		{0x2D0, 54, 34, 0, 10, 16, 0, 0, "setFbbRegs A", 0},
		{0x2C0, 51, 34, 0, 10, 16, 0, 0, "setFbbRegs B", 0}				// SN Systems ProDG
	};
	
	for( i=0; i < length; i+=4 )
	{
		if( *(vu32*)(data+i) != 0x7C0802A6 && *(vu32*)(data+i+4) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( (u8*)(data+i), length, &fp );
		
		for( j=0; j < sizeof(__VIRetraceHandlerSigs)/sizeof(FuncPattern); j++ )
		{
			if( !__VIRetraceHandlerSigs[j].offsetFoundAt && compare_pattern( &fp, &(__VIRetraceHandlerSigs[j]) ) )
			{
				u32 __VIRetraceHandlerAddr = Calc_ProperAddress(data, dataType, i);
				if(__VIRetraceHandlerAddr) {
					print_gecko("Found:[%s] @ %08X\n", __VIRetraceHandlerSigs[j].Name, __VIRetraceHandlerAddr);
					switch(j) {
						case 0:
						case 1: getCurrentFieldEvenOddSigs[0].offsetFoundAt = branchResolve(data, i + 240); break;
						case 2:
						case 3: getCurrentFieldEvenOddSigs[0].offsetFoundAt = branchResolve(data, i + 252); break;
						case 4: getCurrentFieldEvenOddSigs[1].offsetFoundAt = branchResolve(data, i + 272); break;
						case 5: getCurrentFieldEvenOddSigs[0].offsetFoundAt = branchResolve(data, i + 320); break;
					}
					if((swissSettings.gameVMode == 2) || (swissSettings.gameVMode == 7)) {
						switch(j) {
							case 0:
							case 1: *(vu32*)(data+i+228) = 0x38000001; break;
							case 2:
							case 3: *(vu32*)(data+i+240) = 0x38000001; break;
							case 4: *(vu32*)(data+i+260) = 0x38000001; break;
							case 5: *(vu32*)(data+i+308) = 0x38000001; break;
						}
					}
					__VIRetraceHandlerSigs[j].offsetFoundAt = i;
					i += __VIRetraceHandlerSigs[j].Length;
					break;
				}
			}
		}
		if( !VIWaitForRetraceSig.offsetFoundAt && compare_pattern( &fp, &VIWaitForRetraceSig ) )
		{
			u32 VIWaitForRetraceAddr = Calc_ProperAddress(data, dataType, i);
			if(VIWaitForRetraceAddr) {
				print_gecko("Found:[%s] @ %08X\n", VIWaitForRetraceSig.Name, VIWaitForRetraceAddr);
				if((swissSettings.gameVMode == 2) || (swissSettings.gameVMode == 7)) {
					for( k=0; k < sizeof(getCurrentFieldEvenOddSigs)/sizeof(FuncPattern); k++ )
					{
						if( getCurrentFieldEvenOddSigs[k].offsetFoundAt )
						{
							u32 getCurrentFieldEvenOddAddr = Calc_ProperAddress(data, dataType, getCurrentFieldEvenOddSigs[k].offsetFoundAt);
							if(getCurrentFieldEvenOddAddr) {
								print_gecko("Found:[%s] @ %08X\n", getCurrentFieldEvenOddSigs[k].Name, getCurrentFieldEvenOddAddr);
								*(vu32*)(data+i+24) = *(vu32*)(data+i+28);
								*(vu32*)(data+i+28) = 0x4800000C;	// b		+12
								*(vu32*)(data+i+40) = branchAndLink(getCurrentFieldEvenOddAddr, VIWaitForRetraceAddr + 40);
								*(vu32*)(data+i+44) = 0x28030000;	// cmplwi	r3, 0
								break;
							}
						}
					}
				}
				VIWaitForRetraceSig.offsetFoundAt = i;
				i += VIWaitForRetraceSig.Length;
			}
		}
		for( j=0; j < sizeof(VIConfigureSigs)/sizeof(FuncPattern); j++ )
		{
			if( !VIConfigureSigs[j].offsetFoundAt && compare_pattern( &fp, &(VIConfigureSigs[j]) ) )
			{
				u32 VIConfigureAddr = Calc_ProperAddress(data, dataType, i);
				u32 VIConfigurePatchAddr = 0;
				if(VIConfigureAddr) {
					print_gecko("Found:[%s] @ %08X\n", VIConfigureSigs[j].Name, VIConfigureAddr);
					switch(swissSettings.gameVMode) {
						case 1:
						case 2:
							print_gecko("Patched NTSC Interlaced mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE480I);
							if(swissSettings.forceVFilter > 0)
								vfilter = vertical_filters[swissSettings.forceVFilter];
							break;
						case 3:
							print_gecko("Patched NTSC Double-Strike mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE240P);
							vfilter = vertical_reduction[swissSettings.forceVFilter];
							break;
						case 4:
							print_gecko("Patched NTSC Field Rendering mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE960I);
							vfilter = vertical_filters[1];
							break;
						case 5:
							print_gecko("Patched NTSC Progressive mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE480P);
							vfilter = vertical_filters[swissSettings.forceVFilter];
							break;
						case 6:
						case 7:
							print_gecko("Patched PAL Interlaced mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE576I);
							if(swissSettings.forceVFilter > 0)
								vfilter = vertical_filters[swissSettings.forceVFilter];
							break;
						case 8:
							print_gecko("Patched PAL Double-Strike mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE288P);
							vfilter = vertical_reduction[swissSettings.forceVFilter];
							break;
						case 9:
							print_gecko("Patched PAL Field Rendering mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE1152I);
							vfilter = vertical_filters[1];
							Patch_VidTiming(data, length);
							break;
						case 10:
							print_gecko("Patched PAL Progressive mode\n");
							VIConfigurePatchAddr = getPatchAddr(VI_CONFIGURE576P);
							vfilter = vertical_filters[swissSettings.forceVFilter];
							Patch_VidTiming(data, length);
							break;
					}
					switch(j) {
						case 0:
							*(vu32*)(data+i+ 28) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 28);
							*(vu32*)(data+i+140) = 0x60000000;	// nop
							*(vu32*)(data+i+260) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+280) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+284) = 0x60000000;	// nop
							*(vu32*)(data+i+292) = 0xA01F0010;	// lhz		r0, 16 (r31)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 1636);
							break;
						case 1:
							*(vu32*)(data+i+ 28) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 28);
							*(vu32*)(data+i+108) = 0x60000000;	// nop
							*(vu32*)(data+i+228) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+248) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+252) = 0x60000000;	// nop
							*(vu32*)(data+i+260) = 0xA01F0010;	// lhz		r0, 16 (r31)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 1604);
							break;
						case 2:
							*(vu32*)(data+i+ 36) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 36);
							*(vu32*)(data+i+248) = 0x60000000;	// nop
							*(vu32*)(data+i+368) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+388) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+392) = 0x60000000;	// nop
							*(vu32*)(data+i+400) = 0xA01F0010;	// lhz		r0, 16 (r31)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 1780);
							break;
						case 3:
							*(vu32*)(data+i+ 36) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 36);
							*(vu32*)(data+i+260) = 0x60000000;	// nop
							*(vu32*)(data+i+380) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+396) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+416) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+420) = 0x60000000;	// nop
							*(vu32*)(data+i+428) = 0xA01F0010;	// lhz		r0, 16 (r31)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 1872);
							break;
						case 4:
							*(vu32*)(data+i+ 36) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 36);
							*(vu32*)(data+i+388) = 0x60000000;	// nop
							*(vu32*)(data+i+508) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+524) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+544) = 0xA01F0010;	// lhz		r0, 16 (r31)
							*(vu32*)(data+i+548) = 0x60000000;	// nop
							*(vu32*)(data+i+556) = 0xA01F0010;	// lhz		r0, 16 (r31)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 2012);
							break;
						case 5:
							*(vu32*)(data+i+ 32) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 32);
							*(vu32*)(data+i+400) = 0xA11B000C;	// lhz		r8, 12 (r27)
							*(vu32*)(data+i+404) = 0x60000000;	// nop
							*(vu32*)(data+i+492) = 0xA0FB0010;	// lhz		r7, 16 (r27)
							*(vu32*)(data+i+508) = 0xA0FB0010;	// lhz		r7, 16 (r27)
							*(vu32*)(data+i+524) = 0xA0FB0010;	// lhz		r7, 16 (r27)
							*(vu32*)(data+i+532) = 0xA0FB0010;	// lhz		r7, 16 (r27)
							setFbbRegsSigs[1].offsetFoundAt = branchResolve(data, i + 2148);
							break;
						case 6:
							*(vu32*)(data+i+ 36) = branchAndLink(VIConfigurePatchAddr, VIConfigureAddr + 36);
							*(vu32*)(data+i+388) = 0x60000000;	// nop
							*(vu32*)(data+i+508) = 0xA0130010;	// lhz		r0, 16 (r19)
							*(vu32*)(data+i+524) = 0xA0130010;	// lhz		r0, 16 (r19)
							*(vu32*)(data+i+544) = 0xA0130010;	// lhz		r0, 16 (r19)
							*(vu32*)(data+i+548) = 0x60000000;	// nop
							*(vu32*)(data+i+556) = 0xA0130010;	// lhz		r0, 16 (r19)
							setFbbRegsSigs[0].offsetFoundAt = branchResolve(data, i + 1980);
							break;
					}
					if((swissSettings.gameVMode == 4) || (swissSettings.gameVMode == 9)) {
						switch(j) {
							case 0:
								*(vu32*)(data+i+ 820) = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
								*(vu32*)(data+i+ 824) = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
								break;
							case 1:
								*(vu32*)(data+i+ 788) = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
								*(vu32*)(data+i+ 792) = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
								break;
							case 2:
								*(vu32*)(data+i+ 928) = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
								*(vu32*)(data+i+ 932) = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
								break;
							case 3:
								*(vu32*)(data+i+1012) = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
								*(vu32*)(data+i+1016) = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
								break;
							case 4:
								*(vu32*)(data+i+1140) = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
								*(vu32*)(data+i+1144) = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
								break;
							case 5:
								*(vu32*)(data+i+1236) = 0x7D004378;	// mr		r0, r8
								*(vu32*)(data+i+1240) = 0x5160177A;	// rlwimi	r0, r11, 2, 29, 29
								break;
							case 6:
								*(vu32*)(data+i+1152) = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
								*(vu32*)(data+i+1156) = 0x54E9003C;	// rlwinm	r9, r7, 0, 0, 30
								*(vu32*)(data+i+1160) = 0x61290001;	// ori		r9, r9, 1
								break;
						}
					}
					if((swissSettings.gameVMode == 9) || (swissSettings.gameVMode == 10)) {
						switch(j) {
							case 0:
								*(vu32*)(data+i+  40) = 0x2C040006;	// cmpwi	r4, 6
								*(vu32*)(data+i+ 304) = 0x807B0000;	// lwz		r3, 0 (r27)
								*(vu32*)(data+i+ 896) = 0x2C000006;	// cmpwi	r0, 6
								break;
							case 1:
								*(vu32*)(data+i+ 272) = 0x807C0000;	// lwz		r3, 0 (r28)
								*(vu32*)(data+i+ 864) = 0x2C000006;	// cmpwi	r0, 6
								break;
							case 2:
								*(vu32*)(data+i+ 412) = 0x807C0000;	// lwz		r3, 0 (r28)
								*(vu32*)(data+i+1040) = 0x2C000006;	// cmpwi	r0, 6
								break;
							case 3:
								*(vu32*)(data+i+ 476) = 0x38600000;	// li		r3, 0
								*(vu32*)(data+i+1128) = 0x2C000006;	// cmpwi	r0, 6
								*(vu32*)(data+i+1136) = 0x2C000007;	// cmpwi	r0, 7
								break;
							case 4: 
								*(vu32*)(data+i+ 604) = 0x38600000;	// li		r3, 0
								*(vu32*)(data+i+1260) = 0x2C000006;	// cmpwi	r0, 6
								*(vu32*)(data+i+1268) = 0x2C000007;	// cmpwi	r0, 7
								break;
							case 5:
								*(vu32*)(data+i+ 548) = 0x38000000;	// li		r0, 0
								*(vu32*)(data+i+1344) = 0x2C0A0006;	// cmpwi	r10, 6
								*(vu32*)(data+i+1372) = 0x2C0A0007;	// cmpwi	r10, 7
								break;
							case 6:
								*(vu32*)(data+i+ 604) = 0x38600000;	// li		r3, 0
								break;
						}
					}
					VIConfigureSigs[j].offsetFoundAt = i;
					i += VIConfigureSigs[j].Length;
					break;
				}
			}
		}
		if( !VIConfigurePanSig.offsetFoundAt && compare_pattern( &fp, &VIConfigurePanSig ) )
		{
			u32 VIConfigurePanAddr = Calc_ProperAddress(data, dataType, i);
			u32 VIConfigurePanPatchAddr = 0;
			if(VIConfigurePanAddr) {
				print_gecko("Found:[%s] @ %08X\n", VIConfigurePanSig.Name, VIConfigurePanAddr);
				VIConfigurePanPatchAddr = getPatchAddr(VI_CONFIGUREPANHOOK);
				*(vu32*)(data+i+40) = branchAndLink(VIConfigurePanPatchAddr, VIConfigurePanAddr + 40);
				VIConfigurePanSig.offsetFoundAt = i;
				i += VIConfigurePanSig.Length;
			}
		}
		for( j=0; j < sizeof(__GXInitGXSigs)/sizeof(FuncPattern); j++ )
		{
			if( !__GXInitGXSigs[j].offsetFoundAt && compare_pattern( &fp, &(__GXInitGXSigs[j]) ) )
			{
				u32 __GXInitGXAddr = Calc_ProperAddress(data, dataType, i);
				if(__GXInitGXAddr) {
					print_gecko("Found:[%s] @ %08X\n", __GXInitGXSigs[j].Name, __GXInitGXAddr);
					switch(j) {
						case 0:
							GXSetCopyFilterSigs[0].offsetFoundAt = branchResolve(data, i + 3764);
							GXSetViewportSigs[0].offsetFoundAt = branchResolve(data, i + 2212);
							break;
						case 1:
							GXSetCopyFilterSigs[0].offsetFoundAt = branchResolve(data, i + 1964);
							GXSetViewportSigs[0].offsetFoundAt = branchResolve(data, i + 744);
							break;
						case 2:
							GXSetCopyFilterSigs[0].offsetFoundAt = branchResolve(data, i + 2024);
							GXSetViewportSigs[0].offsetFoundAt = branchResolve(data, i + 808);
							break;
						case 3:
							GXSetCopyFilterSigs[0].offsetFoundAt = branchResolve(data, i + 2084);
							GXSetViewportSigs[1].offsetFoundAt = branchResolve(data, i + 860);
							break;
						case 4:
							GXSetCopyFilterSigs[1].offsetFoundAt = branchResolve(data, i + 1996);
							GXSetViewportSigs[2].offsetFoundAt = branchResolve(data, i + 808);
							break;
						case 5:
							GXSetCopyFilterSigs[2].offsetFoundAt = branchResolve(data, i + 2200);
							GXSetViewportSigs[3].offsetFoundAt = branchResolve(data, i + 860);
							break;
					}
					if((swissSettings.gameVMode == 4) || (swissSettings.gameVMode == 9)) {
						switch(j) {
							case 0: *(vu32*)(data+i+3644) = 0x38600001; break;
							case 1: *(vu32*)(data+i+1844) = 0x38600001; break;
							case 2: *(vu32*)(data+i+1904) = 0x38600001; break;
							case 3: *(vu32*)(data+i+1964) = 0x38600001; break;
							case 4: *(vu32*)(data+i+1844) = 0x38600001; break;
							case 5: *(vu32*)(data+i+2080) = 0x38600001; break;
						}
					}
					__GXInitGXSigs[j].offsetFoundAt = i;
					i += __GXInitGXSigs[j].Length;
					break;
				}
			}
		}
		for( j=0; j < sizeof(GXGetYScaleFactorSigs)/sizeof(FuncPattern); j++ )
		{
			if( !GXGetYScaleFactorSigs[j].offsetFoundAt && compare_pattern( &fp, &(GXGetYScaleFactorSigs[j]) ) )
			{
				u32 GXGetYScaleFactorAddr = Calc_ProperAddress(data, dataType, i);
				if(GXGetYScaleFactorAddr) {
					print_gecko("Found:[%s] @ %08X\n", GXGetYScaleFactorSigs[j].Name, GXGetYScaleFactorAddr);
					if((swissSettings.gameVMode >= 1) && (swissSettings.gameVMode <= 5)) {
						top_addr -= GXGetYScaleFactorHook_length;
						checkPatchAddr();
						memcpy((void*)top_addr, GXGetYScaleFactorHook, GXGetYScaleFactorHook_length);
						*(vu32*)(top_addr+16) = branch(GXGetYScaleFactorAddr + 4, top_addr + 16);
						*(vu32*)(data+i) = branch(top_addr, GXGetYScaleFactorAddr);
					}
					GXGetYScaleFactorSigs[j].offsetFoundAt = i;
					i += GXGetYScaleFactorSigs[j].Length;
					break;
				}
			}
		}
	}
	if(vfilter != NULL) {
		for( j=0; j < sizeof(GXSetCopyFilterSigs)/sizeof(FuncPattern); j++ )
		{
			if( (i=GXSetCopyFilterSigs[j].offsetFoundAt) )
			{
				u32 GXSetCopyFilterAddr = Calc_ProperAddress(data, dataType, i);
				if(GXSetCopyFilterAddr) {
					print_gecko("Found:[%s] @ %08X\n", GXSetCopyFilterSigs[j].Name, GXSetCopyFilterAddr);
					switch(j) {
						case 0:
							*(vu32*)(data+i+388) = 0x38000000 | vfilter[0];
							*(vu32*)(data+i+392) = 0x38600000 | vfilter[1];
							*(vu32*)(data+i+400) = 0x38000000 | vfilter[4];
							*(vu32*)(data+i+404) = 0x38800000 | vfilter[2];
							*(vu32*)(data+i+416) = 0x38600000 | vfilter[5];
							*(vu32*)(data+i+428) = 0x38A00000 | vfilter[3];
							*(vu32*)(data+i+432) = 0x38000000 | vfilter[6];
							break;
						case 1:
							*(vu32*)(data+i+492) = 0x38000000 | vfilter[0];
							*(vu32*)(data+i+496) = 0x39200000 | vfilter[1];
							*(vu32*)(data+i+504) = 0x39400000 | vfilter[4];
							*(vu32*)(data+i+508) = 0x39600000 | vfilter[2];
							*(vu32*)(data+i+516) = 0x39000000 | vfilter[5];
							*(vu32*)(data+i+536) = 0x39400000 | vfilter[6];
							*(vu32*)(data+i+540) = 0x39200000 | vfilter[3];
							break;
						case 2:
							*(vu32*)(data+i+372) = 0x38800000 | vfilter[0];
							*(vu32*)(data+i+376) = 0x38600000 | vfilter[4];
							*(vu32*)(data+i+384) = 0x38800000 | vfilter[1];
							*(vu32*)(data+i+392) = 0x38E00000 | vfilter[2];
							*(vu32*)(data+i+400) = 0x38800000 | vfilter[5];
							*(vu32*)(data+i+404) = 0x38A00000 | vfilter[3];
							*(vu32*)(data+i+412) = 0x38600000 | vfilter[6];
							break;
					}
					break;
				}
			}
		}
	}
	if((swissSettings.gameVMode == 3) || (swissSettings.gameVMode == 8)) {
		for( j=0; j < sizeof(setFbbRegsSigs)/sizeof(FuncPattern); j++ )
		{
			if( (i=setFbbRegsSigs[j].offsetFoundAt) )
			{
				u32 setFbbRegsAddr = Calc_ProperAddress(data, dataType, i);
				if(setFbbRegsAddr) {
					print_gecko("Found:[%s] @ %08X\n", setFbbRegsSigs[j].Name, setFbbRegsAddr);
					switch (j) {
						case 0:
							*(vu32*)(data+i+ 80) = 0x81040000;	// lwz		r8, 0 (r4)
							*(vu32*)(data+i+ 84) = 0x60000000;	// nop
							*(vu32*)(data+i+232) = 0x81060000;	// lwz		r8, 0 (r6)
							*(vu32*)(data+i+236) = 0x60000000;	// nop
							break;
						case 1:
							*(vu32*)(data+i+ 72) = 0x81240000;	// lwz		r9, 0 (r4)
							*(vu32*)(data+i+ 76) = 0x60000000;	// nop
							*(vu32*)(data+i+224) = 0x81260000;	// lwz		r9, 0 (r6)
							*(vu32*)(data+i+228) = 0x60000000;	// nop
							break;
					}
					break;
				}
			}
		}
	}
	if((swissSettings.gameVMode == 4) || (swissSettings.gameVMode == 9)) {
		for( j=0; j < sizeof(GXSetViewportSigs)/sizeof(FuncPattern); j++ )
		{
			if( (i=GXSetViewportSigs[j].offsetFoundAt) )
			{
				u32 GXSetViewportAddr = Calc_ProperAddress(data, dataType, i);
				if(GXSetViewportAddr) {
					print_gecko("Found:[%s] @ %08X\n", GXSetViewportSigs[j].Name, GXSetViewportAddr);
					switch(j) {
						case 0: GXSetViewportJitterSigs[0].offsetFoundAt = branchResolve(data, i + 16); break;
						case 1: GXSetViewportJitterSigs[1].offsetFoundAt = branchResolve(data, i + 16); break;
						case 2: GXSetViewportJitterSigs[2].offsetFoundAt = branchResolve(data, i +  4); break;
					}
					break;
				}
			}
		}
		for( j=0; j < sizeof(GXSetViewportJitterSigs)/sizeof(FuncPattern); j++ )
		{
			if( (i=GXSetViewportJitterSigs[j].offsetFoundAt) )
			{
				u32 GXSetViewportJitterAddr = Calc_ProperAddress(data, dataType, i);
				if(GXSetViewportJitterAddr) {
					print_gecko("Found:[%s] @ %08X\n", GXSetViewportJitterSigs[j].Name, GXSetViewportJitterAddr);
					switch(j) {
						case 0:
							memmove(data+i+36, data+i+52, 104);
							*(vu32*)(data+i+140) = 0x3C600000 | (VAR_AREA >> 16);
							*(vu32*)(data+i+144) = 0x88030000 | (VAR_CURRENT_FIELD & 0xFFFF);
							*(vu32*)(data+i+148) = 0x28000000;	// cmplwi	r0, 0
							*(vu32*)(data+i+152) = 0x41820008;	// beq		+8
							*(vu32*)(data+i+156) = 0xEF5A582A;	// fadds	f26, f26, f11
							memmove(data+i+160, data+i+184, 100);
							memset(data+i+260, 0, 24);
							break;
						case 1:
							memmove(data+i+ 4, data+i+ 8, 32);
							memmove(data+i+36, data+i+52, 84);
							*(vu32*)(data+i+120) = 0x3C600000 | (VAR_AREA >> 16);
							*(vu32*)(data+i+124) = 0x88030000 | (VAR_CURRENT_FIELD & 0xFFFF);
							*(vu32*)(data+i+128) = 0x28000000;	// cmplwi	r0, 0
							*(vu32*)(data+i+132) = 0x41820008;	// beq		+8
							*(vu32*)(data+i+136) = 0xEF5A582A;	// fadds	f26, f26, f11
							memmove(data+i+140, data+i+160, 100);
							memset(data+i+240, 0, 20);
							break;
					}
					break;
				}
			}
		}
		for( j=0; j < sizeof(setFbbRegsSigs)/sizeof(FuncPattern); j++ )
		{
			if( (i=setFbbRegsSigs[j].offsetFoundAt) )
			{
				u32 setFbbRegsAddr = Calc_ProperAddress(data, dataType, i);
				if(setFbbRegsAddr) {
					print_gecko("Found:[%s] @ %08X\n", setFbbRegsSigs[j].Name, setFbbRegsAddr);
					for( k=0; k < sizeof(getCurrentFieldEvenOddSigs)/sizeof(FuncPattern); k++ )
					{
						if( getCurrentFieldEvenOddSigs[k].offsetFoundAt )
						{
							u32 getCurrentFieldEvenOddAddr = Calc_ProperAddress(data, dataType, getCurrentFieldEvenOddSigs[k].offsetFoundAt);
							if(getCurrentFieldEvenOddAddr) {
								print_gecko("Found:[%s] @ %08X\n", getCurrentFieldEvenOddSigs[k].Name, getCurrentFieldEvenOddAddr);
								top_addr -= setFbbRegsHook_length;
								checkPatchAddr();
								memcpy((void*)top_addr, setFbbRegsHook, setFbbRegsHook_length);
								*(vu32*)(top_addr+12) = branchAndLink(getCurrentFieldEvenOddAddr, top_addr + 12);
								*(vu32*)(data+i+setFbbRegsSigs[j].Length) = branch(top_addr, setFbbRegsAddr + setFbbRegsSigs[j].Length);
								break;
							}
						}
					}
					break;
				}
			}
		}
	}
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
		if( *(vu32*)(data+i) != 0xED241828 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXFrustumSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXFrustumSig.Name, properAddress);
				top_addr -= MTXFrustumHook_length;
				checkPatchAddr();
				memcpy((void*)top_addr, MTXFrustumHook, MTXFrustumHook_length);
				*(vu32*)(top_addr+28) = branch(properAddress+4, top_addr+28);
				*(vu32*)(data+i) = branch(top_addr, properAddress);
				*(vu32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
				MTXFrustumSig.offsetFoundAt = (u32)data+i;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(vu32*)(data+i+8) != 0xED441828 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXLightFrustumSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightFrustumSig.Name, properAddress);
				top_addr -= MTXLightFrustumHook_length;
				checkPatchAddr();
				memcpy((void*)top_addr, MTXLightFrustumHook, MTXLightFrustumHook_length);
				*(vu32*)(top_addr+28) = branch(properAddress+12, top_addr+28);
				*(vu32*)(data+i+8) = branch(top_addr, properAddress+8);
				*(vu32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(vu32*)(data+i) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXPerspectiveSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXPerspectiveSig.Name, properAddress);
				top_addr -= MTXPerspectiveHook_length;
				checkPatchAddr();
				memcpy((void*)top_addr, MTXPerspectiveHook, MTXPerspectiveHook_length);
				*(vu32*)(top_addr+20) = branch(properAddress+84, top_addr+20);
				*(vu32*)(data+i+80) = branch(top_addr, properAddress+80);
				*(vu32*)VAR_FLOAT9_16 = 0x3F100000;
				MTXPerspectiveSig.offsetFoundAt = (u32)data+i;
				break;
			}
		}
	}
	for( i=0; i < length; i+=4 )
	{
		if( *(vu32*)(data+i) != 0x7C0802A6 )
			continue;
		if( find_pattern( (u8*)(data+i), length, &MTXLightPerspectiveSig ) )
		{
			u32 properAddress = Calc_ProperAddress(data, dataType, i);
			if(properAddress) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightPerspectiveSig.Name, properAddress);
				top_addr -= MTXLightPerspectiveHook_length;
				checkPatchAddr();
				memcpy((void*)top_addr, MTXLightPerspectiveHook, MTXLightPerspectiveHook_length);
				*(vu32*)(top_addr+20) = branch(properAddress+100, top_addr+20);
				*(vu32*)(data+i+96) = branch(top_addr, properAddress+96);
				*(vu32*)VAR_FLOAT9_16 = 0x3F100000;
				break;
			}
		}
	}
	if(swissSettings.forceWidescreen == 2) {
		for( i=0; i < length; i+=4 )
		{
			if( *(vu32*)(data+i) != 0xED041828 )
				continue;
			if( find_pattern( (u8*)(data+i), length, &MTXOrthoSig ) )
			{
				u32 properAddress = Calc_ProperAddress(data, dataType, i);
				if(properAddress) {
					print_gecko("Found:[%s] @ %08X\n", MTXOrthoSig.Name, properAddress);
					top_addr -= MTXOrthoHook_length;
					checkPatchAddr();
					memcpy((void*)top_addr, MTXOrthoHook, MTXOrthoHook_length);
					*(vu32*)(top_addr+128) = branch(properAddress+4, top_addr+128);
					*(vu32*)(data+i) = branch(top_addr, properAddress);
					*(vu32*)VAR_FLOAT1_6 = 0x3E2AAAAA;
					*(vu32*)VAR_FLOATM_1 = 0xBF800000;
					break;
				}
			}
		}
		for( i=0; i < length; i+=4 )
		{
			if( (*(vu32*)(data+i+4) & 0xFC00FFFF) != 0x38000156 )
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
						checkPatchAddr();
						memcpy((void*)top_addr, GXSetScissorHook, GXSetScissorHook_length);
						*(vu32*)(top_addr+ 0) = *(vu32*)(data+i);
						*(vu32*)(top_addr+ 4) = j == 1 ? 0x800801E8:0x800701E8;
						*(vu32*)(top_addr+64) = branch(properAddress+4, top_addr+64);
						*(vu32*)(data+i) = branch(top_addr, properAddress);
						break;
					}
				}
			}
		}
	}
	if( !MTXFrustumSig.offsetFoundAt && !MTXPerspectiveSig.offsetFoundAt ) {
		for( i=0; i < length; i+=4 )
		{
			if( *(vu32*)(data+i+4) != 0x2C040001 )
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
						checkPatchAddr();
						memcpy((void*)top_addr, GXSetProjectionHook, GXSetProjectionHook_length);
						*(vu32*)(top_addr+20) = branch(properAddress+16, top_addr+20);
						*(vu32*)(data+i+12) = branch(top_addr, properAddress+12);
						*(vu32*)VAR_FLOAT3_4 = 0x3F400000;
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
		if( *(vu32*)(data+i+8) != 0xFC030040 && *(vu32*)(data+i+16) != 0xFC030000 )
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
					checkPatchAddr();
					memcpy((void*)top_addr, GXInitTexObjLODHook, GXInitTexObjLODHook_length);
					*(vu32*)(top_addr+24) = *(vu32*)(data+i);
					*(vu32*)(top_addr+28) = branch(properAddress+4, top_addr+28);
					*(vu32*)(data+i) = branch(top_addr, properAddress);
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
					*(vu32*)(addr_start+40) = 0x38000000;
					break;
				case 2:
					*(vu32*)(addr_start+48) = 0x38000001;
					*(vu32*)(addr_start+60) = 0x38000001;
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

// Apply specific per-game hacks/workarounds
int Patch_GameSpecific(void *addr, u32 length, const char* gameID, int dataType) {
	int patched = 0;
	
	// Fix Zelda WW on Wii (__GXSetVAT patch)
	if (!is_gamecube() && (!strncmp(gameID, "GZLP01", 6) || !strncmp(gameID, "GZLE01", 6) || !strncmp(gameID, "GZLJ01", 6))) {
		int mode;
		if(!strncmp(gameID, "GZLP01", 6))
			mode = 1;	//PAL
		else 
			mode = 2;	//NTSC-U,NTSC-J
		void *addr_start = addr;
		void *addr_end = addr+length;
		while(addr_start<addr_end) 
		{
			if(mode == 1 && memcmp(addr_start,GXSETVAT_PAL_orig,sizeof(GXSETVAT_PAL_orig))==0) 
			{
				memcpy(addr_start, GXSETVAT_PAL_patched, sizeof(GXSETVAT_PAL_patched));
				print_gecko("Patched:[__GXSetVAT] PAL\r\n");
				patched=1;
			}
			if(mode == 2 && memcmp(addr_start,GXSETVAT_NTSC_orig,sizeof(GXSETVAT_NTSC_orig))==0) 
			{
				memcpy(addr_start, GXSETVAT_NTSC_patched, sizeof(GXSETVAT_NTSC_patched));
				print_gecko("Patched:[__GXSetVAT] NTSC\r\n");
				patched=1;
			}
			addr_start += 4;
		}
	}
	else if(!strncmp(gameID, "GXX", 3) || !strncmp(gameID, "GC6", 3))
	{
		print_gecko("Patched:[Pokemon memset]\r\n");
		// patch memset to jump to test function
		*(vu32*)(addr+0x2420) = 0x4BFFAC68;
		patched=1;
	}
	else if(!strncmp(gameID, "PZL", 3))
	{
		if(*(vu32*)(addr+0xDE6D8) == 0x2F6D616A) // PAL
		{
			print_gecko("Patched:[Majoras Mask (Zelda CE) PAL]\r\n");
			//save up regs
			u32 patchAddr = getPatchAddr(MAJORA_SAVEREGS);
			*(vu32*)(addr+0x1A6B4) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A6B4));
			*(vu32*)(patchAddr+MajoraSaveRegs_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x1A6B8), patchAddr+MajoraSaveRegs_length-4);
			//frees r28 and secures r10 for us
			*(vu32*)(addr+0x1A76C) = 0x60000000;
			*(vu32*)(addr+0x1A770) = 0x839D0000;
			*(vu32*)(addr+0x1A774) = 0x7D3C4AAE;
			//do audio streaming injection
			patchAddr = getPatchAddr(MAJORA_AUDIOSTREAM);
			*(vu32*)(addr+0x1A784) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A784));
			*(vu32*)(patchAddr+MajoraAudioStream_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x1A788), patchAddr+MajoraAudioStream_length-4);
			//load up regs (and jump back)
			patchAddr = getPatchAddr(MAJORA_LOADREGS);
			*(vu32*)(addr+0x1A878) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A878));
			patched=1;
		}
		else if(*(vu32*)(addr+0xEF78C) == 0x2F6D616A) // NTSC-U
		{
			print_gecko("Patched:[Majoras Mask (Zelda CE) NTSC-U]\r\n");
			//save up regs
			u32 patchAddr = getPatchAddr(MAJORA_SAVEREGS);
			*(vu32*)(addr+0x19DD4) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x19DD4));
			*(vu32*)(patchAddr+MajoraSaveRegs_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x19DD8), patchAddr+MajoraSaveRegs_length-4);
			//frees r28 and secures r10 for us
			*(vu32*)(addr+0x19E8C) = 0x60000000;
			*(vu32*)(addr+0x19E90) = 0x839D0000;
			*(vu32*)(addr+0x19E94) = 0x7D3C4AAE;
			//do audio streaming injection
			patchAddr = getPatchAddr(MAJORA_AUDIOSTREAM);
			*(vu32*)(addr+0x19EA4) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x19EA4));
			*(vu32*)(patchAddr+MajoraAudioStream_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x19EA8), patchAddr+MajoraAudioStream_length-4);
			//load up regs (and jump back)
			patchAddr = getPatchAddr(MAJORA_LOADREGS);
			*(vu32*)(addr+0x19F98) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x19F98));
			patched=1;
		}
		else if(*(vu32*)(addr+0xF324C) == 0x2F6D616A) // NTSC-J
		{
			print_gecko("Patched:[Majoras Mask (Zelda CE) NTSC-J]\r\n");
			//save up regs
			u32 patchAddr = getPatchAddr(MAJORA_SAVEREGS);
			*(vu32*)(addr+0x1A448) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A448));
			*(vu32*)(patchAddr+MajoraSaveRegs_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x1A44C), patchAddr+MajoraSaveRegs_length-4);
			//frees r28 and secures r10 for us
			*(vu32*)(addr+0x1A500) = 0x60000000;
			*(vu32*)(addr+0x1A504) = 0x839D0000;
			*(vu32*)(addr+0x1A508) = 0x7D3C4AAE;
			//do audio streaming injection
			patchAddr = getPatchAddr(MAJORA_AUDIOSTREAM);
			*(vu32*)(addr+0x1A518) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A518));
			*(vu32*)(patchAddr+MajoraAudioStream_length-4) = branch(Calc_ProperAddress(addr, dataType, 0x1A51C), patchAddr+MajoraAudioStream_length-4);
			//load up regs (and jump back)
			patchAddr = getPatchAddr(MAJORA_LOADREGS);
			*(vu32*)(addr+0x1A60C) = branch(patchAddr, Calc_ProperAddress(addr, dataType, 0x1A60C));
			patched=1;
		}
	}
	else if(!strncmp(gameID, "GPQ", 3))
	{
		// Audio Stream force DMA to get Video Sound
		if(*(vu32*)(addr+0xF03D8) == 0x4E800020 && *(vu32*)(addr+0xF0494) == 0x4E800020)
		{
			//Call AXInit after DVDPrepareStreamAbsAsync
			*(vu32*)(addr+0xF03D8) = branch(Calc_ProperAddress(addr, dataType, 0xF6E80), Calc_ProperAddress(addr, dataType, 0xF03D8));
			//Call AXQuit after DVDCancelStreamAsync
			*(vu32*)(addr+0xF0494) = branch(Calc_ProperAddress(addr, dataType, 0xF6EB4), Calc_ProperAddress(addr, dataType, 0xF0494));
			print_gecko("Patched:[Powerpuff Girls NTSC-U]\r\n");
			patched=1;
		}
		else if(*(vu32*)(addr+0xF0EC0) == 0x4E800020 && *(vu32*)(addr+0xF0F7C) == 0x4E800020)
		{
			//Call AXInit after DVDPrepareStreamAbsAsync
			*(vu32*)(addr+0xF0EC0) = branch(Calc_ProperAddress(addr, dataType, 0xF8340), Calc_ProperAddress(addr, dataType, 0xF0EC0));
			//Call AXQuit after DVDCancelStreamAsync
			*(vu32*)(addr+0xF0F7C) = branch(Calc_ProperAddress(addr, dataType, 0xF83B0), Calc_ProperAddress(addr, dataType, 0xF0F7C));
			print_gecko("Patched:[Powerpuff Girls PAL]\r\n");
			patched=1;
		}
	}
	return patched;
}

const u32 SPEC2_MakeStatusA[8] = {
	0x80050000,0x540084BE,0xB0040000,0x80050000,
	0x5400C23E,0x7C000774,0x98040002,0x80050000};

// Hook into PAD for IGR
int Patch_IGR(void *data, u32 length, int dataType) {
	int i;
	
	for( i=0; i < length; i+=4 )
	{
		if( *(vu32*)(data+i) != 0x80050000 &&  *(vu32*)(data+i+4) != 0x540084BE)
			continue;
		
		if(!memcmp((void*)(data+i),SPEC2_MakeStatusA,sizeof(SPEC2_MakeStatusA)))
		{
			u32 iEnd = 0;
			while(*(vu32*)(data + i + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
			u32 properAddress = Calc_ProperAddress(data, dataType, (u32)(data + i + iEnd) -(u32)(data));
			print_gecko("Found:[SPEC2_MakeStatusA] @ %08X (%08X)\n", properAddress, i + iEnd);
			u32 igrJump = (devices[DEVICE_CUR] == &__device_wkf) ? IGR_CHECK_WKF : IGR_CHECK;
			if(devices[DEVICE_CUR] == &__device_dvd) igrJump = IGR_CHECK_DVD;
			*(vu32*)(data+i+iEnd) = branch(igrJump, properAddress);
			return 1;
		}
	}
	return 0;
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
		if(*(vu32*)(data+i+0) == 0x3C808000 &&
			(*(vu32*)(data+i+4) == 0x38000004 || *(vu32*)(data+i+4) == 0x808400E4) &&
			(*(vu32*)(data+i+8) == 0x38000004 || *(vu32*)(data+i+8) == 0x808400E4)) 
		{
			
			// Find the end of the function and replace the blr with a relative branch to CHEATS_ENGINE_START
			int j = 12;
			while( *(vu32*)(data+i+j) != 0x4E800020 )
				j+=4;
			// As the data we're looking at will not be in this exact memory location until it's placed there by our ARAM relocation stub,
			// we'll need to work out where it will end up when it does get placed in memory to write the relative branch.
			u32 properAddress = Calc_ProperAddress(data, type, i+j);
			if(properAddress) {
				print_gecko("Found:[Hook:OSSleepThread] @ %08X\n", properAddress );
				u32 newval = (u32)(CHEATS_ENGINE_START - properAddress);
				newval&= 0x03FFFFFC;
				newval|= 0x48000000;
				*(vu32*)(data+i+j) = newval;
				break;
			}
		}
	}
	// try GX DrawDone and its variants
	for( i=0; i < length; i+=4 )
	{
		if(( *(vu32*)(data+i+0) == 0x3CC0CC01 &&
			*(vu32*)(data+i+4) == 0x3CA04500 &&
			*(vu32*)(data+i+12) == 0x38050002 &&
			*(vu32*)(data+i+16) == 0x90068000 ) ||
			( *(vu32*)(data+i+0) == 0x3CA0CC01 &&
			*(vu32*)(data+i+4) == 0x3C804500 &&
			*(vu32*)(data+i+12) == 0x38040002 &&
			*(vu32*)(data+i+16) == 0x90058000 ) ||
			( *(vu32*)(data+i+0) == 0x3FE04500 &&
			*(vu32*)(data+i+4) == 0x3BFF0002 &&
			*(vu32*)(data+i+20) == 0x3C60CC01  &&
			*(vu32*)(data+i+24) == 0x93E38000  ) ||
			( *(vu32*)(data+i+0) == 0x3C804500  &&
			*(vu32*)(data+i+12) == 0x3CA0CC01  &&
			*(vu32*)(data+i+28) == 0x38040002  &&
			*(vu32*)(data+i+40) == 0x90058000  ) )
		{
			
			// Find the end of the function and replace the blr with a relative branch to CHEATS_ENGINE_START
			int j = 16;
			while( *(vu32*)(data+i+j) != 0x4E800020 )
				j+=4;
			// As the data we're looking at will not be in this exact memory location until it's placed there by our ARAM relocation stub,
			// we'll need to work out where it will end up when it does get placed in memory to write the relative branch.
			u32 properAddress = Calc_ProperAddress(data, type, i+j);
			if(properAddress) {
				print_gecko("Found:[Hook:GXDrawDone] @ %08X\n", properAddress );
				u32 newval = (u32)(CHEATS_ENGINE_START - properAddress);
				newval&= 0x03FFFFFC;
				newval|= 0x48000000;
				*(vu32*)(data+i+j) = newval;
				break;
			}
		}
	}
	return 0;
}


