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
FuncPattern ReadDebug = {56, 23, 18, 3, 2, 4, 0, 0, "Read (Debug)", 0};
FuncPattern ReadCommon = {68, 30, 18, 5, 2, 3, 0, 0, "Read (Common)", 0};
FuncPattern ReadUncommon = {66, 29, 17, 5, 2, 3, 0, 0, "Read (Uncommon)", 0};
FuncPattern ReadProDG = {67, 29, 17, 5, 2, 6, 0, 0, "Read (ProDG)", 0};

// OSExceptionInit
FuncPattern OSExceptionInitSig = {160, 39, 14, 14, 20, 7, 0, 0, "OSExceptionInit", 0};
FuncPattern OSExceptionInitSigDebug = {164, 61, 6, 18, 14, 14, 0, 0, "OSExceptionInit (Debug)", 0};
FuncPattern OSExceptionInitSigProDG = {151, 45, 14, 16, 13, 9, 0, 0, "OSExceptionInit (ProDG)", 0};

// __DVDInterruptHandler
u16 _dvdinterrupthandler_part[3] = {
	0x6000, 0x002A, 0x0054
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
		case EXI_LOCKHOOK:
			patchSize = EXILockHook_length; patchLocation = EXILockHook; break;
		case EXI_LOCKHOOKD:
			patchSize = EXILockHookD_length; patchLocation = EXILockHookD; break;
		case GX_COPYDISPHOOK:
			patchSize = GXCopyDispHook_length; patchLocation = GXCopyDispHook; break;
		case GX_INITTEXOBJLODHOOK:
			patchSize = GXInitTexObjLODHook_length; patchLocation = GXInitTexObjLODHook; break;
		case GX_SETPROJECTIONHOOK:
			patchSize = GXSetProjectionHook_length; patchLocation = GXSetProjectionHook; break;
		case GX_SETSCISSORHOOK:
			patchSize = GXSetScissorHook_length; patchLocation = GXSetScissorHook; break;
		case GX_TOKENINTERRUPTHANDLERHOOK:
			patchSize = GXTokenInterruptHandlerHook_length; patchLocation = GXTokenInterruptHandlerHook; break;
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
		case VI_CONFIGURE540P50:
			patchSize = VIConfigure540p50_length; patchLocation = VIConfigure540p50; break;
		case VI_CONFIGURE540P60:
			patchSize = VIConfigure540p60_length; patchLocation = VIConfigure540p60; break;
		case VI_CONFIGURE576I:
			patchSize = VIConfigure576i_length; patchLocation = VIConfigure576i; break;
		case VI_CONFIGURE576P:
			patchSize = VIConfigure576p_length; patchLocation = VIConfigure576p; break;
		case VI_CONFIGURE960I:
			patchSize = VIConfigure960i_length; patchLocation = VIConfigure960i; break;
		case VI_CONFIGURE1080I50:
			patchSize = VIConfigure1080i50_length; patchLocation = VIConfigure1080i50; break;
		case VI_CONFIGURE1080I60:
			patchSize = VIConfigure1080i60_length; patchLocation = VIConfigure1080i60; break;
		case VI_CONFIGURE1152I:
			patchSize = VIConfigure1152i_length; patchLocation = VIConfigure1152i; break;
		case VI_CONFIGUREHOOK1:
			patchSize = VIConfigureHook1_length; patchLocation = VIConfigureHook1; break;
		case VI_CONFIGUREHOOK1_GCVIDEO:
			patchSize = VIConfigureHook1GCVideo_length; patchLocation = VIConfigureHook1GCVideo; break;
		case VI_CONFIGUREHOOK2:
			patchSize = VIConfigureHook2_length; patchLocation = VIConfigureHook2; break;
		case VI_CONFIGUREPANHOOK:
			patchSize = VIConfigurePanHook_length; patchLocation = VIConfigurePanHook; break;
		case VI_CONFIGUREPANHOOKD:
			patchSize = VIConfigurePanHookD_length; patchLocation = VIConfigurePanHookD; break;
		case VI_GETRETRACECOUNTHOOK:
			patchSize = VIGetRetraceCountHook_length; patchLocation = VIGetRetraceCountHook; break;
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

int install_code(int final)
{
	u32 location = LO_RESERVE;
	u8 *patch = NULL; u32 patchSize = 0;
	
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
		if(swissSettings.alternateReadPatches) {
			patch = (!_ideexi_version)?&ideexi_altv1_bin[0]:&ideexi_altv2_bin[0];
			patchSize = (!_ideexi_version)?ideexi_altv1_bin_size:ideexi_altv2_bin_size;
			location = LO_RESERVE_ALT;
		}
		else {
			patch = (!_ideexi_version)?&ideexi_v1_bin[0]:&ideexi_v2_bin[0];
			patchSize = (!_ideexi_version)?ideexi_v1_bin_size:ideexi_v2_bin_size;
		}
		print_gecko("Installing Patch for IDE-EXI\r\n");
  	}
	// SD Card over EXI
	else if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
		if(swissSettings.alternateReadPatches) {
			patch = &sd_alt_bin[0];
			patchSize = sd_alt_bin_size;
			location = LO_RESERVE_ALT;
		}
		else {
			patch = &sd_bin[0];
			patchSize = sd_bin_size;
		}
		print_gecko("Installing Patch for SD Card over EXI\r\n");
	}
	// DVD 2 disc code
	else if(devices[DEVICE_CUR] == &__device_dvd) {
		patch = &dvd_bin[0]; patchSize = dvd_bin_size;
		location = LO_RESERVE_DVD;
		print_gecko("Installing Patch for DVD\r\n");
	}
	// USB Gecko
	else if(devices[DEVICE_CUR] == &__device_usbgecko) {
		patch = &usbgecko_bin[0]; patchSize = usbgecko_bin_size;
		location = LO_RESERVE_ALT;
		print_gecko("Installing Patch for USB Gecko\r\n");
	}
	// Wiikey Fusion
	else if(devices[DEVICE_CUR] == &__device_wkf) {
		patch = &wkf_bin[0]; patchSize = wkf_bin_size;
		print_gecko("Installing Patch for WKF\r\n");
	}
	// Broadband Adapter
	else if(devices[DEVICE_CUR] == &__device_fsp) {
		patch = &bba_bin[0]; patchSize = bba_bin_size;
		location = LO_RESERVE_ALT;
		print_gecko("Installing Patch for Broadband Adapter\r\n");
	}
	print_gecko("Space for patch remaining: %i\r\n", top_addr - location);
	print_gecko("Space taken by vars/video patches: %i\r\n", VAR_PATCHES_BASE - top_addr);
	if(top_addr - location < patchSize)
		return 0;
	if(final) {
		memcpy((void*)location,patch,patchSize);
		DCFlushRange((void*)location,patchSize);
		ICInvalidateRange((void*)location,patchSize);
	}
	return 1;
}

void make_pattern(u32 *data, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
{
	u32 i, j;
	
	memset(functionPattern, 0, sizeof(FuncPattern));
	
	for (j = i = offsetFoundAt; j >= offsetFoundAt && i < length / sizeof(u32); i++) {
		u32 word = data[i];
		
		functionPattern->Length++;
		
		if ((word == 0x4C000064 || word == 0x4E800020) && j <= i)
			break;
		if ((word & 0xFC000003) == 0x40000000)
			j = i + ((s32)((word & 0x0000FFFC) << 16) >> 18);
		if ((word & 0xFC000003) == 0x48000000)
			j = i + ((s32)((word & 0x03FFFFFC) << 6) >> 8);
		
		if ((word & 0xFF000000) == 0x38000000 ||
			(word & 0xFF000000) == 0x3C000000 ||
			(word & 0xFC000000) == 0x80000000 ||
			(word & 0xFC000000) == 0xC0000000)
			functionPattern->Loads++;
		
		if ((word & 0xFC000000) == 0x90000000 ||
			(word & 0xFC000000) == 0x94000000 ||
			(word & 0xFC000000) == 0xD0000000)
			functionPattern->Stores++;
		
		if ((word & 0xFC000003) == 0x48000001)
			functionPattern->FCalls++;
		
		if ((word & 0xFFFF0000) == 0x40800000 ||
			(word & 0xFFFF0000) == 0x40810000 ||
			(word & 0xFFFF0000) == 0x41800000 ||
			(word & 0xFFFF0000) == 0x41820000 ||
			(word & 0xFC000003) == 0x48000000)
			functionPattern->Branch++;
		
		if ((word & 0xFF000000) == 0x7C000000 ||
			(word & 0xFC0007FE) == 0xFC000090)
			functionPattern->Moves++;
	}
	
	functionPattern->offsetFoundAt = offsetFoundAt;
}

bool compare_pattern(FuncPattern *FPatA, FuncPattern *FPatB)
{
	return memcmp(FPatA, FPatB, sizeof(u32) * 6) == 0;
}

bool find_pattern(u32 *data, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
{
	FuncPattern FP;
	
	make_pattern(data, offsetFoundAt, length, &FP);
	
	if (functionPattern && compare_pattern(&FP, functionPattern)) {
		functionPattern->offsetFoundAt = FP.offsetFoundAt;
		return true;
	}
	
	if (!functionPattern) {
		print_gecko("Length: %d\n", FP.Length);
		print_gecko("Loads: %d\n", FP.Loads);
		print_gecko("Stores: %d\n", FP.Stores);
		print_gecko("FCalls: %d\n", FP.FCalls);
		print_gecko("Branch: %d\n", FP.Branch);
		print_gecko("Moves: %d\n", FP.Moves);
	}
	
	return false;
}

bool find_pattern_before(u32 *data, u32 length, FuncPattern *FPatA, FuncPattern *FPatB)
{
	u32 offsetFoundAt = FPatA->offsetFoundAt - FPatB->Length;
	
	if (offsetFoundAt == FPatB->offsetFoundAt)
		return true;
	
	return find_pattern(data, offsetFoundAt, length, FPatB);
}

bool find_pattern_after(u32 *data, u32 length, FuncPattern *FPatA, FuncPattern *FPatB)
{
	u32 offsetFoundAt = FPatA->offsetFoundAt + FPatA->Length;
	
	if (offsetFoundAt == FPatB->offsetFoundAt)
		return true;
	
	return find_pattern(data, offsetFoundAt, length, FPatB);
}

u32 branch(u32 *dst, u32 *src)
{
	u32 word = (u32)dst - (u32)src;
	word &= 0x03FFFFFC;
	word |= 0x48000000;
	return word;
}

u32 branchAndLink(u32 *dst, u32 *src)
{
	u32 word = (u32)dst - (u32)src;
	word &= 0x03FFFFFC;
	word |= 0x48000001;
	return word;
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
	
	if (functionPattern && functionPattern->offsetFoundAt)
		return offsetFoundAt == functionPattern->offsetFoundAt;
	
	return offsetFoundAt && find_pattern(data, offsetFoundAt, length, functionPattern);
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

int PatchDetectLowMemUsage( void *dst, u32 Length, int dataType )
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
		// bl, flush out and reset
		if( (op & 0xFC000003) == 0x48000001 ) {
			memset(regs, 0, 14*4*2);
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
		if( *(vu32*)(addr+i) != 0x7C0802A6 && *(vu32*)(addr+i+4) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( addr, i / 4, length, &fp );
		
		if(compare_pattern(&fp, &OSExceptionInitSig))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 488)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", OSExceptionInitSig.Name, properAddress);
			*(vu32*)(addr + i + 488) = branchAndLink(PATCHED_MEMCPY_WKF, properAddress);
			*(vu32*)(addr + i + 496) = 0x38800100;
			*(vu32*)(addr + i + 512) = 0x38800100;
			patched |= 0x10;
		}
		// Debug version of the above
		if(compare_pattern(&fp, &OSExceptionInitSigDebug))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 520)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", OSExceptionInitSigDebug.Name, properAddress);
			*(vu32*)(addr + i + 520) = branchAndLink(PATCHED_MEMCPY_WKF, properAddress);
			*(vu32*)(addr + i + 528) = 0x38800100;
			*(vu32*)(addr + i + 544) = 0x38800100;
			patched |= 0x10;
		}
		if(compare_pattern(&fp, &OSExceptionInitSigProDG))
		{
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 460)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", OSExceptionInitSigProDG.Name, properAddress);
			*(vu32*)(addr + i + 460) = branchAndLink(PATCHED_MEMCPY_WKF, properAddress);
			*(vu32*)(addr + i + 468) = 0x38800100;
			*(vu32*)(addr + i + 484) = 0x38800100;
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
		if(compare_pattern(&fp, &ReadProDG)) {
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x74)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for WKF\r\n", ReadProDG.Name, properAddress - 0x74);
			*(vu32*)(addr + i + 0x74) = branchAndLink(ADJUST_LBA_OFFSET, properAddress);
			patched |= 0x100;
		}
	}
	return patched;
}

