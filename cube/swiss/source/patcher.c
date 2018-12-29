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

// Read
FuncPattern ReadDebug = {55, 23, 18, 3, 2, 4, 0, 0, "Read (Debug)", 0};
FuncPattern ReadCommon = {67, 30, 18, 5, 2, 3, 0, 0, "Read (Common)", 0};
FuncPattern ReadUncommon = {65, 29, 17, 5, 2, 3, 0, 0, "Read (Uncommon)", 0};

// OSExceptionInit
FuncPattern OSExceptionInitSig = {159, 39, 14, 14, 20, 7, 0, 0, "OSExceptionInit", 0};
FuncPattern OSExceptionInitSigDBG = {163, 61, 6, 18, 14, 14, 0, 0, "OSExceptionInitDBG", 0};

// __DVDInterruptHandler
u16 _dvdinterrupthandler_part[3] = {
	0x6000, 0x002A, 0x0054
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

void make_pattern( u32 *Data, u32 Length, FuncPattern *FunctionPattern )
{
	u32 i;

	memset( FunctionPattern, 0, sizeof(FuncPattern) );

	for( i = 0; i < Length / sizeof(u32); i++ )
	{
		u32 word = Data[i];
		
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
	
int find_pattern( u32 *data, u32 length, FuncPattern *functionPattern )
{
	u32 i;
	FuncPattern FP;

	memset( &FP, 0, sizeof(FP) );

	for( i = 0; i < length / sizeof(u32); i++ )
	{
		u32 word =  data[i];
		
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
		print_gecko("Length: %d\r\n", FP.Length );
		print_gecko("Loads : %d\r\n", FP.Loads );
		print_gecko("Stores: %d\r\n", FP.Stores );
		print_gecko("FCalls: %d\r\n", FP.FCalls );
		print_gecko("Branch : %d\r\n", FP.Branch);
		print_gecko("Moves: %d\r\n", FP.Moves);
		return 0;
	}
	
	return memcmp( &FP, functionPattern, sizeof(u32) * 6 ) == 0;
}

u32 branch(u32 *dst, u32 *src)
{
	u32 newval = (u32)dst - (u32)src;
	newval &= 0x03FFFFFC;
	newval |= 0x48000000;
	return newval;
}

u32 branchAndLink(u32 *dst, u32 *src)
{
	u32 newval = (u32)dst - (u32)src;
	newval &= 0x03FFFFFC;
	newval |= 0x48000001;
	return newval;
}

u32 branchResolve(u32 *data, int dataType, u32 offsetFoundAt)
{
	u32 *address;
	u32 word = data[offsetFoundAt];
	
	if ((word & 0xFC000000) != 0x48000000)
		return 0;
	
	if ((address = Calc_ProperAddress(data, dataType, offsetFoundAt * sizeof(u32))) == NULL)
		return 0;
	
	address = ((s32)((word & 0x03FFFFFC) << 6) >> 8) + (word & 0x2 ? NULL : address);
	
	if ((address = Calc_Address(data, dataType, (u32)address)) == NULL)
		return 0;
	
	return address - data;
}

bool findx_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
{
	offsetFoundAt = branchResolve(data, dataType, offsetFoundAt);
	
	if (offsetFoundAt && find_pattern(data + offsetFoundAt, length, functionPattern)) {
		functionPattern->offsetFoundAt = offsetFoundAt;
		return true;
	}
	
	return false;
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
				//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
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
					//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
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
					//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
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
				//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
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
				//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
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
					//void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(lisOffset)-(u32)(dst));
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
				void *properAddress = Calc_ProperAddress(dst, dataType, (u32)(dst + i)-(u32)(dst));
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

// HACK: PSO 0x80001800 move to 0x817F1800
u32 PatchPatchBuffer(char *dst)
{
	int i;
	int patched = 0;
	u32 LISReg = -1;
	u32 LISOff = -1;

	for (i = 0;; i += 4)
	{
		u32 op = *(vu32*)(dst + i);

		if ((op & 0xFC1FFFFF) == 0x3C008000)	// lis rX, 0x8000
		{
			LISReg = (op & 0x3E00000) >> 21;
			LISOff = (u32)dst + i;
		}

		if ((op & 0xFC000000) == 0x38000000)	// li rX, x
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dst = (op >> 21) & 0x1F;
			if ((src != LISReg) && (dst == LISReg))
			{
				LISReg = -1;
				LISOff = (u32)dst + i;
			}
		}

		if ((op & 0xFC00FFFF) == 0x38001800) // addi rX, rY, 0x1800 (patch buffer)
		{
			u32 src = (op >> 16) & 0x1F;
			if (src == LISReg)
			{
				*(vu32*)(LISOff) = (LISReg << 21) | 0x3C00817F;
				print_gecko("Patch:[%08X] %08X: lis r%u, 0x817F\r\n", (u32)LISOff, *(u32*)LISOff, LISReg );
				LISReg = -1;
				patched++;
			}
			u32 dst = (op >> 21) & 0x1F;
			if (dst == LISReg)
				LISReg = -1;
		}
		if (op == 0x4E800020)	// blr
			break;
	}
	return patched;
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
		make_pattern( (u32*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &OSExceptionInitSig))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 472)-(u32)(addr));
			print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 472) = branchAndLink(PATCHED_MEMCPY_WKF, properAddress);
			patched |= 0x10;
		}
		// Debug version of the above
		if(compare_pattern(&fp, &OSExceptionInitSigDBG))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 512)-(u32)(addr));
			print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 512) = branchAndLink(PATCHED_MEMCPY_DBG_WKF, properAddress);
			patched |= 0x10;
		}		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
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
		make_pattern( (u32*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &OSExceptionInitSig))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 472)-(u32)(addr));
			print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 472) = branchAndLink(PATCHED_MEMCPY_USB, properAddress);
			patched |= 0x10;
		}
		// Debug version of the above
		if(compare_pattern(&fp, &OSExceptionInitSigDBG))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 512)-(u32)(addr));
			print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
			*(vu32*)(addr + i + 512) = branchAndLink(PATCHED_MEMCPY_DBG_USB, properAddress);
			patched |= 0x10;
		}		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(PERFORM_READ_USBGECKO, properAddress);
			*(vu32*)(addr + i + 0xB8) = 0x60000000;
			*(vu32*)(addr + i + 0xEC) = 0x60000000;
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(PERFORM_READ_USBGECKO, properAddress);
			patched |= 0x100;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
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
		make_pattern( (u32*)(addr+i), length, &fp );
		
		if(compare_pattern(&fp, &ReadCommon)) {
			// Overwrite the DI start to go to our code that will manipulate offsets for frag'd files.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x84)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadCommon.Name, properAddress - 0x84);
			*(vu32*)(addr + i + 0x84) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
			return 1;
		}
		if(compare_pattern(&fp, &ReadDebug)) {	// As above, for debug read now.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x88)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadDebug.Name, properAddress - 0x88);
			*(vu32*)(addr + i + 0x88) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
			return 1;
		}
		if(compare_pattern(&fp, &ReadUncommon)) {	// Same, for the less common read type.
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x7C)-(u32)(addr));
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
	FuncPattern DSPHandler = {264, 103, 23, 34, 32, 9, 0, 0, "__DSPHandler", 0};
	while(addr_start<addr_end) {
		// Patch the memcpy call in OSExceptionInit to copy our code to 0x80000500 instead of anything else.
		if(*(vu32*)(addr_start) == 0x7C0802A6)
		{
			if( find_pattern( (u32*)(addr_start), length, &OSExceptionInitSig ) )
			{
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 472)-(u32)(addr));
				print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 472) = branchAndLink(PATCHED_MEMCPY, properAddress);
				patched |= 0x100;
			}
			// Debug version of the above
			else if( find_pattern( (u32*)(addr_start), length, &OSExceptionInitSigDBG ) )
			{
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 512)-(u32)(addr));
				print_gecko("Found:[OSExceptionInitDBG] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 512) = branchAndLink(PATCHED_MEMCPY_DBG, properAddress);
				patched |= 0x100;
			}
			// Audio Streaming Hook (only if required)
			else if(!swissSettings.muteAudioStreaming && find_pattern( (u32*)(addr_start), length, &DSPHandler ) )
			{	
				if(strncmp((const char*)0x80000000, "PZL", 3)) {	// ZeldaCE uses a special case for MM
					void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+0xF8)-(u32)(addr));
					print_gecko("Found:[__DSPHandler] @ %08X\r\n", properAddress);
					*(vu32*)(addr_start+0xF8) = branchAndLink(DSP_HANDLER_HOOK, properAddress);
				}
			}
			// Read variations
			FuncPattern fp;
			make_pattern( (u32*)(addr_start), length, &fp );
			if(compare_pattern(&fp, &ReadCommon) 			// Common Read function
				|| compare_pattern(&fp, &ReadDebug)			// Debug Read function
				|| compare_pattern(&fp, &ReadUncommon)) 	// Uncommon Read function
			{
				u32 iEnd = 4;
				while(*(vu32*)(addr_start + iEnd) != 0x4E800020) iEnd += 4;	// branch relative from the end
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + iEnd)-(u32)(addr));
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
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+iEnd)-(u32)(addr));
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

u8 video_timing[] = {
	0x06,0x00,0x00,0xF0,
	0x00,0x1A,0x00,0x1B,
	0x00,0x01,0x00,0x00,
	0x0C,0x0D,0x0C,0x0D,
	0x02,0x08,0x02,0x07,
	0x02,0x08,0x02,0x07,
	0x02,0x0D,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C,
	0x06,0x00,0x00,0xF0,
	0x00,0x1C,0x00,0x1C,
	0x00,0x00,0x00,0x00,
	0x0C,0x0C,0x0C,0x0C,
	0x02,0x08,0x02,0x08,
	0x02,0x08,0x02,0x08,
	0x02,0x0E,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C,
	0x05,0x00,0x01,0x20,
	0x00,0x21,0x00,0x22,
	0x00,0x01,0x00,0x00,
	0x0D,0x0C,0x0B,0x0A,
	0x02,0x6B,0x02,0x6A,
	0x02,0x69,0x02,0x6C,
	0x02,0x71,0x01,0xB0,
	0x40,0x4B,0x6A,0xAC,
	0x01,0x7C,0x85,0x00,
	0x01,0xA4,
	0x05,0x00,0x01,0x20,
	0x00,0x21,0x00,0x21,
	0x00,0x00,0x00,0x00,
	0x0D,0x0B,0x0D,0x0B,
	0x02,0x6B,0x02,0x6D,
	0x02,0x6B,0x02,0x6D,
	0x02,0x70,0x01,0xB0,
	0x40,0x4B,0x6A,0xAC,
	0x01,0x7C,0x85,0x00,
	0x01,0xA4,
	0x06,0x00,0x00,0xF0,
	0x00,0x1A,0x00,0x1B,
	0x00,0x01,0x00,0x00,
	0x10,0x0F,0x0E,0x0D,
	0x02,0x06,0x02,0x05,
	0x02,0x04,0x02,0x07,
	0x02,0x0D,0x01,0xAD,
	0x40,0x4E,0x70,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C,
	0x06,0x00,0x00,0xF0,
	0x00,0x1C,0x00,0x1C,
	0x00,0x00,0x00,0x00,
	0x10,0x0E,0x10,0x0E,
	0x02,0x06,0x02,0x08,
	0x02,0x06,0x02,0x08,
	0x02,0x0E,0x01,0xAD,
	0x40,0x4E,0x70,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C,
	0x0C,0x00,0x01,0xE0,
	0x00,0x36,0x00,0x36,
	0x00,0x00,0x00,0x00,
	0x18,0x18,0x18,0x18,
	0x04,0x0E,0x04,0x0E,
	0x04,0x0E,0x04,0x0E,
	0x04,0x1A,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C
};