void Patch_DVDLowLevelReadAlt(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	FuncPattern OSExceptionInitSigs[3] = {
		{ 164, 61,  6, 18, 14, 14, NULL, 0, "OSExceptionInitD A" },
		{ 160, 39, 14, 14, 20,  7, NULL, 0, "OSExceptionInit A" },
		{ 151, 45, 14, 16, 13,  9, NULL, 0, "OSExceptionInit B" }	// SN Systems ProDG
	};
	FuncPattern OSSetCurrentContextSig = 
		{ 23, 4, 4, 0, 0, 5, NULL, 0, "OSSetCurrentContext" };
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSEnableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSEnableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern SelectThreadSigs[4] = {
		{ 123, 39, 10, 11, 14, 12, NULL, 0, "SelectThreadD A" },
		{ 128, 41, 20,  8, 12, 12, NULL, 0, "SelectThread A" },
		{ 138, 44, 20,  8, 12, 12, NULL, 0, "SelectThread B" },
		{ 141, 51, 19,  8, 12, 14, NULL, 0, "SelectThread C" }	// SN Systems ProDG
	};
	FuncPattern __OSRescheduleSig = 
		{ 12, 4, 2, 1, 1, 2, NULL, 0, "__OSReschedule" };
	FuncPattern __OSGetSystemTimeSigs[2] = {
		{ 22, 4, 2, 3, 0, 3, NULL, 0, "__OSGetSystemTimeD" },
		{ 25, 8, 5, 3, 0, 3, NULL, 0, "__OSGetSystemTime" }
	};
	FuncPattern SetExiInterruptMaskSigs[2] = {
		{ 63, 18, 3, 7, 17, 3, NULL, 0, "SetExiInterruptMaskD" },
		{ 61, 19, 3, 7, 17, 2, NULL, 0, "SetExiInterruptMask" }
	};
	FuncPattern EXILockSigs[3] = {
		{ 106, 35, 5, 9, 13, 6, NULL, 0, "EXILockD A" },
		{  61, 18, 7, 5,  5, 6, NULL, 0, "EXILock A" },
		{  61, 17, 7, 5,  5, 7, NULL, 0, "EXILock B" }
	};
	FuncPattern ReadSigs[4] = {
		{ 56, 23, 18, 3, 2, 4, NULL, 0, "ReadD A" },
		{ 66, 29, 17, 5, 2, 3, NULL, 0, "Read A" },
		{ 68, 30, 18, 5, 2, 3, NULL, 0, "Read B" },
		{ 67, 29, 17, 5, 2, 6, NULL, 0, "Read C" }	// SN Systems ProDG
	};
	FuncPattern DVDLowReadSigs[4] = {
		{ 157,  70,  6, 13, 12, 13, NULL, 0, "DVDLowReadD A" },
		{ 166,  68, 19,  9, 14, 18, NULL, 0, "DVDLowRead A" },
		{ 166,  68, 19,  9, 14, 18, NULL, 0, "DVDLowRead B" },
		{ 321, 113, 75, 23, 17, 34, NULL, 0, "DVDLowRead C" }	// SN Systems ProDG
	};
	FuncPattern stateBusySigs[6] = {
		{ 182, 112, 22, 15, 17, 10, NULL, 0, "stateBusyD A" },
		{ 176, 107, 21, 15, 17, 10, NULL, 0, "stateBusy A" },
		{ 176, 107, 21, 15, 17, 10, NULL, 0, "stateBusy B" },
		{ 200, 118, 23, 16, 20, 10, NULL, 0, "stateBusy C" },
		{ 187, 105, 23, 16, 18, 11, NULL, 0, "stateBusy D" },	// SN Systems ProDG
		{ 208, 123, 24, 17, 21, 10, NULL, 0, "stateBusy E" }
	};
	u32 _SDA2_BASE_ = 0, _SDA_BASE_ = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!_SDA2_BASE_ && !_SDA_BASE_) {
			if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
				(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
				(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
				(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
				_SDA2_BASE_ = (data[i + 0] << 16) | (u16)data[i + 1];
				_SDA_BASE_  = (data[i + 2] << 16) | (u16)data[i + 3];
				i += 4;
			}
			continue;
		}
		if ((data[i - 1] != 0x4C000064 && data[i - 1] != 0x4E800020) ||
			(data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6))
			continue;
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
		for (j = 0; j < sizeof(OSExceptionInitSigs) / sizeof(FuncPattern); j++) {
			if (!OSExceptionInitSigs[j].offsetFoundAt && compare_pattern(&fp, &OSExceptionInitSigs[j])) {
				OSExceptionInitSigs[j].offsetFoundAt = i;
				break;
			}
		}
		
		for (j = 0; j < sizeof(SelectThreadSigs) / sizeof(FuncPattern); j++) {
			if (!SelectThreadSigs[j].offsetFoundAt && compare_pattern(&fp, &SelectThreadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 57, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 58, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 62, length, &OSDisableInterruptsSig))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 76, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 77, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 81, length, &OSDisableInterruptsSig))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 81, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 82, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 86, length, &OSDisableInterruptsSig))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 82, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 83, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 87, length, &OSDisableInterruptsSig))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
			if (SelectThreadSigs[j].offsetFoundAt && i == SelectThreadSigs[j].offsetFoundAt + SelectThreadSigs[j].Length) {
				if (!__OSRescheduleSig.offsetFoundAt && compare_pattern(&fp, &__OSRescheduleSig)) {
					if (findx_pattern(data, dataType, i + 7, length, &SelectThreadSigs[j]))
						__OSRescheduleSig.offsetFoundAt = i;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(EXILockSigs) / sizeof(FuncPattern); j++) {
			if (!EXILockSigs[j].offsetFoundAt && compare_pattern(&fp, &EXILockSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 33, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 61, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 80, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 99, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 97, length, &SetExiInterruptMaskSigs[0]))
							EXILockSigs[j].offsetFoundAt = i;
						break;
					case 1:
					case 2:
						if (findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 43, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 52, length, &SetExiInterruptMaskSigs[1]))
							EXILockSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(ReadSigs) / sizeof(FuncPattern); j++) {
			if (!ReadSigs[j].offsetFoundAt && compare_pattern(&fp, &ReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[0]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 15, length, &__OSGetSystemTimeSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 17, length, &__OSGetSystemTimeSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(DVDLowReadSigs) / sizeof(FuncPattern); j++) {
			if (!DVDLowReadSigs[j].offsetFoundAt && compare_pattern(&fp, &DVDLowReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  89, length, &__OSGetSystemTimeSigs[0]))
							DVDLowReadSigs[j].offsetFoundAt = i;
						break;
					case 1:
					case 2:
						if (findx_pattern(data, dataType, i +  28, length, &ReadSigs[j]) &&
							findx_pattern(data, dataType, i +  83, length, &ReadSigs[j]) &&
							findx_pattern(data, dataType, i + 125, length, &ReadSigs[j]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSGetSystemTimeSigs[1]))
							DVDLowReadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  28, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 136, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 191, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 219, length, &__OSGetSystemTimeSigs[1]))
							DVDLowReadSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(stateBusySigs) / sizeof(FuncPattern); j++) {
			if (!stateBusySigs[j].offsetFoundAt && compare_pattern(&fp, &stateBusySigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 53, length, &DVDLowReadSigs[0]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 48, length, &DVDLowReadSigs[1]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 48, length, &DVDLowReadSigs[2]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 65, length, &DVDLowReadSigs[2]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 62, length, &DVDLowReadSigs[3]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 65, length, &DVDLowReadSigs[2]))
							stateBusySigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		i += fp.Length - 1;
	}
	
	for (j = 0; j < sizeof(OSExceptionInitSigs) / sizeof(FuncPattern); j++)
		if (OSExceptionInitSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(OSExceptionInitSigs) / sizeof(FuncPattern) && (i = OSExceptionInitSigs[j].offsetFoundAt)) {
		u32 *OSExceptionInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSExceptionInit) {
			switch (j) {
				case 0:
					data[i + 140] = 0x28000009;	// cmplwi	r0, 9
					data[i + 153] = 0x28000009;	// cmplwi	r0, 9
					break;
				case 1:
					data[i + 133] = 0x28000009;	// cmplwi	r0, 9
					data[i + 149] = 0x28000009;	// cmplwi	r0, 9
					break;
				case 2:
					data[i + 125] = 0x28000009;	// cmplwi	r0, 9
					data[i + 139] = 0x28000009;	// cmplwi	r0, 9
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", OSExceptionInitSigs[j].Name, OSExceptionInit);
		}
	}
	
	for (j = 0; j < sizeof(SelectThreadSigs) / sizeof(FuncPattern); j++)
		if (SelectThreadSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(SelectThreadSigs) / sizeof(FuncPattern) && (i = SelectThreadSigs[j].offsetFoundAt)) {
		u32 *SelectThread = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SelectThread) {
			switch (j) {
				case 0:
					data[i + 12] = branchAndLink(TICKLE_READ, SelectThread + 12);
					data[i + 13] = 0x480001A0;	// b		+104
					data[i + 20] = branchAndLink(TICKLE_READ, SelectThread + 20);
					data[i + 21] = 0x48000180;	// b		+96
					data[i + 48] = branchAndLink(TICKLE_READ, SelectThread + 48);
					data[i + 49] = 0x48000110;	// b		+68
					data[i + 58] = branchAndLink(TICKLE_READ_IDLE, SelectThread + 58);
					data[i + 61] = 0x4182FFF4;	// beq		-3
					break;
				case 1:
					data[i + 11] = branchAndLink(TICKLE_READ, SelectThread + 11);
					data[i + 12] = 0x480001B4;	// b		+109
					data[i + 19] = branchAndLink(TICKLE_READ, SelectThread + 19);
					data[i + 20] = 0x48000194;	// b		+101
					data[i + 67] = branchAndLink(TICKLE_READ, SelectThread + 67);
					data[i + 68] = 0x480000D4;	// b		+53
					data[i + 77] = branchAndLink(TICKLE_READ_IDLE, SelectThread + 77);
					data[i + 80] = 0x4182FFF4;	// beq		-3
					break;
				case 2:
					data[i + 11] = branchAndLink(TICKLE_READ, SelectThread + 11);
					data[i + 12] = 0x480001DC;	// b		+119
					data[i + 19] = branchAndLink(TICKLE_READ, SelectThread + 19);
					data[i + 20] = 0x480001BC;	// b		+111
					data[i + 67] = branchAndLink(TICKLE_READ, SelectThread + 67);
					data[i + 68] = 0x480000FC;	// b		+63
					data[i + 82] = branchAndLink(TICKLE_READ_IDLE, SelectThread + 82);
					data[i + 85] = 0x4182FFF4;	// beq		-3
					break;
				case 3:
					data[i +  8] = branchAndLink(TICKLE_READ, SelectThread +  8);
					data[i +  9] = 0x480001F8;	// b		+126
					data[i + 15] = branchAndLink(TICKLE_READ, SelectThread + 15);
					data[i + 16] = 0x480001DC;	// b		+119
					data[i + 66] = branchAndLink(TICKLE_READ, SelectThread + 66);
					data[i + 67] = 0x48000110;	// b		+68
					data[i + 83] = branchAndLink(TICKLE_READ_IDLE, SelectThread + 83);
					data[i + 86] = 0x4182FFF4;	// beq		-3
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", SelectThreadSigs[j].Name, SelectThread);
		}
		
		if ((i = __OSRescheduleSig.offsetFoundAt)) {
			u32 *__OSReschedule = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (__OSReschedule && SelectThread) {
				data[i + 0] = data[i + 3];
				data[i + 1] = data[i + 4];
				data[i + 2] = data[i + 5];
				data[i + 3] = data[i + 6];
				data[i + 4] = branch(SelectThread, __OSReschedule + 4);
				data[i + 5] = branch(TICKLE_READ,  __OSReschedule + 5);
				
				for (k = 6; k < __OSRescheduleSig.Length; k++)
					data[i + k] = 0;
				
				print_gecko("Found:[%s] @ %08X\n", __OSRescheduleSig.Name, __OSReschedule);
			}
		}
	}
	
	if (devices[DEVICE_CUR] == &__device_fsp) {
		for (j = 0; j < sizeof(SetExiInterruptMaskSigs) / sizeof(FuncPattern); j++)
			if (SetExiInterruptMaskSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(SetExiInterruptMaskSigs) / sizeof(FuncPattern) && (i = SetExiInterruptMaskSigs[j].offsetFoundAt)) {
			u32 *SetExiInterruptMask = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (SetExiInterruptMask) {
				switch (j) {
					case 0:
						data[i + 22] = 0x3C000000 | ((u32)EXI_HANDLER >> 16);
						data[i + 23] = 0x60000000 | ((u32)EXI_HANDLER & 0xFFFF);
						data[i + 24] = 0x901E0000;	// stw		r0, 0 (r30)
						break;
					case 1:
						data[i + 20] = 0x3C000000 | ((u32)EXI_HANDLER >> 16);
						data[i + 21] = 0x60000000 | ((u32)EXI_HANDLER & 0xFFFF);
						data[i + 22] = 0x90040000;	// stw		r0, 0 (r4)
						break;
				}
				print_gecko("Found:[%s] @ %08X\n", SetExiInterruptMaskSigs[j].Name, SetExiInterruptMask);
			}
		}
	}
	
	for (j = 0; j < sizeof(EXILockSigs) / sizeof(FuncPattern); j++)
		if (EXILockSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(EXILockSigs) / sizeof(FuncPattern) && (i = EXILockSigs[j].offsetFoundAt)) {
		u32 *EXILock = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *EXILockHook;
		
		if (EXILock) {
			switch (j) {
				case 0:
					EXILockHook = getPatchAddr(EXI_LOCKHOOKD);
					
					EXILockHook[0]  =  data[i + 34];
					EXILockHook[1] |= (data[i +  4] >> 5) & 0x1F0000;
					EXILockHook[2] |= (data[i +  5] >> 5) & 0x1F0000;
					EXILockHook[6]  = branchAndLink(EXI_LOCK, EXILockHook + 6);
					
					data[i + 34] = branchAndLink(EXILockHook, EXILock + 34);
					
					EXILockHook[13] |= data[i + 98] & 0x3E00000;
					break;
				case 1:
					EXILockHook = getPatchAddr(EXI_LOCKHOOK);
					
					EXILockHook[0]  =  data[i + 13];
					EXILockHook[1] |= (data[i +  4] >> 5) & 0x1F0000;
					EXILockHook[2] |= (data[i +  9] >> 5) & 0x1F0000;
					EXILockHook[6]  = branchAndLink(EXI_LOCK, EXILockHook + 6);
					
					data[i + 13] = data[i + 12];
					data[i + 12] = branchAndLink(EXILockHook, EXILock + 12);
					
					EXILockHook[13] |= data[i + 53] & 0x3E00000;
					break;
				case 2:
					EXILockHook = getPatchAddr(EXI_LOCKHOOK);
					
					EXILockHook[0]  =  data[i + 12];
					EXILockHook[1] |= (data[i +  4] >> 5) & 0x1F0000;
					EXILockHook[2] |= (data[i +  5] >> 5) & 0x1F0000;
					EXILockHook[6]  = branchAndLink(EXI_LOCK, EXILockHook + 6);
					
					data[i + 12] = branchAndLink(EXILockHook, EXILock + 12);
					
					EXILockHook[13] |= data[i + 53] & 0x3E00000;
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", EXILockSigs[j].Name, EXILock);
		}
	}
	
	for (k = 0; k < sizeof(stateBusySigs) / sizeof(FuncPattern); k++)
		if (stateBusySigs[k].offsetFoundAt) break;
	
	for (j = 0; j < sizeof(ReadSigs) / sizeof(FuncPattern); j++)
		if (ReadSigs[j].offsetFoundAt) break;
	
	if (k < sizeof(stateBusySigs) / sizeof(FuncPattern) && (i = stateBusySigs[k].offsetFoundAt)) {
		u32 *stateBusy = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		s16 executing;
		
		if (stateBusy) {
			switch (k) {
				case 0: executing = (s16)data[i + 80]; break;
				case 1:
				case 2: executing = (s16)data[i + 74]; break;
				case 3: executing = (s16)data[i + 91]; break;
				case 4: executing = (s16)data[i + 86]; break;
				case 5: executing = (s16)data[i + 91]; break;
			}
			if (k == 1)
				data[i + 30] = 0x3C800008;	// lis		r4, 8
			
			print_gecko("Found:[%s] @ %08X\n", stateBusySigs[k].Name, stateBusy);
		}
		
		if (j < sizeof(DVDLowReadSigs) / sizeof(FuncPattern) && (i = DVDLowReadSigs[j].offsetFoundAt)) {
			u32 *DVDLowRead = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (DVDLowRead && stateBusy) {
				if (j == 3) {
					data[i +  30] = data[i +  33];
					data[i +  33] = data[i +  34];
					data[i +  34] = data[i +  37];
					data[i +  35] = data[i +  38];
					data[i +  36] = data[i +  39];
					data[i +  37] = data[i +  40];
					data[i +  38] = data[i +  41];
					data[i +  39] = data[i +  42];
					data[i +  40] = 0x806D0000 | (executing & 0xFFFF);
					data[i +  41] = branchAndLink(PERFORM_READ, DVDLowRead + 41);
					data[i +  42] = 0x3C000008;	// lis		r0, 8
					data[i +  43] = 0x7C180040;	// cmplw	r24, r0
					
					data[i + 138] = data[i + 141];
					data[i + 141] = data[i + 142];
					data[i + 142] = data[i + 145];
					data[i + 143] = data[i + 146];
					data[i + 144] = data[i + 147];
					data[i + 145] = data[i + 148];
					data[i + 146] = data[i + 149];
					data[i + 147] = data[i + 150];
					data[i + 148] = 0x806D0000 | (executing & 0xFFFF);
					data[i + 149] = branchAndLink(PERFORM_READ, DVDLowRead + 149);
					data[i + 150] = 0x3C000008;	// lis		r0, 8
					data[i + 151] = 0x7C180040;	// cmplw	r24, r0
					
					data[i + 221] = data[i + 224];
					data[i + 224] = data[i + 225];
					data[i + 225] = data[i + 228];
					data[i + 226] = data[i + 229];
					data[i + 227] = data[i + 230];
					data[i + 228] = data[i + 231];
					data[i + 229] = data[i + 232];
					data[i + 230] = data[i + 233];
					data[i + 231] = 0x806D0000 | (executing & 0xFFFF);
					data[i + 232] = branchAndLink(PERFORM_READ, DVDLowRead + 232);
					data[i + 233] = 0x3C000008;	// lis		r0, 8
					data[i + 234] = 0x7C180040;	// cmplw	r24, r0
				}
				print_gecko("Found:[%s] @ %08X\n", DVDLowReadSigs[j].Name, DVDLowRead);
			}
		}
		
		if (j < sizeof(ReadSigs) / sizeof(FuncPattern) && (i = ReadSigs[j].offsetFoundAt)) {
			u32 *Read = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (Read && stateBusy) {
				switch (j) {
					case 0:
						data[i + 32] = 0x60000000;	// nop
						data[i + 33] = 0x806D0000 | (executing & 0xFFFF);
						data[i + 34] = branchAndLink(PERFORM_READ, Read + 34);
						data[i + 35] = 0x3C000008;	// lis		r0, 8
						break;
					case 1:
						data[i + 18] = data[i + 17];
						data[i + 17] = data[i + 19];
						data[i + 19] = data[i + 20];
						data[i + 20] = data[i + 21];
						data[i + 21] = data[i + 22];
						data[i + 22] = data[i + 23];
						data[i + 23] = data[i + 25];
						data[i + 24] = data[i + 27];
						data[i + 25] = data[i + 28];
						data[i + 26] = data[i + 29];
						data[i + 27] = data[i + 30];
						data[i + 28] = 0x806D0000 | (executing & 0xFFFF);
						data[i + 29] = branchAndLink(PERFORM_READ, Read + 29);
						data[i + 30] = 0x3C000008;	// lis		r0, 8
						data[i + 31] = 0x7C1D0040;	// cmplw	r29, r0
						break;
					case 2:
						data[i + 20] = data[i + 19];
						data[i + 19] = data[i + 21];
						data[i + 21] = data[i + 22];
						data[i + 22] = data[i + 23];
						data[i + 23] = data[i + 24];
						data[i + 24] = data[i + 25];
						data[i + 25] = data[i + 27];
						data[i + 26] = data[i + 29];
						data[i + 27] = data[i + 30];
						data[i + 28] = data[i + 31];
						data[i + 29] = data[i + 32];
						data[i + 30] = 0x806D0000 | (executing & 0xFFFF);
						data[i + 31] = branchAndLink(PERFORM_READ, Read + 31);
						data[i + 32] = 0x3C000008;	// lis		r0, 8
						data[i + 33] = 0x7C1D0040;	// cmplw	r29, r0
						break;
					case 3:
						data[i + 16] = data[i + 19];
						data[i + 19] = data[i + 20];
						data[i + 20] = data[i + 23];
						data[i + 21] = data[i + 24];
						data[i + 22] = data[i + 25];
						data[i + 23] = data[i + 26];
						data[i + 24] = data[i + 27];
						data[i + 25] = data[i + 28];
						data[i + 26] = 0x806D0000 | (executing & 0xFFFF);
						data[i + 27] = branchAndLink(PERFORM_READ, Read + 27);
						data[i + 28] = 0x3C000008;	// lis		r0, 8
						data[i + 29] = 0x7C1E0040;	// cmplw	r30, r0
						break;
				}
				print_gecko("Found:[%s] @ %08X\n", ReadSigs[j].Name, Read);
			}
		}
	}
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
		if( *(vu32*)(addr+i) != 0x7C0802A6 && *(vu32*)(addr+i+4) != 0x7C0802A6 )
			continue;
		
		FuncPattern fp;
		make_pattern( addr, i / 4, length, &fp );
		
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
		if(compare_pattern(&fp, &ReadProDG)) {
			void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr + i + 0x74)-(u32)(addr));
			print_gecko("Found:[%s] @ %08X for DVD\r\n", ReadProDG.Name, properAddress - 0x74);
			*(vu32*)(addr + i + 0x74) = branchAndLink(READ_REAL_OR_PATCHED, properAddress);
			return 1;
		}
	}
	return 0;
}

u32 Patch_DVDLowLevelRead(void *addr, u32 length, int dataType) {
	void *addr_start = addr;
	void *addr_end = addr+length;
	int patched = 0;
	FuncPattern DSPHandler = {265, 103, 23, 34, 32, 9, 0, 0, "__DSPHandler", 0};
	while(addr_start<addr_end) {
		// Patch the memcpy call in OSExceptionInit to copy our code to 0x80000500 instead of anything else.
		if(*(vu32*)(addr_start) == 0x7C0802A6 || *(vu32*)(addr_start + 4) == 0x7C0802A6)
		{
			if( find_pattern( addr, (u32*)(addr_start)-(u32*)(addr), length, &OSExceptionInitSig ) )
			{
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 488)-(u32)(addr));
				print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 488) = branchAndLink(PATCHED_MEMCPY, properAddress);
				*(vu32*)(addr_start + 496) = 0x38800100;
				*(vu32*)(addr_start + 512) = 0x38800100;
				patched |= 0x100;
			}
			// Debug version of the above
			else if( find_pattern( addr, (u32*)(addr_start)-(u32*)(addr), length, &OSExceptionInitSigDebug ) )
			{
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 520)-(u32)(addr));
				print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 520) = branchAndLink(PATCHED_MEMCPY, properAddress);
				*(vu32*)(addr_start + 528) = 0x38800100;
				*(vu32*)(addr_start + 544) = 0x38800100;
				patched |= 0x100;
			}
			else if( find_pattern( addr, (u32*)(addr_start)-(u32*)(addr), length, &OSExceptionInitSigProDG ) )
			{
				void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start + 460)-(u32)(addr));
				print_gecko("Found:[OSExceptionInit] @ %08X\r\n", properAddress);
				*(vu32*)(addr_start + 460) = branchAndLink(PATCHED_MEMCPY, properAddress);
				*(vu32*)(addr_start + 468) = 0x38800100;
				*(vu32*)(addr_start + 484) = 0x38800100;
				patched |= 0x100;
			}
			// Audio Streaming Hook (only if required)
			else if(GCMDisk.AudioStreaming && find_pattern( addr, (u32*)(addr_start)-(u32*)(addr), length, &DSPHandler ) )
			{	
				if(strncmp((const char*)0x80000000, "PZL", 3)) {	// ZeldaCE uses a special case for MM
					void *properAddress = Calc_ProperAddress(addr, dataType, (u32)(addr_start+0xF8)-(u32)(addr));
					print_gecko("Found:[__DSPHandler] @ %08X\r\n", properAddress);
					*(vu32*)(addr_start+0xF8) = branchAndLink(DSP_HANDLER_HOOK, properAddress);
				}
			}
			// Read variations
			FuncPattern fp;
			make_pattern( addr, (u32*)(addr_start)-(u32*)(addr), length, &fp );
			if(compare_pattern(&fp, &ReadCommon) 			// Common Read function
				|| compare_pattern(&fp, &ReadDebug)			// Debug Read function
				|| compare_pattern(&fp, &ReadUncommon)		// Uncommon Read function
				|| compare_pattern(&fp, &ReadProDG))		// ProDG Read function
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

u8 video_timing_540p60[] = {
	0x0A,0x00,0x02,0x1C,
	0x00,0x10,0x00,0x10,
	0x00,0x00,0x00,0x00,
	0x28,0x28,0x28,0x28,
	0x04,0x60,0x04,0x60,
	0x04,0x60,0x04,0x60,
	0x04,0x66,0x01,0x90,
	0x10,0x15,0x37,0x73,
	0x01,0x63,0x4B,0x00,
	0x01,0x8A
};

u8 video_timing_1080i60[] = {
	0x05,0x00,0x02,0x1C,
	0x00,0x1D,0x00,0x1E,
	0x00,0x01,0x00,0x00,
	0x28,0x29,0x28,0x29,
	0x04,0x60,0x04,0x5F,
	0x04,0x60,0x04,0x5F,
	0x04,0x65,0x01,0x90,
	0x10,0x15,0x37,0x73,
	0x01,0x63,0x4B,0x00,
	0x01,0x8A
};

u8 video_timing_960i[] = {
	0x06,0x00,0x01,0xE0,
	0x00,0x46,0x00,0x47,
	0x00,0x01,0x00,0x00,
	0x18,0x19,0x18,0x19,
	0x04,0x0E,0x04,0x0D,
	0x04,0x0E,0x04,0x0D,
	0x04,0x19,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C
};

u8 video_timing_ntsc50[] = {
	0x06,0x00,0x01,0x20,
	0x00,0x22,0x00,0x23,
	0x00,0x01,0x00,0x00,
	0x0D,0x0C,0x0B,0x0A,
	0x02,0x6B,0x02,0x6A,
	0x02,0x69,0x02,0x6C,
	0x02,0x75,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C,
	0x06,0x00,0x01,0x20,
	0x00,0x24,0x00,0x24,
	0x00,0x00,0x00,0x00,
	0x0D,0x0B,0x0D,0x0B,
	0x02,0x6B,0x02,0x6D,
	0x02,0x6B,0x02,0x6D,
	0x02,0x76,0x01,0xAD,
	0x40,0x47,0x69,0xA2,
	0x01,0x75,0x7A,0x00,
	0x01,0x9C
};

u8 video_timing_540p50[] = {
	0x0A,0x00,0x02,0x1C,
	0x00,0x10,0x00,0x10,
	0x00,0x00,0x00,0x00,
	0x28,0x28,0x28,0x28,
	0x04,0x60,0x04,0x60,
	0x04,0x60,0x04,0x60,
	0x04,0x66,0x01,0xE0,
	0x10,0x15,0x37,0x73,
	0x01,0x13,0x4B,0x00,
	0x01,0x3A
};

u8 video_timing_1080i50[] = {
	0x05,0x00,0x02,0x1C,
	0x00,0x1D,0x00,0x1E,
	0x00,0x01,0x00,0x00,
	0x28,0x29,0x28,0x29,
	0x04,0x60,0x04,0x5F,
	0x04,0x60,0x04,0x5F,
	0x04,0x65,0x01,0xE0,
	0x10,0x15,0x37,0x73,
	0x01,0x13,0x4B,0x00,
	0x01,0x3A
};

u8 video_timing_1152i[] = {
	0x05,0x00,0x02,0x40,
	0x00,0x51,0x00,0x52,
	0x00,0x01,0x00,0x00,
	0x14,0x15,0x14,0x15,
	0x04,0xD8,0x04,0xD7,
	0x04,0xD8,0x04,0xD7,
	0x04,0xE1,0x01,0xB0,
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
	FuncPattern OSSetCurrentContextSig = 
		{ 23, 4, 4, 0, 0, 5, NULL, 0, "OSSetCurrentContext" };
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern OSSleepThreadSigs[2] = {
		{ 93, 32, 14, 9, 8, 8, NULL, 0, "OSSleepThreadD" },
		{ 59, 17, 16, 3, 7, 5, NULL, 0, "OSSleepThread" }
	};
	FuncPattern VISetRegsSigs[2] = {
		{ 54, 21, 6, 3, 3, 12, NULL, 0, "VISetRegsD A" },
		{ 58, 21, 7, 3, 3, 12, NULL, 0, "VISetRegsD B" }
	};
	FuncPattern __VIRetraceHandlerSigs[8] = {
		{ 120, 41,  7, 10, 13,  6, NULL, 0, "__VIRetraceHandlerD A" },
		{ 121, 41,  7, 11, 13,  6, NULL, 0, "__VIRetraceHandlerD B" },
		{ 132, 39,  7, 10, 15, 13, NULL, 0, "__VIRetraceHandler A" },
		{ 137, 42,  8, 11, 15, 13, NULL, 0, "__VIRetraceHandler B" },
		{ 138, 42,  9, 11, 15, 13, NULL, 0, "__VIRetraceHandler C" },
		{ 140, 43, 10, 11, 15, 13, NULL, 0, "__VIRetraceHandler D" },
		{ 147, 46, 13, 10, 15, 15, NULL, 0, "__VIRetraceHandler E" },	// SN Systems ProDG
		{ 157, 50, 10, 15, 16, 13, NULL, 0, "__VIRetraceHandler F" }
	};
	FuncPattern getTimingSigs[8] = {
		{  30,  12,  2,  0,  7,  2,           NULL,                     0, "getTimingD A" },
		{  40,  16,  2,  0, 12,  2, getTimingPatch, getTimingPatch_length, "getTimingD B" },
		{  26,  11,  0,  0,  0,  3,           NULL,                     0, "getTiming A" },
		{  30,  13,  0,  0,  0,  3,           NULL,                     0, "getTiming B" },
		{  36,  15,  0,  0,  0,  4, getTimingPatch, getTimingPatch_length, "getTiming C" },
		{  40,  19,  0,  0,  0,  2,           NULL,                     0, "getTiming D" },
		{ 559, 112, 44, 14, 53, 48,           NULL,                     0, "getTiming E" },	// SN Systems ProDG
		{  42,  20,  0,  0,  0,  2,           NULL,                     0, "getTiming F" }
	};
	FuncPattern __VIInitSigs[7] = {
		{ 159, 44, 5, 4,  4, 16, NULL, 0, "__VIInitD A" },
		{ 161, 44, 5, 4,  5, 16, NULL, 0, "__VIInitD B" },
		{ 122, 21, 8, 1,  5,  5, NULL, 0, "__VIInit A" },
		{ 126, 23, 8, 1,  6,  5, NULL, 0, "__VIInit B" },
		{ 128, 23, 8, 1,  7,  5, NULL, 0, "__VIInit C" },
		{ 176, 56, 4, 0, 21,  3, NULL, 0, "__VIInit D" },	// SN Systems ProDG
		{ 129, 24, 7, 1,  8,  5, NULL, 0, "__VIInit E" }
	};
	FuncPattern AdjustPositionSig = 
		{ 135, 9, 1, 0, 17, 47, NULL, 0, "AdjustPositionD" };
	FuncPattern ImportAdjustingValuesSig = 
		{ 26, 9, 3, 3, 0, 4, NULL, 0, "ImportAdjustingValuesD" };
	FuncPattern VIInitSigs[8] = {
		{ 208, 60, 18, 8,  1, 24, NULL, 0, "VIInitD A" },
		{ 218, 64, 21, 8,  1, 22, NULL, 0, "VIInitD B" },
		{ 269, 37, 18, 7, 18, 57, NULL, 0, "VIInit A" },
		{ 270, 37, 19, 7, 18, 57, NULL, 0, "VIInit B" },
		{ 283, 40, 24, 7, 18, 49, NULL, 0, "VIInit C" },
		{ 286, 41, 25, 7, 18, 49, NULL, 0, "VIInit D" },
		{ 300, 48, 27, 8, 17, 48, NULL, 0, "VIInit E" },
		{ 335, 85, 23, 9, 17, 42, NULL, 0, "VIInit F" }	// SN Systems ProDG
	};
	FuncPattern VIWaitForRetraceSigs[2] = {
		{ 19, 5, 2, 3, 1, 4, NULL, 0, "VIWaitForRetraceD" },
		{ 21, 7, 4, 3, 1, 4, NULL, 0, "VIWaitForRetrace" }
	};
	FuncPattern setFbbRegsSigs[3] = {
		{ 167, 62, 22, 2,  8, 24, NULL, 0, "setFbbRegsD A" },
		{ 181, 54, 34, 0, 10, 16, NULL, 0, "setFbbRegs A" },
		{ 177, 51, 34, 0, 10, 16, NULL, 0, "setFbbRegs B" }	// SN Systems ProDG
	};
	FuncPattern setVerticalRegsSigs[4] = {
		{ 118, 17, 11, 0, 4, 23, NULL, 0, "setVerticalRegsD A" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs A" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs B" },
		{ 114, 19, 13, 0, 4, 25, NULL, 0, "setVerticalRegs C" }	// SN Systems ProDG
	};
	FuncPattern VIConfigureSigs[11] = {
		{ 280,  74, 15, 20, 21, 20, NULL, 0, "VIConfigureD A" },
		{ 314,  86, 15, 21, 27, 20, NULL, 0, "VIConfigureD B" },
		{ 428,  90, 43,  6, 32, 60, NULL, 0, "VIConfigure A" },
		{ 420,  87, 41,  6, 31, 60, NULL, 0, "VIConfigure B" },
		{ 423,  88, 41,  6, 31, 61, NULL, 0, "VIConfigure C" },
		{ 464, 100, 43, 13, 34, 61, NULL, 0, "VIConfigure D" },
		{ 462,  99, 43, 12, 34, 61, NULL, 0, "VIConfigure E" },
		{ 487, 105, 44, 12, 38, 63, NULL, 0, "VIConfigure F" },
		{ 522, 111, 44, 13, 53, 64, NULL, 0, "VIConfigure G" },
		{ 559, 112, 44, 14, 53, 48, NULL, 0, "VIConfigure H" },	// SN Systems ProDG
		{ 514, 110, 44, 13, 49, 63, NULL, 0, "VIConfigure I" }
	};
	FuncPattern VIConfigurePanSigs[2] = {
		{ 100, 26,  3, 9,  6,  4, NULL, 0, "VIConfigurePanD" },
		{ 229, 40, 11, 4, 25, 35, NULL, 0, "VIConfigurePan" }
	};
	FuncPattern VISetBlackSigs[3] = {
		{ 30, 6, 5, 3, 0, 3, NULL, 0, "VISetBlackD A" },
		{ 31, 7, 6, 3, 0, 3, NULL, 0, "VISetBlack A" },
		{ 30, 7, 6, 3, 0, 4, NULL, 0, "VISetBlack B" }	// SN Systems ProDG
	};
	FuncPattern VIGetRetraceCountSig = 
		{ 2, 1, 0, 0, 0, 0, NULL, 0, "VIGetRetraceCount" };
	FuncPattern GetCurrentDisplayPositionSig = 
		{ 15, 3, 2, 0, 0, 1, NULL, 0, "GetCurrentDisplayPosition" };
	FuncPattern getCurrentHalfLineSigs[2] = {
		{ 26, 9, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD A" },
		{ 24, 7, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD B" }
	};
	FuncPattern getCurrentFieldEvenOddSigs[5] = {
		{ 33,  7, 2, 3, 4, 5, NULL, 0, "getCurrentFieldEvenOddD A" },
		{ 15,  5, 2, 1, 2, 3, NULL, 0, "getCurrentFieldEvenOddD B" },
		{ 47, 14, 2, 2, 4, 8, NULL, 0, "getCurrentFieldEvenOdd A" },
		{ 26,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd B" },
		{ 26,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd C" }	// SN Systems ProDG
	};
	FuncPattern VIGetNextFieldSigs[4] = {
		{ 20,  4, 2, 3, 0, 3, NULL, 0, "VIGetNextFieldD A" },
		{ 61, 16, 4, 4, 4, 9, NULL, 0, "VIGetNextField A" },
		{ 42, 10, 3, 2, 2, 8, NULL, 0, "VIGetNextField B" },
		{ 39, 13, 4, 3, 2, 6, NULL, 0, "VIGetNextField C" }
	};
	FuncPattern VIGetDTVStatusSigs[2] = {
		{ 17, 3, 2, 2, 0, 3, NULL, 0, "VIGetDTVStatusD" },
		{ 15, 4, 3, 2, 0, 2, NULL, 0, "VIGetDTVStatus" }
	};
	FuncPattern __GXInitGXSigs[9] = {
		{ 1130, 567, 66, 133, 46, 46, NULL, 0, "__GXInitGXD A" },
		{  544, 319, 33, 109, 18,  5, NULL, 0, "__GXInitGXD B" },
		{  976, 454, 81, 119, 43, 36, NULL, 0, "__GXInitGX A" },
		{  530, 307, 35, 107, 18, 10, NULL, 0, "__GXInitGX B" },
		{  545, 310, 35, 108, 24, 11, NULL, 0, "__GXInitGX C" },
		{  561, 313, 36, 110, 28, 11, NULL, 0, "__GXInitGX D" },
		{  546, 293, 37, 110,  7,  9, NULL, 0, "__GXInitGX E" },	// SN Systems ProDG
		{  549, 289, 38, 110,  7,  9, NULL, 0, "__GXInitGX F" },	// SN Systems ProDG
		{  590, 333, 34, 119, 28, 11, NULL, 0, "__GXInitGX G" }
	};
	FuncPattern GXTokenInterruptHandlerSigs[5] = {
		{ 33, 11, 3, 4, 1, 2, NULL, 0, "GXTokenInterruptHandlerD A" },
		{ 34, 12, 4, 4, 1, 3, NULL, 0, "GXTokenInterruptHandler A" },
		{ 33, 11, 4, 4, 0, 4, NULL, 0, "GXTokenInterruptHandler B" },	// SN Systems ProDG
		{ 36,  9, 5, 4, 0, 4, NULL, 0, "GXTokenInterruptHandler C" },	// SN Systems ProDG
		{ 34, 13, 4, 4, 1, 3, NULL, 0, "GXTokenInterruptHandler D" }
	};
	FuncPattern GXAdjustForOverscanSigs[4] = {
		{ 57,  6,  4, 0, 3, 11, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscanD A" },
		{ 72, 17, 15, 0, 3,  5, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan A" },
		{ 63, 11,  8, 1, 2, 10, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan B" },	// SN Systems ProDG
		{ 81, 17, 15, 0, 3,  7, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan C" }
	};
	FuncPattern GXSetDispCopySrcSigs[5] = {
		{ 104, 44, 10, 5, 5, 6, NULL, 0, "GXSetDispCopySrcD A" },
		{  48, 19,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc A" },
		{  36,  9,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc B" },
		{  14,  2,  2, 0, 0, 2, NULL, 0, "GXSetDispCopySrc C" },	// SN Systems ProDG
		{  31, 11,  8, 0, 0, 0, NULL, 0, "GXSetDispCopySrc D" }
	};
	FuncPattern GXSetDispCopyYScaleSigs[7] = {
		{ 100, 33, 8, 8, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD A" },
		{  85, 32, 4, 6, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD B" },
		{  47, 15, 8, 2, 0, 4, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale A" },
		{  53, 17, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale B" },
		{  50, 14, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale C" },
		{  44,  8, 4, 1, 2, 3, GXSetDispCopyYScalePatch2, GXSetDispCopyYScalePatch2_length, "GXSetDispCopyYScale D" },	// SN Systems ProDG
		{  51, 16, 4, 1, 5, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale E" }
	};
	FuncPattern GXSetCopyFilterSigs[5] = {
		{ 567, 183, 44, 32, 36, 38, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilterD A" },
		{ 138,  15,  7,  0,  4,  5, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter A" },
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter B" },	// SN Systems ProDG
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter C" },	// SN Systems ProDG
		{ 130,  25,  7,  0,  4,  0, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter D" }
	};
	FuncPattern GXSetDispCopyGammaSigs[4] = {
		{ 34, 12, 3, 2, 2, 3, NULL, 0, "GXSetDispCopyGammaD A" },
		{  7,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma A" },
		{  6,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma B" },	// SN Systems ProDG
		{  5,  2, 1, 0, 0, 0, NULL, 0, "GXSetDispCopyGamma C" }
	};
	FuncPattern GXCopyDispSigs[5] = {
		{ 149, 62,  3, 14, 14, 3, NULL, 0, "GXCopyDispD A" },
		{  92, 34, 14,  0,  3, 1, NULL, 0, "GXCopyDisp A" },
		{  87, 29, 14,  0,  3, 1, NULL, 0, "GXCopyDisp B" },
		{  69, 15, 12,  0,  1, 1, NULL, 0, "GXCopyDisp C" },	// SN Systems ProDG
		{  90, 35, 14,  0,  3, 0, NULL, 0, "GXCopyDisp D" }
	};
	FuncPattern GXSetBlendModeSigs[5] = {
		{ 154, 66, 10, 7, 9, 17, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendModeD A" },
		{  65, 20,  8, 0, 2,  6, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendMode A" },
		{  21,  6,  2, 0, 0,  2, GXSetBlendModePatch2, GXSetBlendModePatch2_length, "GXSetBlendMode B" },
		{  36,  2,  2, 0, 0,  6, GXSetBlendModePatch3, GXSetBlendModePatch3_length, "GXSetBlendMode C" },	// SN Systems ProDG
		{  38,  2,  2, 0, 0,  8, GXSetBlendModePatch3, GXSetBlendModePatch3_length, "GXSetBlendMode D" }	// SN Systems ProDG
	};
	FuncPattern __GXSetViewportSig = 
		{ 36, 15, 7, 0, 0, 0, GXSetViewportPatch, GXSetViewportPatch_length, "__GXSetViewport" };
	FuncPattern GXSetViewportJitterSigs[5] = {
		{ 193, 76, 22, 4, 15, 22, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitterD A" },
		{  71, 20, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter A" },
		{  65, 14, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter B" },
		{  31,  6, 10, 1,  0,  4,                     NULL,                               0, "GXSetViewportJitter C" },	// SN Systems ProDG
		{  22,  6,  8, 1,  0,  2,                     NULL,                               0, "GXSetViewportJitter D" }
	};
	FuncPattern GXSetViewportSigs[5] = {
		{ 21, 9, 8, 1, 0, 2, NULL, 0, "GXSetViewportD A" },
		{  9, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport A" },
		{  9, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport B" },
		{  2, 1, 0, 0, 1, 0, NULL, 0, "GXSetViewport C" },	// SN Systems ProDG
		{ 18, 5, 8, 1, 0, 2, NULL, 0, "GXSetViewport D" }
	};
	u32 _SDA2_BASE_ = 0, _SDA_BASE_ = 0;
	
	switch (swissSettings.gameVMode) {
		case 6: case 13: case 7: case 14:
			if (!swissSettings.forceHScale)
				swissSettings.forceHScale = 1;
		case 3: case 10: case 4: case 11:
			swissSettings.forceVOffset &= ~1;
		case 2: case  9: case 5: case 12:
			if (!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
	}
	
	if (swissSettings.gameVMode == 3 || swissSettings.gameVMode == 10)
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
		if (!_SDA2_BASE_ && !_SDA_BASE_) {
			if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
				(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
				(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
				(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
				_SDA2_BASE_ = (data[i + 0] << 16) | (u16)data[i + 1];
				_SDA_BASE_  = (data[i + 2] << 16) | (u16)data[i + 3];
				i += 4;
			}
			continue;
		}
		if ((data[i - 1] != 0x4C000064 && data[i - 1] != 0x4E800020) ||
			(data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6))
			continue;
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
		for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++) {
			if (!__VIRetraceHandlerSigs[j].offsetFoundAt && compare_pattern(&fp, &__VIRetraceHandlerSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  45, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  61, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 114, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  74, length, &VISetRegsSigs[0]))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  45, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  61, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 115, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  74, length, &VISetRegsSigs[1]))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 126, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 60, length, &getCurrentFieldEvenOddSigs[2]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[2], &VIGetRetraceCountSig);
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 131, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 60, length, &getCurrentFieldEvenOddSigs[3]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[3], &VIGetRetraceCountSig);
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 132, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 63, length, &getCurrentFieldEvenOddSigs[3]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[3], &VIGetRetraceCountSig);
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 134, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 63, length, &getCurrentFieldEvenOddSigs[3]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[3], &VIGetRetraceCountSig);
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 139, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 68, length, &getCurrentFieldEvenOddSigs[4]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[4], &VIGetRetraceCountSig);
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  44, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  67, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 151, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 80, length, &getCurrentFieldEvenOddSigs[3]))
							find_pattern_before(data, length, &getCurrentFieldEvenOddSigs[3], &VIGetRetraceCountSig);
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(VIInitSigs) / sizeof(FuncPattern); j++) {
			if (!VIInitSigs[j].offsetFoundAt && compare_pattern(&fp, &VIInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  15, length, &__VIInitSigs[0]) &&
							findx_pattern(data, dataType, i + 120, length, &ImportAdjustingValuesSig) &&
							findx_pattern(data, dataType, i + 136, length, &AdjustPositionSig))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  15, length, &__VIInitSigs[1]) &&
							findx_pattern(data, dataType, i + 122, length, &ImportAdjustingValuesSig) &&
							findx_pattern(data, dataType, i + 160, length, &AdjustPositionSig))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
					case 3:
						if (findx_pattern(data, dataType, i +  16, length, &__VIInitSigs[2]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  19, length, &__VIInitSigs[2]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  19, length, &__VIInitSigs[2]) ||
							findx_pattern(data, dataType, i +  19, length, &__VIInitSigs[3]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  25, length, &__VIInitSigs[4]) ||
							findx_pattern(data, dataType, i +  25, length, &__VIInitSigs[6]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  18, length, &__VIInitSigs[5]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
			if (VIInitSigs[j].offsetFoundAt && i == VIInitSigs[j].offsetFoundAt + VIInitSigs[j].Length) {
				for (k = 0; k < sizeof(VIWaitForRetraceSigs) / sizeof(FuncPattern); k++) {
					if (!VIWaitForRetraceSigs[k].offsetFoundAt && compare_pattern(&fp, &VIWaitForRetraceSigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 13, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  8, length, &OSSleepThreadSigs[0]))
									VIWaitForRetraceSigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 14, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  9, length, &OSSleepThreadSigs[1]))
									VIWaitForRetraceSigs[k].offsetFoundAt = i;
								break;
						}
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(VIConfigureSigs) / sizeof(FuncPattern); j++) {
			if (!VIConfigureSigs[j].offsetFoundAt && compare_pattern(&fp, &VIConfigureSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  53, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 274, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 135, length, &AdjustPositionSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 131, length, &getTimingSigs[0]);
						findx_pattern(data, dataType, i + 261, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 272, length, &setVerticalRegsSigs[0]);
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  59, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 308, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 143, length, &AdjustPositionSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 139, length, &getTimingSigs[1]);
						findx_pattern(data, dataType, i + 295, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 306, length, &setVerticalRegsSigs[0]);
						break;
					case 2:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 422, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  77, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 409, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 420, length, &setVerticalRegsSigs[1]);
						break;
					case 3:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 414, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  69, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 401, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 412, length, &setVerticalRegsSigs[1]);
						break;
					case 4:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 417, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  72, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 404, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 415, length, &setVerticalRegsSigs[1]);
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 458, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 104, length, &getTimingSigs[3]);
						findx_pattern(data, dataType, i + 445, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 456, length, &setVerticalRegsSigs[1]);
						break;
					case 6:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 456, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 107, length, &getTimingSigs[3]);
						findx_pattern(data, dataType, i + 443, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 454, length, &setVerticalRegsSigs[1]);
						break;
					case 7:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 481, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 123, length, &getTimingSigs[4]);
						findx_pattern(data, dataType, i + 468, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 479, length, &setVerticalRegsSigs[1]);
						break;
					case 8:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 516, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[5]);
						findx_pattern(data, dataType, i + 503, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 514, length, &setVerticalRegsSigs[2]);
						break;
					case 9:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 552, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = getTimingSigs[6].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 537, length, &setFbbRegsSigs[2]);
						findx_pattern(data, dataType, i + 550, length, &setVerticalRegsSigs[3]);
						break;
					case 10:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 508, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[7]);
						findx_pattern(data, dataType, i + 495, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 506, length, &setVerticalRegsSigs[2]);
						break;
				}
				break;
			}
			if (VIConfigureSigs[j].offsetFoundAt && i == VIConfigureSigs[j].offsetFoundAt + VIConfigureSigs[j].Length) {
				for (k = 0; k < sizeof(VIConfigurePanSigs) / sizeof(FuncPattern); k++) {
					if (!VIConfigurePanSigs[k].offsetFoundAt && compare_pattern(&fp, &VIConfigurePanSigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i +  31, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i +  94, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  59, length, &AdjustPositionSig))
									VIConfigurePanSigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i +  10, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 223, length, &OSRestoreInterruptsSig))
									VIConfigurePanSigs[k].offsetFoundAt = i;
								break;
						}
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(setVerticalRegsSigs) / sizeof(FuncPattern); j++) {
			if (setVerticalRegsSigs[j].offsetFoundAt) {
				for (k = 0; k < sizeof(VISetBlackSigs) / sizeof(FuncPattern); k++) {
					if (!VISetBlackSigs[k].offsetFoundAt && compare_pattern(&fp, &VISetBlackSigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 24, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i + 22, length, &setVerticalRegsSigs[j]))
									VISetBlackSigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i +  8, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 24, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i + 22, length, &setVerticalRegsSigs[j]))
									VISetBlackSigs[k].offsetFoundAt = i;
								break;
							case 2:
								if (findx_pattern(data, dataType, i +  6, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 23, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i + 21, length, &setVerticalRegsSigs[j]))
									VISetBlackSigs[k].offsetFoundAt = i;
								break;
						}
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern); j++) {
			if (!getCurrentFieldEvenOddSigs[j].offsetFoundAt && compare_pattern(&fp, &getCurrentFieldEvenOddSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 22, length, &getCurrentHalfLineSigs[0])) {
							find_pattern_before(data, length, &getCurrentHalfLineSigs[0], &VIGetRetraceCountSig);
							getCurrentFieldEvenOddSigs[j].offsetFoundAt = i;
						}
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  3, length, &getCurrentHalfLineSigs[1])) {
							find_pattern_before(data, length, &getCurrentHalfLineSigs[1], &VIGetRetraceCountSig);
							getCurrentFieldEvenOddSigs[j].offsetFoundAt = i;
						}
						break;
				}
				break;
			}
			if (getCurrentFieldEvenOddSigs[j].offsetFoundAt && i == getCurrentFieldEvenOddSigs[j].offsetFoundAt + getCurrentFieldEvenOddSigs[j].Length) {
				for (k = 0; k < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern); k++) {
					if (!VIGetNextFieldSigs[k].offsetFoundAt && compare_pattern(&fp, &VIGetNextFieldSigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i +  9, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  6, length, &getCurrentFieldEvenOddSigs[j]))
									VIGetNextFieldSigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 48, length, &OSRestoreInterruptsSig))
									VIGetNextFieldSigs[k].offsetFoundAt = i;
								break;
							case 2:
								if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 30, length, &OSRestoreInterruptsSig))
									VIGetNextFieldSigs[k].offsetFoundAt = i;
								break;
							case 3:
								if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 26, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  9, length, &GetCurrentDisplayPositionSig)) {
									find_pattern_before(data, length, &GetCurrentDisplayPositionSig, &VIGetRetraceCountSig);
									VIGetNextFieldSigs[k].offsetFoundAt = i;
								}
								break;
						}
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern); j++) {
			if (!VIGetDTVStatusSigs[j].offsetFoundAt && compare_pattern(&fp, &VIGetDTVStatusSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 10, length, &OSRestoreInterruptsSig))
							VIGetDTVStatusSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  8, length, &OSRestoreInterruptsSig))
							VIGetDTVStatusSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern); j++) {
			if (!__GXInitGXSigs[j].offsetFoundAt && compare_pattern(&fp, &__GXInitGXSigs[j])) {
				__GXInitGXSigs[j].offsetFoundAt = i;
				
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 1069, length, &GXSetDispCopySrcSigs[0]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[0], &GXAdjustForOverscanSigs[0]);
						
						findx_pattern(data, dataType, i + 1088, length, &GXSetDispCopyYScaleSigs[0]);
						findx_pattern(data, dataType, i + 1095, length, &GXSetCopyFilterSigs[0]);
						
						if (findx_pattern(data, dataType, i + 1097, length, &GXSetDispCopyGammaSigs[0]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[0], &GXCopyDispSigs[0]);
						
						findx_pattern(data, dataType, i + 1033, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i +  727, length, &GXSetViewportSigs[0]);
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 480, length, &GXSetDispCopySrcSigs[0]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[0], &GXAdjustForOverscanSigs[0]);
						
						findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[1]);
						findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[0]);
						
						if (findx_pattern(data, dataType, i + 508, length, &GXSetDispCopyGammaSigs[0]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[0], &GXCopyDispSigs[0]);
						
						findx_pattern(data, dataType, i + 444, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[0]);
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 917, length, &GXSetDispCopySrcSigs[1]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[1], &GXAdjustForOverscanSigs[1]);
						
						findx_pattern(data, dataType, i + 934, length, &GXSetDispCopyYScaleSigs[2]);
						findx_pattern(data, dataType, i + 941, length, &GXSetCopyFilterSigs[1]);
						
						if (findx_pattern(data, dataType, i + 943, length, &GXSetDispCopyGammaSigs[1]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[1], &GXCopyDispSigs[1]);
						
						findx_pattern(data, dataType, i + 881, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 553, length, &GXSetViewportSigs[1]);
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 467, length, &GXSetDispCopySrcSigs[1]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[1], &GXAdjustForOverscanSigs[1]);
						
						findx_pattern(data, dataType, i + 484, length, &GXSetDispCopyYScaleSigs[2]);
						findx_pattern(data, dataType, i + 491, length, &GXSetCopyFilterSigs[1]);
						
						if (findx_pattern(data, dataType, i + 493, length, &GXSetDispCopyGammaSigs[1]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[1], &GXCopyDispSigs[1]);
						
						findx_pattern(data, dataType, i + 431, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 186, length, &GXSetViewportSigs[1]);
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 482, length, &GXSetDispCopySrcSigs[1]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[1], &GXAdjustForOverscanSigs[1]);
						
						if (data[i + __GXInitGXSigs[j].Length - 2] == 0x7C0803A6)
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[3]);
						else
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[2]);
						
						findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[1]);
						
						if (findx_pattern(data, dataType, i + 508, length, &GXSetDispCopyGammaSigs[1]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[1], &GXCopyDispSigs[1]);
						
						findx_pattern(data, dataType, i + 446, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[1]);
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 497, length, &GXSetDispCopySrcSigs[2]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[2], &GXAdjustForOverscanSigs[1]);
						
						findx_pattern(data, dataType, i + 514, length, &GXSetDispCopyYScaleSigs[4]);
						findx_pattern(data, dataType, i + 521, length, &GXSetCopyFilterSigs[1]);
						
						if (findx_pattern(data, dataType, i + 523, length, &GXSetDispCopyGammaSigs[1]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[1], &GXCopyDispSigs[2]);
						
						findx_pattern(data, dataType, i + 461, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[2]);
						break;
					case 6:
						if (findx_pattern(data, dataType, i + 473, length, &GXSetDispCopySrcSigs[3]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[3], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 492, length, &GXSetDispCopyYScaleSigs[5]);
						findx_pattern(data, dataType, i + 499, length, &GXSetCopyFilterSigs[2]);
						
						if (findx_pattern(data, dataType, i + 501, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[3]);
						
						findx_pattern(data, dataType, i + 433, length, &GXSetBlendModeSigs[3]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[3]);
						break;
					case 7:
						if (findx_pattern(data, dataType, i + 475, length, &GXSetDispCopySrcSigs[3]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[3], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 494, length, &GXSetDispCopyYScaleSigs[5]);
						findx_pattern(data, dataType, i + 501, length, &GXSetCopyFilterSigs[3]);
						
						if (findx_pattern(data, dataType, i + 503, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[3]);
						
						findx_pattern(data, dataType, i + 435, length, &GXSetBlendModeSigs[4]);
						findx_pattern(data, dataType, i + 204, length, &GXSetViewportSigs[3]);
						break;
					case 8:
						if (findx_pattern(data, dataType, i + 526, length, &GXSetDispCopySrcSigs[4]))
							find_pattern_before(data, length, &GXSetDispCopySrcSigs[4], &GXAdjustForOverscanSigs[3]);
						
						findx_pattern(data, dataType, i + 543, length, &GXSetDispCopyYScaleSigs[6]);
						findx_pattern(data, dataType, i + 550, length, &GXSetCopyFilterSigs[4]);
						
						if (findx_pattern(data, dataType, i + 552, length, &GXSetDispCopyGammaSigs[3]))
							find_pattern_after(data, length, &GXSetDispCopyGammaSigs[3], &GXCopyDispSigs[4]);
						
						findx_pattern(data, dataType, i + 490, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[4]);
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(GXTokenInterruptHandlerSigs) / sizeof(FuncPattern); j++) {
			if (!GXTokenInterruptHandlerSigs[j].offsetFoundAt && compare_pattern(&fp, &GXTokenInterruptHandlerSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 13, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 21, length, &OSSetCurrentContextSig))
							GXTokenInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 1:
					case 2:
					case 4:
						if (findx_pattern(data, dataType, i + 14, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 22, length, &OSSetCurrentContextSig))
							GXTokenInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 16, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 24, length, &OSSetCurrentContextSig))
							GXTokenInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		i += fp.Length - 1;
	}
	
	for (k = 0; k < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern); k++)
		if (getCurrentFieldEvenOddSigs[k].offsetFoundAt) break;
	
	for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++)
		if (__VIRetraceHandlerSigs[j].offsetFoundAt) break;
	
	if (k < sizeof(getCurrentFieldEvenOddSigs) / sizeof(FuncPattern) && (i = getCurrentFieldEvenOddSigs[k].offsetFoundAt)) {
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
		
		if (j < sizeof(VISetRegsSigs) / sizeof(FuncPattern) && (i = VISetRegsSigs[j].offsetFoundAt)) {
			u32 *VISetRegs = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (VISetRegs && getCurrentFieldEvenOdd) {
				switch (j) {
					case 0:
						data[i +  8] = 0x2C030000;	// cmpwi	r3, 0
						data[i +  9] = 0x4082009C;	// bne		+39
						break;
					case 1:
						data[i + 10] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 11] = 0x408200A4;	// bne		+41
						break;
				}
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
					switch (j) {
						case 0:
							data[i +  4] = branchAndLink(getCurrentFieldEvenOdd, VISetRegs + 4);
							data[i +  5] = 0x2C030000;	// cmpwi	r3, 0
							data[i +  6] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i +  7] = 0x3C800000 | (VAR_AREA >> 16);
							data[i +  8] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 1:
							data[i +  6] = branchAndLink(getCurrentFieldEvenOdd, VISetRegs + 6);
							data[i +  7] = 0x2C030000;	// cmpwi	r3, 0
							data[i +  8] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i +  9] = 0x3C800000 | (VAR_AREA >> 16);
							data[i + 10] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
					}
				}
				print_gecko("Found:[%s] @ %08X\n", VISetRegsSigs[j].Name, VISetRegs);
			}
		}
		
		if (j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern) && (i = __VIRetraceHandlerSigs[j].offsetFoundAt)) {
			u32 *__VIRetraceHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *__VIRetraceHandlerHook;
			
			if (__VIRetraceHandler && getCurrentFieldEvenOdd) {
				switch (j) {
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
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
					switch (j) {
						case 2:
						case 3:
							data[i + 57] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 57);
							data[i + 58] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 59] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 60] = 0x3C800000 | (VAR_AREA >> 16);
							data[i + 61] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 4:
						case 5:
							data[i + 60] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 60);
							data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 62] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 63] = 0x3C800000 | (VAR_AREA >> 16);
							data[i + 64] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 6:
							data[i + 65] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 65);
							data[i + 66] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 67] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 68] = 0x3C800000 | (VAR_AREA >> 16);
							data[i + 69] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 7:
							data[i + 77] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 77);
							data[i + 78] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 79] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 80] = 0x3C800000 | (VAR_AREA >> 16);
							data[i + 81] = 0x98640000 | (VAR_CURRENT_FIELD & 0xFFFF);
							break;
					}
				}
				if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11 ||
					swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13) {
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
					data[i + __VIRetraceHandlerSigs[j].Length - 1] = branch(__VIRetraceHandlerHook, __VIRetraceHandler + __VIRetraceHandlerSigs[j].Length - 1);
				}
				print_gecko("Found:[%s] @ %08X\n", __VIRetraceHandlerSigs[j].Name, __VIRetraceHandler);
			}
		}
	}
	
	for (j = 0; j < sizeof(getTimingSigs) / sizeof(FuncPattern); j++)
		if (getTimingSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(getTimingSigs) / sizeof(FuncPattern) && (i = getTimingSigs[j].offsetFoundAt)) {
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
			
			if (swissSettings.gameVMode == 7 || swissSettings.gameVMode == 14)
				memcpy(timingTable + 6, video_timing_540p60, sizeof(video_timing_540p60));
			else if (swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13)
				memcpy(timingTable + 6, video_timing_1080i60, sizeof(video_timing_1080i60));
			else if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11)
				memcpy(timingTable + 6, video_timing_960i, sizeof(video_timing_960i));
			
			if (swissSettings.aveCompat == 3)
				memcpy(timingTable + 2, video_timing_ntsc50, sizeof(video_timing_ntsc50));
			
			if (j == 1 || j >= 4) {
				if (swissSettings.gameVMode == 7 || swissSettings.gameVMode == 14)
					memcpy(timingTable + 7, video_timing_540p50, sizeof(video_timing_540p50));
				else if (swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13)
					memcpy(timingTable + 7, video_timing_1080i50, sizeof(video_timing_1080i50));
				else if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11)
					memcpy(timingTable + 7, video_timing_1152i, sizeof(video_timing_1152i));
				else
					memcpy(timingTable + 7, video_timing_576p, sizeof(video_timing_576p));
				
				jump = jumpTable[3];
				jumpTable[3] = jumpTable[6];
				jumpTable[6] = jump;
			} else if (swissSettings.gameVMode >= 8 && swissSettings.gameVMode <= 14) {
				if (swissSettings.gameVMode == 14)
					memcpy(timingTable + 6, video_timing_540p50, sizeof(video_timing_540p50));
				else if (swissSettings.gameVMode == 13)
					memcpy(timingTable + 6, video_timing_1080i50, sizeof(video_timing_1080i50));
				else if (swissSettings.gameVMode == 11)
					memcpy(timingTable + 6, video_timing_1152i, sizeof(video_timing_1152i));
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
			data[i +  30] = 0xA89F00F2;	// lha		r4, 242 (r31)
			data[i +  31] = data[i + 33];
			data[i +  32] = 0x549E07FE;	// clrlwi	r30, r4, 31
			data[i +  33] = 0x7FC0F214;	// add		r30, r30, r0
			data[i +  43] = 0x57C007FE;	// clrlwi	r0, r30, 31
			data[i +  44] = 0x5466083C;	// slwi		r6, r3, 1
			data[i +  45] = 0x7CC03050;	// sub		r6, r6, r0
			data[i +  53] = 0x57C007FE;	// clrlwi	r0, r30, 31
			data[i +  54] = 0x5466083C;	// slwi		r6, r3, 1
			data[i +  55] = 0x7CC03050;	// sub		r6, r6, r0
			data[i +  94] = 0x57C007FE;	// clrlwi	r0, r30, 31
			data[i +  95] = 0x5466083C;	// slwi		r6, r3, 1
			data[i +  96] = 0x7CC03050;	// sub		r6, r6, r0
			data[i + 104] = 0x57C007FE;	// clrlwi	r0, r30, 31
			data[i + 105] = 0x5466083C;	// slwi		r6, r3, 1
			data[i + 106] = 0x7CC03050;	// sub		r6, r6, r0
			
			print_gecko("Found:[%s] @ %08X\n", AdjustPositionSig.Name, AdjustPosition);
		}
	}
	
	if ((i = ImportAdjustingValuesSig.offsetFoundAt)) {
		u32 *ImportAdjustingValues = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (ImportAdjustingValues) {
			data[i + 17] = 0x38000000 | (swissSettings.forceVOffset & 0xFFFF);
			
			print_gecko("Found:[%s] @ %08X\n", ImportAdjustingValuesSig.Name, ImportAdjustingValues);
		}
	}
	
	for (k = 0; k < sizeof(VIInitSigs) / sizeof(FuncPattern); k++)
		if (VIInitSigs[k].offsetFoundAt) break;
	
	for (j = 0; j < sizeof(VIWaitForRetraceSigs) / sizeof(FuncPattern); j++)
		if (VIWaitForRetraceSigs[j].offsetFoundAt) break;
	
	if (k < sizeof(VIInitSigs) / sizeof(FuncPattern) && (i = VIInitSigs[k].offsetFoundAt)) {
		u32 *VIInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		s16 flushFlag;
		
		if (VIInit) {
			switch (k) {
				case 0: flushFlag = (s16)data[i + 29]; break;
				case 1: flushFlag = (s16)data[i + 31]; break;
				case 2: flushFlag = (s16)data[i + 28]; break;
				case 3: flushFlag = (s16)data[i + 29]; break;
				case 4:
				case 5: flushFlag = (s16)data[i + 31]; break;
				case 6: flushFlag = (s16)data[i + 37]; break;
				case 7: flushFlag = (s16)data[i + 35]; break;
			}
			print_gecko("Found:[%s] @ %08X\n", VIInitSigs[k].Name, VIInit);
		}
		
		if (j < sizeof(VIWaitForRetraceSigs) / sizeof(FuncPattern) && (i = VIWaitForRetraceSigs[j].offsetFoundAt)) {
			u32 *VIWaitForRetrace = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (VIWaitForRetrace && VIInit) {
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
					switch (j) {
						case 0:
							data[i +  9] = 0x800D0000 | (flushFlag & 0xFFFF);
							data[i + 10] = 0x28000001;	// cmplwi	r0, 1
							break;
						case 1:
							data[i + 10] = 0x800D0000 | (flushFlag & 0xFFFF);
							data[i + 11] = 0x28000001;	// cmplwi	r0, 1
							break;
					}
				}
				print_gecko("Found:[%s] @ %08X\n", VIWaitForRetraceSigs[j].Name, VIWaitForRetrace);
			}
		}
	}
	
	for (j = 0; j < sizeof(setFbbRegsSigs) / sizeof(FuncPattern); j++)
		if (setFbbRegsSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(setFbbRegsSigs) / sizeof(FuncPattern) && (i = setFbbRegsSigs[j].offsetFoundAt)) {
		u32 *setFbbRegs = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (setFbbRegs) {
			switch (j) {
				case 0:
					data[i - 20] = 0x7D0B0734;	// extsh	r11, r8
					data[i - 15] = 0x2C000000;	// cmpwi	r0, 0
					data[i - 14] = 0x41820014;	// beq		+5
					data[i + 16] = 0xA91F000A;	// lha		r8, 10 (r31)
					data[i + 28] = 0xA91F000A;	// lha		r8, 10 (r31)
					break;
				case 1:
					data[i + 11] = 0xA983000A;	// lha		r12, 10 (r3)
					data[i + 27] = 0x2C000000;	// cmpwi	r0, 0
					data[i + 28] = 0x41820014;	// beq		+5
					data[i + 49] = 0xA983000A;	// lha		r12, 10 (r3)
					data[i + 65] = 0x2C000000;	// cmpwi	r0, 0
					data[i + 66] = 0x41820014;	// beq		+5
					break;
				case 2:
					data[i + 10] = 0xA983000A;	// lha		r12, 10 (r3)
					data[i + 48] = 0xA983000A;	// lha		r12, 10 (r3)
					break;
			}
			if (swissSettings.gameVMode == 3 || swissSettings.gameVMode == 10) {
				switch (j) {
					case 0:
						data[i - 22] = 0x7C000378;	// mr		r0, r0
						break;
					case 1:
						data[i + 21] = 0x7C080378;	// mr		r8, r0
						data[i + 59] = 0x7C080378;	// mr		r8, r0
						break;
					case 2:
						data[i + 19] = 0x7C090378;	// mr		r9, r0
						data[i + 57] = 0x7C090378;	// mr		r9, r0
						break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", setFbbRegsSigs[j].Name, setFbbRegs);
		}
	}
	
	for (j = 0; j < sizeof(setVerticalRegsSigs) / sizeof(FuncPattern); j++)
		if (setVerticalRegsSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(setVerticalRegsSigs) / sizeof(FuncPattern) && (i = setVerticalRegsSigs[j].offsetFoundAt)) {
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
	
	if (j < sizeof(VIConfigureSigs) / sizeof(FuncPattern) && (i = VIConfigureSigs[j].offsetFoundAt)) {
		u32 *VIConfigure = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *VIConfigureHook1, *VIConfigureHook2;
		
		if (VIConfigure) {
			VIConfigureHook2 = getPatchAddr(VI_CONFIGUREHOOK2);
			if (swissSettings.aveCompat == 1)
				VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1_GCVIDEO);
			else
				VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1);
			
			switch (swissSettings.gameVMode) {
				case  1:
				case  2: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE480I);    break;
				case  3: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE240P);    break;
				case  4: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE960I);    break;
				case  5: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE480P);    break;
				case  6: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE1080I60); break;
				case  7: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE540P60);  break;
				case  8:
				case  9: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE576I);    break;
				case 10: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE288P);    break;
				case 11: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE1152I);   break;
				case 12: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE576P);    break;
				case 13: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE1080I50); break;
				case 14: VIConfigureHook1 = getPatchAddr(VI_CONFIGURE540P50);  break;
			}
			
			switch (j) {
				case 0:
					data[i +  54] = data[i + 6];
					data[i +  53] = data[i + 5];
					data[i +   5] = branchAndLink(VIConfigureHook1, VIConfigure + 5);
					data[i +   6] = 0x7C771B78;	// mr		r23, r3
					data[i +  18] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  35] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  64] = 0x541807BE;	// clrlwi	r24, r0, 30
					data[i +  91] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i + 101] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 107] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 218] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 219] = 0x28000002;	// cmplwi	r0, 2
					data[i + 264] = 0xA87F00FA;	// lha		r3, 250 (r31)
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
					data[i +  38] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  84] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i +  94] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 100] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 249] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 250] = 0x28000002;	// cmplwi	r0, 2
					data[i + 252] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 253] = 0x28000003;	// cmplwi	r0, 3
					data[i + 298] = 0xA87F00FA;	// lha		r3, 250 (r31)
					data[i + 308] = branchAndLink(VIConfigureHook2, VIConfigure + 308);
					break;
				case 2:
					data[i +   7] = branchAndLink(VIConfigureHook1, VIConfigure + 7);
					data[i +  19] = 0x548307BE;	// clrlwi	r3, r4, 30
					data[i +  29] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  41] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  54] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i + 101] = 0xA8F70000;	// lha		r7, 0 (r23)
					data[i + 102] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 103] = 0x54E407FE;	// clrlwi	r4, r7, 31
					data[i + 104] = 0x7C842A14;	// add		r4, r4, r5
					data[i + 111] = 0x548807FE;	// clrlwi	r8, r4, 31
					data[i + 114] = 0x7CC83050;	// sub		r6, r6, r8
					data[i + 222] = 0x801B0000;	// lwz		r0, 0 (r27)
					data[i + 224] = 0x28000002;	// cmplwi	r0, 2
					data[i + 412] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 422] = branchAndLink(VIConfigureHook2, VIConfigure + 422);
					break;
				case 3:
					data[i +   7] = branchAndLink(VIConfigureHook1, VIConfigure + 7);
					data[i +  21] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  33] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  46] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i +  93] = 0xA8F70000;	// lha		r7, 0 (r23)
					data[i +  94] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i +  95] = 0x54E407FE;	// clrlwi	r4, r7, 31
					data[i +  96] = 0x7C842A14;	// add		r4, r4, r5
					data[i + 103] = 0x548807FE;	// clrlwi	r8, r4, 31
					data[i + 106] = 0x7CC83050;	// sub		r6, r6, r8
					data[i + 214] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 216] = 0x28000002;	// cmplwi	r0, 2
					data[i + 404] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 414] = branchAndLink(VIConfigureHook2, VIConfigure + 414);
					break;
				case 4:
					data[i +   7] = branchAndLink(VIConfigureHook1, VIConfigure + 7);
					data[i +  21] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  33] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  46] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i +  96] = 0xA8F60000;	// lha		r7, 0 (r22)
					data[i +  97] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i +  98] = 0x54E407FE;	// clrlwi	r4, r7, 31
					data[i +  99] = 0x7C842A14;	// add		r4, r4, r5
					data[i + 106] = 0x548807FE;	// clrlwi	r8, r4, 31
					data[i + 109] = 0x7CC83050;	// sub		r6, r6, r8
					data[i + 217] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 219] = 0x28000002;	// cmplwi	r0, 2
					data[i + 407] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 417] = branchAndLink(VIConfigureHook2, VIConfigure + 417);
					break;
				case 5:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  56] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  68] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  81] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i + 128] = 0xA8F70000;	// lha		r7, 0 (r23)
					data[i + 129] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 130] = 0x54E407FE;	// clrlwi	r4, r7, 31
					data[i + 131] = 0x7C842A14;	// add		r4, r4, r5
					data[i + 138] = 0x548807FE;	// clrlwi	r8, r4, 31
					data[i + 141] = 0x7CC83050;	// sub		r6, r6, r8
					data[i + 258] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 260] = 0x28000002;	// cmplwi	r0, 2
					data[i + 448] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 458] = branchAndLink(VIConfigureHook2, VIConfigure + 458);
					break;
				case 6:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  59] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  71] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  84] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i + 131] = 0xA8D70000;	// lha		r6, 0 (r23)
					data[i + 132] = 0x38800000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 133] = 0x54C307FE;	// clrlwi	r3, r6, 31
					data[i + 134] = 0x7C632214;	// add		r3, r3, r4
					data[i + 141] = 0x546807FE;	// clrlwi	r8, r3, 31
					data[i + 144] = 0x7CA82850;	// sub		r5, r5, r8
					data[i + 256] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 258] = 0x28000002;	// cmplwi	r0, 2
					data[i + 446] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 456] = branchAndLink(VIConfigureHook2, VIConfigure + 456);
					break;
				case 7:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  59] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i +  71] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i +  84] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i + 147] = 0xA8F70000;	// lha		r7, 0 (r23)
					data[i + 148] = 0x38C00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 149] = 0x54E507FE;	// clrlwi	r5, r7, 31
					data[i + 150] = 0x7CA53214;	// add		r5, r5, r6
					data[i + 157] = 0x54A907FE;	// clrlwi	r9, r5, 31
					data[i + 160] = 0x7C090050;	// sub		r0, r0, r9
					data[i + 280] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 282] = 0x28000002;	// cmplwi	r0, 2
					data[i + 284] = 0x28000003;	// cmplwi	r0, 3
					data[i + 471] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 481] = branchAndLink(VIConfigureHook2, VIConfigure + 481);
					break;
				case 8:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  91] = 0xA01F0012;	// lhz		r0, 18 (r31)
					data[i + 103] = 0xA01F0014;	// lhz		r0, 20 (r31)
					data[i + 116] = 0xA07F0016;	// lhz		r3, 22 (r31)
					data[i + 179] = 0xA8F70000;	// lha		r7, 0 (r23)
					data[i + 180] = 0x38C00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 181] = 0x54E507FE;	// clrlwi	r5, r7, 31
					data[i + 182] = 0x7CA53214;	// add		r5, r5, r6
					data[i + 189] = 0x54A907FE;	// clrlwi	r9, r5, 31
					data[i + 192] = 0x7C090050;	// sub		r0, r0, r9
					data[i + 313] = 0x801C0000;	// lwz		r0, 0 (r28)
					data[i + 315] = 0x28000002;	// cmplwi	r0, 2
					data[i + 316] = 0x41800020;	// blt		+8
					data[i + 317] = 0x28000002;	// cmplwi	r0, 2
					data[i + 319] = 0x28000003;	// cmplwi	r0, 3
					data[i + 506] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 516] = branchAndLink(VIConfigureHook2, VIConfigure + 516);
					break;
				case 9:
					data[i +   8] = branchAndLink(VIConfigureHook1, VIConfigure + 8);
					data[i +  96] = 0xA09B0012;	// lhz		r4, 18 (r27)
					data[i + 106] = 0xA0FB0014;	// lhz		r7, 20 (r27)
					data[i + 112] = 0xA07B0016;	// lhz		r3, 22 (r27)
					data[i + 209] = 0xA8060002;	// lha		r0, 2 (r6)
					data[i + 211] = 0x38E00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 213] = 0x541907FE;	// clrlwi	r25, r0, 31
					data[i + 217] = 0x7F393A14;	// add		r25, r25, r7
					data[i + 226] = 0x573A07FE;	// clrlwi	r26, r25, 31
					data[i + 234] = 0x7D9A6050;	// sub		r12, r12, r26
					data[i + 332] = 0x815D0024;	// lwz		r10, 36 (r29)
					data[i + 336] = 0x280A0002;	// cmplwi	r10, 2
					data[i + 342] = 0x41800020;	// blt		+8
					data[i + 343] = 0x280A0002;	// cmplwi	r10, 2
					data[i + 345] = 0x280A0003;	// cmplwi	r10, 3
					data[i + 542] = 0xA87E000A;	// lha		r3, 10 (r30)
					data[i + 552] = branchAndLink(VIConfigureHook2, VIConfigure + 552);
					break;
				case 10:
					data[i +   9] = branchAndLink(VIConfigureHook1, VIConfigure + 9);
					data[i +  91] = 0xA0130012;	// lhz		r0, 18 (r19)
					data[i + 103] = 0xA0130014;	// lhz		r0, 20 (r19)
					data[i + 116] = 0xA0730016;	// lhz		r3, 22 (r19)
					data[i + 179] = 0xA8F80000;	// lha		r7, 0 (r24)
					data[i + 180] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i + 181] = 0x54E407FE;	// clrlwi	r4, r7, 31
					data[i + 182] = 0x7C842A14;	// add		r4, r4, r5
					data[i + 189] = 0x548807FE;	// clrlwi	r8, r4, 31
					data[i + 192] = 0x7CC83050;	// sub		r6, r6, r8
					data[i + 498] = 0xA8730000;	// lha		r3, 0 (r19)
					data[i + 508] = branchAndLink(VIConfigureHook2, VIConfigure + 508);
					break;
			}
			if (swissSettings.gameVMode > 0) {
				switch (j) {
					case 0:
						data[i +  18] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  35] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  38] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  55] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  63] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  73] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 107] = 0x881E0016;	// lbz		r0, 22 (r30)
						data[i + 120] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 122] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 123] = 0x28000001;	// cmplwi	r0, 1
						data[i + 125] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 126] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 128] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 130] = 0xA07E0000;	// lhz		r3, 0 (r30)
						break;
					case 1:
						data[i +  18] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  24] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  38] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  41] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  44] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  61] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  69] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  90] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 100] = 0x881E0016;	// lbz		r0, 22 (r30)
						data[i + 113] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 118] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 120] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 121] = 0x28000001;	// cmplwi	r0, 1
						data[i + 123] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 124] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 126] = 0xA01E0010;	// lhz		r0, 16 (r30)
						break;
					case 2:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  35] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  54] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  67] = 0x801B0000;	// lwz		r0, 0 (r27)
						data[i +  68] = 0x28000001;	// cmplwi	r0, 1
						data[i +  70] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  71] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  73] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  76] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 3:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  27] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  46] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  57] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  59] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  60] = 0x28000001;	// cmplwi	r0, 1
						data[i +  62] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  63] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  68] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 4:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  27] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  46] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  57] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  59] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  60] = 0x28000001;	// cmplwi	r0, 1
						data[i +  62] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  63] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 5:
						data[i +  10] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  45] = 0xA07F0000;	// lhz		r3, 0 (r31)
						data[i +  62] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  81] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  92] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  94] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  95] = 0x28000001;	// cmplwi	r0, 1
						data[i +  97] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  98] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 103] = 0xA07F0000;	// lhz		r3, 0 (r31)
						data[i + 237] = 0xA0DF0000;	// lhz		r6, 0 (r31)
						break;
					case 6:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  84] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  97] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  98] = 0x28000001;	// cmplwi	r0, 1
						data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 101] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 103] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 106] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 7:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  84] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  99] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 101] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i + 102] = 0x28000001;	// cmplwi	r0, 1
						data[i + 104] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 105] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 107] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 8:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 116] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i + 127] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 131] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 133] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i + 134] = 0x28000001;	// cmplwi	r0, 1
						data[i + 136] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 137] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 139] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 9:
						data[i +  10] = 0xA0BB0000;	// lhz		r5, 0 (r27)
						data[i +  20] = 0xA01B0000;	// lhz		r0, 0 (r27)
						data[i + 101] = 0x5408003C;	// clrrwi	r8, r0, 1
						data[i + 112] = 0x887B0016;	// lbz		r3, 22 (r27)
						data[i + 123] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 127] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 129] = 0x28090001;	// cmplwi	r9, 1
						data[i + 131] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 133] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						break;
					case 10:
						data[i +  10] = 0xA0930000;	// lhz		r4, 0 (r19)
						data[i +  20] = 0xA0130000;	// lhz		r0, 0 (r19)
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 116] = 0x88730016;	// lbz		r3, 22 (r19)
						data[i + 127] = 0xA0130010;	// lhz		r0, 16 (r19)
						data[i + 131] = 0xA0130010;	// lhz		r0, 16 (r19)
						data[i + 133] = 0x801D0000;	// lwz		r0, 0 (r29)
						data[i + 134] = 0x28000001;	// cmplwi	r0, 1
						data[i + 136] = 0xA0130010;	// lhz		r0, 16 (r19)
						data[i + 137] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 139] = 0xA0130010;	// lhz		r0, 16 (r19)
						break;
				}
			}
			if (swissSettings.aveCompat != 2 && (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11 ||
												 swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13)) {
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
						data[i + 200] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 201] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 5:
						data[i + 232] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 233] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 6:
						data[i + 235] = 0x546307B8;	// rlwinm	r3, r3, 0, 30, 28
						data[i + 236] = 0x5003177A;	// rlwimi	r3, r0, 2, 29, 29
						break;
					case 7:
						data[i + 253] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 254] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 8:
						data[i + 285] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 286] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 9:
						data[i + 309] = 0x7D004378;	// mr		r0, r8
						data[i + 310] = 0x5160177A;	// rlwimi	r0, r11, 2, 29, 29
						break;
					case 10:
						data[i + 288] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						data[i + 289] = 0x54E9003C;	// rlwinm	r9, r7, 0, 0, 30
						data[i + 290] = 0x61290001;	// ori		r9, r9, 1
						break;
				}
			}
			if (swissSettings.aveCompat == 3) {
				switch (j) {
					case 0:
						data[i + 170] = 0x801F0118;	// lwz		r0, 280 (r31)
						data[i + 171] = 0x28000001;	// cmplwi	r0, 1
						data[i + 173] = 0x38000000;	// li		r0, 0
						break;
					case 1:
						data[i + 178] = 0x801F0118;	// lwz		r0, 280 (r31)
						data[i + 179] = 0x28000001;	// cmplwi	r0, 1
						data[i + 181] = 0x38000004;	// li		r0, 4
						break;
					case 2:
						data[i + 167] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 168] = 0x28000001;	// cmplwi	r0, 1
						data[i + 170] = 0x38000000;	// li		r0, 0
						break;
					case 3:
						data[i + 159] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 160] = 0x28000001;	// cmplwi	r0, 1
						data[i + 162] = 0x38000000;	// li		r0, 0
						break;
					case 4:
						data[i + 162] = 0x80180000;	// lwz		r0, 0 (r24)
						data[i + 163] = 0x28000001;	// cmplwi	r0, 1
						data[i + 165] = 0x38000000;	// li		r0, 0
						break;
					case 5:
						data[i + 194] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 195] = 0x28000001;	// cmplwi	r0, 1
						data[i + 197] = 0x38000000;	// li		r0, 0
						break;
					case 6:
						data[i + 197] = 0x801D0118;	// lwz		r0, 280 (r29)
						data[i + 198] = 0x28000001;	// cmplwi	r0, 1
						data[i + 200] = 0x38000004;	// li		r0, 4
						break;
					case 7:
						data[i + 213] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 214] = 0x28000001;	// cmplwi	r0, 1
						data[i + 216] = 0x38000004;	// li		r0, 4
						break;
					case 8:
						data[i + 245] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 246] = 0x28000001;	// cmplwi	r0, 1
						data[i + 248] = 0x38000004;	// li		r0, 4
						break;
					case 9:
						data[i + 232] = 0x81250028;	// lwz		r9, 40 (r5)
						data[i + 250] = 0x28090001;	// cmplwi	r9, 1
						data[i + 273] = 0x38000004;	// li		r0, 4
						break;
					case 10:
						data[i + 245] = 0x80160000;	// lwz		r0, 0 (r22)
						data[i + 246] = 0x28000001;	// cmplwi	r0, 1
						data[i + 248] = 0x38000004;	// li		r0, 4
						break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", VIConfigureSigs[j].Name, VIConfigure);
		}
	}
	
	for (j = 0; j < sizeof(VIConfigurePanSigs) / sizeof(FuncPattern); j++)
		if (VIConfigurePanSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(VIConfigurePanSigs) / sizeof(FuncPattern) && (i = VIConfigurePanSigs[j].offsetFoundAt)) {
		u32 *VIConfigurePan = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *VIConfigurePanHook;
		
		if (VIConfigurePan) {
			switch (j) {
				case 0:
					VIConfigurePanHook = getPatchAddr(VI_CONFIGUREPANHOOKD);
					
					data[i +  31] = branchAndLink(VIConfigurePanHook, VIConfigurePan + 31);
					data[i +  84] = 0xA87F00FA;	// lha		r3, 250 (r31)
					data[i +  85] = 0xA09F00FC;	// lhz		r4, 252 (r31)
					break;
				case 1:
					VIConfigurePanHook = getPatchAddr(VI_CONFIGUREPANHOOK);
					
					data[i +  10] = branchAndLink(VIConfigurePanHook, VIConfigurePan + 10);
					data[i +  61] = 0xAB3D00F2;	// lha		r25, 242 (r29)
					data[i +  63] = 0x38A00000 | (swissSettings.forceVOffset & 0xFFFF);
					data[i +  64] = 0x572307FE;	// clrlwi	r3, r25, 31
					data[i +  65] = 0x7C632A14;	// add		r3, r3, r5
					data[i +  72] = 0x547907FE;	// clrlwi	r25, r3, 31
					data[i +  75] = 0x7CF93850;	// sub		r7, r7, r25
					data[i + 213] = 0xA87F0000;	// lha		r3, 0 (r31)
					data[i + 214] = 0xA09D00FC;	// lhz		r4, 252 (r29)
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", VIConfigurePanSigs[j].Name, VIConfigurePan);
		}
	}
	
	for (j = 0; j < sizeof(VISetBlackSigs) / sizeof(FuncPattern); j++)
		if (VISetBlackSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(VISetBlackSigs) / sizeof(FuncPattern) && (i = VISetBlackSigs[j].offsetFoundAt)) {
		u32 *VISetBlack = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VISetBlack) {
			switch (j) {
				case 0:
					data[i + 14] = 0xA87E00FA;	// lha		r3, 250 (r30)
					data[i + 15] = 0xA09E00FC;	// lhz		r4, 252 (r30)
					break;
				case 1:
					data[i + 14] = 0xA87F00FA;	// lha		r3, 250 (r31)
					data[i + 15] = 0xA09F00FC;	// lhz		r4, 252 (r31)
					break;
				case 2:
					data[i + 13] = 0xA864000A;	// lha		r3, 10 (r4)
					data[i + 14] = 0xA084000C;	// lhz		r4, 12 (r4)
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", VISetBlackSigs[j].Name, VISetBlack);
		}
	}
	
	if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
		if ((i = VIGetRetraceCountSig.offsetFoundAt)) {
			u32 *VIGetRetraceCount = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *VIGetRetraceCountHook;
			
			if (VIGetRetraceCount) {
				VIGetRetraceCountHook = getPatchAddr(VI_GETRETRACECOUNTHOOK);
				
				data[i + 1] = branch(VIGetRetraceCountHook, VIGetRetraceCount + 1);
				
				print_gecko("Found:[%s] @ %08X\n", VIGetRetraceCountSig.Name, VIGetRetraceCount);
			}
		}
	}
	
	for (j = 0; j < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern); j++)
		if (VIGetNextFieldSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern) && (i = VIGetNextFieldSigs[j].offsetFoundAt)) {
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
	
	if (swissSettings.forceDTVStatus) {
		for (j = 0; j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern); j++)
			if (VIGetDTVStatusSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern) && (i = VIGetDTVStatusSigs[j].offsetFoundAt)) {
			u32 *VIGetDTVStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (VIGetDTVStatus) {
				memset(data + i, 0, VIGetDTVStatusSigs[j].Length * sizeof(u32));
				data[i + 0] = 0x38600001;	// li		r3, 1
				data[i + 1] = 0x4E800020;	// blr
				
				print_gecko("Found:[%s] @ %08X\n", VIGetDTVStatusSigs[j].Name, VIGetDTVStatus);
			}
		}
	}
	
	for (j = 0; j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern); j++)
		if (__GXInitGXSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern) && (i = __GXInitGXSigs[j].offsetFoundAt)) {
		u32 *__GXInitGX = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__GXInitGX) {
			if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11 ||
				swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13) {
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
	
	for (j = 0; j < sizeof(GXTokenInterruptHandlerSigs) / sizeof(FuncPattern); j++)
		if (GXTokenInterruptHandlerSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(GXTokenInterruptHandlerSigs) / sizeof(FuncPattern) && (i = GXTokenInterruptHandlerSigs[j].offsetFoundAt)) {
		u32 *GXTokenInterruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *GXTokenInterruptHandlerHook;
		
		if (GXTokenInterruptHandler) {
			GXTokenInterruptHandlerHook = getPatchAddr(GX_TOKENINTERRUPTHANDLERHOOK);
			
			switch (j) {
				case 0:
					data[i + 16] = 0x7D8903A6;	// mtctr	r12
					data[i + 17] = branchAndLink(GXTokenInterruptHandlerHook, GXTokenInterruptHandler + 17);
					break;
				case 1:
				case 4:
					data[i + 17] = 0x7D8903A6;	// mtctr	r12
				case 2:
					data[i + 18] = branchAndLink(GXTokenInterruptHandlerHook, GXTokenInterruptHandler + 18);
					break;
				case 3:
					data[i + 20] = branchAndLink(GXTokenInterruptHandlerHook, GXTokenInterruptHandler + 20);
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", GXTokenInterruptHandlerSigs[j].Name, GXTokenInterruptHandler);
		}
	}
	
	if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
		for (j = 0; j < sizeof(GXAdjustForOverscanSigs) / sizeof(FuncPattern); j++)
			if (GXAdjustForOverscanSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(GXAdjustForOverscanSigs) / sizeof(FuncPattern) && (i = GXAdjustForOverscanSigs[j].offsetFoundAt)) {
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
		
		if (j < sizeof(GXSetDispCopyYScaleSigs) / sizeof(FuncPattern) && (i = GXSetDispCopyYScaleSigs[j].offsetFoundAt)) {
			u32 *GXSetDispCopyYScale = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (GXSetDispCopyYScale) {
				if (GXSetDispCopyYScaleSigs[j].Patch) {
					u32 op = j >= 2 ? data[i + 7] : data[i + 65];
					memset(data + i, 0, GXSetDispCopyYScaleSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetDispCopyYScaleSigs[j].Patch, GXSetDispCopyYScaleSigs[j].PatchLength);
					
					if (GXSetDispCopyYScaleSigs[j].Patch != GXSetDispCopyYScalePatch2)
						data[i + 6] |= op & 0x1FFFFF;
				}
				print_gecko("Found:[%s] @ %08X\n", GXSetDispCopyYScaleSigs[j].Name, GXSetDispCopyYScale);
			}
		}
	}
	
	if (swissSettings.forceVFilter) {
		for (j = 0; j < sizeof(GXSetCopyFilterSigs) / sizeof(FuncPattern); j++)
			if (GXSetCopyFilterSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(GXSetCopyFilterSigs) / sizeof(FuncPattern) && (i = GXSetCopyFilterSigs[j].offsetFoundAt)) {
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
		
		if (j < sizeof(GXSetBlendModeSigs) / sizeof(FuncPattern) && (i = GXSetBlendModeSigs[j].offsetFoundAt)) {
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
	
	if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11 ||
		swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13) {
		for (j = 0; j < sizeof(GXCopyDispSigs) / sizeof(FuncPattern); j++)
			if (GXCopyDispSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(GXCopyDispSigs) / sizeof(FuncPattern) && (i = GXCopyDispSigs[j].offsetFoundAt)) {
			u32 *GXCopyDisp = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *GXCopyDispHook;
			
			if (GXCopyDisp) {
				GXCopyDispHook = getPatchAddr(GX_COPYDISPHOOK);
				
				data[i + GXCopyDispSigs[j].Length - 1] = branch(GXCopyDispHook, GXCopyDisp + GXCopyDispSigs[j].Length - 1);
				
				print_gecko("Found:[%s] @ %08X\n", GXCopyDispSigs[j].Name, GXCopyDisp);
			}
		}
		
		for (j = 0; j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern); j++)
			if (GXSetViewportSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern) && (i = GXSetViewportSigs[j].offsetFoundAt)) {
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
		
		if (j < sizeof(GXSetViewportJitterSigs) / sizeof(FuncPattern) && (i = GXSetViewportJitterSigs[j].offsetFoundAt)) {
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
		{ 39, 4, 16, 0, 0, 0, NULL, 0, "C_MTXFrustum" };
	FuncPattern MTXLightFrustumSig = 
		{ 37, 6, 13, 0, 0, 0, NULL, 0, "C_MTXLightFrustum" };
	FuncPattern MTXPerspectiveSig = 
		{ 52, 8, 19, 1, 0, 6, NULL, 0, "C_MTXPerspective" };
	FuncPattern MTXLightPerspectiveSig = 
		{ 51, 8, 15, 1, 0, 8, NULL, 0, "C_MTXLightPerspective" };
	FuncPattern MTXOrthoSig = 
		{ 38, 4, 16, 0, 0, 0, NULL, 0, "C_MTXOrtho" };
	FuncPattern GXSetScissorSigs[3] = {
		{ 44, 19, 6, 0, 0, 6, NULL, 0, "GXSetScissor A" },
		{ 36, 12, 6, 0, 0, 6, NULL, 0, "GXSetScissor B" },
		{ 30, 13, 6, 0, 0, 1, NULL, 0, "GXSetScissor C" }
	};
	FuncPattern GXSetProjectionSigs[3] = {
		{ 53, 30, 17, 0, 1, 0, NULL, 0, "GXSetProjection A" },
		{ 45, 22, 17, 0, 1, 0, NULL, 0, "GXSetProjection B" },
		{ 41, 18, 11, 0, 1, 0, NULL, 0, "GXSetProjection C" }
	};
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i] != 0xED241828)
			continue;
		if (find_pattern(data, i, length, &MTXFrustumSig)) {
			u32 *MTXFrustum = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXFrustumHook = NULL;
			if (MTXFrustum) {
				print_gecko("Found:[%s] @ %08X\n", MTXFrustumSig.Name, MTXFrustum);
				MTXFrustumHook = installPatch(MTX_FRUSTUMHOOK);
				MTXFrustumHook[7] = branch(MTXFrustum + 1, MTXFrustumHook + 7);
				data[i] = branch(MTXFrustumHook, MTXFrustum);
				*(u32*)VAR_FLOAT1_6 = 0x3E2AAAAB;
				break;
			}
		}
	}
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i + 2] != 0xED441828)
			continue;
		if (find_pattern(data, i, length, &MTXLightFrustumSig)) {
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
		if (find_pattern(data, i, length, &MTXPerspectiveSig)) {
			u32 *MTXPerspective = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXPerspectiveHook = NULL;
			if (MTXPerspective) {
				print_gecko("Found:[%s] @ %08X\n", MTXPerspectiveSig.Name, MTXPerspective);
				MTXPerspectiveHook = installPatch(MTX_PERSPECTIVEHOOK);
				MTXPerspectiveHook[5] = branch(MTXPerspective + 21, MTXPerspectiveHook + 5);
				data[i + 20] = branch(MTXPerspectiveHook, MTXPerspective + 20);
				*(u32*)VAR_FLOAT9_16 = 0x3F100000;
				break;
			}
		}
	}
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i] != 0x7C0802A6)
			continue;
		if (find_pattern(data, i, length, &MTXLightPerspectiveSig)) {
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
			if (find_pattern(data, i, length, &MTXOrthoSig)) {
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
			make_pattern(data, i, length, &fp);
			
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
			make_pattern(data, i, length, &fp);
			
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
		{ 101, 29, 11, 0, 11, 11, NULL, 0, "GXInitTexObjLOD A" },
		{  89, 16,  4, 0,  8,  6, NULL, 0, "GXInitTexObjLOD B" },	// SN Systems ProDG
		{  89, 30, 11, 0, 11,  6, NULL, 0, "GXInitTexObjLOD C" }
	};
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (data[i + 2] != 0xFC030040 && data[i + 4] != 0xFC030000)
			continue;
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
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

u32 _fontencode_part[] = {
	0x2C000000,	// cmpwi	r0, 0
	0x4182000C,	// beq		+3
	0x4180002C,	// blt		+11
	0x48000028,	// b		+10
	0x3C60CC00,	// lis		r3, 0xCC00
	0xA003206E,	// lhz		r0, 0x206E (r3)
	0x540007BD,	// rlwinm.	r0, r0, 0, 30, 30
	0x4182000C	// beq		+3
};

int Patch_FontEncode(u32 *data, u32 length)
{
	int patched = 0;
	int i;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!memcmp(data + i, _fontencode_part, sizeof(_fontencode_part))) {
			switch (swissSettings.forceEncoding) {
				case 1:
					data[i +  8] = 0x38000000;
					break;
				case 2:
					data[i + 10] = 0x38000001;
					data[i + 13] = 0x38000001;
					break;
			}
			patched++;
		}
	}
	print_gecko("Patch:[fontEncode] applied %d times\n", patched);
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
	else if(!strncmp(gameID, "GFZE01", 6) || !strncmp(gameID, "GFZP01", 6))
	{
		*(vu32*)(addr+0x2608) = 0x48000038;
		*(vu32*)(addr+0x260C) = 0x60000000;
		*(vu32*)(addr+0x2610) = 0x60000000;
		*(vu32*)(addr+0x2614) = 0x60000000;
		*(vu32*)(addr+0x2618) = 0x60000000;
		*(vu32*)(addr+0x261C) = 0x60000000;
		*(vu32*)(addr+0x2620) = 0x60000000;
		*(vu32*)(addr+0x2624) = 0x60000000;
		*(vu32*)(addr+0x2628) = 0x60000000;
		*(vu32*)(addr+0x262C) = 0x60000000;
		*(vu32*)(addr+0x2630) = 0x60000000;
		*(vu32*)(addr+0x2634) = 0x60000000;
		*(vu32*)(addr+0x2638) = 0x60000000;
		*(vu32*)(addr+0x263C) = 0x60000000;
		*(vu32*)(addr+0x2640) = 0x38000000;
		print_gecko("Patched:[%.6s]\n", gameID);
		patched=1;
	}
	else if(!strncmp(gameID, "GFZJ01", 6))
	{
		*(vu32*)(addr+0x2608) = 0x48000050;
		*(vu32*)(addr+0x260C) = 0x60000000;
		*(vu32*)(addr+0x2610) = 0x60000000;
		*(vu32*)(addr+0x2614) = 0x60000000;
		*(vu32*)(addr+0x2618) = 0x60000000;
		*(vu32*)(addr+0x261C) = 0x60000000;
		*(vu32*)(addr+0x2620) = 0x60000000;
		*(vu32*)(addr+0x2624) = 0x60000000;
		*(vu32*)(addr+0x2628) = 0x60000000;
		*(vu32*)(addr+0x262C) = 0x60000000;
		*(vu32*)(addr+0x2630) = 0x60000000;
		*(vu32*)(addr+0x2634) = 0x60000000;
		*(vu32*)(addr+0x2638) = 0x60000000;
		*(vu32*)(addr+0x263C) = 0x60000000;
		*(vu32*)(addr+0x2640) = 0x60000000;
		*(vu32*)(addr+0x2644) = 0x60000000;
		*(vu32*)(addr+0x2648) = 0x60000000;
		*(vu32*)(addr+0x264C) = 0x60000000;
		*(vu32*)(addr+0x2650) = 0x60000000;
		*(vu32*)(addr+0x2654) = 0x60000000;
		*(vu32*)(addr+0x2658) = 0x38000005;
		print_gecko("Patched:[%.6s]\n", gameID);
		patched=1;
	}
	return patched;
}