u8 video_timing_1080i60[] = {
	0x07,0x00,0x02,0x1C,
	0x00,0x17,0x00,0x18,
	0x00,0x01,0x00,0x00,
	0x28,0x29,0x28,0x29,
	0x04,0x60,0x04,0x5F,
	0x04,0x60,0x04,0x5F,
	0x04,0x65,0x01,0x90,
	0x10,0x15,0x37,0x73,
	0x01,0x63,0x4B,0x00,
	0x01,0x8A
};

u8 video_timing_1080i50[] = {
	0x07,0x00,0x02,0x1C,
	0x00,0x17,0x00,0x18,
	0x00,0x01,0x00,0x00,
	0x28,0x29,0x28,0x29,
	0x04,0x60,0x04,0x5F,
	0x04,0x60,0x04,0x5F,
	0x04,0x65,0x01,0xE0,
	0x10,0x15,0x37,0x73,
	0x01,0x13,0x4B,0x00,
	0x01,0x3A
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

static void *patch_locations[PATCHES_MAX];

void checkPatchAddr() {
	if(top_addr < 0x80001800) {
		print_gecko("Too many patches applied, top_addr has gone below the reserved area\r\n");
		// Display something on screen
		while(1);
	}
}

// Returns where the ASM patch has been copied to
void *installPatch(int patchId) {
	u32 patchSize = 0;
	void* patchLocation = 0;
	switch(patchId) {
		case GX_COPYDISPHOOK:
			patchSize = GXCopyDispHook_length; patchLocation = GXCopyDispHook; break;
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
		case VI_CONFIGURE1080I50:
			patchSize = VIConfigure1080i50_length; patchLocation = VIConfigure1080i50; break;
		case VI_CONFIGURE1080I60:
			patchSize = VIConfigure1080i60_length; patchLocation = VIConfigure1080i60; break;
		case VI_CONFIGUREHOOK1:
			patchSize = VIConfigureHook1_length; patchLocation = VIConfigureHook1; break;
		case VI_CONFIGUREHOOK2:
			patchSize = VIConfigureHook2_length; patchLocation = VIConfigureHook2; break;
		case VI_CONFIGUREPANHOOK:
			patchSize = VIConfigurePanHook_length; patchLocation = VIConfigurePanHook; break;
		case VI_RETRACEHANDLERHOOK:
			patchSize = VIRetraceHandlerHook_length; patchLocation = VIRetraceHandlerHook; break;
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
	return (void*)top_addr;
}

// See patchIds enum in patcher.h
void *getPatchAddr(int patchId) {
	if(patchId > PATCHES_MAX || patchId < 0) {
		print_gecko("Invalid Patch location requested\r\n");
		return NULL;
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

void Patch_VideoMode(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	FuncPattern __VIRetraceHandlerSigs[8] = {
		{ 119, 41,  7, 10, 13,  6, NULL, 0, "__VIRetraceHandlerD A" },
		{ 120, 41,  7, 11, 13,  6, NULL, 0, "__VIRetraceHandlerD B" },
		{ 131, 39,  7, 10, 15, 13, NULL, 0, "__VIRetraceHandler A" },
		{ 136, 42,  8, 11, 15, 13, NULL, 0, "__VIRetraceHandler B" },
		{ 137, 42,  9, 11, 15, 13, NULL, 0, "__VIRetraceHandler C" },
		{ 139, 43, 10, 11, 15, 13, NULL, 0, "__VIRetraceHandler D" },
		{ 146, 46, 13, 10, 15, 15, NULL, 0, "__VIRetraceHandler E" },	// SN Systems ProDG
		{ 156, 50, 10, 15, 16, 13, NULL, 0, "__VIRetraceHandler F" }
	};
	FuncPattern getTimingSigs[8] = {
		{  29,  12,  2,  0,  7,  2,           NULL,                     0, "getTimingD A" },
		{  39,  16,  2,  0, 12,  2, getTimingPatch, getTimingPatch_length, "getTimingD B" },
		{  11,   4,  0,  0,  0,  3,           NULL,                     0, "getTiming A" },
		{  11,   4,  0,  0,  0,  3,           NULL,                     0, "getTiming B" },
		{  11,   4,  0,  0,  0,  3, getTimingPatch, getTimingPatch_length, "getTiming C" },
		{  11,   5,  0,  0,  0,  2,           NULL,                     0, "getTiming D" },
		{ 558, 112, 44, 14, 53, 48,           NULL,                     0, "getTiming E" },	// SN Systems ProDG
		{  11,   5,  0,  0,  0,  2,           NULL,                     0, "getTiming F" }
	};
	FuncPattern AdjustPositionSig = 
		{ 134, 9, 1, 0, 17, 47, NULL, 0, "AdjustPositionD" };
	FuncPattern VIWaitForRetraceSig = 
		{ 20, 7, 4, 3, 1, 4, NULL, 0, "VIWaitForRetrace" };
	FuncPattern setFbbRegsSigs[3] = {
		{ 166, 62, 22, 2,  8, 24, NULL, 0, "setFbbRegsD A" },
		{ 180, 54, 34, 0, 10, 16, NULL, 0, "setFbbRegs A" },
		{ 176, 51, 34, 0, 10, 16, NULL, 0, "setFbbRegs B" }				// SN Systems ProDG
	};
	FuncPattern setVerticalRegsSigs[4] = {
		{ 117, 17, 11, 0, 4, 23, NULL, 0, "setVerticalRegsD A" },
		{ 103, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs A" },
		{ 103, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs B" },
		{ 113, 19, 13, 0, 4, 25, NULL, 0, "setVerticalRegs C" }			// SN Systems ProDG
	};
	FuncPattern VIConfigureSigs[10] = {
		{ 279,  74, 15, 20, 21, 20, NULL, 0, "VIConfigureD A" },
		{ 313,  86, 15, 21, 27, 20, NULL, 0, "VIConfigureD B" },
		{ 427,  90, 43,  6, 32, 60, NULL, 0, "VIConfigure A" },
		{ 419,  87, 41,  6, 31, 60, NULL, 0, "VIConfigure B" },
		{ 463, 100, 43, 13, 34, 61, NULL, 0, "VIConfigure C" },
		{ 461,  99, 43, 12, 34, 61, NULL, 0, "VIConfigure D" },
		{ 486, 105, 44, 12, 38, 63, NULL, 0, "VIConfigure E" },
		{ 521, 111, 44, 13, 53, 64, NULL, 0, "VIConfigure F" },
		{ 558, 112, 44, 14, 53, 48, NULL, 0, "VIConfigure G" },			// SN Systems ProDG
		{ 513, 110, 44, 13, 49, 63, NULL, 0, "VIConfigure H" }
	};
	FuncPattern VIConfigurePanSig = 
		{ 228, 40, 11, 4, 25, 35, NULL, 0, "VIConfigurePan" };
	FuncPattern getCurrentFieldEvenOddSigs[5] = {
		{ 32,  7, 2, 3, 4, 5, NULL, 0, "getCurrentFieldEvenOddD A" },
		{ 14,  5, 2, 1, 2, 3, NULL, 0, "getCurrentFieldEvenOddD B" },
		{ 46, 14, 2, 2, 4, 8, NULL, 0, "getCurrentFieldEvenOdd A" },
		{ 25,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd B" },
		{ 25,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd C" }		// SN Systems ProDG
	};
	FuncPattern VIGetNextFieldSigs[4] = {
		{ 19,  4, 2, 3, 0, 3, NULL, 0, "VIGetNextFieldD A" },
		{ 60, 16, 4, 4, 4, 9, NULL, 0, "VIGetNextField A" },
		{ 41, 10, 3, 2, 2, 8, NULL, 0, "VIGetNextField B" },
		{ 38, 13, 4, 3, 2, 6, NULL, 0, "VIGetNextField C" }
	};
	FuncPattern __GXInitGXSigs[8] = {
		{ 1129, 567, 66, 133, 46, 46, NULL, 0, "__GXInitGXD A" },
		{  543, 319, 33, 109, 18,  5, NULL, 0, "__GXInitGXD B" },
		{  975, 454, 81, 119, 43, 36, NULL, 0, "__GXInitGX A" },
		{  529, 307, 35, 107, 18, 10, NULL, 0, "__GXInitGX B" },
		{  544, 310, 35, 108, 24, 11, NULL, 0, "__GXInitGX C" },
		{  560, 313, 36, 110, 28, 11, NULL, 0, "__GXInitGX D" },
		{  545, 293, 37, 110,  7,  9, NULL, 0, "__GXInitGX E" },		// SN Systems ProDG
		{  589, 333, 34, 119, 28, 11, NULL, 0, "__GXInitGX F" }
	};
	FuncPattern GXAdjustForOverscanSigs[3] = {
		{ 56,  6,  4, 0, 3, 11, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscanD A" },
		{ 71, 17, 15, 0, 3,  5, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan A" },
		{ 80, 17, 15, 0, 3,  7, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan B" }
	};
	FuncPattern GXSetDispCopyYScaleSigs[7] = {
		{ 99, 33, 8, 8, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD A" },
		{ 84, 32, 4, 6, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD B" },
		{ 46, 15, 8, 2, 0, 4, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale A" },
		{ 52, 17, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale B" },
		{ 49, 14, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale C" },
		{ 43,  8, 4, 1, 2, 3, GXSetDispCopyYScalePatch2, GXSetDispCopyYScalePatch2_length, "GXSetDispCopyYScale D" },	// SN Systems ProDG
		{ 50, 16, 4, 1, 5, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale E" }
	};
	FuncPattern GXSetCopyFilterSigs[4] = {
		{ 566, 183, 44, 32, 36, 38, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilterD A" },
		{ 137,  15,  7,  0,  4,  5, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter A" },
		{ 162,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter B" },	// SN Systems ProDG
		{ 129,  25,  7,  0,  4,  0, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter C" }
	};
	FuncPattern GXCopyDispSigs[5] = {
		{ 148, 62,  3, 14, 14, 3, NULL, 0, "GXCopyDispD A" },
		{  91, 34, 14,  0,  3, 1, NULL, 0, "GXCopyDisp A" },
		{  86, 29, 14,  0,  3, 1, NULL, 0, "GXCopyDisp B" },
		{  68, 15, 12,  0,  1, 1, NULL, 0, "GXCopyDisp C" },			// SN Systems ProDG
		{  89, 35, 14,  0,  3, 0, NULL, 0, "GXCopyDisp D" }
	};
	FuncPattern GXSetBlendModeSigs[4] = {
		{ 153, 66, 10, 7, 9, 17, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendModeD A" },
		{  64, 20,  8, 0, 2,  6, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendMode A" },
		{  20,  6,  2, 0, 0,  2, GXSetBlendModePatch2, GXSetBlendModePatch2_length, "GXSetBlendMode B" },
		{  35,  2,  2, 0, 0,  6, GXSetBlendModePatch3, GXSetBlendModePatch3_length, "GXSetBlendMode C" }	// SN Systems ProDG
	};
	FuncPattern __GXSetViewportSig = 
		{ 35, 15, 7, 0, 0, 0, GXSetViewportPatch, GXSetViewportPatch_length, "__GXSetViewport" };
	FuncPattern GXSetViewportJitterSigs[5] = {
		{ 192, 76, 22, 4, 15, 22, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitterD A" },
		{  70, 20, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter A" },
		{  64, 14, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter B" },
		{  30,  6, 10, 1,  0,  4,                     NULL,                               0, "GXSetViewportJitter C" },	// SN Systems ProDG
		{  21,  6,  8, 1,  0,  2,                     NULL,                               0, "GXSetViewportJitter D" }
	};
	FuncPattern GXSetViewportSigs[5] = {
		{ 20, 9, 8, 1, 0, 2, NULL, 0, "GXSetViewportD A" },
		{  8, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport A" },
		{  8, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport B" },
		{ 14, 7, 6, 0, 1, 0, NULL, 0, "GXSetViewport C" },				// SN Systems ProDG
		{ 17, 5, 8, 1, 0, 2, NULL, 0, "GXSetViewport D" }
	};
	
	switch (swissSettings.gameVMode) {
		case 4: case 9:
			if (!swissSettings.forceHScale)
				swissSettings.forceHScale = 1;
		case 3: case 8:
			swissSettings.forceVOffset &= ~1;
		case 2: case 7: case 5: case 10:
			if (!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
	}
	
	if (swissSettings.gameVMode == 3 || swissSettings.gameVMode == 8)
		memcpy((u8 *)VAR_VFILTER, vertical_reduction[swissSettings.forceVFilter], 7);
	else
		memcpy((u8 *)VAR_VFILTER, vertical_filters[swissSettings.forceVFilter], 7);
	
	*(u8 *)VAR_VFILTER_ON = !!swissSettings.forceVFilter;
	
	switch (swissSettings.forceHScale) {
		default:
			*(u16 *)VAR_SAR_WIDTH = 0;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 1:
			*(u16 *)VAR_SAR_WIDTH = 1;
			*(u8 *)VAR_SAR_HEIGHT = 1;
			break;
		case 2:
			*(u16 *)VAR_SAR_WIDTH = 11;
			*(u8 *)VAR_SAR_HEIGHT = 10;
			break;
		case 3:
			*(u16 *)VAR_SAR_WIDTH = 9;
			*(u8 *)VAR_SAR_HEIGHT = 8;
			break;
		case 4:
			*(u16 *)VAR_SAR_WIDTH = 640;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 5:
			*(u16 *)VAR_SAR_WIDTH = 704;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 6:
			*(u16 *)VAR_SAR_WIDTH = 720;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
	}
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!GXAdjustForOverscanSigs[0].offsetFoundAt && !memcmp(data + i, GXAdjustForOverscanD, GXAdjustForOverscanD_length))
			GXAdjustForOverscanSigs[0].offsetFoundAt = i;
		if (!GXAdjustForOverscanSigs[1].offsetFoundAt && !memcmp(data + i, GXAdjustForOverscan1, GXAdjustForOverscan1_length))
			GXAdjustForOverscanSigs[1].offsetFoundAt = i;
		if (!GXAdjustForOverscanSigs[2].offsetFoundAt && !memcmp(data + i, GXAdjustForOverscan2, GXAdjustForOverscan2_length))
			GXAdjustForOverscanSigs[2].offsetFoundAt = i;
		
		if (data[i] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6)
			continue;
		
		FuncPattern fp;
		make_pattern(data + i, length, &fp);
		
		for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++) {
			if (!__VIRetraceHandlerSigs[j].offsetFoundAt && compare_pattern(&fp, &__VIRetraceHandlerSigs[j])) {
				__VIRetraceHandlerSigs[j].offsetFoundAt = i;
				
				switch (j) {
					case 0: findx_pattern(data, dataType, i - 47, length, &getCurrentFieldEvenOddSigs[0]);       break;
					case 1: findx_pattern(data, dataType, i - 49, length, &getCurrentFieldEvenOddSigs[1]);       break;
					case 2: findx_pattern(data, dataType, i + 60, length, &getCurrentFieldEvenOddSigs[2]);       break;
					case 3: getCurrentFieldEvenOddSigs[3].offsetFoundAt = branchResolve(data, dataType, i + 60); break;
					case 4:
					case 5: getCurrentFieldEvenOddSigs[3].offsetFoundAt = branchResolve(data, dataType, i + 63); break;
					case 6: getCurrentFieldEvenOddSigs[4].offsetFoundAt = branchResolve(data, dataType, i + 68); break;
					case 7: getCurrentFieldEvenOddSigs[3].offsetFoundAt = branchResolve(data, dataType, i + 80); break;
				}
				break;
			}
		}
		
		if (!VIWaitForRetraceSig.offsetFoundAt && compare_pattern(&fp, &VIWaitForRetraceSig))
			VIWaitForRetraceSig.offsetFoundAt = i;
		
		for (j = 0; j < sizeof(VIConfigureSigs) / sizeof(FuncPattern); j++) {
			if (!VIConfigureSigs[j].offsetFoundAt && compare_pattern(&fp, &VIConfigureSigs[j])) {
				VIConfigureSigs[j].offsetFoundAt = i;
				
				switch (j) {
					case 0:
						findx_pattern(data, dataType, i + 131, length, &getTimingSigs[0]);
						findx_pattern(data, dataType, i + 135, length, &AdjustPositionSig);
						findx_pattern(data, dataType, i + 261, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 272, length, &setVerticalRegsSigs[0]);
						break;
					case 1:
						findx_pattern(data, dataType, i + 139, length, &getTimingSigs[1]);
						findx_pattern(data, dataType, i + 143, length, &AdjustPositionSig);
						findx_pattern(data, dataType, i + 295, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 306, length, &setVerticalRegsSigs[0]);
						break;
					case 2:
						findx_pattern(data, dataType, i +  77, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 409, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 420, length, &setVerticalRegsSigs[1]);
						break;
					case 3:
						findx_pattern(data, dataType, i +  69, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 401, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 412, length, &setVerticalRegsSigs[1]);
						break;
					case 4:
						findx_pattern(data, dataType, i + 104, length, &getTimingSigs[3]);
						findx_pattern(data, dataType, i + 445, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 456, length, &setVerticalRegsSigs[1]);
						break;
					case 5:
						findx_pattern(data, dataType, i + 107, length, &getTimingSigs[3]);
						findx_pattern(data, dataType, i + 443, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 454, length, &setVerticalRegsSigs[1]);
						break;
					case 6:
						findx_pattern(data, dataType, i + 123, length, &getTimingSigs[4]);
						findx_pattern(data, dataType, i + 468, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 479, length, &setVerticalRegsSigs[1]);
						break;
					case 7:
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[5]);
						findx_pattern(data, dataType, i + 503, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 514, length, &setVerticalRegsSigs[2]);
						break;
					case 8:
						getTimingSigs[6].offsetFoundAt = i;
						findx_pattern(data, dataType, i + 537, length, &setFbbRegsSigs[2]);
						findx_pattern(data, dataType, i + 550, length, &setVerticalRegsSigs[3]);
						break;
					case 9:
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[7]);
						findx_pattern(data, dataType, i + 495, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 506, length, &setVerticalRegsSigs[2]);
						break;
				}
				break;
			}
			if (VIConfigureSigs[j].offsetFoundAt && i == VIConfigureSigs[j].offsetFoundAt + VIConfigureSigs[j].Length + 1) {
				if (!VIConfigurePanSig.offsetFoundAt && compare_pattern(&fp, &VIConfigurePanSig))
					VIConfigurePanSig.offsetFoundAt = i;
				break;
			}
		}
		
		for (j = 0; j < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern); j++) {
			if (getCurrentFieldEvenOddSigs[j].offsetFoundAt && i == getCurrentFieldEvenOddSigs[j].offsetFoundAt + getCurrentFieldEvenOddSigs[j].Length + 1) {
				for (k = 0; k < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern); k++) {
					if (!VIGetNextFieldSigs[k].offsetFoundAt && compare_pattern(&fp, &VIGetNextFieldSigs[k])) {
						VIGetNextFieldSigs[k].offsetFoundAt = i;
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern); j++) {
			if (!__GXInitGXSigs[j].offsetFoundAt && compare_pattern(&fp, &__GXInitGXSigs[j])) {
				__GXInitGXSigs[j].offsetFoundAt = i;
				
				switch (j) {
					case 0:
						findx_pattern(data, dataType, i + 1088, length, &GXSetDispCopyYScaleSigs[0]);
						
						if (findx_pattern(data, dataType, i + 1095, length, &GXSetCopyFilterSigs[0]))
							GXCopyDispSigs[0].offsetFoundAt = GXSetCopyFilterSigs[0].offsetFoundAt + 601;
						
						findx_pattern(data, dataType, i + 1033, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i +  727, length, &GXSetViewportSigs[0]);
						break;
					case 1:
						findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[1]);
						
						if (findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[0]))
							GXCopyDispSigs[0].offsetFoundAt = GXSetCopyFilterSigs[0].offsetFoundAt + 601;
						
						findx_pattern(data, dataType, i + 444, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[0]);
						break;
					case 2:
						findx_pattern(data, dataType, i + 933, length, &GXSetDispCopyYScaleSigs[2]);
						
						if (findx_pattern(data, dataType, i + 941, length, &GXSetCopyFilterSigs[1]))
							GXCopyDispSigs[1].offsetFoundAt = GXSetCopyFilterSigs[1].offsetFoundAt + 145;
						
						findx_pattern(data, dataType, i + 881, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 553, length, &GXSetViewportSigs[1]);
						break;
					case 3:
						findx_pattern(data, dataType, i + 484, length, &GXSetDispCopyYScaleSigs[2]);
						
						if (findx_pattern(data, dataType, i + 491, length, &GXSetCopyFilterSigs[1]))
							GXCopyDispSigs[1].offsetFoundAt = GXSetCopyFilterSigs[1].offsetFoundAt + 145;
						
						findx_pattern(data, dataType, i + 431, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 186, length, &GXSetViewportSigs[1]);
						break;
					case 4:
						if (data[i + __GXInitGXSigs[j].Length - 1] == 0x7C0803A6)
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[3]);
						else
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[2]);
						
						if (findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[1]))
							GXCopyDispSigs[1].offsetFoundAt = GXSetCopyFilterSigs[1].offsetFoundAt + 145;
						
						findx_pattern(data, dataType, i + 446, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[1]);
						break;
					case 5:
						findx_pattern(data, dataType, i + 514, length, &GXSetDispCopyYScaleSigs[4]);
						
						if (findx_pattern(data, dataType, i + 521, length, &GXSetCopyFilterSigs[1]))
							GXCopyDispSigs[2].offsetFoundAt = GXSetCopyFilterSigs[1].offsetFoundAt + 145;
						
						findx_pattern(data, dataType, i + 461, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[2]);
						break;
					case 6:
						findx_pattern(data, dataType, i + 492, length, &GXSetDispCopyYScaleSigs[5]);
						
						if (findx_pattern(data, dataType, i + 499, length, &GXSetCopyFilterSigs[2]))
							GXCopyDispSigs[3].offsetFoundAt = GXSetCopyFilterSigs[2].offsetFoundAt + 169;
						
						findx_pattern(data, dataType, i + 433, length, &GXSetBlendModeSigs[3]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[3]);
						break;
					case 7:
						findx_pattern(data, dataType, i + 543, length, &GXSetDispCopyYScaleSigs[6]);
						
						if (findx_pattern(data, dataType, i + 550, length, &GXSetCopyFilterSigs[3]))
							GXCopyDispSigs[4].offsetFoundAt = GXSetCopyFilterSigs[3].offsetFoundAt + 135;
						
						findx_pattern(data, dataType, i + 490, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[4]);
						break;
				}
				break;
			}
		}
		i += fp.Length;
	}
	
	for (k = 0; k < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern); k++)
		if (getCurrentFieldEvenOddSigs[k].offsetFoundAt) break;
	
	for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++)
		if (__VIRetraceHandlerSigs[j].offsetFoundAt) break;
	
	if ((i = getCurrentFieldEvenOddSigs[k].offsetFoundAt)) {
		u32 *getCurrentFieldEvenOdd = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (getCurrentFieldEvenOdd) {
			switch (k) {
				case 0:
					data[i + 24] = 0x4180000C;	// blt		+3
					data[i + 25] = 0x575A083C;	// slwi		r26, r26, 1
					data[i + 26] = 0x7C7A1850;	// sub		r3, r3, r26
					data[i + 27] = 0x7C630E70;	// srawi	r3, r3, 1
					break;
				case 1:
					data[i +  7] = 0x4180000C;	// blt		+3
					data[i +  8] = 0x5400083C;	// slwi		r0, r0, 1
					data[i +  9] = 0x7C601850;	// sub		r3, r3, r0
					data[i + 10] = 0x7C630E70;	// srawi	r3, r3, 1
					break;
				case 2:
					data[i + 39] = 0x4180000C;	// blt		+3
					data[i + 40] = 0x5529083C;	// slwi		r9, r9, 1
					data[i + 41] = 0x7C090050;	// sub		r0, r0, r9
					data[i + 42] = 0x7C030E70;	// srawi	r3, r0, 1
					break;
				case 3:
				case 4:
					data[i + 21] = 0x4180000C;	// blt		+3
					data[i + 22] = 0x5400083C;	// slwi		r0, r0, 1
					data[i + 23] = 0x7C601850;	// sub		r3, r3, r0
					data[i + 24] = 0x7C630E70;	// srawi	r3, r3, 1
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", getCurrentFieldEvenOddSigs[k].Name, getCurrentFieldEvenOdd);
		}
		
		if ((i = __VIRetraceHandlerSigs[j].offsetFoundAt)) {
			u32 *__VIRetraceHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *__VIRetraceHandlerHook;
			
			if (__VIRetraceHandler && getCurrentFieldEvenOdd) {
				switch (j) {
					case 0:
						data[i - 46] = 0x2C030000;	// cmpwi	r3, 0
						data[i - 45] = 0x4082009C;	// bne		+39
						break;
					case 1:
						data[i - 48] = 0x2C030000;	// cmpwi	r3, 0
						data[i - 47] = 0x408200A4;	// bne		+41
						break;
					case 2:
						data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 62] = 0x408200B4;	// bne		+45
						break;
					case 3:
						data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 62] = 0x408200C4;	// bne		+49
						break;
					case 4:
						data[i + 64] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 65] = 0x408200BC;	// bne		+47
						break;
					case 5:
						data[i + 64] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 65] = 0x408200C4;	// bne		+49
						break;
					case 6:
						data[i + 69] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 70] = 0x408200C4;	// bne		+49
						break;
					case 7:
						data[i + 81] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 82] = 0x408200C4;	// bne		+49
						break;
				}
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 7) {
					switch (j) {
						case 0: data[i - 50] = 0x38000001; break;
						case 1: data[i - 52] = 0x38000001; break;
						case 2:
						case 3: data[i + 57] = 0x38000001; break;
						case 4:
						case 5: data[i + 60] = 0x38000001; break;
						case 6: data[i + 65] = 0x38000001; break;
						case 7: data[i + 77] = 0x38000001; break;
					}
				}
				if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9) {
					__VIRetraceHandlerHook = getPatchAddr(VI_RETRACEHANDLERHOOK);
					
					switch (j) {
						case 0:
							data[i + 118] = data[i + 117];
							data[i + 117] = data[i + 116];
							data[i + 116] = data[i + 115];
							data[i + 115] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 115);
							break;
						case 1:
							data[i + 119] = data[i + 118];
							data[i + 118] = data[i + 117];
							data[i + 117] = data[i + 116];
							data[i + 116] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 116);
							break;
						case 2:
							data[i + 130] = data[i + 129];
							data[i + 129] = data[i + 128];
							data[i + 128] = data[i + 127];
							data[i + 127] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 127);
							break;
						case 3:
							data[i + 135] = data[i + 134];
							data[i + 134] = data[i + 133];
							data[i + 133] = data[i + 132];
							data[i + 132] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 132);
							break;
						case 4:
							data[i + 136] = data[i + 135];
							data[i + 135] = data[i + 134];
							data[i + 134] = data[i + 133];
							data[i + 133] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 133);
							break;
						case 5:
							data[i + 138] = data[i + 137];
							data[i + 137] = data[i + 136];
							data[i + 136] = data[i + 135];
							data[i + 135] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 135);
							break;
						case 6:
							data[i + 144] = data[i + 143];
							data[i + 143] = data[i + 142];
							data[i + 142] = data[i + 141];
							data[i + 141] = data[i + 140];
							data[i + 140] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 140);
							break;
						case 7:
							data[i + 155] = data[i + 154];
							data[i + 154] = data[i + 153];
							data[i + 153] = data[i + 152];
							data[i + 152] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 152);
							break;
					}
					data[i + __VIRetraceHandlerSigs[j].Length] = branch(__VIRetraceHandlerHook, __VIRetraceHandler + __VIRetraceHandlerSigs[j].Length);
				}
				print_gecko("Found:[%s] @ %08X\n", __VIRetraceHandlerSigs[j].Name, __VIRetraceHandler);
			}
		}
	}
	
	for (j = 0; j < sizeof(getTimingSigs) / sizeof(FuncPattern); j++)
		if (getTimingSigs[j].offsetFoundAt) break;
	
	if ((i = getTimingSigs[j].offsetFoundAt)) {
		u32 *getTiming = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		u32 jumpTableAddr;
		u32 *jumpTable, jump;
		
		u32 timingTableAddr;
		u8 (*timingTable)[38];
		
		if (getTiming) {
			if (j == 6) {
				timingTableAddr = (data[i +   5] << 16) + (s16)data[i +   7];
				jumpTableAddr   = (data[i + 147] << 16) + (s16)data[i + 149];
				k = (u16)data[i + 144] + 1;
			} else if (j >= 2) {
				timingTableAddr = (data[i + 1] << 16) + (s16)data[i + 2];
				jumpTableAddr   = (data[i + 4] << 16) + (s16)data[i + 5];
				k = (u16)data[i] + 1;
			} else {
				timingTableAddr = (data[i + 2] << 16) + (s16)data[i + 3];
				jumpTableAddr   = (data[i + 6] << 16) + (s16)data[i + 7];
				k = (u16)data[i + 4] + 1;
			}
			if (j > 4) timingTableAddr += 68;
			
			timingTable = Calc_Address(data, dataType, timingTableAddr);
			jumpTable   = Calc_Address(data, dataType, jumpTableAddr);
			
			memcpy(timingTable, video_timing, sizeof(video_timing));
			
			if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9)
				memcpy(timingTable + 6, video_timing_1080i60, sizeof(video_timing_1080i60));
			
			if (j == 1 || j >= 4) {
				if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9)
					memcpy(timingTable + 7, video_timing_1080i50, sizeof(video_timing_1080i50));
				else
					memcpy(timingTable + 7, video_timing_576p, sizeof(video_timing_576p));
				
				jump = jumpTable[3];
				jumpTable[3] = jumpTable[6];
				jumpTable[6] = jump;
			} else if (swissSettings.gameVMode >= 6 && swissSettings.gameVMode <= 10) {
				if (swissSettings.gameVMode == 9)
					memcpy(timingTable + 6, video_timing_1080i50, sizeof(video_timing_1080i50));
				else
					memcpy(timingTable + 6, video_timing_576p, sizeof(video_timing_576p));
				
				jump = jumpTable[2];
				jumpTable[2] = jumpTable[6];
				jumpTable[6] = jump;
			}
			if (k > 10) jumpTable[10] = jumpTable[2];
			if (k > 18) jumpTable[18] = jumpTable[6];
			if (k > 22) jumpTable[22] = jumpTable[2];
			
			if (getTimingSigs[j].Patch) {
				memset(jumpTable, 0, k * sizeof(u32));
				memset(data + i, 0, getTimingSigs[j].Length * sizeof(u32));
				memcpy(data + i, getTimingSigs[j].Patch, getTimingSigs[j].PatchLength);
				
				data[i + 1] |= ((u32)getTiming + getTimingSigs[j].PatchLength + 0x8000) >> 16;
				data[i + 2] |= ((u32)getTiming + getTimingSigs[j].PatchLength) & 0xFFFF;
				
				for (k = 6; k < getTimingSigs[j].PatchLength / sizeof(u32); k++)
					data[i + k] += timingTableAddr;
			}
			print_gecko("Found:[%s] @ %08X\n", getTimingSigs[j].Name, getTiming);
		}
	}
	
	if ((i = AdjustPositionSig.offsetFoundAt)) {
		u32 *AdjustPosition = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (AdjustPosition) {
			data[i +  31] = 0x3BC00000 | (swissSettings.forceVOffset & 0xFFFF);
			data[i +  33] = 0x7FC0F378;	// mr		r0, r30
			data[i +  38] = 0x7FC0F378;	// mr		r0, r30
			data[i +  46] = 0x7FC5F378;	// mr		r5, r30
			data[i +  56] = 0x7FC5F378;	// mr		r5, r30
			data[i +  66] = 0x7FC0F378;	// mr		r0, r30
			data[i +  71] = 0x7FC0F378;	// mr		r0, r30
			data[i +  80] = 0x7FC0F378;	// mr		r0, r30
			data[i +  85] = 0x7FC0F378;	// mr		r0, r30
			data[i +  97] = 0x7FC5F378;	// mr		r5, r30
			data[i + 107] = 0x7FC5F378;	// mr		r5, r30
			data[i + 118] = 0x7FC0F378;	// mr		r0, r30
			data[i + 123] = 0x7FC0F378;	// mr		r0, r30
			
			print_gecko("Found:[%s] @ %08X\n", AdjustPositionSig.Name, AdjustPosition);
		}
	}
	
	if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 7) {
		for (k = 0; k < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern); k++)
			if (getCurrentFieldEvenOddSigs[k].offsetFoundAt) break;
		
		if ((i = getCurrentFieldEvenOddSigs[k].offsetFoundAt)) {
			u32 *getCurrentFieldEvenOdd = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if ((i = VIWaitForRetraceSig.offsetFoundAt)) {
				u32 *VIWaitForRetrace = Calc_ProperAddress(data, dataType, i * sizeof(u32));
				
				if (VIWaitForRetrace && getCurrentFieldEvenOdd) {
					data[i +  6] = data[i + 7];
					data[i +  7] = 0x4800000C;	// b		+3
					data[i + 10] = branchAndLink(getCurrentFieldEvenOdd, VIWaitForRetrace + 10);
					data[i + 11] = 0x28030000;	// cmplwi	r3, 0
					
					print_gecko("Found:[%s] @ %08X\n", VIWaitForRetraceSig.Name, VIWaitForRetrace);
				}
			}
		}
	}
	
	if (swissSettings.gameVMode == 3 || swissSettings.gameVMode == 8) {
		for (j = 0; j < sizeof(setFbbRegsSigs) / sizeof(FuncPattern); j++)
			if (setFbbRegsSigs[j].offsetFoundAt) break;
		
		if ((i = setFbbRegsSigs[j].offsetFoundAt)) {
			u32 *setFbbRegs = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (setFbbRegs) {
				switch (j) {
					case 0:
						data[i - 22] = 0x60000000;	// nop
						break;
					case 1:
						data[i + 20] = 0x81040000;	// lwz		r8, 0 (r4)
						data[i + 21] = 0x60000000;	// nop
						data[i + 58] = 0x81060000;	// lwz		r8, 0 (r6)
						data[i + 59] = 0x60000000;	// nop
						break;
					case 2:
						data[i + 18] = 0x81240000;	// lwz		r9, 0 (r4)
						data[i + 19] = 0x60000000;	// nop
						data[i + 56] = 0x81260000;	// lwz		r9, 0 (r6)
						data[i + 57] = 0x60000000;	// nop
						break;
				}
				print_gecko("Found:[%s] @ %08X\n", setFbbRegsSigs[j].Name, setFbbRegs);
			}
		}
	}
	
	for (j = 0; j < sizeof(setVerticalRegsSigs) / sizeof(FuncPattern); j++)
		if (setVerticalRegsSigs[j].offsetFoundAt) break;
	
	if ((i = setVerticalRegsSigs[j].offsetFoundAt)) {
		u32 *setVerticalRegs = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (setVerticalRegs) {
			switch (j) {
				case 0:
					data[i +  4] = 0xA01D006C;	// lhz		r0, 108 (r29)
					data[i +  5] = 0x540007FF;	// clrlwi.	r0, r0, 31
					data[i +  6] = 0x41820010;	// beq		+4
					data[i + 12] = 0x7C6B0734;	// extsh	r11, r3
					break;
				case 1:
					data[i +  1] = data[i + 2];
					data[i +  2] = data[i + 6];
					data[i +  4] = data[i + 5];
					data[i +  5] = data[i + 7];
					data[i +  6] = data[i + 8];
					data[i +  7] = 0xA00B006C;	// lhz		r0, 108 (r11)
					data[i +  8] = 0x540007FF;	// clrlwi.	r0, r0, 31
					data[i +  9] = 0x41820010;	// beq		+4
					data[i + 15] = 0x7C7E0734;	// extsh	r30, r3
					break;
				case 2:
					data[i + 15] = 0x7C7E0734;	// extsh	r30, r3
					break;
				case 3:
					data[i + 14] = 0x7C7F0734;	// extsh	r31, r3
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", setVerticalRegsSigs[j].Name, setVerticalRegs);
		}
	}
	
	for (j = 0; j < sizeof(VIConfigureSigs) / sizeof(FuncPattern); j++)
		if (VIConfigureSigs[j].offsetFoundAt) break;
	
	if ((i = VIConfigureSigs[j].offsetFoundAt)) {
		u32 *VIConfigure = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *VIConfigureHook1, *VIConfigureHook2;
		
		if (VIConfigure) {
			VIConfigureHook2 = getPatchAddr(VI_CONFIGUREHOOK2);
			VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1);
			
			switch (swissSettings.gameVMode) {
				case  1:
				case  2: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE480I);    break;
				case  3: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE240P);    break;
				case  4: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE1080I60); break;
				case  5: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE480P);    break;
				case  6:
				case  7: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE576I);    break;
				case  8: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE288P);    break;
				case  9: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE1080I50); break;
				case 10: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE576P);    break;
			}
			
			switch (j) {
				case 0:
					data[i +  54] = data[i + 6];
					data[i +  53] = data[i + 5];
					data[i +   5] = branchAndLink(VIConfigureHook1, VIConfigure + 5);
					data[i +   6] = 0x7C771B78;	// mr		r23, r3
					data[i +  18] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  35] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  38] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  55] = 0xA01E0000;	// lhz		r0, 0 (r30)
					data[i +  63] = 0xA01E0000;	// lhz		r0, 0 (r30)
					data[i +  64] = 0x541807BE;	// clrlwi	r24, r0, 30
					data[i +  73] = 0xA01E0000;	// lhz		r0, 0 (r30)
					data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 101] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i + 107] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 120] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 122] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 123] = 0x28000001;	// cmplwi	r0, 1
					data[i + 125] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 126] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 128] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 130] = 0xA07E0000;	// lhz		r3, 0 (r30)
					data[i + 218] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 219] = 0x28000002;	// cmplwi	r0, 2
					data[i + 274] = branchAndLink(VIConfigureHook2, VIConfigure + 274);
					break;
				case 1:
					data[i +  60] = data[i + 8];
					data[i +  59] = data[i + 7];
					data[i +   8] = data[i + 6];
					data[i +   7] = data[i + 5];
					data[i +   5] = branchAndLink(VIConfigureHook1, VIConfigure + 5);
					data[i +   6] = 0x7C781B78;	// mr		r24, r3
					data[i +  18] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  24] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  38] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  41] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  44] = 0xA01E0002;	// lhz		r0, 2 (r30)
					data[i +  61] = 0xA01E0000;	// lhz		r0, 0 (r30)
					data[i +  69] = 0xA01E0000;	// lhz		r0, 0 (r30)
					data[i +  90] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  94] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i + 100] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 113] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 118] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 120] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 121] = 0x28000001;	// cmplwi	r0, 1
					data[i + 123] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 124] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 126] = 0xA01E0010;	// lhz		r0, 16 (r30)
					data[i + 249] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 250] = 0x28000002;	// cmplwi	r0, 2
					data[i + 252] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 253] = 0x28000003;	// cmplwi	r0, 3
					data[i + 308] = branchAndLink(VIConfigureHook2, VIConfigure + 308);
					break;
				case 2:
					data[i +   7] = branchAndLink(VIConfigureHook1, VIConfigure + 7);
					data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
					data[i +  19] = 0x548307BE;	// clrlwi	r3, r4, 30
					data[i +  35] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  41] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  55] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  67] = 0x801B0000;	// lwz		r0, 0 (r27)
					data[i +  68] = 0x28000001;	// cmplwi	r0, 1
					data[i +  70] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  71] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  73] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  76] = 0xA07F0000;	// lhz		r3, 0 (r31)
					data[i + 102] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 104] = 0x7CA42B78;	// mr		r4, r5
					data[i + 222] = 0x801B0000;	// lwz		r0, 0 (r27)
					data[i + 224] = 0x28000002;	// cmplwi	r0, 2
					data[i + 422] = branchAndLink(VIConfigureHook2, VIConfigure + 422);
					break;
				case 3:
					data[i +   7] = branchAndLink(VIConfigureHook1, VIConfigure + 7);
					data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
					data[i +  27] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  33] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  46] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i +  57] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  59] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i +  60] = 0x28000001;	// cmplwi	r0, 1
					data[i +  62] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  63] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  68] = 0xA07F0000;	// lhz		r3, 0 (r31)
					data[i +  94] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i +  96] = 0x7CA42B78;	// mr		r4, r5
					data[i + 214] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 216] = 0x28000002;	// cmplwi	r0, 2
					data[i + 414] = branchAndLink(VIConfigureHook2, VIConfigure + 414);
					break;
				case 4:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  10] = 0xA01F0000;	// lhz		r0, 0 (r31)
					data[i +  45] = 0xA07F0000;	// lhz		r3, 0 (r31)
					data[i +  62] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  68] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  81] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i +  92] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  94] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i +  95] = 0x28000001;	// cmplwi	r0, 1
					data[i +  97] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  98] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 103] = 0xA07F0000;	// lhz		r3, 0 (r31)
					data[i + 129] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 131] = 0x7CA42B78;	// mr		r4, r5
					data[i + 237] = 0xA0DF0000;	// lhz		r6, 0 (r31)
					data[i + 258] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 260] = 0x28000002;	// cmplwi	r0, 2
					data[i + 458] = branchAndLink(VIConfigureHook2, VIConfigure + 458);
					break;
				case 5:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
					data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
					data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  71] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  84] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  97] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i +  98] = 0x28000001;	// cmplwi	r0, 1
					data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 101] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 103] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 106] = 0xA07F0000;	// lhz		r3, 0 (r31)
					data[i + 132] = 0x38800000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 134] = 0x7C832378;	// mr		r3, r4
					data[i + 256] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 258] = 0x28000002;	// cmplwi	r0, 2
					data[i + 456] = branchAndLink(VIConfigureHook2, VIConfigure + 456);
					break;
				case 6:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
					data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
					data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i +  71] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  84] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i +  99] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 101] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 102] = 0x28000001;	// cmplwi	r0, 1
					data[i + 104] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 105] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 107] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 148] = 0x38C00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 150] = 0x7CC53378;	// mr		r5, r6
					data[i + 280] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 282] = 0x28000002;	// cmplwi	r0, 2
					data[i + 284] = 0x28000003;	// cmplwi	r0, 3
					data[i + 481] = branchAndLink(VIConfigureHook2, VIConfigure + 481);
					break;
				case 7:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
					data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
					data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 103] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i + 116] = 0xA07F0014;	// lhz		r3, 20 (r31)
					data[i + 127] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 131] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 133] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 134] = 0x28000001;	// cmplwi	r0, 1
					data[i + 136] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 137] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 139] = 0xA01F0010;	// lhz		r0, 16 (r31)
					data[i + 180] = 0x38C00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 182] = 0x7CC53378;	// mr		r5, r6
					data[i + 313] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 315] = 0x28000002;	// cmplwi	r0, 2
					data[i + 317] = 0x28000003;	// cmplwi	r0, 3
					data[i + 319] = 0x28000002;	// cmplwi	r0, 2
					data[i + 516] = branchAndLink(VIConfigureHook2, VIConfigure + 516);
					break;
				case 8:
					data[i +   8] = branchAndLink(VIConfigureHook1, VIConfigure + 8);
					data[i +  10] = 0xA0BB0000;	// lhz		r5, 0 (r27)
					data[i +  20] = 0xA01B0000;	// lhz		r0, 0 (r27)
					data[i + 101] = 0x5408003C;	// clrrwi	r8, r0, 1
					data[i + 106] = 0xA0FB0012;	// lhz		r7, 18 (r27)
					data[i + 112] = 0xA07B0014;	// lhz		r3, 20 (r27)
					data[i + 123] = 0xA0FB0010;	// lhz		r7, 16 (r27)
					data[i + 127] = 0xA0FB0010;	// lhz		r7, 16 (r27)
					data[i + 129] = 0x28090001;	// cmplwi	r9, 1
					data[i + 131] = 0xA0FB0010;	// lhz		r7, 16 (r27)
					data[i + 133] = 0xA0FB0010;	// lhz		r7, 16 (r27)
					data[i + 211] = 0x38E00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 217] = 0x7CF93B78;	// mr		r25, r7
					data[i + 332] = 0x815D0024;	// lwz		r10, 36 (r29)
					data[i + 336] = 0x280A0002;	// cmplwi	r10, 2
					data[i + 343] = 0x280A0003;	// cmplwi	r10, 3
					data[i + 345] = 0x280A0002;	// cmplwi	r10, 2
					data[i + 552] = branchAndLink(VIConfigureHook2, VIConfigure + 552);
					break;
				case 9:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  10] = 0xA0930000;	// lhz		r4, 0 (r19)
					data[i +  20] = 0xA0130000;	// lhz		r0, 0 (r19)
					data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 103] = 0xA0130012;	// lhz		r0, 18 (r19)
					data[i + 116] = 0xA0730014;	// lhz		r3, 20 (r19)
					data[i + 127] = 0xA0130010;	// lhz		r0, 16 (r19)
					data[i + 131] = 0xA0130010;	// lhz		r0, 16 (r19)
					data[i + 133] = 0x801D0000;	// lwz		r0, 0 (r29)
					data[i + 134] = 0x28000001;	// cmplwi	r0, 1
					data[i + 136] = 0xA0130010;	// lhz		r0, 16 (r19)
					data[i + 137] = 0x5400003C;	// clrrwi	r0, r0, 1
					data[i + 139] = 0xA0130010;	// lhz		r0, 16 (r19)
					data[i + 180] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 182] = 0x7CA42B78;	// mr		r4, r5
					data[i + 508] = branchAndLink(VIConfigureHook2, VIConfigure + 508);
					break;
			}
			if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9) {
				switch (j) {
					case 0:
						data[i + 181] = 0x579C07B8;	// rlwinm	r28, r28, 0, 30, 28
						data[i + 182] = 0x501C177A;	// rlwimi	r28, r0, 2, 29, 29
						break;
					case 1:
						data[i + 192] = 0x579C07B8;	// rlwinm	r28, r28, 0, 30, 28
						data[i + 193] = 0x501C177A;	// rlwimi	r28, r0, 2, 29, 29
						break;
					case 2:
						data[i + 205] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 206] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 3:
						data[i + 197] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 198] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 4:
						data[i + 232] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 233] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 5:
						data[i + 235] = 0x546307B8;	// rlwinm	r3, r3, 0, 30, 28
						data[i + 236] = 0x5003177A;	// rlwimi	r3, r0, 2, 29, 29
						break;
					case 6:
						data[i + 253] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 254] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 7:
						data[i + 285] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 286] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 8:
						data[i + 309] = 0x7D004378;	// mr		r0, r8
						data[i + 310] = 0x5160177A;	// rlwimi	r0, r11, 2, 29, 29
						break;
					case 9:
						data[i + 288] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						data[i + 289] = 0x54E9003C;	// rlwinm	r9, r7, 0, 0, 30
						data[i + 290] = 0x61290001;	// ori		r9, r9, 1
						break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", VIConfigureSigs[j].Name, VIConfigure);
		}
	}
	
	if ((i = VIConfigurePanSig.offsetFoundAt)) {
		u32 *VIConfigurePan = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *VIConfigurePanHook;
		
		if (VIConfigurePan) {
			VIConfigurePanHook = getPatchAddr(VI_CONFIGUREPANHOOK);
			
			data[i + 10] = branchAndLink(VIConfigurePanHook, VIConfigurePan + 10);
			data[i + 63] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
			data[i + 65] = 0x7CA32B78;	// mr		r3, r5
			
			print_gecko("Found:[%s] @ %08X\n", VIConfigurePanSig.Name, VIConfigurePan);
		}
	}
	
	for (j = 0; j < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern); j++)
		if (VIGetNextFieldSigs[j].offsetFoundAt) break;
	
	if ((i = VIGetNextFieldSigs[j].offsetFoundAt)) {
		u32 *VIGetNextField = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VIGetNextField) {
			if (j == 0)
				data[i + 7] = 0x547F0FFE;	// srwi		r31, r3, 31
			
			if (swissSettings.gameVMode == 3 || swissSettings.gameVMode == 8) {
				memset(data + i, 0, VIGetNextFieldSigs[j].Length * sizeof(u32));
				data[i + 0] = 0x38600001;	// li		r3, 1
				data[i + 1] = 0x4E800020;	// blr
			}
			print_gecko("Found:[%s] @ %08X\n", VIGetNextFieldSigs[j].Name, VIGetNextField);
		}
	}
	
	for (j = 0; j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern); j++)
		if (__GXInitGXSigs[j].offsetFoundAt) break;
	
	if ((i = __GXInitGXSigs[j].offsetFoundAt)) {
		u32 *__GXInitGX = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__GXInitGX) {
			if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9) {
				switch (j) {
					case 0: data[i + 1055] = 0x38600001; break;
					case 1: data[i +  466] = 0x38600001; break;
					case 2: data[i +  911] = 0x38600001; break;
					case 3: data[i +  461] = 0x38600001; break;
					case 4: data[i +  476] = 0x38600001; break;
					case 5: data[i +  491] = 0x38600001; break;
					case 6: data[i +  461] = 0x38600001; break;
					case 7: data[i +  520] = 0x38600001; break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", __GXInitGXSigs[j].Name, __GXInitGX);
		}
	}
	
	if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 5) {
		for (j = 0; j < sizeof(GXAdjustForOverscanSigs) / sizeof(FuncPattern); j++)
			if (GXAdjustForOverscanSigs[j].offsetFoundAt) break;
		
		if ((i = GXAdjustForOverscanSigs[j].offsetFoundAt)) {
			u32 *GXAdjustForOverscan = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXAdjustForOverscan) {
				if (GXAdjustForOverscanSigs[j].Patch) {
					memset(data + i, 0, GXAdjustForOverscanSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXAdjustForOverscanSigs[j].Patch, GXAdjustForOverscanSigs[j].PatchLength);
				}
				print_gecko("Found:[%s] @ %08X\n", GXAdjustForOverscanSigs[j].Name, GXAdjustForOverscan);
			}
		}
		
		for (j = 0; j < sizeof(GXSetDispCopyYScaleSigs) / sizeof(FuncPattern); j++)
			if (GXSetDispCopyYScaleSigs[j].offsetFoundAt) break;
		
		if ((i = GXSetDispCopyYScaleSigs[j].offsetFoundAt)) {
			u32 *GXSetDispCopyYScale = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetDispCopyYScale) {
				if (GXSetDispCopyYScaleSigs[j].Patch) {
					u32 op = j >= 2 ? data[i + 7] : data[i + 65];
					memset(data + i, 0, GXSetDispCopyYScaleSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetDispCopyYScaleSigs[j].Patch, GXSetDispCopyYScaleSigs[j].PatchLength);
					
					if (GXSetDispCopyYScaleSigs[j].Patch != GXSetDispCopyYScalePatch2)
						data[i] |= op & 0x1FFFFF;
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetDispCopyYScaleSigs[j].Name, GXSetDispCopyYScale);
			}
		}
	}
	
	if (swissSettings.forceVFilter) {
		for (j = 0; j < sizeof(GXSetCopyFilterSigs) / sizeof(FuncPattern); j++)
			if (GXSetCopyFilterSigs[j].offsetFoundAt) break;
		
		if ((i = GXSetCopyFilterSigs[j].offsetFoundAt)) {
			u32 *GXSetCopyFilter = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetCopyFilter) {
				if (GXSetCopyFilterSigs[j].Patch) {
					memset(data + i, 0, GXSetCopyFilterSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetCopyFilterSigs[j].Patch, GXSetCopyFilterSigs[j].PatchLength);
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetCopyFilterSigs[j].Name, GXSetCopyFilter);
			}
		}
	}
	
	if (swissSettings.disableDithering) {
		for (j = 0; j < sizeof(GXSetBlendModeSigs) / sizeof(FuncPattern); j++)
			if (GXSetBlendModeSigs[j].offsetFoundAt) break;
		
		if ((i = GXSetBlendModeSigs[j].offsetFoundAt)) {
			u32 *GXSetBlendMode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetBlendMode) {
				if (GXSetBlendModeSigs[j].Patch) {
					u32 op = j >= 2 ? data[i] : data[i + 41];
					memset(data + i, 0, GXSetBlendModeSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetBlendModeSigs[j].Patch, GXSetBlendModeSigs[j].PatchLength);
					
					if (GXSetBlendModeSigs[j].Patch != GXSetBlendModePatch3)
						data[i +  0] |= op & 0x1FFFFF;
					if (GXSetBlendModeSigs[j].Patch == GXSetBlendModePatch2)
						data[i + 21] |= op & 0x1FFFFF;
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetBlendModeSigs[j].Name, GXSetBlendMode);
			}
		}
	}
	
	if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 9) {
		for (j = 0; j < sizeof(GXCopyDispSigs) / sizeof(FuncPattern); j++)
			if (GXCopyDispSigs[j].offsetFoundAt) break;
		
		if ((i = GXCopyDispSigs[j].offsetFoundAt)) {
			u32 *GXCopyDisp = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *GXCopyDispHook;
			
			if (GXCopyDisp) {
				GXCopyDispHook = getPatchAddr(GX_COPYDISPHOOK);
				
				data[i + GXCopyDispSigs[j].Length] = branch(GXCopyDispHook, GXCopyDisp + GXCopyDispSigs[j].Length);
				
				print_gecko("Found:[%s] @ %08X\n", GXCopyDispSigs[j].Name, GXCopyDisp);
			}
		}
		
		for (j = 0; j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern); j++)
			if (GXSetViewportSigs[j].offsetFoundAt) break;
		
		if ((i = GXSetViewportSigs[j].offsetFoundAt)) {
			u32 *GXSetViewport = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetViewport) {
				switch (j) {
					case 0: findx_pattern(data, dataType, i + 16, length, &GXSetViewportJitterSigs[0]); break;
					case 1: findx_pattern(data, dataType, i +  4, length, &GXSetViewportJitterSigs[1]); break;
					case 2: findx_pattern(data, dataType, i +  4, length, &GXSetViewportJitterSigs[2]); break;
					case 3: findx_pattern(data, dataType, i +  1, length, &GXSetViewportJitterSigs[3]); break;
					case 4:
						findx_pattern(data, dataType, i + 10, length, &__GXSetViewportSig);
						
						if ((i - __GXSetViewportSig.offsetFoundAt) == 58) {
							GXSetViewportJitterSigs[4].offsetFoundAt = i - 22;
							
							memset(data + i - 22, 0, 22 * sizeof(u32));
							data[i - 22] = branch(GXSetViewport, GXSetViewport - 22);
						}
						break;
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetViewportSigs[j].Name, GXSetViewport);
			}
		}
		
		for (j = 0; j < sizeof(GXSetViewportJitterSigs) / sizeof(FuncPattern); j++)
			if (GXSetViewportJitterSigs[j].offsetFoundAt) break;
		
		if ((i = GXSetViewportJitterSigs[j].offsetFoundAt)) {
			u32 *GXSetViewportJitter = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetViewportJitter) {
				if (GXSetViewportJitterSigs[j].Patch) {
					u32 op = j >= 1 ? data[i + 18] : data[i + 54];
					memset(data + i, 0, GXSetViewportJitterSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetViewportJitterSigs[j].Patch, GXSetViewportJitterSigs[j].PatchLength);
					
					data[i + 0] |= op & 0x1FFFFF;
					data[i + 2] |= ((u32)GXSetViewportJitter + GXSetViewportJitterSigs[j].PatchLength + 0x8000) >> 16;
					data[i + 4] |= ((u32)GXSetViewportJitter + GXSetViewportJitterSigs[j].PatchLength) & 0xFFFF;
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetViewportJitterSigs[j].Name, GXSetViewportJitter);
			}
		}
		
		if ((i = __GXSetViewportSig.offsetFoundAt)) {
			u32 *__GXSetViewport = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (__GXSetViewport) {
				if (__GXSetViewportSig.Patch) {
					memset(data + i, 0, __GXSetViewportSig.Length * sizeof(u32));
					memcpy(data + i, __GXSetViewportSig.Patch, __GXSetViewportSig.PatchLength);
					
					data[i + 1] |= ((u32)__GXSetViewport + __GXSetViewportSig.PatchLength + 0x8000) >> 16;
					data[i + 3] |= ((u32)__GXSetViewport + __GXSetViewportSig.PatchLength) & 0xFFFF;
				}
				print_gecko("Found:[%s] @ %08X\n", __GXSetViewportSig.Name, __GXSetViewport);
			}
		}
	}
}