// Overwrite game specific file content
int Patch_GameSpecificFile(void *data, u32 length, const char *gameID, const char *fileName)
{
	void *addr;
	int patched = 0;
	
	if (!swissSettings.disableVideoPatches) {
		if (!strncmp(gameID, "GS8P7D", 6)) {
			if (!strcasecmp(fileName, "SPYROCFG_NGC.CFG")) {
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					addr = strnstr(data, "\tHeight:\t\t\t496\r\n", length);
					if (addr) memcpy(addr, "\tHeight:\t\t\t448\r\n", 16);
				}
				print_gecko("Patched:[%s]\n", fileName);
				patched++;
			}
		}
	}
	return patched;
}

void Patch_GameSpecificVideo(void *data, u32 length, const char *gameID, int dataType)
{
	if (!strncmp(gameID, "GAEJ01", 6)) {
		switch (length) {
			case 1069088:
				*(s16 *)(data + 0x80006CBE - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x80006CC2 - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x80006CC6 - 0x80005760 + 0x26C0) = 640;
				*(s16 *)(data + 0x80006CCA - 0x80005760 + 0x26C0) = 480;
				
				*(s16 *)(data + 0x8000774A - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x8000774E - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x80007752 - 0x80005760 + 0x26C0) = 640;
				*(s16 *)(data + 0x80007756 - 0x80005760 + 0x26C0) = 480;
				
				*(s16 *)(data + 0x8000775E - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x80007762 - 0x80005760 + 0x26C0) = 0;
				*(s16 *)(data + 0x80007766 - 0x80005760 + 0x26C0) = 640;
				*(s16 *)(data + 0x8000776A - 0x80005760 + 0x26C0) = 480;
				
				*(u16 *)(data + 0x800D6DBA - 0x800D67E0 + 0xD37E0) = 40;
				*(u16 *)(data + 0x800D6DBE - 0x800D67E0 + 0xD37E0) = 640;
				
				*(u16 *)(data + 0x800D6DF6 - 0x800D67E0 + 0xD37E0) = 40;
				*(u16 *)(data + 0x800D6DFA - 0x800D67E0 + 0xD37E0) = 640;
				
				*(u16 *)(data + 0x800D6E32 - 0x800D67E0 + 0xD37E0) = 40;
				*(u16 *)(data + 0x800D6E36 - 0x800D67E0 + 0xD37E0) = 640;
				
				*(u16 *)(data + 0x800D6E6E - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x800D6E72 - 0x800D67E0 + 0xD37E0) = 704;
				
				*(u16 *)(data + 0x800D6EAA - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x800D6EAE - 0x800D67E0 + 0xD37E0) = 704;
				
				*(u16 *)(data + 0x80105DEA - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x80105DEE - 0x800D67E0 + 0xD37E0) = 704;
				
				*(u16 *)(data + 0x80105E26 - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x80105E2A - 0x800D67E0 + 0xD37E0) = 704;
				
				*(u16 *)(data + 0x80105E62 - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x80105E66 - 0x800D67E0 + 0xD37E0) = 704;
				
				*(u16 *)(data + 0x80105EDA - 0x800D67E0 + 0xD37E0) = 8;
				*(u16 *)(data + 0x80105EDE - 0x800D67E0 + 0xD37E0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 1076832:
				*(s16 *)(data + 0x80006BFE - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x80006C02 - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x80006C06 - 0x800056A0 + 0x2600) = 640;
				*(s16 *)(data + 0x80006C0A - 0x800056A0 + 0x2600) = 480;
				
				*(s16 *)(data + 0x8000769A - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x8000769E - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x800076A2 - 0x800056A0 + 0x2600) = 640;
				*(s16 *)(data + 0x800076A6 - 0x800056A0 + 0x2600) = 480;
				
				*(s16 *)(data + 0x800076AE - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x800076B2 - 0x800056A0 + 0x2600) = 0;
				*(s16 *)(data + 0x800076B6 - 0x800056A0 + 0x2600) = 640;
				*(s16 *)(data + 0x800076BA - 0x800056A0 + 0x2600) = 480;
				
				*(u16 *)(data + 0x800D8982 - 0x800D83A0 + 0xD53A0) = 40;
				*(u16 *)(data + 0x800D8986 - 0x800D83A0 + 0xD53A0) = 640;
				
				*(u16 *)(data + 0x800D89BE - 0x800D83A0 + 0xD53A0) = 40;
				*(u16 *)(data + 0x800D89C2 - 0x800D83A0 + 0xD53A0) = 640;
				
				*(u16 *)(data + 0x800D89FA - 0x800D83A0 + 0xD53A0) = 40;
				*(u16 *)(data + 0x800D89FE - 0x800D83A0 + 0xD53A0) = 640;
				
				*(u16 *)(data + 0x800D8A36 - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x800D8A3A - 0x800D83A0 + 0xD53A0) = 704;
				
				*(u16 *)(data + 0x800D8A72 - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x800D8A76 - 0x800D83A0 + 0xD53A0) = 704;
				
				*(u16 *)(data + 0x80107BAA - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x80107BAE - 0x800D83A0 + 0xD53A0) = 704;
				
				*(u16 *)(data + 0x80107BE6 - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x80107BEA - 0x800D83A0 + 0xD53A0) = 704;
				
				*(u16 *)(data + 0x80107C22 - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x80107C26 - 0x800D83A0 + 0xD53A0) = 704;
				
				*(u16 *)(data + 0x80107C9A - 0x800D83A0 + 0xD53A0) = 8;
				*(u16 *)(data + 0x80107C9E - 0x800D83A0 + 0xD53A0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFE01", 6)) {
		switch (length) {
			case 918720:
				*(s16 *)(data + 0x8000672E - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006732 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006736 - 0x800056C0 + 0x2620) = 640;
				*(s16 *)(data + 0x8000673A - 0x800056C0 + 0x2620) = 480;
				
				*(s16 *)(data + 0x80006EF2 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006EF6 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006EFA - 0x800056C0 + 0x2620) = 640;
				*(s16 *)(data + 0x80006EFE - 0x800056C0 + 0x2620) = 480;
				
				*(u16 *)(data + 0x800AFE62 - 0x800AF860 + 0xAC860) = 40;
				*(u16 *)(data + 0x800AFE66 - 0x800AF860 + 0xAC860) = 640;
				
				*(u16 *)(data + 0x800AFE9E - 0x800AF860 + 0xAC860) = 40;
				*(u16 *)(data + 0x800AFEA2 - 0x800AF860 + 0xAC860) = 640;
				
				*(u16 *)(data + 0x800AFEDA - 0x800AF860 + 0xAC860) = 40;
				*(u16 *)(data + 0x800AFEDE - 0x800AF860 + 0xAC860) = 640;
				
				*(u16 *)(data + 0x800AFF16 - 0x800AF860 + 0xAC860) = 40;
				*(u16 *)(data + 0x800AFF1A - 0x800AF860 + 0xAC860) = 640;
				
				*(u16 *)(data + 0x800AFF52 - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800AFF56 - 0x800AF860 + 0xAC860) = 704;
				
				*(u16 *)(data + 0x800AFF8E - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800AFF92 - 0x800AF860 + 0xAC860) = 704;
				
				*(u16 *)(data + 0x800E15BA - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800E15BE - 0x800AF860 + 0xAC860) = 704;
				
				*(u16 *)(data + 0x800E15F6 - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800E15FA - 0x800AF860 + 0xAC860) = 704;
				
				*(u16 *)(data + 0x800E1632 - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800E1636 - 0x800AF860 + 0xAC860) = 704;
				
				*(u16 *)(data + 0x800E16AA - 0x800AF860 + 0xAC860) = 8;
				*(u16 *)(data + 0x800E16AE - 0x800AF860 + 0xAC860) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFJ01", 6)) {
		switch (length) {
			case 888704:
				*(s16 *)(data + 0x80006622 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x80006626 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000662A - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x8000662E - 0x800055C0 + 0x2520) = 480;
				
				*(s16 *)(data + 0x80006636 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000663A - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000663E - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x80006642 - 0x800055C0 + 0x2520) = 480;
				
				*(s16 *)(data + 0x80006896 - 0x800055C0 + 0x2520) = 256;
				
				*(s16 *)(data + 0x80006916 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000691E - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x80006922 - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x8000692A - 0x800055C0 + 0x2520) = 480;
				
				*(u16 *)(data + 0x800DA9AE - 0x800AC1C0 + 0xA91C0) = 8;
				*(u16 *)(data + 0x800DA9B2 - 0x800AC1C0 + 0xA91C0) = 704;
				
				*(u16 *)(data + 0x800DA9EA - 0x800AC1C0 + 0xA91C0) = 8;
				*(u16 *)(data + 0x800DA9EE - 0x800AC1C0 + 0xA91C0) = 704;
				
				*(u16 *)(data + 0x800DAA26 - 0x800AC1C0 + 0xA91C0) = 8;
				*(u16 *)(data + 0x800DAA2A - 0x800AC1C0 + 0xA91C0) = 704;
				
				*(u16 *)(data + 0x800DAA62 - 0x800AC1C0 + 0xA91C0) = 8;
				*(u16 *)(data + 0x800DAA66 - 0x800AC1C0 + 0xA91C0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 890048:
				*(s16 *)(data + 0x80006622 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x80006626 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000662A - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x8000662E - 0x800055C0 + 0x2520) = 480;
				
				*(s16 *)(data + 0x80006636 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000663A - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000663E - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x80006642 - 0x800055C0 + 0x2520) = 480;
				
				*(s16 *)(data + 0x80006896 - 0x800055C0 + 0x2520) = 256;
				
				*(s16 *)(data + 0x80006916 - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x8000691E - 0x800055C0 + 0x2520) = 0;
				*(s16 *)(data + 0x80006922 - 0x800055C0 + 0x2520) = 640;
				*(s16 *)(data + 0x8000692A - 0x800055C0 + 0x2520) = 480;
				
				*(u16 *)(data + 0x800DAEEE - 0x800AC4E0 + 0xA94E0) = 8;
				*(u16 *)(data + 0x800DAEF2 - 0x800AC4E0 + 0xA94E0) = 704;
				
				*(u16 *)(data + 0x800DAF2A - 0x800AC4E0 + 0xA94E0) = 8;
				*(u16 *)(data + 0x800DAF2E - 0x800AC4E0 + 0xA94E0) = 704;
				
				*(u16 *)(data + 0x800DAF66 - 0x800AC4E0 + 0xA94E0) = 8;
				*(u16 *)(data + 0x800DAF6A - 0x800AC4E0 + 0xA94E0) = 704;
				
				*(u16 *)(data + 0x800DAFA2 - 0x800AC4E0 + 0xA94E0) = 8;
				*(u16 *)(data + 0x800DAFA6 - 0x800AC4E0 + 0xA94E0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFP01", 6)) {
		switch (length) {
			case 971712:
				*(s16 *)(data + 0x80006B4A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B6E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B72 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B76 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B7A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B92 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B96 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B9A - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B9E - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x800074A2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074A6 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074AA - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x800074AE - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BB662 - 0x800BAFC0 + 0xB7FC0) = 40;
				*(u16 *)(data + 0x800BB666 - 0x800BAFC0 + 0xB7FC0) = 640;
				
				*(u16 *)(data + 0x800BB69E - 0x800BAFC0 + 0xB7FC0) = 40;
				*(u16 *)(data + 0x800BB6A2 - 0x800BAFC0 + 0xB7FC0) = 640;
				
				*(u16 *)(data + 0x800BB6DA - 0x800BAFC0 + 0xB7FC0) = 40;
				*(u16 *)(data + 0x800BB6DE - 0x800BAFC0 + 0xB7FC0) = 640;
				
				*(u16 *)(data + 0x800BB714 - 0x800BAFC0 + 0xB7FC0) = 574;
				*(u16 *)(data + 0x800BB716 - 0x800BAFC0 + 0xB7FC0) = 40;
				*(u16 *)(data + 0x800BB718 - 0x800BAFC0 + 0xB7FC0) = 0;
				*(u16 *)(data + 0x800BB71A - 0x800BAFC0 + 0xB7FC0) = 640;
				*(u16 *)(data + 0x800BB71C - 0x800BAFC0 + 0xB7FC0) = 574;
				
				*(u16 *)(data + 0x800BB750 - 0x800BAFC0 + 0xB7FC0) = 574;
				*(u16 *)(data + 0x800BB752 - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800BB754 - 0x800BAFC0 + 0xB7FC0) = 0;
				*(u16 *)(data + 0x800BB756 - 0x800BAFC0 + 0xB7FC0) = 704;
				*(u16 *)(data + 0x800BB758 - 0x800BAFC0 + 0xB7FC0) = 574;
				
				*(u16 *)(data + 0x800BB78E - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800BB792 - 0x800BAFC0 + 0xB7FC0) = 704;
				
				*(u16 *)(data + 0x800EE1CA - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800EE1CE - 0x800BAFC0 + 0xB7FC0) = 704;
				
				*(u16 *)(data + 0x800EE206 - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800EE20A - 0x800BAFC0 + 0xB7FC0) = 704;
				
				*(u16 *)(data + 0x800EE242 - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800EE246 - 0x800BAFC0 + 0xB7FC0) = 704;
				
				*(u16 *)(data + 0x800EE2BA - 0x800BAFC0 + 0xB7FC0) = 8;
				*(u16 *)(data + 0x800EE2BE - 0x800BAFC0 + 0xB7FC0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 974304:
				*(s16 *)(data + 0x80006B4A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B6E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B72 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B76 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B7A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B92 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B96 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B9A - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B9E - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x800074A2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074A6 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074AA - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x800074AE - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BB402 - 0x800BAD60 + 0xB7D60) = 40;
				*(u16 *)(data + 0x800BB406 - 0x800BAD60 + 0xB7D60) = 640;
				
				*(u16 *)(data + 0x800BB43E - 0x800BAD60 + 0xB7D60) = 40;
				*(u16 *)(data + 0x800BB442 - 0x800BAD60 + 0xB7D60) = 640;
				
				*(u16 *)(data + 0x800BB47A - 0x800BAD60 + 0xB7D60) = 40;
				*(u16 *)(data + 0x800BB47E - 0x800BAD60 + 0xB7D60) = 640;
				
				*(u16 *)(data + 0x800BB4B4 - 0x800BAD60 + 0xB7D60) = 574;
				*(u16 *)(data + 0x800BB4B6 - 0x800BAD60 + 0xB7D60) = 40;
				*(u16 *)(data + 0x800BB4B8 - 0x800BAD60 + 0xB7D60) = 0;
				*(u16 *)(data + 0x800BB4BA - 0x800BAD60 + 0xB7D60) = 640;
				*(u16 *)(data + 0x800BB4BC - 0x800BAD60 + 0xB7D60) = 574;
				
				*(u16 *)(data + 0x800BB4F0 - 0x800BAD60 + 0xB7D60) = 574;
				*(u16 *)(data + 0x800BB4F2 - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800BB4F4 - 0x800BAD60 + 0xB7D60) = 0;
				*(u16 *)(data + 0x800BB4F6 - 0x800BAD60 + 0xB7D60) = 704;
				*(u16 *)(data + 0x800BB4F8 - 0x800BAD60 + 0xB7D60) = 574;
				
				*(u16 *)(data + 0x800BB52E - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800BB532 - 0x800BAD60 + 0xB7D60) = 704;
				
				*(u16 *)(data + 0x800EEBEA - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800EEBEE - 0x800BAD60 + 0xB7D60) = 704;
				
				*(u16 *)(data + 0x800EEC26 - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800EEC2A - 0x800BAD60 + 0xB7D60) = 704;
				
				*(u16 *)(data + 0x800EEC62 - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800EEC66 - 0x800BAD60 + 0xB7D60) = 704;
				
				*(u16 *)(data + 0x800EECDA - 0x800BAD60 + 0xB7D60) = 8;
				*(u16 *)(data + 0x800EECDE - 0x800BAD60 + 0xB7D60) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 978080:
				*(s16 *)(data + 0x80006B4A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B6E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B72 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B76 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B7A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B92 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B96 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B9A - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B9E - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x800074A2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074A6 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074AA - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x800074AE - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BB522 - 0x800BAE80 + 0xB7E80) = 40;
				*(u16 *)(data + 0x800BB526 - 0x800BAE80 + 0xB7E80) = 640;
				
				*(u16 *)(data + 0x800BB55E - 0x800BAE80 + 0xB7E80) = 40;
				*(u16 *)(data + 0x800BB562 - 0x800BAE80 + 0xB7E80) = 640;
				
				*(u16 *)(data + 0x800BB59A - 0x800BAE80 + 0xB7E80) = 40;
				*(u16 *)(data + 0x800BB59E - 0x800BAE80 + 0xB7E80) = 640;
				
				*(u16 *)(data + 0x800BB5D4 - 0x800BAE80 + 0xB7E80) = 574;
				*(u16 *)(data + 0x800BB5D6 - 0x800BAE80 + 0xB7E80) = 40;
				*(u16 *)(data + 0x800BB5D8 - 0x800BAE80 + 0xB7E80) = 0;
				*(u16 *)(data + 0x800BB5DA - 0x800BAE80 + 0xB7E80) = 640;
				*(u16 *)(data + 0x800BB5DC - 0x800BAE80 + 0xB7E80) = 574;
				
				*(u16 *)(data + 0x800BB610 - 0x800BAE80 + 0xB7E80) = 574;
				*(u16 *)(data + 0x800BB612 - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800BB614 - 0x800BAE80 + 0xB7E80) = 0;
				*(u16 *)(data + 0x800BB616 - 0x800BAE80 + 0xB7E80) = 704;
				*(u16 *)(data + 0x800BB618 - 0x800BAE80 + 0xB7E80) = 574;
				
				*(u16 *)(data + 0x800BB64E - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800BB652 - 0x800BAE80 + 0xB7E80) = 704;
				
				*(u16 *)(data + 0x800EFAAA - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800EFAAE - 0x800BAE80 + 0xB7E80) = 704;
				
				*(u16 *)(data + 0x800EFAE6 - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800EFAEA - 0x800BAE80 + 0xB7E80) = 704;
				
				*(u16 *)(data + 0x800EFB22 - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800EFB26 - 0x800BAE80 + 0xB7E80) = 704;
				
				*(u16 *)(data + 0x800EFB9A - 0x800BAE80 + 0xB7E80) = 8;
				*(u16 *)(data + 0x800EFB9E - 0x800BAE80 + 0xB7E80) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 973920:
				*(s16 *)(data + 0x80006B4A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B6E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B72 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B76 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B7A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B92 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B96 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B9A - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B9E - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x800074A2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074A6 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074AA - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x800074AE - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BB282 - 0x800BABE0 + 0xB7BE0) = 40;
				*(u16 *)(data + 0x800BB286 - 0x800BABE0 + 0xB7BE0) = 640;
				
				*(u16 *)(data + 0x800BB2BE - 0x800BABE0 + 0xB7BE0) = 40;
				*(u16 *)(data + 0x800BB2C2 - 0x800BABE0 + 0xB7BE0) = 640;
				
				*(u16 *)(data + 0x800BB2FA - 0x800BABE0 + 0xB7BE0) = 40;
				*(u16 *)(data + 0x800BB2FE - 0x800BABE0 + 0xB7BE0) = 640;
				
				*(u16 *)(data + 0x800BB334 - 0x800BABE0 + 0xB7BE0) = 574;
				*(u16 *)(data + 0x800BB336 - 0x800BABE0 + 0xB7BE0) = 40;
				*(u16 *)(data + 0x800BB338 - 0x800BABE0 + 0xB7BE0) = 0;
				*(u16 *)(data + 0x800BB33A - 0x800BABE0 + 0xB7BE0) = 640;
				*(u16 *)(data + 0x800BB33C - 0x800BABE0 + 0xB7BE0) = 574;
				
				*(u16 *)(data + 0x800BB370 - 0x800BABE0 + 0xB7BE0) = 574;
				*(u16 *)(data + 0x800BB372 - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800BB374 - 0x800BABE0 + 0xB7BE0) = 0;
				*(u16 *)(data + 0x800BB376 - 0x800BABE0 + 0xB7BE0) = 704;
				*(u16 *)(data + 0x800BB378 - 0x800BABE0 + 0xB7BE0) = 574;
				
				*(u16 *)(data + 0x800BB3AE - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800BB3B2 - 0x800BABE0 + 0xB7BE0) = 704;
				
				*(u16 *)(data + 0x800EEA6A - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800EEA6E - 0x800BABE0 + 0xB7BE0) = 704;
				
				*(u16 *)(data + 0x800EEAA6 - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800EEAAA - 0x800BABE0 + 0xB7BE0) = 704;
				
				*(u16 *)(data + 0x800EEAE2 - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800EEAE6 - 0x800BABE0 + 0xB7BE0) = 704;
				
				*(u16 *)(data + 0x800EEB5A - 0x800BABE0 + 0xB7BE0) = 8;
				*(u16 *)(data + 0x800EEB5E - 0x800BABE0 + 0xB7BE0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 977760:
				*(s16 *)(data + 0x80006B4A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B6E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B72 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B76 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B7A - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x80006B92 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B96 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006B9A - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006B9E - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x800074A2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074A6 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x800074AA - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x800074AE - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BB3A2 - 0x800BAD00 + 0xB7D00) = 40;
				*(u16 *)(data + 0x800BB3A6 - 0x800BAD00 + 0xB7D00) = 640;
				
				*(u16 *)(data + 0x800BB3DE - 0x800BAD00 + 0xB7D00) = 40;
				*(u16 *)(data + 0x800BB3E2 - 0x800BAD00 + 0xB7D00) = 640;
				
				*(u16 *)(data + 0x800BB41A - 0x800BAD00 + 0xB7D00) = 40;
				*(u16 *)(data + 0x800BB41E - 0x800BAD00 + 0xB7D00) = 640;
				
				*(u16 *)(data + 0x800BB454 - 0x800BAD00 + 0xB7D00) = 574;
				*(u16 *)(data + 0x800BB456 - 0x800BAD00 + 0xB7D00) = 40;
				*(u16 *)(data + 0x800BB458 - 0x800BAD00 + 0xB7D00) = 0;
				*(u16 *)(data + 0x800BB45A - 0x800BAD00 + 0xB7D00) = 640;
				*(u16 *)(data + 0x800BB45E - 0x800BAD00 + 0xB7D00) = 574;
				
				*(u16 *)(data + 0x800BB490 - 0x800BAD00 + 0xB7D00) = 574;
				*(u16 *)(data + 0x800BB492 - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800BB494 - 0x800BAD00 + 0xB7D00) = 0;
				*(u16 *)(data + 0x800BB496 - 0x800BAD00 + 0xB7D00) = 704;
				*(u16 *)(data + 0x800BB498 - 0x800BAD00 + 0xB7D00) = 574;
				
				*(u16 *)(data + 0x800BB4CE - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800BB4D2 - 0x800BAD00 + 0xB7D00) = 704;
				
				*(u16 *)(data + 0x800EF96A - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800EF96E - 0x800BAD00 + 0xB7D00) = 704;
				
				*(u16 *)(data + 0x800EF9A6 - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800EF9AA - 0x800BAD00 + 0xB7D00) = 704;
				
				*(u16 *)(data + 0x800EF9E2 - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800EF9E6 - 0x800BAD00 + 0xB7D00) = 704;
				
				*(u16 *)(data + 0x800EFA5A - 0x800BAD00 + 0xB7D00) = 8;
				*(u16 *)(data + 0x800EFA5E - 0x800BAD00 + 0xB7D00) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFU01", 6)) {
		switch (length) {
			case 964128:
				*(s16 *)(data + 0x80006726 - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x8000674A - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x8000674E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006752 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006756 - 0x80005680 + 0x25E0) = 574;
				
				*(s16 *)(data + 0x8000676E - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006772 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006776 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x8000677A - 0x80005680 + 0x25E0) = 480;
				
				*(s16 *)(data + 0x80006FBE - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006FC2 - 0x80005680 + 0x25E0) = 0;
				*(s16 *)(data + 0x80006FC6 - 0x80005680 + 0x25E0) = 640;
				*(s16 *)(data + 0x80006FCA - 0x80005680 + 0x25E0) = 574;
				
				*(u16 *)(data + 0x800BA3A2 - 0x800B9DA0 + 0xB6DA0) = 40;
				*(u16 *)(data + 0x800BA3A6 - 0x800B9DA0 + 0xB6DA0) = 640;
				
				*(u16 *)(data + 0x800BA3DE - 0x800B9DA0 + 0xB6DA0) = 40;
				*(u16 *)(data + 0x800BA3E2 - 0x800B9DA0 + 0xB6DA0) = 640;
				
				*(u16 *)(data + 0x800BA41A - 0x800B9DA0 + 0xB6DA0) = 40;
				*(u16 *)(data + 0x800BA41E - 0x800B9DA0 + 0xB6DA0) = 640;
				
				*(u16 *)(data + 0x800BA454 - 0x800B9DA0 + 0xB6DA0) = 574;
				*(u16 *)(data + 0x800BA456 - 0x800B9DA0 + 0xB6DA0) = 40;
				*(u16 *)(data + 0x800BA458 - 0x800B9DA0 + 0xB6DA0) = 0;
				*(u16 *)(data + 0x800BA45A - 0x800B9DA0 + 0xB6DA0) = 640;
				*(u16 *)(data + 0x800BA45C - 0x800B9DA0 + 0xB6DA0) = 574;
				
				*(u16 *)(data + 0x800BA490 - 0x800B9DA0 + 0xB6DA0) = 574;
				*(u16 *)(data + 0x800BA492 - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800BA494 - 0x800B9DA0 + 0xB6DA0) = 0;
				*(u16 *)(data + 0x800BA496 - 0x800B9DA0 + 0xB6DA0) = 704;
				*(u16 *)(data + 0x800BA498 - 0x800B9DA0 + 0xB6DA0) = 574;
				
				*(u16 *)(data + 0x800BA4CE - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800BA4D2 - 0x800B9DA0 + 0xB6DA0) = 704;
				
				*(u16 *)(data + 0x800EC44A - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800EC44E - 0x800B9DA0 + 0xB6DA0) = 704;
				
				*(u16 *)(data + 0x800EC486 - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800EC48A - 0x800B9DA0 + 0xB6DA0) = 704;
				
				*(u16 *)(data + 0x800EC4C2 - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800EC4C6 - 0x800B9DA0 + 0xB6DA0) = 704;
				
				*(u16 *)(data + 0x800EC53A - 0x800B9DA0 + 0xB6DA0) = 8;
				*(u16 *)(data + 0x800EC53E - 0x800B9DA0 + 0xB6DA0) = 704;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPTP41", 6)) {
		switch (length) {
			case 4017536:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(u32 *)(data + 0x801B25C4 - 0x8002B240 + 0x2600) = 
					*(u32 *)(data + 0x801B25C0 - 0x8002B240 + 0x2600);
					*(u32 *)(data + 0x801B25C0 - 0x8002B240 + 0x2600) = 
					*(u32 *)(data + 0x801B25B0 - 0x8002B240 + 0x2600);
					
					*(s16 *)(data + 0x801B2592 - 0x8002B240 + 0x2600) = (0x803B5010 + 0x8000) >> 16;
					*(s16 *)(data + 0x801B2596 - 0x8002B240 + 0x2600) = (0x803B5010) & 0xFFFF;
					*(s16 *)(data + 0x801B25AA - 0x8002B240 + 0x2600) = 16;
					*(s16 *)(data + 0x801B25B2 - 0x8002B240 + 0x2600) = 640;
					*(s16 *)(data + 0x801B25C2 - 0x8002B240 + 0x2600) = 448;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 639584:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8002E65E - 0x8000B400 + 0x2600) = (0x80098534 + 0x8000) >> 16;
					*(s16 *)(data + 0x8002E666 - 0x8000B400 + 0x2600) = (0x80098534) & 0xFFFF;
					*(s16 *)(data + 0x8002E676 - 0x8000B400 + 0x2600) = 40;
					*(s16 *)(data + 0x8002E67E - 0x8000B400 + 0x2600) = 40;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GX2D52", 6) || !strncmp(gameID, "GX2P52", 6) || !strncmp(gameID, "GX2S52", 6)) {
		switch (length) {
			case 4055712:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80010A1A - 0x80005760 + 0x2540) = (0x80368D5C + 0x8000) >> 16;
					*(s16 *)(data + 0x80010A1E - 0x80005760 + 0x2540) = 5;
					*(s16 *)(data + 0x80010A22 - 0x80005760 + 0x2540) = (0x80368D5C) & 0xFFFF;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLP52", 6)) {
		switch (length) {
			case 4182304:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x80384784 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = 5;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x80384784) & 0xFFFF;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLX52", 6)) {
		switch (length) {
			case 4182624:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x803848C4 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = 5;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x803848C4) & 0xFFFF;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	}
}

void Patch_PADStatus(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern UpdateOriginSigs[4] = {
		{ 105, 15, 1, 0, 20, 4, NULL, 0, "UpdateOriginD A" },
		{ 107, 13, 2, 1, 20, 6, NULL, 0, "UpdateOriginD B" },
		{ 101, 14, 0, 0, 18, 5, NULL, 0, "UpdateOrigin A" },
		{ 105, 14, 3, 1, 20, 5, NULL, 0, "UpdateOrigin B" }
	};
	FuncPattern PADOriginCallbackSigs[4] = {
		{  39, 17,  4, 5,  3,  3, NULL, 0, "PADOriginCallbackD A" },
		{  71, 27, 10, 6,  2,  9, NULL, 0, "PADOriginCallback A" },
		{  49, 21,  6, 6,  1,  8, NULL, 0, "PADOriginCallback B" },
		{ 143, 30,  6, 6, 21, 11, NULL, 0, "PADOriginCallback C" }	// SN Systems ProDG
	};
	FuncPattern PADOriginUpdateCallbackSigs[6] = {
		{  31, 10,  4, 2,  3,  5, NULL, 0, "PADOriginUpdateCallbackD A" },
		{  34,  8,  2, 3,  4,  4, NULL, 0, "PADOriginUpdateCallbackD B" },
		{  15,  4,  2, 1,  1,  4, NULL, 0, "PADOriginUpdateCallback A" },
		{  48, 13,  9, 5,  2,  9, NULL, 0, "PADOriginUpdateCallback B" },
		{ 143, 23, 10, 5, 22, 13, NULL, 0, "PADOriginUpdateCallback C" },	// SN Systems ProDG
		{  51, 14, 10, 5,  2, 10, NULL, 0, "PADOriginUpdateCallback D" }
	};
	FuncPattern PADInitSigs[10] = {
		{  88, 37,  4,  8, 3, 11, NULL, 0, "PADInitD A" },
		{  90, 37,  5,  8, 6, 11, NULL, 0, "PADInitD B" },
		{ 113, 31, 11, 11, 1, 17, NULL, 0, "PADInit A" },
		{ 129, 42, 13, 12, 2, 18, NULL, 0, "PADInit B" },
		{ 132, 43, 14, 12, 5, 18, NULL, 0, "PADInit C" },
		{ 133, 43, 14, 12, 5, 18, NULL, 0, "PADInit D" },
		{ 132, 42, 14, 12, 5, 18, NULL, 0, "PADInit E" },
		{ 134, 43, 14, 13, 5, 18, NULL, 0, "PADInit F" },
		{ 123, 38, 16, 10, 5, 27, NULL, 0, "PADInit G" },	// SN Systems ProDG
		{  84, 24,  8,  9, 4,  9, NULL, 0, "PADInit H" }
	};
	FuncPattern PADReadSigs[7] = {
		{ 172, 65,  3, 15, 16, 18, NULL, 0, "PADReadD A" },
		{ 171, 66,  4, 20, 17, 14, NULL, 0, "PADReadD B" },
		{ 206, 78,  7, 20, 17, 19, NULL, 0, "PADRead A" },
		{ 237, 87, 13, 27, 17, 25, NULL, 0, "PADRead B" },
		{ 235, 86, 13, 27, 17, 24, NULL, 0, "PADRead C" },
		{ 233, 71, 13, 29, 17, 27, NULL, 0, "PADRead D" },	// SN Systems ProDG
		{ 192, 73,  8, 23, 16, 15, NULL, 0, "PADRead E" }
	};
	FuncPattern PADSetSpecSigs[2] = {
		{ 42, 15, 8, 1, 9, 3, NULL, 0, "PADSetSpecD" },
		{ 24,  7, 5, 0, 8, 0, NULL, 0, "PADSetSpec" }
	};
	FuncPattern SPEC0_MakeStatusSigs[3] = {
		{ 96, 28, 0, 0, 12, 9, NULL, 0, "SPEC0_MakeStatusD A" },
		{ 93, 26, 0, 0, 11, 9, NULL, 0, "SPEC0_MakeStatus A" },
		{ 85, 18, 0, 0,  2, 8, NULL, 0, "SPEC0_MakeStatus B" }	// SN Systems ProDG
	};
	FuncPattern SPEC1_MakeStatusSigs[3] = {
		{ 96, 28, 0, 0, 12, 9, NULL, 0, "SPEC1_MakeStatusD A" },
		{ 93, 26, 0, 0, 11, 9, NULL, 0, "SPEC1_MakeStatus A" },
		{ 85, 18, 0, 0,  2, 8, NULL, 0, "SPEC1_MakeStatus B" }	// SN Systems ProDG
	};
	FuncPattern SPEC2_MakeStatusSigs[4] = {
		{ 186, 43, 3, 6, 20, 14, NULL, 0, "SPEC2_MakeStatusD A" },
		{ 254, 46, 0, 0, 42, 71, NULL, 0, "SPEC2_MakeStatus A" },
		{ 234, 46, 0, 0, 42, 51, NULL, 0, "SPEC2_MakeStatus B" },	// SN Systems ProDG
		{ 284, 55, 2, 0, 43, 76, NULL, 0, "SPEC2_MakeStatus C" }
	};
	u32 _SDA2_BASE_ = 0, _SDA_BASE_ = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!_SDA2_BASE_ && !_SDA_BASE_) {
			if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
				(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
				(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
				(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
				_SDA2_BASE_ = (data[i + 0] << 16) | (u16)data[i + 1];
				_SDA_BASE_  = (data[i + 2] << 16) | (u16)data[i + 3];
				i += 4;
			}
			continue;
		}
		if ((data[i - 1] != 0x4C000064 && data[i - 1] != 0x4E800020) ||
			(data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6))
			continue;
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
		for (j = 0; j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern); j++) {
			if (!PADOriginCallbackSigs[j].offsetFoundAt && compare_pattern(&fp, &PADOriginCallbackSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 31, length, &UpdateOriginSigs[0]) ||
							findx_pattern(data, dataType, i + 31, length, &UpdateOriginSigs[1]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 10, length, &UpdateOriginSigs[2]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  7, length, &UpdateOriginSigs[3]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 3:
						PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
			if (PADOriginCallbackSigs[j].offsetFoundAt && i == PADOriginCallbackSigs[j].offsetFoundAt + PADOriginCallbackSigs[j].Length) {
				for (k = 0; k < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern); k++) {
					if (!PADOriginUpdateCallbackSigs[k].offsetFoundAt && compare_pattern(&fp, &PADOriginUpdateCallbackSigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i +  25, length, &UpdateOriginSigs[0]))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i +  24, length, &UpdateOriginSigs[1]))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
							case 2:
								if (findx_pattern(data, dataType, i +  10, length, &UpdateOriginSigs[2]))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
							case 3:
								if (findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i +  40, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  16, length, &UpdateOriginSigs[3]))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
							case 4:
								if (findx_pattern(data, dataType, i + 113, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i + 134, length, &OSRestoreInterruptsSig))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
							case 5:
								if (findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
									findx_pattern(data, dataType, i +  43, length, &OSRestoreInterruptsSig) &&
									findx_pattern(data, dataType, i +  16, length, &UpdateOriginSigs[3]))
									PADOriginUpdateCallbackSigs[k].offsetFoundAt = i;
								break;
						}
						break;
					}
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(PADInitSigs) / sizeof(FuncPattern); j++) {
			if (!PADInitSigs[j].offsetFoundAt && compare_pattern(&fp, &PADInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  11, length, &PADSetSpecSigs[0]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  13, length, &PADSetSpecSigs[0]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  75, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 106, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  12, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  74, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 122, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  12, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 125, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 126, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  76, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 125, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  78, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 127, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  16, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  67, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 115, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  15, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  16, length, &PADSetSpecSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		
		for (j = 0; j < sizeof(PADReadSigs) / sizeof(FuncPattern); j++) {
			if (!PADReadSigs[j].offsetFoundAt && compare_pattern(&fp, &PADReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  55, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  75, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 164, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  64, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  81, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  83, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 114, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  26, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 159, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 230, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  24, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 157, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 228, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 225, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  90, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 114, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 185, length, &OSRestoreInterruptsSig))
							PADReadSigs[j].offsetFoundAt = i;
						break;
				}
				break;
			}
		}
		i += fp.Length - 1;
	}
	
	for (j = 0; j < sizeof(UpdateOriginSigs) / sizeof(FuncPattern); j++)
		if (UpdateOriginSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(UpdateOriginSigs) / sizeof(FuncPattern) && (i = UpdateOriginSigs[j].offsetFoundAt)) {
		u32 *UpdateOrigin = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (UpdateOrigin) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0: data[i + 79] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 1: data[i + 82] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 77] = 0x20A5007F; break;	// subfic	r5, r5, 127
					case 3: data[i + 81] = 0x2084007F; break;	// subfic	r4, r4, 127
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0: data[i + 82] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 1: data[i + 85] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 80] = 0x20A5007F; break;	// subfic	r5, r5, 127
					case 3: data[i + 84] = 0x2084007F; break;	// subfic	r4, r4, 127
				}
			}
			print_gecko("Found:[%s] @ %08X\n", UpdateOriginSigs[j].Name, UpdateOrigin);
		}
	}
	
	for (j = 0; j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern); j++)
		if (PADOriginCallbackSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern) && (i = PADOriginCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginCallback) {
			if (j == 3) {
				if (swissSettings.invertCStick & 1)
					data[i + 86] = 0x2004007F;	// subfic	r0, r4, 127
				if (swissSettings.invertCStick & 2)
					data[i + 89] = 0x2004007F;	// subfic	r0, r4, 127
			}
			print_gecko("Found:[%s] @ %08X\n", PADOriginCallbackSigs[j].Name, PADOriginCallback);
		}
	}
	
	for (j = 0; j < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern); j++)
		if (PADOriginUpdateCallbackSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern) && (i = PADOriginUpdateCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginUpdateCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginUpdateCallback) {
			if (j == 4) {
				if (swissSettings.invertCStick & 1)
					data[i + 93] = 0x2003007F;	// subfic	r0, r3, 127
				if (swissSettings.invertCStick & 2)
					data[i + 96] = 0x2003007F;	// subfic	r0, r3, 127
			}
			print_gecko("Found:[%s] @ %08X\n", PADOriginUpdateCallbackSigs[j].Name, PADOriginUpdateCallback);
		}
	}
	
	for (j = 0; j < sizeof(PADInitSigs) / sizeof(FuncPattern); j++)
		if (PADInitSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(PADInitSigs) / sizeof(FuncPattern) && (i = PADInitSigs[j].offsetFoundAt)) {
		u32 *PADInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADInit) {
			print_gecko("Found:[%s] @ %08X\n", PADInitSigs[j].Name, PADInit);
		}
	}
	
	for (k = 0; k < sizeof(PADReadSigs) / sizeof(FuncPattern); k++)
		if (PADReadSigs[k].offsetFoundAt) break;
	
	for (j = 0; j < sizeof(PADSetSpecSigs) / sizeof(FuncPattern); j++)
		if (PADSetSpecSigs[j].offsetFoundAt) break;
	
	if (k < sizeof(PADReadSigs) / sizeof(FuncPattern) && (i = PADReadSigs[k].offsetFoundAt)) {
		u32 *PADRead = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *PADReadHook;
		
		u32 MakeStatusAddr, *MakeStatus;
		
		if (PADRead) {
			if (devices[DEVICE_CUR] == &__device_dvd)
				PADReadHook = IGR_CHECK_DVD;
			else if (devices[DEVICE_CUR] == &__device_wkf)
				PADReadHook = IGR_CHECK_WKF;
			else if ((devices[DEVICE_CUR]->features & FEAT_ALT_READ_PATCHES) || swissSettings.alternateReadPatches)
				PADReadHook = IGR_CHECK_ALT;
			else
				PADReadHook = IGR_CHECK;
			
			switch (k) {
				case 0: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 132]; break;
				case 1: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 128]; break;
				case 2: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 163]; break;
				case 3: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 194]; break;
				case 4: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 192]; break;
				case 5: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 188]; break;
				case 6: MakeStatusAddr = _SDA_BASE_ + (s16)data[i + 149]; break;
			}
			
			if ((MakeStatus = Calc_Address(data, dataType, MakeStatusAddr)) &&
				(MakeStatus = Calc_Address(data, dataType, MakeStatusAddr = *MakeStatus))) {
				
				switch (k) {
					case 0:
					case 1: find_pattern(data, MakeStatus - data, length, &SPEC2_MakeStatusSigs[0]); break;
					case 2:
					case 3:
					case 4: find_pattern(data, MakeStatus - data, length, &SPEC2_MakeStatusSigs[1]); break;
					case 5: find_pattern(data, MakeStatus - data, length, &SPEC2_MakeStatusSigs[2]); break;
					case 6: find_pattern(data, MakeStatus - data, length, &SPEC2_MakeStatusSigs[3]); break;
				}
			}
			
			if (swissSettings.igrType != IGR_OFF) {
				switch (k) {
					case 0:
						data[i + 159] = 0x387E0000;	// addi		r3, r30, 0
						data[i + 160] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 161] = branchAndLink(PADReadHook, PADRead + 161);
						break;
					case 1:
						data[i + 156] = 0x387E0000;	// addi		r3, r30, 0
						data[i + 157] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 158] = branchAndLink(PADReadHook, PADRead + 158);
						break;
					case 2:
						data[i + 190] = 0x38770000;	// addi		r3, r23, 0
						data[i + 191] = 0x38950000;	// addi		r4, r21, 0
						data[i + 192] = branchAndLink(PADReadHook, PADRead + 192);
						break;
					case 3:
						data[i + 221] = 0x38750000;	// addi		r3, r21, 0
						data[i + 222] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 223] = branchAndLink(PADReadHook, PADRead + 223);
						break;
					case 4:
						data[i + 219] = 0x38750000;	// addi		r3, r21, 0
						data[i + 220] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 221] = branchAndLink(PADReadHook, PADRead + 221);
						break;
					case 5:
						data[i + 216] = 0x7F63DB78;	// mr		r3, r27
						data[i + 217] = 0x7F24CB78;	// mr		r4, r25
						data[i + 218] = branchAndLink(PADReadHook, PADRead + 218);
						break;
					case 6:
						data[i + 176] = 0x38790000;	// addi		r3, r25, 0
						data[i + 177] = 0x38970000;	// addi		r4, r23, 0
						data[i + 178] = branchAndLink(PADReadHook, PADRead + 178);
						break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", PADReadSigs[k].Name, PADRead);
		}
		
		if (j < sizeof(PADSetSpecSigs) / sizeof(FuncPattern) && (i = PADSetSpecSigs[j].offsetFoundAt)) {
			u32 *PADSetSpec = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			u32 SPEC0_MakeStatusAddr, *SPEC0_MakeStatus;
			u32 SPEC1_MakeStatusAddr, *SPEC1_MakeStatus;
			u32 SPEC2_MakeStatusAddr, *SPEC2_MakeStatus;
			
			if (PADSetSpec && PADRead) {
				if (j >= 1) {
					SPEC0_MakeStatusAddr = (data[i + 11] << 16) + (s16)data[i + 12];
					SPEC1_MakeStatusAddr = (data[i + 15] << 16) + (s16)data[i + 16];
					SPEC2_MakeStatusAddr = (data[i + 19] << 16) + (s16)data[i + 20];
				} else {
					SPEC0_MakeStatusAddr = (data[i + 25] << 16) + (s16)data[i + 26];
					SPEC1_MakeStatusAddr = (data[i + 29] << 16) + (s16)data[i + 30];
					SPEC2_MakeStatusAddr = (data[i + 33] << 16) + (s16)data[i + 34];
				}
				
				SPEC0_MakeStatus = Calc_Address(data, dataType, SPEC0_MakeStatusAddr);
				SPEC1_MakeStatus = Calc_Address(data, dataType, SPEC1_MakeStatusAddr);
				SPEC2_MakeStatus = Calc_Address(data, dataType, SPEC2_MakeStatusAddr);
				
				switch (k) {
					case 0:
					case 1:
						find_pattern(data, SPEC0_MakeStatus - data, length, &SPEC0_MakeStatusSigs[0]);
						find_pattern(data, SPEC1_MakeStatus - data, length, &SPEC1_MakeStatusSigs[0]);
						find_pattern(data, SPEC2_MakeStatus - data, length, &SPEC2_MakeStatusSigs[0]);
						break;
					case 2:
					case 3:
					case 4:
						find_pattern(data, SPEC0_MakeStatus - data, length, &SPEC0_MakeStatusSigs[1]);
						find_pattern(data, SPEC1_MakeStatus - data, length, &SPEC1_MakeStatusSigs[1]);
						find_pattern(data, SPEC2_MakeStatus - data, length, &SPEC2_MakeStatusSigs[1]);
						break;
					case 5:
						find_pattern(data, SPEC0_MakeStatus - data, length, &SPEC0_MakeStatusSigs[2]);
						find_pattern(data, SPEC1_MakeStatus - data, length, &SPEC1_MakeStatusSigs[2]);
						find_pattern(data, SPEC2_MakeStatus - data, length, &SPEC2_MakeStatusSigs[2]);
						break;
					case 6:
						find_pattern(data, SPEC0_MakeStatus - data, length, &SPEC0_MakeStatusSigs[1]);
						find_pattern(data, SPEC1_MakeStatus - data, length, &SPEC1_MakeStatusSigs[1]);
						find_pattern(data, SPEC2_MakeStatus - data, length, &SPEC2_MakeStatusSigs[3]);
						break;
				}
				print_gecko("Found:[%s] @ %08X\n", PADSetSpecSigs[j].Name, PADSetSpec);
			}
		}
	}
	
	for (j = 0; j < sizeof(SPEC0_MakeStatusSigs) / sizeof(FuncPattern); j++)
		if (SPEC0_MakeStatusSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(SPEC0_MakeStatusSigs) / sizeof(FuncPattern) && (i = SPEC0_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC0_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC0_MakeStatus) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0: data[i + 90] = 0x20030080; break;	// subfic	r0, r3, 128
					case 1: data[i + 87] = 0x20030080; break;	// subfic	r0, r3, 128
					case 2: data[i + 79] = 0x20030080; break;	// subfic	r0, r3, 128
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0: data[i + 93] = 0x20030080; break;	// subfic	r0, r3, 128
					case 1: data[i + 90] = 0x20030080; break;	// subfic	r0, r3, 128
					case 2: data[i + 82] = 0x20030080; break;	// subfic	r0, r3, 128
				}
			}
			print_gecko("Found:[%s] @ %08X\n", SPEC0_MakeStatusSigs[j].Name, SPEC0_MakeStatus);
		}
	}
	
	for (j = 0; j < sizeof(SPEC1_MakeStatusSigs) / sizeof(FuncPattern); j++)
		if (SPEC1_MakeStatusSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(SPEC1_MakeStatusSigs) / sizeof(FuncPattern) && (i = SPEC1_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC1_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC1_MakeStatus) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0: data[i + 90] = 0x20030080; break;	// subfic	r0, r3, 128
					case 1: data[i + 87] = 0x20030080; break;	// subfic	r0, r3, 128
					case 2: data[i + 79] = 0x20030080; break;	// subfic	r0, r3, 128
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0: data[i + 93] = 0x20030080; break;	// subfic	r0, r3, 128
					case 1: data[i + 90] = 0x20030080; break;	// subfic	r0, r3, 128
					case 2: data[i + 82] = 0x20030080; break;	// subfic	r0, r3, 128
				}
			}
			print_gecko("Found:[%s] @ %08X\n", SPEC1_MakeStatusSigs[j].Name, SPEC1_MakeStatus);
		}
	}
	
	for (j = 0; j < sizeof(SPEC2_MakeStatusSigs) / sizeof(FuncPattern); j++)
		if (SPEC2_MakeStatusSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(SPEC2_MakeStatusSigs) / sizeof(FuncPattern) && (i = SPEC2_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC2_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC2_MakeStatus) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0: data[i + 147] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 1: data[i + 142] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 2: data[i + 130] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 3: data[i + 142] = 0x2006007F; break;	// subfic	r0, r6, 127
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0: data[i + 150] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 1: data[i + 145] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 2: data[i + 133] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 3: data[i + 145] = 0x2006007F; break;	// subfic	r0, r6, 127
				}
			}
			print_gecko("Found:[%s] @ %08X\n", SPEC2_MakeStatusSigs[j].Name, SPEC2_MakeStatus);
		}
	}
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