void Patch_Widescreen(u32 *data, u32 length, int dataType)
{
	int i, j;
	FuncPattern MTXFrustumSig = 
		{ 38, 4, 16, 0, 0, 0, NULL, 0, "C_MTXFrustum" };
	FuncPattern MTXLightFrustumSig = 
		{ 36, 6, 13, 0, 0, 0, NULL, 0, "C_MTXLightFrustum" };
	FuncPattern MTXPerspectiveSig = 
		{ 51, 8, 19, 1, 0, 6, NULL, 0, "C_MTXPerspective" };
	FuncPattern MTXLightPerspectiveSig = 
		{ 50, 8, 15, 1, 0, 8, NULL, 0, "C_MTXLightPerspective" };
	FuncPattern MTXOrthoSig = 
		{ 37, 4, 16, 0, 0, 0, NULL, 0, "C_MTXOrtho" };
	FuncPattern GXSetScissorSigs[3] = {
		{ 43, 19, 6, 0, 0, 6, NULL, 0, "GXSetScissor A" },
		{ 35, 12, 6, 0, 0, 6, NULL, 0, "GXSetScissor B" },
		{ 29, 13, 6, 0, 0, 1, NULL, 0, "GXSetScissor C" }
	};
	FuncPattern GXSetProjectionSigs[3] = {
		{ 52, 30, 17, 0, 1, 0, NULL, 0, "GXSetProjection A" },
		{ 44, 22, 17, 0, 1, 0, NULL, 0, "GXSetProjection B" },
		{ 40, 18, 11, 0, 1, 0, NULL, 0, "GXSetProjection C" }
	};
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i] != 0xED241828)
			continue;
		if (find_pattern(data + i, length, &MTXFrustumSig)) {
			u32 *MTXFrustum = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXFrustumHook = NULL;
			if (MTXFrustum) {
				print_gecko("Found:[%s] @ %08X\n", MTXFrustumSig.Name, MTXFrustum);
				MTXFrustumHook = installPatch(MTX_FRUSTUMHOOK);
				MTXFrustumHook[7] = branch(MTXFrustum + 1, MTXFrustumHook + 7);
				data[i] = branch(MTXFrustumHook, MTXFrustum);
				*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAB;
				MTXFrustumSig.offsetFoundAt = i;
				break;
			}
		}
	}
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i + 2] != 0xED441828)
			continue;
		if (find_pattern(data + i, length, &MTXLightFrustumSig)) {
			u32 *MTXLightFrustum = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXLightFrustumHook = NULL;
			if (MTXLightFrustum) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightFrustumSig.Name, MTXLightFrustum);
				MTXLightFrustumHook = installPatch(MTX_LIGHTFRUSTUMHOOK);
				MTXLightFrustumHook[7] = branch(MTXLightFrustum + 3, MTXLightFrustumHook + 7);
				data[i + 2] = branch(MTXLightFrustumHook, MTXLightFrustum + 2);
				*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAB;
				break;
			}
		}
	}
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i] != 0x7C0802A6)
			continue;
		if (find_pattern(data + i, length, &MTXPerspectiveSig)) {
			u32 *MTXPerspective = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXPerspectiveHook = NULL;
			if (MTXPerspective) {
				print_gecko("Found:[%s] @ %08X\n", MTXPerspectiveSig.Name, MTXPerspective);
				MTXPerspectiveHook = installPatch(MTX_PERSPECTIVEHOOK);
				MTXPerspectiveHook[5] = branch(MTXPerspective + 21, MTXPerspectiveHook + 5);
				data[i + 20] = branch(MTXPerspectiveHook, MTXPerspective + 20);
				*(u32*)VAR_FLOAT9_16 = 0x3F100000;
				MTXPerspectiveSig.offsetFoundAt = i;
				break;
			}
		}
	}
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i] != 0x7C0802A6)
			continue;
		if (find_pattern(data + i, length, &MTXLightPerspectiveSig)) {
			u32 *MTXLightPerspective = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXLightPerspectiveHook = NULL;
			if (MTXLightPerspective) {
				print_gecko("Found:[%s] @ %08X\n", MTXLightPerspectiveSig.Name, MTXLightPerspective);
				MTXLightPerspectiveHook = installPatch(MTX_LIGHTPERSPECTIVEHOOK);
				MTXLightPerspectiveHook[5] = branch(MTXLightPerspective + 25, MTXLightPerspectiveHook + 5);
				data[i + 24] = branch(MTXLightPerspectiveHook, MTXLightPerspective + 24);
				*(u32*)VAR_FLOAT9_16 = 0x3F100000;
				break;
			}
		}
	}
	if (swissSettings.forceWidescreen == 2) {
		for (i = 0; i < length / sizeof(u32); i++) {
			if (data[i] != 0xED041828)
				continue;
			if (find_pattern(data + i, length, &MTXOrthoSig)) {
				u32 *MTXOrtho = Calc_ProperAddress(data, dataType, i * sizeof(u32));
				u32 *MTXOrthoHook = NULL;
				if (MTXOrtho) {
					print_gecko("Found:[%s] @ %08X\n", MTXOrthoSig.Name, MTXOrtho);
					MTXOrthoHook = installPatch(MTX_ORTHOHOOK);
					MTXOrthoHook[32] = branch(MTXOrtho + 1, MTXOrthoHook + 32);
					data[i] = branch(MTXOrthoHook, MTXOrtho);
					*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAB;
					*(u32*)VAR_FLOATM_1 = 0xBF800000;
					break;
				}
			}
		}
		for (i = 0; i < length / sizeof(u32); i++) {
			if ((data[i + 1] & 0xFC00FFFF) != 0x38000156)
				continue;
			
			FuncPattern fp;
			make_pattern(data + i, length, &fp);
			
			for (j = 0; j < sizeof(GXSetScissorSigs) / sizeof(FuncPattern); j++) {
				if (compare_pattern(&fp, &GXSetScissorSigs[j])) {
					u32 *GXSetScissor = Calc_ProperAddress(data, dataType, i * sizeof(u32));
					u32 *GXSetScissorHook = NULL;
					if (GXSetScissor) {
						print_gecko("Found:[%s] @ %08X\n", GXSetScissorSigs[j].Name, GXSetScissor);
						GXSetScissorHook = installPatch(GX_SETSCISSORHOOK);
						GXSetScissorHook[ 0] = data[i];
						GXSetScissorHook[ 1] = j == 1 ? 0x800801E8 : 0x800701E8;
						GXSetScissorHook[16] = branch(GXSetScissor + 1, GXSetScissorHook + 16);
						data[i] = branch(GXSetScissorHook, GXSetScissor);
						break;
					}
				}
			}
		}
	}
	if (!MTXFrustumSig.offsetFoundAt && !MTXPerspectiveSig.offsetFoundAt) {
		for (i = 0; i < length / sizeof(u32); i++) {
			if (data[i + 1] != 0x2C040001)
				continue;
			
			FuncPattern fp;
			make_pattern(data + i, length, &fp);
			
			for (j = 0; j < sizeof(GXSetProjectionSigs) / sizeof(FuncPattern); j++) {
				if (compare_pattern(&fp, &GXSetProjectionSigs[j])) {
					u32 *GXSetProjection = Calc_ProperAddress(data, dataType, i * sizeof(u32));
					u32 *GXSetProjectionHook = NULL;
					if (GXSetProjection) {
						print_gecko("Found:[%s] @ %08X\n", GXSetProjectionSigs[j].Name, GXSetProjection);
						GXSetProjectionHook = installPatch(GX_SETPROJECTIONHOOK);
						GXSetProjectionHook[5] = branch(GXSetProjection + 4, GXSetProjectionHook + 5);
						data[i + 3] = branch(GXSetProjectionHook, GXSetProjection + 3);
						*(u32*)VAR_FLOAT3_4 = 0x3F400000;
						break;
					}
				}
			}
		}
	}
}

int Patch_TexFilt(u32 *data, u32 length, int dataType)
{
	int i, j;
	FuncPattern GXInitTexObjLODSigs[3] = {
		{ 100, 29, 11, 0, 11, 11, NULL, 0, "GXInitTexObjLOD A" },
		{  88, 16,  4, 0,  8,  6, NULL, 0, "GXInitTexObjLOD B" },	// SN Systems ProDG
		{  88, 30, 11, 0, 11,  6, NULL, 0, "GXInitTexObjLOD C" }
	};
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i + 2] != 0xFC030040 && data[i + 4] != 0xFC030000)
			continue;
		
		FuncPattern fp;
		make_pattern(data + i, length, &fp);
		
		for (j = 0; j < sizeof(GXInitTexObjLODSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &GXInitTexObjLODSigs[j])) {
				u32 *GXInitTexObjLOD = Calc_ProperAddress(data, dataType, i * sizeof(u32));
				u32 *GXInitTexObjLODHook = NULL;
				if (GXInitTexObjLOD) {
					print_gecko("Found:[%s] @ %08X\n", GXInitTexObjLODSigs[j].Name, GXInitTexObjLOD);
					GXInitTexObjLODHook = installPatch(GX_INITTEXOBJLODHOOK);
					GXInitTexObjLODHook[6] = data[i];
					GXInitTexObjLODHook[7] = branch(GXInitTexObjLOD + 1, GXInitTexObjLODHook + 7);
					data[i] = branch(GXInitTexObjLODHook, GXInitTexObjLOD);
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
			ptr[1] = _dvdlowreset_new[1] | ((u32)ENABLE_BACKUP_DISC&0xFFFF);
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
	else if((*(u32*)&gameID[0] == 0x47504F45) 
		&& (*(u32*)&gameID[4] == 0x38500002) 
		&& (*(u32*)&gameID[8] == 0x01000000)) {
		// Nasty PSO 1 & 2+ hack to redirect a lowmem buffer to highmem 
		void *addr_start = addr;
		void *addr_end = addr+length;
		while(addr_start<addr_end) 
		{
			addr_start += 4;
			if( *(vu32*)(addr_start) != 0x7C0802A6 )
				continue;
			if(PatchPatchBuffer(addr_start)) {
				print_gecko("Patched: PSO 1 & 2 +\r\n");
				patched=1;
			}
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
			void *patchAddr = getPatchAddr(MAJORA_SAVEREGS);
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
			void *patchAddr = getPatchAddr(MAJORA_SAVEREGS);
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
			void *patchAddr = getPatchAddr(MAJORA_SAVEREGS);
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
			void *properAddress = Calc_ProperAddress(data, dataType, (u32)(data + i + iEnd) -(u32)(data));
			print_gecko("Found:[SPEC2_MakeStatusA] @ %08X (%08X)\n", properAddress, i + iEnd);
			void *igrJump = (devices[DEVICE_CUR] == &__device_wkf) ? IGR_CHECK_WKF : IGR_CHECK;
			if(devices[DEVICE_CUR] == &__device_dvd) igrJump = IGR_CHECK_DVD;
			*(vu32*)(data+i+iEnd) = branch(igrJump, properAddress);
			return 1;
		}
	}
	return 0;
}

void *Calc_ProperAddress(void *data, int dataType, u32 offsetFoundAt) {
	if(dataType == PATCH_DOL) {
		int i;
		DOLHEADER *hdr = (DOLHEADER *) data;

		// Doesn't look valid
		if (hdr->textOffset[0] != DOLHDRLENGTH) {
			print_gecko("DOL Header doesn't look valid %08X\r\n",hdr->textOffset[0]);
			return NULL;
		}

		// Inspect text sections to see if what we found lies in here
		for (i = 0; i < MAXTEXTSECTION; i++) {
			if (hdr->textAddress[i] && hdr->textLength[i]) {
				// Does what we found lie in this section?
				if((offsetFoundAt >= hdr->textOffset[i]) && offsetFoundAt <= (hdr->textOffset[i] + hdr->textLength[i])) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return (void*)(offsetFoundAt+hdr->textAddress[i]-hdr->textOffset[i]);
				}
			}
		}

		// Inspect data sections (shouldn't really need to unless someone was sneaky..)
		for (i = 0; i < MAXDATASECTION; i++) {
			if (hdr->dataAddress[i] && hdr->dataLength[i]) {
				// Does what we found lie in this section?
				if((offsetFoundAt >= hdr->dataOffset[i]) && offsetFoundAt <= (hdr->dataOffset[i] + hdr->dataLength[i])) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return (void*)(offsetFoundAt+hdr->dataAddress[i]-hdr->dataOffset[i]);
				}
			}
		}
	
	}
	else if(dataType == PATCH_ELF) {
		if(!valid_elf_image((u32)data))
			return NULL;
			
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
					return (void*)(offsetFoundAt+shdr->sh_addr-shdr->sh_offset);
				}
			}
		}	
	}
	else if(dataType == PATCH_LOADER) {
		return (void*)(offsetFoundAt+0x81300000);
	}
	print_gecko("No cases matched, returning NULL for proper address\r\n");
	return NULL;
}

void *Calc_Address(void *data, int dataType, u32 properAddress) {
	if(dataType == PATCH_DOL) {
		int i;
		DOLHEADER *hdr = (DOLHEADER *) data;

		// Doesn't look valid
		if (hdr->textOffset[0] != DOLHDRLENGTH) {
			print_gecko("DOL Header doesn't look valid %08X\r\n",hdr->textOffset[0]);
			return NULL;
		}

		// Inspect text sections to see if what we found lies in here
		for (i = 0; i < MAXTEXTSECTION; i++) {
			if (hdr->textAddress[i] && hdr->textLength[i]) {
				// Does what we found lie in this section?
				if((properAddress >= hdr->textAddress[i]) && properAddress <= (hdr->textAddress[i] + hdr->textLength[i])) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-hdr->textAddress[i]+hdr->textOffset[i];
				}
			}
		}

		// Inspect data sections (shouldn't really need to unless someone was sneaky..)
		for (i = 0; i < MAXDATASECTION; i++) {
			if (hdr->dataAddress[i] && hdr->dataLength[i]) {
				// Does what we found lie in this section?
				if((properAddress >= hdr->dataAddress[i]) && properAddress <= (hdr->dataAddress[i] + hdr->dataLength[i])) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-hdr->dataAddress[i]+hdr->dataOffset[i];
				}
			}
		}
	
	}
	else if(dataType == PATCH_ELF) {
		if(!valid_elf_image((u32)data))
			return NULL;
			
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
				if((properAddress >= shdr->sh_addr) && properAddress <= (shdr->sh_addr + shdr->sh_size)) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-shdr->sh_addr+shdr->sh_offset;
				}
			}
		}	
	}
	else if(dataType == PATCH_LOADER) {
		return data+properAddress-0x81300000;
	}
	print_gecko("No cases matched, returning NULL for address\r\n");
	return NULL;
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
			void *properAddress = Calc_ProperAddress(data, type, i+j);
			if(properAddress) {
				print_gecko("Found:[Hook:OSSleepThread] @ %08X\n", properAddress );
				*(vu32*)(data+i+j) = branch(CHEATS_ENGINE_START, properAddress);
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
			void *properAddress = Calc_ProperAddress(data, type, i+j);
			if(properAddress) {
				print_gecko("Found:[Hook:GXDrawDone] @ %08X\n", properAddress );
				*(vu32*)(data+i+j) = branch(CHEATS_ENGINE_START, properAddress);
				break;
			}
		}
	}
	return 0;
}


