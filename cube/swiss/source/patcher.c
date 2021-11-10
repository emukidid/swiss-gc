/* DOL Patching code by emu_kidid */


#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdarg.h>
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

static u32 top_addr;

static void *patch_locations[PATCHES_MAX];

// Returns where the ASM patch has been copied to
void *installPatch(int patchId) {
	u32 patchSize = 0;
	void *patchLocation = NULL;
	switch(patchId) {
		case DVD_LOWTESTALARMHOOK:
			patchSize = DVDLowTestAlarmHook_length; patchLocation = DVDLowTestAlarmHook; break;
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
		case OS_RESERVED:
			patchSize = 0x1800; break;
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
		default:
			break;
	}
	patchLocation = installPatch2(patchLocation, patchSize);
	print_gecko("Installed patch %i to %08X\r\n", patchId, patchLocation);
	return patchLocation;
}

void *installPatch2(void *patchLocation, u32 patchSize) {
	if (top_addr == 0x81800000)
		top_addr -= 8;
	top_addr -= patchSize;
	if (patchLocation) {
		top_addr &= ~3;
		patchLocation = memcpy((void*)top_addr, patchLocation, patchSize);
	} else {
		top_addr &= ~31;
		patchLocation = memset((void*)top_addr, 0, patchSize);
	}
	DCFlushRange(patchLocation, patchSize);
	ICInvalidateRange(patchLocation, patchSize);
	return patchLocation;
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

void setTopAddr(u32 addr) {
	top_addr = addr & ~3;
	int patchId;
	for (patchId = 0; patchId < PATCHES_MAX; patchId++)
		if (patch_locations[patchId] < (void*)top_addr)
			patch_locations[patchId] = NULL;
}

u32 getTopAddr() {
	return top_addr & ~31;
}

int install_code(int final)
{
	u32 location = LO_RESERVE;
	u8 *patch = NULL; u32 patchSize = 0;
	
	// Reload Stub
	if (GCMDisk.DVDMagicWord != DVD_MAGIC) {
		location = 0x80001800;
		setTopAddr(0x80003000);
		patch     = stub_bin;
		patchSize = stub_bin_size;
		
		print_gecko("Installing Reload Stub\r\n");
	}
	// IDE-EXI
	else if(devices[DEVICE_CUR] == &__device_ata_a || devices[DEVICE_CUR] == &__device_ata_b || devices[DEVICE_CUR] == &__device_ata_c) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ:
			case EMU_READ | EMU_READ_SPEED:
				patch     = !_ideexi_version ? ideexi_v1_bin      : ideexi_v2_bin;
				patchSize = !_ideexi_version ? ideexi_v1_bin_size : ideexi_v2_bin_size;
				break;
			case EMU_READ | EMU_AUDIO_STREAMING:
				patch     = !_ideexi_version ? ideexi_v1_dtk_bin      : ideexi_v2_dtk_bin;
				patchSize = !_ideexi_version ? ideexi_v1_dtk_bin_size : ideexi_v2_dtk_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD:
				patch     = !_ideexi_version ? ideexi_v1_card_bin      : ideexi_v2_card_bin;
				patchSize = !_ideexi_version ? ideexi_v1_card_bin_size : ideexi_v2_card_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for IDE-EXI\r\n");
	}
	// SD Card over EXI
	else if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ:
			case EMU_READ | EMU_READ_SPEED:
				patch     = sd_bin;
				patchSize = sd_bin_size;
				break;
			case EMU_READ | EMU_AUDIO_STREAMING:
				patch     = sd_dtk_bin;
				patchSize = sd_dtk_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD:
				patch     = sd_card_bin;
				patchSize = sd_card_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for SD Card over EXI\r\n");
	}
	// DVD
	else if(devices[DEVICE_CUR] == &__device_dvd || devices[DEVICE_CUR] == &__device_wode) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_NONE:
				patch     = dvd_bin;
				patchSize = dvd_bin_size;
				break;
			case EMU_MEMCARD:
				patch     = dvd_card_bin;
				patchSize = dvd_card_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for DVD\r\n");
	}
	// USB Gecko
	else if(devices[DEVICE_CUR] == &__device_usbgecko) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ:
				patch     = usbgecko_bin;
				patchSize = usbgecko_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for USB Gecko\r\n");
	}
	// Wiikey Fusion
	else if(devices[DEVICE_CUR] == &__device_wkf) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ:
				patch     = wkf_bin;
				patchSize = wkf_bin_size;
				break;
			case EMU_READ | EMU_AUDIO_STREAMING:
				patch     = wkf_dtk_bin;
				patchSize = wkf_dtk_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD:
				patch     = wkf_card_bin;
				patchSize = wkf_card_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for WKF\r\n");
	}
	// Broadband Adapter
	else if(devices[DEVICE_CUR] == &__device_fsp) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ:
				patch     = fsp_bin;
				patchSize = fsp_bin_size;
				break;
			default:
				return 0;
		}
		print_gecko("Installing Patch for File Service Protocol\r\n");
	}
	// GC Loader
	else if(devices[DEVICE_CUR] == &__device_gcloader) {
		if (devices[DEVICE_PATCHES] != &__device_gcloader) {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_NONE:
				case EMU_READ_SPEED:
					patch     = gcloader_v1_bin;
					patchSize = gcloader_v1_bin_size;
					break;
				case EMU_MEMCARD:
					patch     = gcloader_v1_card_bin;
					patchSize = gcloader_v1_card_bin_size;
					break;
				default:
					return 0;
			}
		} else {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_NONE:
				case EMU_READ_SPEED:
					patch     = gcloader_v2_bin;
					patchSize = gcloader_v2_bin_size;
					break;
				case EMU_MEMCARD:
					patch     = gcloader_v2_card_bin;
					patchSize = gcloader_v2_card_bin_size;
					break;
				default:
					return 0;
			}
		}
		print_gecko("Installing Patch for GC Loader\r\n");
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
		
		if ((word == 0x4E800020 && j <= i) || word == 0x4C000064)
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
	
	if (offsetFoundAt >= length / sizeof(u32))
		return false;
	
	while (offsetFoundAt &&
		data[offsetFoundAt - 1] != 0x4E800020 &&
		data[offsetFoundAt - 1] != 0x4C000064)
		offsetFoundAt--;
	
	return find_pattern(data, offsetFoundAt, length, FPatB);
}

bool find_pattern_after(u32 *data, u32 length, FuncPattern *FPatA, FuncPattern *FPatB)
{
	u32 offsetFoundAt = FPatA->offsetFoundAt + FPatA->Length;
	
	if (offsetFoundAt == FPatB->offsetFoundAt)
		return true;
	
	if (offsetFoundAt >= length / sizeof(u32))
		return false;
	
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

static u32 _SDA2_BASE_, _SDA_BASE_;

bool get_immediate(u32 *data, u32 offsetFoundAt, u32 offsetFoundAt2, u32 *immediate)
{
	u32 word = data[offsetFoundAt];
	
	switch (word >> 26) {
		case 15:
		{
			u32 word2 = data[offsetFoundAt2];
			
			switch (word2 >> 26) {
				case 10:
				{
					if (((word2 >> 16) & 0x1F) != ((word >> 21) & 0x1F))
						return false;
					
					*immediate = -(word << 16) + (u16)word2;
					return true;
				}
				case 11:
				{
					if (((word2 >> 16) & 0x1F) != ((word >> 21) & 0x1F))
						return false;
					
					*immediate = -(word << 16) + (s16)word2;
					return true;
				}
			}
			
			if (((word >> 16) & 0x1F) != 0)
				return false;
			
			switch (word2 >> 26) {
				case 14:
				case 32 ... 55:
				{
					if (((word2 >> 16) & 0x1F) != ((word >> 21) & 0x1F))
						return false;
					
					*immediate = (word << 16) + (s16)word2;
					return true;
				}
				case 24:
				{
					if (((word2 >> 21) & 0x1F) != ((word >> 21) & 0x1F))
						return false;
					
					*immediate = (word << 16) | (u16)word2;
					return true;
				}
			}
		}
		case 14:
		case 32 ... 55:
		{
			switch (offsetFoundAt2) {
				case 0:
				{
					if (((word >> 16) & 0x1F) != 0)
						return false;
					
					*immediate = (s16)word;
					return true;
				}
				case 2:
				{
					if (((word >> 16) & 0x1F) != 2)
						return false;
					
					*immediate = _SDA2_BASE_ + (s16)word;
					return true;
				}
				case 13:
				{
					if (((word >> 16) & 0x1F) != 13)
						return false;
					
					*immediate = _SDA_BASE_ + (s16)word;
					return true;
				}
			}
		}
	}
	
	return false;
}

void *loadResolve(u32 *data, int dataType, u32 offsetFoundAt, u32 offsetFoundAt2)
{
	u32 address;
	
	if (get_immediate(data, offsetFoundAt, offsetFoundAt2, &address))
		return Calc_Address(data, dataType, address);
	
	return NULL;
}

bool findi_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 offsetFoundAt2, u32 length, FuncPattern *functionPattern)
{
	u32 *address = loadResolve(data, dataType, offsetFoundAt, offsetFoundAt2);
	
	if (functionPattern && functionPattern->offsetFoundAt)
		return address == data + functionPattern->offsetFoundAt;
	
	return address && find_pattern(data, address - data, length, functionPattern);
}

bool findi_patterns(u32 *data, int dataType, u32 offsetFoundAt, u32 offsetFoundAt2, u32 length, ...)
{
	FuncPattern *functionPattern;
	
	va_list args;
	va_start(args, length);
	
	while ((functionPattern = va_arg(args, FuncPattern *)))
		if (findi_pattern(data, dataType, offsetFoundAt, offsetFoundAt2, length, functionPattern))
			break;
	
	va_end(args);
	
	return functionPattern;
}

bool findp_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 offsetFoundAt2, u32 length, FuncPattern *functionPattern)
{
	u32 *address;
	
	if ((address = loadResolve(data, dataType, offsetFoundAt, offsetFoundAt2)))
		address = Calc_Address(data, dataType, *address);
	
	if (functionPattern && functionPattern->offsetFoundAt)
		return address == data + functionPattern->offsetFoundAt;
	
	return address && find_pattern(data, address - data, length, functionPattern);
}

bool findp_patterns(u32 *data, int dataType, u32 offsetFoundAt, u32 offsetFoundAt2, u32 length, ...)
{
	FuncPattern *functionPattern;
	
	va_list args;
	va_start(args, length);
	
	while ((functionPattern = va_arg(args, FuncPattern *)))
		if (findp_pattern(data, dataType, offsetFoundAt, offsetFoundAt2, length, functionPattern))
			break;
	
	va_end(args);
	
	return functionPattern;
}

bool findx_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
{
	offsetFoundAt = branchResolve(data, dataType, offsetFoundAt);
	
	if (functionPattern && functionPattern->offsetFoundAt)
		return offsetFoundAt == functionPattern->offsetFoundAt;
	
	return offsetFoundAt && find_pattern(data, offsetFoundAt, length, functionPattern);
}

bool findx_patterns(u32 *data, int dataType, u32 offsetFoundAt, u32 length, ...)
{
	FuncPattern *functionPattern;
	
	va_list args;
	va_start(args, length);
	
	while ((functionPattern = va_arg(args, FuncPattern *)))
		if (findx_pattern(data, dataType, offsetFoundAt, length, functionPattern))
			break;
	
	va_end(args);
	
	return functionPattern;
}

u32 _ppchalt[] = {
	0x7C0004AC,	// sync
	0x60000000,	// nop
	0x38600000,	// li		r3, 0
	0x60000000,	// nop
	0x4BFFFFF4	// b		-3
};

u32 _dvdgettransferredsize[] = {
	0x8003000C,	// lwz		r0, 12 (r3)
	0x2C000002,	// cmpwi	r0, 2
	0x41820040,	// beq		+16
	0x40800018,	// bge		+6
	0x2C000001,	// cmpwi	r0, 1
	0x4080003C,	// bge		+15
	0x2C00FFFF,	// cmpwi	r0, -1
	0x40800024,	// bge		+9
	0x4E800020,	// blr
	0x2C00000A,	// cmpwi	r0, 10
	0x40800010,	// bge		+4
	0x2C000008,	// cmpwi	r0, 8
	0x4C800020,	// bgelr
	0x4800000C,	// b		+3
	0x2C00000C,	// cmpwi	r0, 12
	0x4C800020,	// bgelr
	0x80630020,	// lwz		r3, 32 (r3)
	0x4E800020,	// blr
	0x38600000,	// li		r3, 0
	0x4E800020,	// blr
	0x3C80CC00,	// lis		r4, 0xCC00
	0x8003001C,	// lwz		r0, 28 (r3)
	0x80A30020,	// lwz		r5, 32 (r3)
	0x80646018,	// lwz		r3, 0x6018 (r4)
	0x7C030050,	// subf		r0, r3, r0
	0x7C650214,	// add		r3, r5, r0
	0x4E800020	// blr
};

u32 _aigetdmastartaddrd[] = {
	0x3C60CC00,	// lis		r3, 0xCC00
	0xA0835030,	// lhz		r4, 0x5030 (r3)
	0x3C60CC00,	// lis		r3, 0xCC00
	0xA0035032,	// lhz		r0, 0x5032 (r3)
	0x54030434,	// rlwinm	r3, r0, 0, 16, 26
	0x5083819E,	// rlwimi	r3, r4, 16, 6, 15
	0x4E800020	// blr
};

u32 _aigetdmastartaddr[] = {
	0x3C60CC00,	// lis		r3, 0xCC00
	0x38635000,	// addi		r3, r3, 0x5000
	0xA0830030,	// lhz		r4, 0x0030 (r3)
	0xA0030032,	// lhz		r0, 0x0032 (r3)
	0x54030434,	// rlwinm	r3, r0, 0, 16, 26
	0x5083819E,	// rlwimi	r3, r4, 16, 6, 15
	0x4E800020	// blr
};

u32 _gxpeekz_a[] = {
	0x546013BA,	// clrlslwi	r0, r3, 16, 2
	0x6400C800,	// oris		r0, r0, 0xC800
	0x54030512,	// rlwinm	r3, r0, 0, 20, 9
	0x54806126,	// clrlslwi	r0, r4, 16, 12
	0x7C600378,	// or		r0, r3, r0
	0x5400028E,	// rlwinm	r0, r0, 0, 10, 7
	0x64030040,	// oris		r3, r0, 0x0040
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

u32 _gxpeekz_b[] = {
	0x5463103A,	// slwi		r3, r3, 2
	0x54846026,	// slwi		r4, r4, 12
	0x6463C800,	// oris		r3, r3, 0xC800
	0x54600512,	// rlwinm	r0, r3, 0, 20, 9
	0x7C032378,	// or		r3, r0, r4
	0x5460028E,	// rlwinm	r0, r3, 0, 10, 7
	0x64030040,	// oris		r3, r0, 0x0040
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

u32 _gxpeekz_c[] = {
	0x5460043E,	// clrlwi	r0, r3, 16
	0x3C60C800,	// lis		r3, 0xC800
	0x5003153A,	// insrwi	r3, r0, 10, 20
	0x38000001,	// li		r0, 1
	0x508362A6,	// insrwi	r3, r4, 10, 10
	0x5003B212,	// insrwi	r3, r0, 2, 8
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

int Patch_Hypervisor(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	int patched = 0;
	u32 address;
	FuncPattern PrepareExecSigs[2] = {
		{ 54, 19, 3, 17,  3, 3, NULL, 0, "PrepareExecD" },
		{ 60, 15, 3, 16, 13, 2, NULL, 0, "PrepareExec" }
	};
	FuncPattern PPCHaltSig = 
		{ 5, 1, 0, 0, 1, 1, NULL, 0, "PPCHalt" };
	FuncPattern OSInitSigs[29] = {
		{ 200,  79, 12, 34, 22,  9, NULL, 0, "OSInitD" },
		{ 201,  79, 12, 35, 22,  9, NULL, 0, "OSInitD" },
		{ 202,  79, 12, 36, 22,  9, NULL, 0, "OSInitD" },
		{ 206,  79, 12, 37, 22, 10, NULL, 0, "OSInitD" },
		{ 234,  90, 17, 44, 24,  9, NULL, 0, "OSInitD" },
		{ 237,  91, 17, 44, 24,  9, NULL, 0, "OSInitD" },
		{ 253,  97, 17, 49, 24,  9, NULL, 0, "OSInitD" },
		{ 261, 101, 17, 49, 26,  9, NULL, 0, "OSInitD" },
		{ 231,  82, 14, 49, 25,  8, NULL, 0, "OSInitD" },
		{ 256,  91, 14, 56, 30, 10, NULL, 0, "OSInitD" },
		{ 258,  92, 14, 57, 30, 10, NULL, 0, "OSInitD" },
		{ 153,  53,  7, 29, 20,  7, NULL, 0, "OSInit" },
		{ 161,  56, 11, 30, 21,  7, NULL, 0, "OSInit" },
		{ 182,  67, 14, 30, 23,  8, NULL, 0, "OSInit" },
		{ 183,  67, 14, 31, 23,  8, NULL, 0, "OSInit" },
		{ 186,  67, 14, 33, 23,  8, NULL, 0, "OSInit" },
		{ 190,  67, 16, 35, 23,  8, NULL, 0, "OSInit" },
		{ 192,  67, 16, 37, 23,  8, NULL, 0, "OSInit" },
		{ 208,  72, 19, 36, 25,  8, NULL, 0, "OSInit" },
		{ 209,  72, 19, 37, 25,  8, NULL, 0, "OSInit" },
		{ 208,  72, 19, 36, 25,  8, NULL, 0, "OSInit" },
		{ 212,  73, 19, 37, 25,  8, NULL, 0, "OSInit" },
		{ 212,  75, 19, 37, 26,  8, NULL, 0, "OSInit" },
		{ 230,  80, 20, 42, 25,  8, NULL, 0, "OSInit" },
		{ 238,  84, 20, 42, 27,  8, NULL, 0, "OSInit" },
		{ 222,  74, 17, 43, 27,  8, NULL, 0, "OSInit" },
		{ 277,  90, 20, 63, 30, 15, NULL, 0, "OSInit" },	// SN Systems ProDG
		{ 246,  83, 17, 50, 32, 10, NULL, 0, "OSInit" },
		{ 312,  97, 17, 71, 39, 17, NULL, 0, "OSInit" }
	};
	FuncPattern OSExceptionInitSigs[4] = {
		{ 164, 61,  6, 18, 14, 14, NULL, 0, "OSExceptionInitD" },
		{ 160, 39, 14, 14, 19,  7, NULL, 0, "OSExceptionInit" },
		{ 160, 39, 14, 14, 20,  7, NULL, 0, "OSExceptionInit" },
		{ 151, 45, 14, 16, 13,  9, NULL, 0, "OSExceptionInit" }	// SN Systems ProDG
	};
	FuncPattern __OSSetExceptionHandlerSigs[3] = {
		{ 35, 12, 3, 1, 2, 4, NULL, 0, "__OSSetExceptionHandlerD" },
		{  7,  2, 1, 0, 0, 1, NULL, 0, "__OSSetExceptionHandler" },
		{  5,  1, 0, 0, 0, 2, NULL, 0, "__OSSetExceptionHandler" }	// SN Systems ProDG
	};
	FuncPattern SetTimerSig = 
		{ 43, 13, 3, 4, 4, 10, NULL, 0, "SetTimerD" };
	FuncPattern InsertAlarmSigs[3] = {
		{ 123, 38, 17,  6, 11, 24, NULL, 0, "InsertAlarmD" },
		{ 148, 30, 16, 10, 17, 45, NULL, 0, "InsertAlarm" },
		{ 154, 31, 16, 12, 17, 51, NULL, 0, "InsertAlarm" }	// SN Systems ProDG
	};
	FuncPattern OSSetAlarmSigs[3] = {
		{ 58, 18, 4, 8, 0, 9, NULL, 0, "OSSetAlarmD" },
		{ 26,  5, 4, 4, 0, 5, NULL, 0, "OSSetAlarm" },
		{ 28,  3, 4, 6, 0, 9, NULL, 0, "OSSetAlarm" }	// SN Systems ProDG
	};
	FuncPattern OSCancelAlarmSigs[3] = {
		{ 52, 15,  7, 6, 5,  4, NULL, 0, "OSCancelAlarmD" },
		{ 71, 18, 10, 7, 9, 12, NULL, 0, "OSCancelAlarm" },
		{ 70, 18, 10, 7, 9, 13, NULL, 0, "OSCancelAlarm" }	// SN Systems ProDG
	};
	FuncPattern OSGetArenaHiSigs[3] = {
		{ 30, 15, 3, 2, 1, 3, NULL, 0, "OSGetArenaHiD" },
		{  2,  1, 0, 0, 0, 0, NULL, 0, "OSGetArenaHi" },
		{  2,  1, 0, 0, 0, 0, NULL, 0, "OSGetArenaHi" }	// SN Systems ProDG
	};
	FuncPattern OSGetArenaLoSigs[3] = {
		{ 30, 15, 3, 2, 1, 3, NULL, 0, "OSGetArenaLoD" },
		{  2,  1, 0, 0, 0, 0, NULL, 0, "OSGetArenaLo" },
		{  2,  1, 0, 0, 0, 0, NULL, 0, "OSGetArenaLo" }	// SN Systems ProDG
	};
	FuncPattern OSSetArenaHiSig = 
		{ 2, 0, 1, 0, 0, 0, NULL, 0, "OSSetArenaHi" };
	FuncPattern OSSetArenaLoSig = 
		{ 2, 0, 1, 0, 0, 0, NULL, 0, "OSSetArenaLo" };
	FuncPattern DCFlushRangeNoSyncSigs[2] = {
		{ 12, 3, 0, 0, 1, 2, NULL, 0, "DCFlushRangeNoSync" },
		{ 11, 2, 0, 0, 0, 3, NULL, 0, "DCFlushRangeNoSync" }
	};
	FuncPattern ICFlashInvalidateSig = 
		{ 4, 0, 0, 0, 0, 2, NULL, 0, "ICFlashInvalidate" };
	FuncPattern OSSetCurrentContextSig = 
		{ 23, 4, 4, 0, 0, 5, NULL, 0, "OSSetCurrentContext" };
	FuncPattern OSLoadContextSigs[3] = {
		{ 40, 16, 1, 0, 3, 10, NULL, 0, "OSLoadContext" },
		{ 54, 23, 1, 0, 3, 17, NULL, 0, "OSLoadContext" },
		{ 54, 23, 1, 0, 4, 17, NULL, 0, "OSLoadContext" }
	};
	FuncPattern OSClearContextSigs[3] = {
		{ 12, 6, 1, 0, 0, 1, NULL, 0, "OSClearContextD" },
		{  9, 3, 1, 0, 0, 1, NULL, 0, "OSClearContext" },
		{  9, 3, 1, 0, 0, 1, NULL, 0, "OSClearContext" }	// SN Systems ProDG
	};
	FuncPattern __OSUnhandledExceptionSigs[7] = {
		{ 106, 34, 2, 13, 13,  5, NULL, 0, "__OSUnhandledExceptionD" },
		{ 147, 58, 2, 23,  6,  7, NULL, 0, "__OSUnhandledExceptionD" },
		{ 211, 75, 7, 33, 10, 10, NULL, 0, "__OSUnhandledExceptionD" },
		{  93, 27, 2, 12, 13,  4, NULL, 0, "__OSUnhandledException" },
		{ 128, 43, 2, 22,  6,  5, NULL, 0, "__OSUnhandledException" },
		{ 186, 56, 7, 32, 10,  7, NULL, 0, "__OSUnhandledException" },
		{ 194, 53, 7, 34, 10, 15, NULL, 0, "__OSUnhandledException" }	// SN Systems ProDG
	};
	FuncPattern __OSBootDolSimpleSigs[3] = {
		{ 109,  35, 15, 25,  3, 10, NULL, 0, "__OSBootDolSimpleD" },
		{ 310, 100, 17, 53, 29, 40, NULL, 0, "__OSBootDolSimple" },
		{ 289, 104, 17, 53, 29, 16, NULL, 0, "__OSBootDolSimple" }
	};
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSEnableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSEnableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern __OSSetInterruptHandlerSigs[3] = {
		{ 49, 19, 3, 2, 3, 4, NULL, 0, "__OSSetInterruptHandlerD" },
		{  7,  2, 1, 0, 0, 2, NULL, 0, "__OSSetInterruptHandler" },
		{  6,  1, 0, 0, 0, 3, NULL, 0, "__OSSetInterruptHandler" }	// SN Systems ProDG
	};
	FuncPattern __OSInterruptInitSigs[3] = {
		{ 39, 19, 8, 7, 0, 2, NULL, 0, "__OSInterruptInitD" },
		{ 29, 15, 7, 3, 0, 2, NULL, 0, "__OSInterruptInit" },
		{ 26, 14, 6, 3, 0, 2, NULL, 0, "__OSInterruptInit" }	// SN Systems ProDG
	};
	FuncPattern SetInterruptMaskSigs[7] = {
		{ 175, 31, 7, 0,  6, 3, NULL, 0, "SetInterruptMaskD" },
		{ 179, 31, 7, 0,  6, 3, NULL, 0, "SetInterruptMaskD" },
		{ 189, 29, 7, 0, 17, 1, NULL, 0, "SetInterruptMaskD" },
		{ 168, 28, 5, 0,  6, 6, NULL, 0, "SetInterruptMask" },
		{ 172, 28, 5, 0,  6, 6, NULL, 0, "SetInterruptMask" },
		{ 182, 26, 5, 0, 17, 4, NULL, 0, "SetInterruptMask" },
		{  39,  2, 0, 0,  9, 1, NULL, 0, "SetInterruptMask" }	// SN Systems ProDG
	};
	FuncPattern __OSMaskInterruptsSigs[3] = {
		{ 34, 7, 3, 6, 1, 5, NULL, 0, "__OSMaskInterruptsD" },
		{ 34, 8, 6, 3, 3, 4, NULL, 0, "__OSMaskInterrupts" },
		{ 32, 8, 6, 3, 1, 4, NULL, 0, "__OSMaskInterrupts" }	// SN Systems ProDG
	};
	FuncPattern __OSUnmaskInterruptsSigs[3] = {
		{ 34, 7, 3, 6, 1, 5, NULL, 0, "__OSUnmaskInterruptsD" },
		{ 34, 8, 6, 3, 3, 4, NULL, 0, "__OSUnmaskInterrupts" },
		{ 32, 8, 6, 3, 1, 4, NULL, 0, "__OSUnmaskInterrupts" }	// SN Systems ProDG
	};
	FuncPattern __OSDispatchInterruptSigs[7] = {
		{ 263, 55, 5, 16, 38, 14, NULL, 0, "__OSDispatchInterruptD" },
		{ 267, 55, 5, 16, 39, 14, NULL, 0, "__OSDispatchInterruptD" },
		{ 276, 56, 8, 17, 40, 14, NULL, 0, "__OSDispatchInterruptD" },
		{ 197, 32, 5,  6, 38,  9, NULL, 0, "__OSDispatchInterrupt" },
		{ 201, 32, 5,  6, 39,  9, NULL, 0, "__OSDispatchInterrupt" },
		{ 209, 33, 8,  7, 40,  9, NULL, 0, "__OSDispatchInterrupt" },
		{ 167, 28, 8,  7, 38, 10, NULL, 0, "__OSDispatchInterrupt" }	// SN Systems ProDG
	};
	FuncPattern __OSRebootSigs[14] = {
		{  81, 34, 6, 16,  1,  2, NULL, 0, "__OSRebootD" },
		{  89, 39, 8, 17,  1,  2, NULL, 0, "__OSRebootD" },
		{  88, 39, 7, 17,  1,  2, NULL, 0, "__OSRebootD" },
		{  90, 39, 7, 19,  1,  2, NULL, 0, "__OSRebootD" },
		{ 159, 56, 8, 32,  8, 18, NULL, 0, "__OSRebootD" },
		{  25, 10, 5,  6,  0,  2, NULL, 0, "__OSRebootD" },
		{ 108, 36, 7, 16, 26,  3, NULL, 0, "__OSReboot" },
		{ 115, 40, 9, 17, 26,  3, NULL, 0, "__OSReboot" },
		{ 112, 39, 8, 17, 26,  2, NULL, 0, "__OSReboot" },
		{ 114, 39, 8, 19, 26,  2, NULL, 0, "__OSReboot" },
		{ 199, 56, 7, 42, 13, 36, NULL, 0, "__OSReboot" },	// SN Systems ProDG
		{ 208, 54, 7, 40, 23, 34, NULL, 0, "__OSReboot" },
		{ 204, 54, 6, 40, 23, 35, NULL, 0, "__OSReboot" },
		{  28, 11, 5,  6,  0,  2, NULL, 0, "__OSReboot" }
	};
	FuncPattern __OSDoHotResetSigs[3] = {
		{ 17, 6, 3, 3, 0, 2, NULL, 0, "__OSDoHotResetD" },
		{ 18, 6, 3, 3, 0, 3, NULL, 0, "__OSDoHotReset" },
		{ 17, 5, 3, 3, 0, 3, NULL, 0, "__OSDoHotReset" }	// SN Systems ProDG
	};
	FuncPattern OSResetSystemSigs[18] = {
		{  64, 13, 2, 15,  6, 8, NULL, 0, "OSResetSystemD" },
		{  65, 13, 2, 16,  6, 8, NULL, 0, "OSResetSystemD" },
		{  90, 28, 3, 25,  5, 7, NULL, 0, "OSResetSystemD" },
		{  97, 29, 3, 27,  5, 8, NULL, 0, "OSResetSystemD" },
		{ 102, 32, 3, 29,  5, 8, NULL, 0, "OSResetSystemD" },
		{ 115, 36, 4, 30,  8, 8, NULL, 0, "OSResetSystemD" },
		{  87, 28, 2, 21,  4, 6, NULL, 0, "OSResetSystemD" },
		{ 110, 19, 2, 16, 24, 9, NULL, 0, "OSResetSystem" },
		{ 111, 19, 2, 17, 24, 9, NULL, 0, "OSResetSystem" },
		{ 134, 34, 2, 22, 23, 9, NULL, 0, "OSResetSystem" },
		{ 147, 37, 2, 21, 30, 8, NULL, 0, "OSResetSystem" },
		{ 154, 38, 2, 23, 30, 9, NULL, 0, "OSResetSystem" },
		{ 158, 41, 2, 24, 30, 9, NULL, 0, "OSResetSystem" },
		{ 148, 44, 2, 26, 16, 9, NULL, 0, "OSResetSystem" },	// SN Systems ProDG
		{ 162, 41, 2, 24, 34, 9, NULL, 0, "OSResetSystem" },
		{ 174, 44, 3, 25, 37, 9, NULL, 0, "OSResetSystem" },
		{ 175, 44, 3, 25, 37, 9, NULL, 0, "OSResetSystem" },
		{ 128, 38, 6, 31, 16, 6, NULL, 0, "OSResetSystem" }
	};
	FuncPattern SystemCallVectorSig = 
		{ 7, 0, 0, 0, 0, 1, NULL, 0, "SystemCallVector" };
	FuncPattern __OSInitSystemCallSigs[3] = {
		{ 28, 12, 3, 4, 0, 5, NULL, 0, "__OSInitSystemCallD" },
		{ 25, 10, 3, 3, 0, 4, NULL, 0, "__OSInitSystemCall" },
		{ 24, 14, 2, 3, 0, 4, NULL, 0, "__OSInitSystemCall" }	// SN Systems ProDG
	};
	FuncPattern SelectThreadSigs[5] = {
		{ 123, 39, 10, 11, 14, 12, NULL, 0, "SelectThreadD" },
		{ 122, 38,  9, 12, 14, 12, NULL, 0, "SelectThreadD" },
		{ 128, 41, 20,  8, 12, 12, NULL, 0, "SelectThread" },
		{ 138, 44, 20,  8, 12, 12, NULL, 0, "SelectThread" },
		{ 141, 51, 19,  8, 12, 14, NULL, 0, "SelectThread" }	// SN Systems ProDG
	};
	FuncPattern OSGetTimeSig = 
		{ 6, 0, 0, 0, 0, 4, NULL, 0, "OSGetTime" };
	FuncPattern __OSGetSystemTimeSigs[3] = {
		{ 22, 4, 2, 3, 0, 3, NULL, 0, "__OSGetSystemTimeD" },
		{ 25, 8, 5, 3, 0, 3, NULL, 0, "__OSGetSystemTime" },
		{ 25, 8, 5, 3, 0, 3, NULL, 0, "__OSGetSystemTime" }	// SN Systems ProDG
	};
	FuncPattern SetExiInterruptMaskSigs[7] = {
		{ 63, 18, 3, 7, 17, 3, NULL, 0, "SetExiInterruptMaskD" },
		{ 63, 18, 3, 7, 17, 3, NULL, 0, "SetExiInterruptMaskD" },
		{ 63, 18, 3, 7, 17, 3, NULL, 0, "SetExiInterruptMaskD" },
		{ 61, 19, 3, 7, 17, 2, NULL, 0, "SetExiInterruptMask" },
		{ 61, 19, 3, 7, 17, 2, NULL, 0, "SetExiInterruptMask" },
		{ 63, 18, 3, 7, 17, 2, NULL, 0, "SetExiInterruptMask" },
		{ 61, 19, 3, 7, 17, 2, NULL, 0, "SetExiInterruptMask" }
	};
	FuncPattern CompleteTransferSigs[3] = {
		{ 53, 16, 3, 1, 7, 4, NULL, 0, "CompleteTransferD" },
		{ 53, 16, 3, 1, 7, 4, NULL, 0, "CompleteTransferD" },
		{ 54, 17, 3, 1, 7, 4, NULL, 0, "CompleteTransferD" }
	};
	FuncPattern EXIImmSigs[7] = {
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 151, 27, 8, 5, 12, 17, NULL, 0, "EXIImm" },
		{ 151, 27, 8, 5, 12, 17, NULL, 0, "EXIImm" },
		{  87, 24, 7, 5,  7,  9, NULL, 0, "EXIImm" },
		{ 151, 36, 8, 5, 12, 32, NULL, 0, "EXIImm" }
	};
	FuncPattern EXIDmaSigs[7] = {
		{ 113, 42, 5, 10, 8, 6, NULL, 0, "EXIDmaD" },
		{ 113, 42, 5, 10, 8, 6, NULL, 0, "EXIDmaD" },
		{ 113, 42, 5, 10, 8, 6, NULL, 0, "EXIDmaD" },
		{  59, 17, 7,  5, 2, 4, NULL, 0, "EXIDma" },
		{  59, 17, 7,  5, 2, 4, NULL, 0, "EXIDma" },
		{  74, 28, 8,  5, 2, 8, NULL, 0, "EXIDma" },
		{  59, 17, 7,  5, 2, 5, NULL, 0, "EXIDma" }
	};
	FuncPattern EXISyncSigs[12] = {
		{  80, 25, 2, 6,  7,  7, NULL, 0, "EXISyncD" },
		{  80, 25, 2, 6,  7,  7, NULL, 0, "EXISyncD" },
		{  80, 24, 2, 6,  7,  6, NULL, 0, "EXISyncD" },
		{ 102, 33, 2, 6,  8,  8, NULL, 0, "EXISyncD" },
		{ 107, 34, 2, 7,  9,  8, NULL, 0, "EXISyncD" },
		{ 116, 27, 6, 2, 10, 17, NULL, 0, "EXISync" },
		{ 130, 31, 3, 3, 11, 17, NULL, 0, "EXISync" },
		{ 130, 31, 3, 3, 11, 17, NULL, 0, "EXISync" },
		{  92, 26, 3, 3,  9,  7, NULL, 0, "EXISync" },
		{ 142, 35, 3, 3, 12, 17, NULL, 0, "EXISync" },
		{ 142, 39, 3, 3, 12, 19, NULL, 0, "EXISync" },
		{ 147, 40, 3, 4, 13, 19, NULL, 0, "EXISync" }
	};
	FuncPattern EXIClearInterruptsSigs[4] = {
		{ 48, 13, 5, 1, 5, 3, NULL, 0, "EXIClearInterruptsD" },
		{ 49, 14, 5, 1, 5, 3, NULL, 0, "EXIClearInterruptsD" },
		{ 18,  3, 1, 0, 3, 2, NULL, 0, "EXIClearInterrupts" },
		{ 27,  5, 1, 0, 3, 0, NULL, 0, "EXIClearInterrupts" }
	};
	FuncPattern EXIProbeResetSigs[2] = {
		{ 16,  7, 4, 2, 0, 2, NULL, 0, "EXIProbeResetD" },
		{ 23, 12, 6, 2, 0, 2, NULL, 0, "EXIProbeResetD" }
	};
	FuncPattern __EXIProbeSigs[7] = {
		{ 108, 38, 2, 7, 10, 10, NULL, 0, "EXIProbeD" },
		{ 111, 38, 5, 7, 10, 10, NULL, 0, "__EXIProbeD" },
		{ 112, 39, 5, 7, 10, 10, NULL, 0, "__EXIProbeD" },
		{  90, 30, 4, 5,  8, 10, NULL, 0, "EXIProbe" },
		{  93, 30, 7, 5,  8, 10, NULL, 0, "__EXIProbe" },
		{ 109, 34, 6, 5,  8,  8, NULL, 0, "__EXIProbe" },
		{  93, 30, 7, 5,  8,  9, NULL, 0, "__EXIProbe" }
	};
	FuncPattern __EXIAttachSigs[2] = {
		{ 55, 18, 5, 7, 3, 5, NULL, 0, "__EXIAttachD" },
		{ 56, 19, 5, 7, 3, 5, NULL, 0, "__EXIAttachD" }
	};
	FuncPattern EXIAttachSigs[7] = {
		{ 59, 19, 5,  8, 5, 5, NULL, 0, "EXIAttachD" },
		{ 43, 11, 3,  6, 3, 5, NULL, 0, "EXIAttachD" },
		{ 44, 12, 3,  6, 3, 5, NULL, 0, "EXIAttachD" },
		{ 57, 18, 9,  6, 3, 4, NULL, 0, "EXIAttach" },
		{ 67, 19, 4, 11, 3, 3, NULL, 0, "EXIAttach" },
		{ 74, 19, 5, 11, 3, 7, NULL, 0, "EXIAttach" },
		{ 67, 18, 4, 11, 3, 5, NULL, 0, "EXIAttach" }
	};
	FuncPattern EXIDetachSigs[7] = {
		{ 53, 16, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 53, 16, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 54, 17, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 47, 15, 6, 5, 3, 3, NULL, 0, "EXIDetach" },
		{ 47, 15, 6, 5, 3, 3, NULL, 0, "EXIDetach" },
		{ 43, 12, 3, 5, 3, 4, NULL, 0, "EXIDetach" },
		{ 47, 15, 6, 5, 3, 4, NULL, 0, "EXIDetach" }
	};
	FuncPattern EXISelectSDSig = 
		{ 85, 23, 4, 7, 14, 6, NULL, 0, "EXISelectSD" };
	FuncPattern EXISelectSigs[7] = {
		{ 116, 33, 3, 10, 17, 6, NULL, 0, "EXISelectD" },
		{ 116, 33, 3, 10, 17, 6, NULL, 0, "EXISelectD" },
		{ 116, 33, 3, 10, 17, 6, NULL, 0, "EXISelectD" },
		{  75, 18, 4,  6, 11, 7, NULL, 0, "EXISelect" },
		{  75, 18, 4,  6, 11, 7, NULL, 0, "EXISelect" },
		{  80, 20, 4,  6, 11, 6, NULL, 0, "EXISelect" },
		{  75, 18, 4,  6, 11, 8, NULL, 0, "EXISelect" }
	};
	FuncPattern EXIDeselectSigs[8] = {
		{ 76, 21, 3, 7, 14, 5, NULL, 0, "EXIDeselectD" },
		{ 76, 21, 3, 7, 14, 5, NULL, 0, "EXIDeselectD" },
		{ 77, 22, 3, 7, 14, 5, NULL, 0, "EXIDeselectD" },
		{ 68, 20, 8, 6, 11, 3, NULL, 0, "EXIDeselect" },
		{ 68, 20, 8, 6, 12, 3, NULL, 0, "EXIDeselect" },
		{ 68, 20, 8, 6, 12, 3, NULL, 0, "EXIDeselect" },
		{ 66, 17, 3, 6, 12, 4, NULL, 0, "EXIDeselect" },
		{ 68, 20, 8, 6, 12, 4, NULL, 0, "EXIDeselect" }
	};
	FuncPattern EXIIntrruptHandlerSigs[7] = {
		{ 42, 16, 3, 2, 3, 2, NULL, 0, "EXIIntrruptHandlerD" },
		{ 50, 20, 2, 6, 3, 3, NULL, 0, "EXIIntrruptHandlerD" },
		{ 51, 21, 2, 6, 3, 3, NULL, 0, "EXIIntrruptHandlerD" },
		{ 32, 10, 3, 0, 1, 7, NULL, 0, "EXIIntrruptHandler" },
		{ 50, 19, 6, 4, 1, 8, NULL, 0, "EXIIntrruptHandler" },
		{ 48, 16, 3, 4, 1, 3, NULL, 0, "EXIIntrruptHandler" },
		{ 50, 19, 6, 4, 1, 7, NULL, 0, "EXIIntrruptHandler" }
	};
	FuncPattern TCIntrruptHandlerSigs[7] = {
		{  50, 18, 4, 4, 3,  4, NULL, 0, "TCIntrruptHandlerD" },
		{  58, 22, 3, 8, 3,  3, NULL, 0, "TCIntrruptHandlerD" },
		{  59, 23, 3, 8, 3,  3, NULL, 0, "TCIntrruptHandlerD" },
		{ 125, 34, 9, 1, 8, 21, NULL, 0, "TCIntrruptHandler" },
		{ 134, 37, 9, 5, 8, 21, NULL, 0, "TCIntrruptHandler" },
		{  87, 28, 5, 5, 6,  4, NULL, 0, "TCIntrruptHandler" },
		{ 134, 41, 9, 5, 8, 22, NULL, 0, "TCIntrruptHandler" }
	};
	FuncPattern EXTIntrruptHandlerSigs[8] = {
		{ 52, 18, 5, 2, 3, 4, NULL, 0, "EXTIntrruptHandlerD" },
		{ 60, 22, 4, 6, 3, 5, NULL, 0, "EXTIntrruptHandlerD" },
		{ 61, 23, 4, 6, 3, 5, NULL, 0, "EXTIntrruptHandlerD" },
		{ 55, 20, 4, 6, 3, 4, NULL, 0, "EXTIntrruptHandlerD" },
		{ 43, 17, 6, 1, 1, 7, NULL, 0, "EXTIntrruptHandler" },
		{ 50, 17, 4, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" },
		{ 50, 18, 4, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" },
		{ 52, 20, 8, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" }
	};
	FuncPattern EXIInitSigs[10] = {
		{  58, 36,  6, 11, 1, 2, NULL, 0, "EXIInitD" },
		{  60, 37,  6, 12, 1, 2, NULL, 0, "EXIInitD" },
		{ 107, 61,  6, 16, 8, 2, NULL, 0, "EXIInitD" },
		{  65, 32, 12, 12, 1, 2, NULL, 0, "EXIInit" },
		{  69, 34, 14, 12, 1, 2, NULL, 0, "EXIInit" },
		{  73, 46, 10, 12, 1, 2, NULL, 0, "EXIInit" },
		{  71, 35, 14, 13, 1, 2, NULL, 0, "EXIInit" },
		{  89, 43, 14, 14, 4, 2, NULL, 0, "EXIInit" },
		{  87, 42, 14, 13, 4, 2, NULL, 0, "EXIInit" },
		{ 117, 58, 14, 17, 8, 2, NULL, 0, "EXIInit" }
	};
	FuncPattern EXILockSigs[7] = {
		{ 106, 35, 5, 9, 13, 6, NULL, 0, "EXILockD" },
		{ 106, 35, 5, 9, 13, 6, NULL, 0, "EXILockD" },
		{ 106, 35, 5, 9, 13, 6, NULL, 0, "EXILockD" },
		{  61, 18, 7, 5,  5, 6, NULL, 0, "EXILock" },
		{  61, 18, 7, 5,  5, 6, NULL, 0, "EXILock" },
		{  63, 19, 5, 5,  6, 6, NULL, 0, "EXILock" },
		{  61, 17, 7, 5,  5, 7, NULL, 0, "EXILock" }
	};
	FuncPattern EXIUnlockSigs[7] = {
		{ 60, 22, 4, 6, 5, 4, NULL, 0, "EXIUnlockD" },
		{ 60, 22, 4, 6, 5, 4, NULL, 0, "EXIUnlockD" },
		{ 61, 23, 4, 6, 5, 4, NULL, 0, "EXIUnlockD" },
		{ 55, 21, 8, 5, 3, 2, NULL, 0, "EXIUnlock" },
		{ 55, 21, 8, 5, 3, 2, NULL, 0, "EXIUnlock" },
		{ 50, 18, 4, 5, 3, 3, NULL, 0, "EXIUnlock" },
		{ 55, 21, 8, 5, 3, 3, NULL, 0, "EXIUnlock" }
	};
	FuncPattern EXIGetIDSigs[9] = {
		{  97, 27,  4, 11,  7,  9, NULL, 0, "EXIGetIDD" },
		{ 152, 45,  6, 14, 14, 15, NULL, 0, "EXIGetIDD" },
		{ 153, 46,  6, 14, 14, 15, NULL, 0, "EXIGetIDD" },
		{ 168, 49,  7, 16, 16, 16, NULL, 0, "EXIGetIDD" },
		{ 181, 54,  8, 24, 14, 13, NULL, 0, "EXIGetID" },
		{ 223, 70, 11, 26, 20, 17, NULL, 0, "EXIGetID" },
		{ 228, 66, 11, 26, 19, 19, NULL, 0, "EXIGetID" },
		{ 223, 69, 11, 26, 20, 20, NULL, 0, "EXIGetID" },
		{ 236, 71, 12, 28, 22, 21, NULL, 0, "EXIGetID" }
	};
	FuncPattern __DVDInterruptHandlerSigs[10] = {
		{ 157, 49, 21,  9, 18, 12, NULL, 0, "__DVDInterruptHandlerD" },
		{ 161, 50, 22,  8, 18, 12, NULL, 0, "__DVDInterruptHandlerD" },
		{ 161, 50, 22,  8, 18, 12, NULL, 0, "__DVDInterruptHandlerD" },
		{ 121, 32, 13,  4, 15, 14, NULL, 0, "__DVDInterruptHandler" },
		{ 123, 33, 13,  5, 15, 14, NULL, 0, "__DVDInterruptHandler" },
		{ 184, 59, 26,  9, 21, 17, NULL, 0, "__DVDInterruptHandler" },
		{ 186, 60, 26, 10, 21, 17, NULL, 0, "__DVDInterruptHandler" },
		{ 189, 60, 27,  9, 21, 17, NULL, 0, "__DVDInterruptHandler" },
		{ 184, 56, 23,  9, 21, 16, NULL, 0, "__DVDInterruptHandler" },
		{ 190, 63, 23, 11, 21, 17, NULL, 0, "__DVDInterruptHandler" }	// SN Systems ProDG
	};
	FuncPattern AlarmHandlerForTimeoutSigs[3] = {
		{ 29, 11, 5, 5, 1, 2, NULL, 0, "AlarmHandlerForTimeoutD" },
		{ 28, 10, 4, 5, 1, 2, NULL, 0, "AlarmHandlerForTimeout" },
		{ 28, 10, 4, 5, 1, 3, NULL, 0, "AlarmHandlerForTimeout" }	// SN Systems ProDG
	};
	FuncPattern SetTimeoutAlarmSigs[2] = {
		{ 19, 10, 4, 2, 0, 2, NULL, 0, "SetTimeoutAlarmD" },
		{ 25, 12, 5, 2, 0, 2, NULL, 0, "SetTimeoutAlarm" }
	};
	FuncPattern ReadSigs[6] = {
		{ 54, 22, 17, 3, 2, 4, NULL, 0, "ReadD" },
		{ 56, 23, 18, 3, 2, 4, NULL, 0, "ReadD" },
		{ 55, 27, 12, 4, 2, 3, NULL, 0, "Read" },
		{ 66, 29, 17, 5, 2, 3, NULL, 0, "Read" },
		{ 68, 30, 18, 5, 2, 3, NULL, 0, "Read" },
		{ 67, 29, 17, 5, 2, 6, NULL, 0, "Read" }	// SN Systems ProDG
	};
	FuncPattern DoJustReadSigs[2] = {
		{ 21, 9, 8, 1, 0, 2, NULL, 0, "DoJustReadD" },
		{ 13, 5, 4, 1, 0, 2, NULL, 0, "DoJustRead" }
	};
	FuncPattern DVDLowReadSigs[6] = {
		{ 157,  70,  6, 13, 12, 13, NULL, 0, "DVDLowReadD" },
		{ 157,  70,  6, 13, 12, 13, NULL, 0, "DVDLowReadD" },
		{  13,   4,  7,  0,  0,  0, NULL, 0, "DVDLowRead" },
		{ 166,  68, 19,  9, 14, 18, NULL, 0, "DVDLowRead" },
		{ 166,  68, 19,  9, 14, 18, NULL, 0, "DVDLowRead" },
		{ 321, 113, 75, 23, 17, 34, NULL, 0, "DVDLowRead" }	// SN Systems ProDG
	};
	FuncPattern DVDLowSeekSigs[6] = {
		{ 38, 18, 8, 2, 1, 3, NULL, 0, "DVDLowSeekD" },
		{ 40, 19, 9, 2, 1, 3, NULL, 0, "DVDLowSeekD" },
		{ 10,  4, 4, 0, 0, 0, NULL, 0, "DVDLowSeek" },
		{ 34, 17, 8, 2, 0, 2, NULL, 0, "DVDLowSeek" },
		{ 37, 19, 9, 2, 0, 2, NULL, 0, "DVDLowSeek" },
		{ 34, 16, 8, 2, 0, 2, NULL, 0, "DVDLowSeek" }	// SN Systems ProDG
	};
	FuncPattern DVDLowWaitCoverCloseSigs[5] = {
		{  8, 4, 3, 0, 0, 0, NULL, 0, "DVDLowWaitCoverCloseD" },
		{ 10, 5, 4, 0, 0, 0, NULL, 0, "DVDLowWaitCoverCloseD" },
		{  8, 4, 3, 0, 0, 0, NULL, 0, "DVDLowWaitCoverClose" },
		{ 11, 6, 4, 0, 0, 0, NULL, 0, "DVDLowWaitCoverClose" },
		{ 10, 5, 4, 0, 0, 0, NULL, 0, "DVDLowWaitCoverClose" }	// SN Systems ProDG
	};
	FuncPattern DVDLowReadDiskIDSigs[6] = {
		{ 47, 25, 11, 2, 1, 3, NULL, 0, "DVDLowReadDiskIDD" },
		{ 49, 26, 12, 2, 1, 3, NULL, 0, "DVDLowReadDiskIDD" },
		{ 16,  8,  7, 0, 0, 0, NULL, 0, "DVDLowReadDiskID" },
		{ 40, 20, 11, 2, 0, 2, NULL, 0, "DVDLowReadDiskID" },
		{ 41, 19, 12, 2, 0, 2, NULL, 0, "DVDLowReadDiskID" },
		{ 39, 17, 11, 2, 0, 3, NULL, 0, "DVDLowReadDiskID" }	// SN Systems ProDG
	};
	FuncPattern DVDLowStopMotorSigs[6] = {
		{ 23, 11, 6, 1, 0, 2, NULL, 0, "DVDLowStopMotorD" },
		{ 25, 12, 7, 1, 0, 2, NULL, 0, "DVDLowStopMotorD" },
		{  9,  5, 3, 0, 0, 0, NULL, 0, "DVDLowStopMotor" },
		{ 33, 18, 7, 2, 0, 2, NULL, 0, "DVDLowStopMotor" },
		{ 35, 19, 8, 2, 0, 2, NULL, 0, "DVDLowStopMotor" },
		{ 32, 17, 7, 2, 0, 2, NULL, 0, "DVDLowStopMotor" }	// SN Systems ProDG
	};
	FuncPattern DVDLowRequestErrorSigs[6] = {
		{ 23, 11, 6, 1, 0, 2, NULL, 0, "DVDLowRequestErrorD" },
		{ 25, 12, 7, 1, 0, 2, NULL, 0, "DVDLowRequestErrorD" },
		{  9,  5, 3, 0, 0, 0, NULL, 0, "DVDLowRequestError" },
		{ 33, 18, 7, 2, 0, 2, NULL, 0, "DVDLowRequestError" },
		{ 35, 19, 8, 2, 0, 2, NULL, 0, "DVDLowRequestError" },
		{ 32, 17, 7, 2, 0, 2, NULL, 0, "DVDLowRequestError" }	// SN Systems ProDG
	};
	FuncPattern DVDLowInquirySigs[6] = {
		{ 33, 17, 10, 1, 0, 2, NULL, 0, "DVDLowInquiryD" },
		{ 35, 18, 11, 1, 0, 2, NULL, 0, "DVDLowInquiryD" },
		{ 13,  6,  6, 0, 0, 0, NULL, 0, "DVDLowInquiry" },
		{ 37, 19, 10, 2, 0, 2, NULL, 0, "DVDLowInquiry" },
		{ 39, 20, 11, 2, 0, 2, NULL, 0, "DVDLowInquiry" },
		{ 37, 16, 10, 2, 0, 3, NULL, 0, "DVDLowInquiry" }	// SN Systems ProDG
	};
	FuncPattern DVDLowAudioStreamSigs[6] = {
		{ 34, 15, 11, 1, 0, 2, NULL, 0, "DVDLowAudioStreamD" },
		{ 36, 16, 12, 1, 0, 2, NULL, 0, "DVDLowAudioStreamD" },
		{ 11,  3,  5, 0, 0, 0, NULL, 0, "DVDLowAudioStream" },
		{ 35, 16,  9, 2, 0, 2, NULL, 0, "DVDLowAudioStream" },
		{ 38, 18, 10, 2, 0, 2, NULL, 0, "DVDLowAudioStream" },
		{ 35, 15,  9, 2, 0, 2, NULL, 0, "DVDLowAudioStream" }	// SN Systems ProDG
	};
	FuncPattern DVDLowRequestAudioStatusSigs[6] = {
		{ 25, 11, 7, 1, 0, 2, NULL, 0, "DVDLowRequestAudioStatusD" },
		{ 27, 12, 8, 1, 0, 2, NULL, 0, "DVDLowRequestAudioStatusD" },
		{  8,  3, 3, 0, 0, 0, NULL, 0, "DVDLowRequestAudioStatus" },
		{ 32, 16, 7, 2, 0, 2, NULL, 0, "DVDLowRequestAudioStatus" },
		{ 35, 18, 8, 2, 0, 2, NULL, 0, "DVDLowRequestAudioStatus" },
		{ 32, 16, 7, 2, 0, 2, NULL, 0, "DVDLowRequestAudioStatus" }	// SN Systems ProDG
	};
	FuncPattern DVDLowAudioBufferConfigSigs[6] = {
		{ 52, 20, 7, 3, 4, 3, NULL, 0, "DVDLowAudioBufferConfigD" },
		{ 54, 21, 8, 3, 4, 3, NULL, 0, "DVDLowAudioBufferConfigD" },
		{ 15,  6, 3, 0, 2, 1, NULL, 0, "DVDLowAudioBufferConfig" },
		{ 39, 19, 7, 2, 2, 3, NULL, 0, "DVDLowAudioBufferConfig" },
		{ 39, 19, 8, 2, 1, 3, NULL, 0, "DVDLowAudioBufferConfig" },
		{ 38, 17, 7, 2, 0, 7, NULL, 0, "DVDLowAudioBufferConfig" }	// SN Systems ProDG
	};
	FuncPattern DVDLowResetSigs[3] = {
		{ 49, 14, 8, 3, 0,  9, NULL, 0, "DVDLowResetD" },
		{ 47, 10, 8, 3, 0,  9, NULL, 0, "DVDLowReset" },
		{ 49,  9, 8, 5, 0, 11, NULL, 0, "DVDLowReset" }	// SN Systems ProDG
	};
	FuncPattern DVDLowSetResetCoverCallbackSigs[2] = {
		{ 18, 4, 4, 2, 0, 3, NULL, 0, "DVDLowSetResetCoverCallbackD" },
		{ 17, 5, 5, 2, 0, 3, NULL, 0, "DVDLowSetResetCoverCallback" }
	};
	FuncPattern DoBreakSigs[2] = {
		{ 12, 6, 4, 0, 0, 0, NULL, 0, "DoBreakD" },
		{  7, 3, 2, 0, 0, 0, NULL, 0, "DoBreak" }
	};
	FuncPattern AlarmHandlerForBreakSigs[2] = {
		{ 23, 10, 2, 2, 2, 4, NULL, 0, "AlarmHandlerForBreakD" },
		{ 29, 13, 4, 1, 2, 4, NULL, 0, "AlarmHandlerForBreak" }
	};
	FuncPattern SetBreakAlarmSigs[2] = {
		{ 19, 10, 4, 2, 0, 2, NULL, 0, "SetBreakAlarmD" },
		{ 25, 12, 5, 2, 0, 2, NULL, 0, "SetBreakAlarm" }
	};
	FuncPattern DVDLowBreakSigs[6] = {
		{ 24, 11, 2, 2, 2, 4, NULL, 0, "DVDLowBreakD" },
		{  6,  3, 2, 0, 0, 0, NULL, 0, "DVDLowBreakD" },
		{  8,  4, 2, 0, 0, 0, NULL, 0, "DVDLowBreak" },
		{ 42, 22, 6, 2, 2, 4, NULL, 0, "DVDLowBreak" },
		{  5,  2, 2, 0, 0, 0, NULL, 0, "DVDLowBreak" },
		{  5,  2, 2, 0, 0, 0, NULL, 0, "DVDLowBreak" }	// SN Systems ProDG
	};
	FuncPattern DVDLowClearCallbackSigs[5] = {
		{ 12, 6, 4, 0, 0, 0, NULL, 0, "DVDLowClearCallbackD" },
		{ 14, 7, 5, 0, 0, 0, NULL, 0, "DVDLowClearCallbackD" },
		{  6, 3, 2, 0, 0, 0, NULL, 0, "DVDLowClearCallback" },
		{  6, 3, 2, 0, 0, 0, NULL, 0, "DVDLowClearCallback" },	// SN Systems ProDG
		{  7, 3, 3, 0, 0, 0, NULL, 0, "DVDLowClearCallback" }
	};
	FuncPattern DVDLowGetCoverStatusSigs[3] = {
		{ 35, 13, 2, 1, 4,  9, NULL, 0, "DVDLowGetCoverStatusD" },
		{ 37, 14, 2, 1, 4,  9, NULL, 0, "DVDLowGetCoverStatus" },
		{ 35, 13, 2, 1, 2, 10, NULL, 0, "DVDLowGetCoverStatus" }	// SN Systems ProDG
	};
	FuncPattern __DVDLowTestAlarmSig = 
		{ 14, 7, 0, 0, 0, 2, NULL, 0, "__DVDLowTestAlarm" };
	FuncPattern DVDGetTransferredSizeSig = 
		{ 18, 2, 0, 0, 6, 0, NULL, 0, "DVDGetTransferredSize" };
	FuncPattern DVDInitSigs[8] = {
		{ 66, 28,  9, 13, 2, 2, NULL, 0, "DVDInitD" },
		{ 54, 25,  8, 10, 2, 2, NULL, 0, "DVDInitD" },
		{ 56, 26,  9, 10, 2, 2, NULL, 0, "DVDInitD" },
		{ 61, 24, 10, 10, 2, 2, NULL, 0, "DVDInit" },
		{ 63, 24, 10, 12, 2, 2, NULL, 0, "DVDInit" },
		{ 51, 21,  9,  9, 2, 2, NULL, 0, "DVDInit" },
		{ 49, 21,  8,  9, 2, 2, NULL, 0, "DVDInit" },	// SN Systems ProDG
		{ 54, 21, 11,  9, 2, 2, NULL, 0, "DVDInit" }
	};
	FuncPattern stateGettingErrorSigs[3] = {
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateGettingErrorD" },
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateGettingError" },
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateGettingError" }	// SN Systems ProDG
	};
	FuncPattern cbForStateGettingErrorSigs[8] = {
		{ 127, 39,  9, 13, 16, 4, NULL, 0, "cbForStateGettingErrorD" },
		{ 115, 31,  5, 13, 16, 4, NULL, 0, "cbForStateGettingErrorD" },
		{ 231, 82, 21,  9, 36, 2, NULL, 0, "cbForStateGettingError" },
		{ 249, 94, 29,  9, 36, 2, NULL, 0, "cbForStateGettingError" },
		{ 162, 68, 16, 17, 18, 2, NULL, 0, "cbForStateGettingError" },
		{ 165, 70, 17, 17, 18, 2, NULL, 0, "cbForStateGettingError" },
		{ 202, 73, 22, 14, 27, 3, NULL, 0, "cbForStateGettingError" },	// SN Systems ProDG
		{ 153, 61, 13, 17, 18, 2, NULL, 0, "cbForStateGettingError" }
	};
	FuncPattern cbForUnrecoveredErrorSigs[6] = {
		{ 32, 11, 4, 4, 4, 3, NULL, 0, "cbForUnrecoveredErrorD" },
		{ 29,  9, 3, 4, 4, 3, NULL, 0, "cbForUnrecoveredErrorD" },
		{ 23,  7, 2, 5, 3, 2, NULL, 0, "cbForUnrecoveredError" },
		{ 26,  9, 3, 5, 3, 2, NULL, 0, "cbForUnrecoveredError" },
		{ 21,  6, 3, 3, 3, 2, NULL, 0, "cbForUnrecoveredError" },	// SN Systems ProDG
		{ 23,  7, 2, 5, 3, 2, NULL, 0, "cbForUnrecoveredError" }
	};
	FuncPattern cbForUnrecoveredErrorRetrySigs[6] = {
		{ 30, 12, 5, 3, 3, 3, NULL, 0, "cbForUnrecoveredErrorRetryD" },
		{ 24,  8, 3, 3, 3, 3, NULL, 0, "cbForUnrecoveredErrorRetryD" },
		{ 35, 16, 3, 7, 3, 2, NULL, 0, "cbForUnrecoveredErrorRetry" },
		{ 38, 18, 4, 7, 3, 2, NULL, 0, "cbForUnrecoveredErrorRetry" },
		{ 32, 14, 4, 5, 3, 2, NULL, 0, "cbForUnrecoveredErrorRetry" },	// SN Systems ProDG
		{ 32, 14, 2, 7, 3, 2, NULL, 0, "cbForUnrecoveredErrorRetry" }
	};
	FuncPattern stateMotorStoppedSigs[3] = {
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateMotorStoppedD" },
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateMotorStopped" },
		{ 10, 4, 2, 1, 0, 2, NULL, 0, "stateMotorStopped" }	// SN Systems ProDG
	};
	FuncPattern cbForStateMotorStoppedSigs[6] = {
		{ 24, 11, 5, 2, 1, 2, NULL, 0, "cbForStateMotorStoppedD" },
		{ 50, 19, 6, 5, 9, 2, NULL, 0, "cbForStateMotorStopped" },
		{ 52, 20, 7, 5, 9, 2, NULL, 0, "cbForStateMotorStopped" },
		{ 57, 22, 6, 5, 9, 3, NULL, 0, "cbForStateMotorStopped" },
		{ 56, 23, 5, 5, 9, 3, NULL, 0, "cbForStateMotorStopped" },	// SN Systems ProDG
		{ 59, 23, 7, 5, 9, 3, NULL, 0, "cbForStateMotorStopped" }
	};
	FuncPattern stateBusySigs[11] = {
		{ 182, 112, 22, 15, 17, 10, NULL, 0, "stateBusyD" },
		{ 182, 112, 22, 15, 17, 10, NULL, 0, "stateBusyD" },
		{ 208, 124, 23, 16, 20, 10, NULL, 0, "stateBusyD" },
		{ 216, 129, 24, 17, 21, 10, NULL, 0, "stateBusyD" },
		{ 176, 107, 21, 15, 17, 10, NULL, 0, "stateBusy" },
		{ 176, 107, 21, 15, 17, 10, NULL, 0, "stateBusy" },
		{ 176, 107, 21, 15, 17, 10, NULL, 0, "stateBusy" },
		{ 183, 111, 21, 15, 18, 10, NULL, 0, "stateBusy" },
		{ 200, 118, 23, 16, 20, 10, NULL, 0, "stateBusy" },
		{ 187, 105, 23, 16, 18, 11, NULL, 0, "stateBusy" },	// SN Systems ProDG
		{ 208, 123, 24, 17, 21, 10, NULL, 0, "stateBusy" }
	};
	FuncPattern cbForStateBusySigs[12] = {
		{ 329, 142, 27, 21, 45,  6, NULL, 0, "cbForStateBusyD" },
		{ 305, 133, 27, 24, 39,  6, NULL, 0, "cbForStateBusyD" },
		{ 296, 127, 24, 24, 39,  6, NULL, 0, "cbForStateBusyD" },
		{ 316, 136, 27, 25, 41,  6, NULL, 0, "cbForStateBusyD" },
		{ 261, 108, 22, 12, 43,  5, NULL, 0, "cbForStateBusy" },
		{ 279, 116, 32, 12, 43,  5, NULL, 0, "cbForStateBusy" },
		{ 370, 156, 40, 21, 52,  6, NULL, 0, "cbForStateBusy" },
		{ 373, 158, 41, 21, 52,  6, NULL, 0, "cbForStateBusy" },
		{ 398, 167, 41, 21, 58, 11, NULL, 0, "cbForStateBusy" },
		{ 395, 160, 40, 19, 56, 12, NULL, 0, "cbForStateBusy" },	// SN Systems ProDG
		{ 389, 161, 38, 21, 58, 11, NULL, 0, "cbForStateBusy" },
		{ 406, 167, 41, 22, 60, 11, NULL, 0, "cbForStateBusy" }
	};
	FuncPattern DVDResetSigs[3] = {
		{ 19, 9, 6, 1, 0, 2, NULL, 0, "DVDResetD" },
		{ 17, 7, 6, 1, 0, 2, NULL, 0, "DVDReset" },
		{ 16, 6, 6, 1, 0, 2, NULL, 0, "DVDReset" }	// SN Systems ProDG
	};
	FuncPattern DVDCancelAsyncSigs[9] = {
		{ 166, 53, 13, 10, 26, 8, NULL, 0, "DVDCancelAsyncD" },
		{ 166, 53, 13, 10, 26, 8, NULL, 0, "DVDCancelAsyncD" },
		{ 171, 56, 15, 10, 26, 8, NULL, 0, "DVDCancelAsyncD" },
		{ 171, 56, 20,  8, 20, 8, NULL, 0, "DVDCancelAsync" },
		{ 156, 48, 17,  8, 25, 6, NULL, 0, "DVDCancelAsync" },
		{ 156, 47, 16,  9, 25, 5, NULL, 0, "DVDCancelAsync" },
		{ 156, 47, 16,  9, 25, 5, NULL, 0, "DVDCancelAsync" },
		{ 159, 49, 17,  9, 25, 5, NULL, 0, "DVDCancelAsync" },
		{ 158, 43, 17,  9, 25, 8, NULL, 0, "DVDCancelAsync" }	// SN Systems ProDG
	};
	FuncPattern DVDGetCurrentDiskIDSigs[2] = {
		{ 9, 3, 2, 1, 0, 2, NULL, 0, "DVDGetCurrentDiskIDD" },
		{ 2, 1, 0, 0, 0, 0, NULL, 0, "DVDGetCurrentDiskID" }
	};
	FuncPattern DVDCheckDiskSigs[5] = {
		{ 61, 16, 2, 2, 10, 7, NULL, 0, "DVDCheckDiskD" },
		{ 66, 17, 2, 2, 12, 7, NULL, 0, "DVDCheckDiskD" },
		{ 57, 19, 3, 2, 10, 5, NULL, 0, "DVDCheckDisk" },
		{ 61, 19, 3, 2, 12, 5, NULL, 0, "DVDCheckDisk" },	// SN Systems ProDG
		{ 62, 20, 3, 2, 12, 5, NULL, 0, "DVDCheckDisk" }
	};
	FuncPattern __DVDTestAlarmSigs[2] = {
		{ 18, 6, 3, 1, 1, 4, NULL, 0, "__DVDTestAlarmD" },
		{ 14, 5, 2, 1, 1, 3, NULL, 0, "__DVDTestAlarm" }
	};
	FuncPattern __DVDClearWaitingQueueSigs[3] = {
		{ 16, 3, 3, 0, 2, 0, NULL, 0, "__DVDClearWaitingQueueD" },
		{ 14, 5, 8, 0, 0, 0, NULL, 0, "__DVDClearWaitingQueue" },
		{ 14, 5, 8, 0, 0, 0, NULL, 0, "__DVDClearWaitingQueue" }	// SN Systems ProDG
	};
	FuncPattern __DVDDequeueWaitingQueueSigs[3] = {
		{ 29, 8, 4, 3, 2, 6, NULL, 0, "__DVDDequeueWaitingQueueD" },
		{ 24, 7, 5, 3, 2, 3, NULL, 0, "__DVDDequeueWaitingQueue" },
		{ 26, 7, 5, 3, 2, 5, NULL, 0, "__DVDDequeueWaitingQueue" }	// SN Systems ProDG
	};
	FuncPattern AIInitDMASigs[2] = {
		{ 44, 12, 2, 3, 1, 6, NULL, 0, "AIInitDMAD" },
		{ 34,  8, 4, 2, 0, 5, NULL, 0, "AIInitDMA" }
	};
	FuncPattern AIGetDMAStartAddrSigs[2] = {
		{ 7, 2, 0, 0, 0, 0, NULL, 0, "AIGetDMAStartAddrD" },
		{ 7, 2, 0, 0, 0, 0, NULL, 0, "AIGetDMAStartAddr" }
	};
	FuncPattern GXPeekZSigs[3] = {
		{ 10, 1, 1, 0, 0, 1, NULL, 0, "GXPeekZ" },
		{ 10, 1, 1, 0, 0, 1, NULL, 0, "GXPeekZ" },	// SN Systems ProDG
		{  9, 3, 1, 0, 0, 0, NULL, 0, "GXPeekZ" }
	};
	FuncPattern __VMBASESetupExceptionHandlersSigs[2] = {
		{ 95, 38, 12, 6, 0, 20, NULL, 0, "__VMBASESetupExceptionHandlers" },
		{ 95, 42, 10, 6, 0, 16, NULL, 0, "__VMBASESetupExceptionHandlers" }
	};
	FuncPattern __VMBASEDSIExceptionHandlerSig = 
		{ 54, 6, 9, 0, 3, 27, NULL, 0, "__VMBASEDSIExceptionHandler" };
	FuncPattern __VMBASEISIExceptionHandlerSig = 
		{ 54, 6, 9, 0, 3, 27, NULL, 0, "__VMBASEISIExceptionHandler" };
	_SDA2_BASE_ = _SDA_BASE_ = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!_SDA2_BASE_ && !_SDA_BASE_) {
			if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
				(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
				(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
				(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
				get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
				get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
				i += 4;
			}
			continue;
		}
		if ((data[i - 1] != 0x4E800020 &&
			(data[i + 0] == 0x60000000 || data[i - 1] != 0x4C000064) &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
			continue;
		
		if (data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6) {
			if (!memcmp(data + i, _ppchalt, sizeof(_ppchalt)))
				PPCHaltSig.offsetFoundAt = i;
			else if (!memcmp(data + i, _dvdgettransferredsize, sizeof(_dvdgettransferredsize)))
				DVDGetTransferredSizeSig.offsetFoundAt = i;
			else if (!memcmp(data + i, _aigetdmastartaddrd, sizeof(_aigetdmastartaddrd)))
				AIGetDMAStartAddrSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _aigetdmastartaddr, sizeof(_aigetdmastartaddr)))
				AIGetDMAStartAddrSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_a, sizeof(_gxpeekz_a)))
				GXPeekZSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_b, sizeof(_gxpeekz_b)))
				GXPeekZSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_c, sizeof(_gxpeekz_c)))
				GXPeekZSigs[2].offsetFoundAt = i;
			continue;
		}
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
		for (j = 0; j < sizeof(PrepareExecSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &PrepareExecSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 10, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 40, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i + 42, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 44, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 45, length, &OSEnableInterruptsSig))
							PrepareExecSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 44, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 46, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 48, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 49, length, &OSEnableInterruptsSig))
							PrepareExecSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  39, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  65, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  66, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  67, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 178, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 180, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 194, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  39, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  65, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  66, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  67, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 179, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 181, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 195, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  39, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  65, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  66, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  67, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 180, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 182, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 196, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  39, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  65, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  66, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  67, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 184, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 186, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 200, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  11, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  68, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  94, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  95, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  96, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  99, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 211, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 213, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 228, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  11, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  68, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  94, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  95, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i +  96, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i +  99, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 214, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 216, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 231, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  88, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  98, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  99, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i + 100, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 107, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 218, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 220, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 235, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  88, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  98, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  99, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i + 100, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 107, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 218, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 220, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 235, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  88, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  98, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  99, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i + 100, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 107, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 186, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 188, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 205, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i + 100, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i + 110, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i + 111, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i + 112, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i + 115, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 119, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 211, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 213, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 230, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  86, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i + 102, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i + 112, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i + 113, length, &OSExceptionInitSigs[0]) &&
							findx_pattern(data, dataType, i + 114, length, &__OSInitSystemCallSigs[0]) &&
							findx_pattern(data, dataType, i + 117, length, &__OSInterruptInitSigs[0]) &&
							findx_pattern(data, dataType, i + 121, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 213, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 215, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 232, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 11:
						if (findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  27, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  42, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  51, length, &OSExceptionInitSigs[1]) &&
							findx_pattern(data, dataType, i +  52, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  53, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  57, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 131, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 133, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 146, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 12:
						if (findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  58, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  61, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 139, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 141, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 154, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 13:
						if (findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  58, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  61, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 160, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 162, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 175, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 14:
						if (findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  58, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  61, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 161, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 163, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 176, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 15:
						if (findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  58, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  61, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 164, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 166, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 179, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 16:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  37, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  52, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  60, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  61, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  62, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 168, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 170, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 183, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 17:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  37, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  52, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  60, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  61, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  62, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  65, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  69, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 169, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 171, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 185, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 18:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  54, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  69, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  78, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  79, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  82, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  86, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 185, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 187, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 201, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 19:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  54, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  69, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  78, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  79, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  82, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  86, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 186, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 188, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 202, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 20:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  54, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  69, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  78, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  79, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  82, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  86, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 186, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 188, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 201, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 21:
					case 22:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  54, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  69, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  78, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  79, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  82, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  86, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 189, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 191, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 205, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 23:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  74, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  82, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  83, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  84, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  87, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  91, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 194, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 196, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 210, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 24:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  74, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  82, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  83, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  84, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  87, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  91, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 194, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 196, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 210, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 25:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  74, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  82, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  83, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  84, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  87, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i +  91, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 176, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 178, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 194, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 26:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  52, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  67, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  74, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  75, length, &OSExceptionInitSigs[3]) &&
							findx_pattern(data, dataType, i +  76, length, &__OSInitSystemCallSigs[2]) &&
							findx_pattern(data, dataType, i +  79, length, &__OSInterruptInitSigs[2]) &&
							findx_pattern(data, dataType, i +  83, length, &__OSSetInterruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 165, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 167, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 247, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 27:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  71, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  86, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  94, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  95, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  96, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i +  99, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i + 103, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 200, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 202, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 218, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 28:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  73, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  88, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  96, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  97, length, &OSExceptionInitSigs[2]) &&
							findx_pattern(data, dataType, i +  98, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern(data, dataType, i + 101, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern(data, dataType, i + 105, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 202, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 204, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 284, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSExceptionInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSExceptionInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 150, length, &__OSSetExceptionHandlerSigs[0]))
							OSExceptionInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
					case 2:
						if (findx_pattern(data, dataType, i + 146, length, &__OSSetExceptionHandlerSigs[1]))
							OSExceptionInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 136, length, &__OSSetExceptionHandlerSigs[2]))
							OSExceptionInitSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSSetAlarmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSSetAlarmSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern (data, dataType, i + 31, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 37, length, &OSGetTimeSig,
							                                               &__OSGetSystemTimeSigs[0], NULL) &&
							findx_pattern (data, dataType, i + 42, length, &InsertAlarmSigs[0]) &&
							findx_pattern (data, dataType, i + 52, length, &OSRestoreInterruptsSig))
							OSSetAlarmSigs[j].offsetFoundAt = i;
					case 1:
						if (findx_pattern (data, dataType, i +  8, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 13, length, &OSGetTimeSig,
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 18, length, &InsertAlarmSigs[1]) &&
							findx_pattern (data, dataType, i + 20, length, &OSRestoreInterruptsSig))
							OSSetAlarmSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 19, length, &InsertAlarmSigs[2]) &&
							findx_pattern(data, dataType, i + 21, length, &OSRestoreInterruptsSig))
							OSSetAlarmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSCancelAlarmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSCancelAlarmSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern (data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 11, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 32, length, &SetTimerSig) &&
							findx_pattern (data, dataType, i + 46, length, &OSRestoreInterruptsSig))
							OSCancelAlarmSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 13, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 32, length, &OSGetTimeSig,
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 63, length, &OSRestoreInterruptsSig))
							OSCancelAlarmSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 12, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 31, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 62, length, &OSRestoreInterruptsSig))
							OSCancelAlarmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSUnhandledExceptionSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSUnhandledExceptionSigs[j])) {
				switch (j) {
					case 0:
						if (findx_patterns(data, dataType, i +  38, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL) &&
							findx_patterns(data, dataType, i +  43, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  41, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  46, length, &OSLoadContextSigs[1]))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  88, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 106, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 111, length, &OSLoadContextSigs[2]))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_patterns(data, dataType, i +  33, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL) &&
							findx_patterns(data, dataType, i +  38, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  38, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  43, length, &OSLoadContextSigs[1]))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  78, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i +  97, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 102, length, &OSLoadContextSigs[2]))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  84, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 103, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 108, length, &OSLoadContextSigs[2]))
							__OSUnhandledExceptionSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSBootDolSimpleSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSBootDolSimpleSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  43, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  45, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  46, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  52, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i + 100, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 101, length, &ICFlashInvalidateSig))
							__OSBootDolSimpleSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  44, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  45, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  70, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 118, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 245, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 291, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 301, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 302, length, &ICFlashInvalidateSig))
							__OSBootDolSimpleSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  44, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  45, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  53, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  83, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 152, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 210, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 241, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 270, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 280, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 281, length, &ICFlashInvalidateSig))
							__OSBootDolSimpleSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSUnmaskInterruptsSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSUnmaskInterruptsSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern (data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 22, length, &SetInterruptMaskSigs[0],
							                                               &SetInterruptMaskSigs[1],
							                                               &SetInterruptMaskSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &__OSMaskInterruptsSigs[0]))
							__OSUnmaskInterruptsSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 21, length, &SetInterruptMaskSigs[3],
							                                               &SetInterruptMaskSigs[4],
							                                               &SetInterruptMaskSigs[5], NULL) &&
							findx_pattern (data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &__OSMaskInterruptsSigs[1]))
							__OSUnmaskInterruptsSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 19, length, &SetInterruptMaskSigs[6]) &&
							findx_pattern(data, dataType, i + 23, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &__OSMaskInterruptsSigs[2]))
							__OSUnmaskInterruptsSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSDispatchInterruptSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSDispatchInterruptSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  27, length, &OSLoadContextSigs[0]) &&
							findx_pattern(data, dataType, i + 232, length, &OSLoadContextSigs[0]) &&
							findx_pattern(data, dataType, i + 257, length, &OSLoadContextSigs[0]))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  27, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i + 236, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i + 261, length, &OSLoadContextSigs[1]))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_patterns(data, dataType, i +  27, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 231, length, &OSGetTimeSig) &&
							findx_patterns(data, dataType, i + 245, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 270, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  18, length, &OSLoadContextSigs[0]) &&
							findx_pattern(data, dataType, i + 187, length, &OSLoadContextSigs[0]) &&
							findx_pattern(data, dataType, i + 189, length, &OSLoadContextSigs[0]))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_patterns(data, dataType, i +  18, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 191, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 193, length, &OSLoadContextSigs[0],
							                                                &OSLoadContextSigs[1], NULL))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_patterns(data, dataType, i +  18, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 185, length, &OSGetTimeSig) &&
							findx_patterns(data, dataType, i + 199, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 201, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  15, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 143, length, &OSGetTimeSig) &&
							findx_pattern(data, dataType, i + 157, length, &OSLoadContextSigs[2]) &&
							findx_pattern(data, dataType, i + 159, length, &OSLoadContextSigs[2]))
							__OSDispatchInterruptSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSRebootSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSRebootSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  27, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  29, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  39, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  41, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  43, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  44, length, &OSEnableInterruptsSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  16, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  33, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  35, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  47, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  49, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  51, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  52, length, &OSEnableInterruptsSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  32, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  46, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  48, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  50, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  51, length, &OSEnableInterruptsSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  32, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  46, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  48, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  50, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  51, length, &OSEnableInterruptsSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  32, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  34, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  45, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  47, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  48, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  73, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i + 114, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i + 150, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 151, length, &ICFlashInvalidateSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  11, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  13, length, &OSSetCurrentContextSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  18, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  20, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  29, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  31, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  33, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  34, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  61, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  94, length, &__OSDoHotResetSigs[1]))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  23, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  25, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  36, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  40, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  41, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  68, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 101, length, &__OSDoHotResetSigs[1]))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  21, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  23, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  34, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  39, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  66, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  99, length, &__OSDoHotResetSigs[1]))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  21, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  23, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  34, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  39, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  66, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i +  99, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 104, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 105, length, &ICFlashInvalidateSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  19, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i +  21, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  32, length, &__OSMaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  34, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  35, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  59, length, &__OSDoHotResetSigs[2]) &&
							findx_pattern(data, dataType, i + 102, length, &__OSDoHotResetSigs[2]) &&
							findx_pattern(data, dataType, i + 140, length, &__OSDoHotResetSigs[2]) &&
							findx_pattern(data, dataType, i + 181, length, &__OSDoHotResetSigs[2]) &&
							findx_pattern(data, dataType, i + 189, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 190, length, &ICFlashInvalidateSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 11:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  20, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  22, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  32, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  34, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  35, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  59, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 108, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 148, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 191, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 199, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 200, length, &ICFlashInvalidateSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 12:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  18, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  20, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  31, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  33, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  34, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i +  58, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 106, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 145, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 187, length, &__OSDoHotResetSigs[1]) &&
							findx_pattern(data, dataType, i + 195, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 196, length, &ICFlashInvalidateSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
					case 13:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  13, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  15, length, &OSSetCurrentContextSig))
							__OSRebootSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSDoHotResetSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSDoHotResetSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  9, length, &ICFlashInvalidateSig))
							__OSDoHotResetSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 10, length, &ICFlashInvalidateSig))
							__OSDoHotResetSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  9, length, &ICFlashInvalidateSig))
							__OSDoHotResetSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSResetSystemSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSResetSystemSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  27, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  49, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  57, length, &OSRestoreInterruptsSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  27, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  50, length, &__OSDoHotResetSigs[0]) &&
							findx_pattern(data, dataType, i +  58, length, &OSRestoreInterruptsSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  27, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  50, length, &__OSDoHotResetSigs[0]))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  32, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  55, length, &__OSDoHotResetSigs[0]))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  32, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  55, length, &__OSDoHotResetSigs[0]))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  37, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  60, length, &__OSDoHotResetSigs[0]))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  38, length, &__OSDoHotResetSigs[0]))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &ICFlashInvalidateSig) &&
							findx_pattern(data, dataType, i + 103, length, &OSRestoreInterruptsSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  73, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  78, length, &ICFlashInvalidateSig) &&
							findx_pattern(data, dataType, i + 104, length, &OSRestoreInterruptsSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  73, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  78, length, &ICFlashInvalidateSig) &&
							findx_pattern(data, dataType, i + 127, length, &OSRestoreInterruptsSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if (findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 11:
						if (findx_pattern(data, dataType, i +  57, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  82, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 12:
						if (findx_pattern(data, dataType, i +  57, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  82, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 13:
						if (findx_pattern(data, dataType, i +  48, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  66, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  70, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 14:
						if (findx_pattern(data, dataType, i +  59, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  81, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  86, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 15:
						if (findx_pattern(data, dataType, i +  64, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  86, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  91, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 16:
						if (findx_pattern(data, dataType, i +  65, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  87, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  92, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 17:
						if (findx_pattern(data, dataType, i +  67, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  74, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  79, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSInitSystemCallSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSInitSystemCallSigs[j])) {
				switch (j) {
					case 0:
						if (findi_pattern(data, dataType, i +  8, i +  9, length, &SystemCallVectorSig) &&
							findi_pattern(data, dataType, i + 10, i + 11, length, &SystemCallVectorSig))
							__OSInitSystemCallSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findi_pattern(data, dataType, i +  5, i +  9, length, &SystemCallVectorSig))
							__OSInitSystemCallSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findi_pattern(data, dataType, i +  2, i +  5, length, &SystemCallVectorSig))
							__OSInitSystemCallSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(SelectThreadSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &SelectThreadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 57, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 58, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 62, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 67, length, &OSClearContextSigs[0]))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 56, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 57, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 61, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 66, length, &OSClearContextSigs[0]))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 76, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 77, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 81, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 86, length, &OSClearContextSigs[1]))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 81, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 82, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 86, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 91, length, &OSClearContextSigs[1]))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 82, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 83, length, &OSEnableInterruptsSig) &&
							findx_pattern(data, dataType, i + 87, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 93, length, &OSClearContextSigs[2]))
							SelectThreadSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIImmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIImmSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 11] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  48, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  69, length, &EXIClearInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSUnmaskInterruptsSigs[0]) &&
							get_immediate(data,  i +  93, i +  94, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 111, i + 112, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 115, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 11] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  48, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  69, length, &EXIClearInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  73, length, &__OSUnmaskInterruptsSigs[0]) &&
							get_immediate(data,  i +  93, i +  94, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 111, i + 112, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 115, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 11] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  48, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  57, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  69, length, &EXIClearInterruptsSigs[1]) &&
							findx_patterns(data, dataType, i +  73, length, &__OSUnmaskInterruptsSigs[0],
							                                                &__OSUnmaskInterruptsSigs[1], NULL) &&
							get_immediate (data,  i +  93, i +  94, &address) && address == 0xCC006800 &&
							get_immediate (data,  i + 111, i + 112, &address) && address == 0xCC006800 &&
							findx_pattern (data, dataType, i + 115, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 6] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i + 119, i + 120, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 133, i + 134, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 141, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 6] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i + 119, i + 120, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 133, i + 134, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 141, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &EXIClearInterruptsSigs[3]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i +  58, i +  59, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  76, i +  77, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  80, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_patterns(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1],
							                                                &__OSUnmaskInterruptsSigs[2], NULL) &&
							get_immediate (data,  i + 118, i + 119, &address) && address == 0xCC006800 &&
							get_immediate (data,  i + 135, i + 136, &address) && address == 0xCC006800 &&
							findx_pattern (data, dataType, i + 141, length, &OSRestoreInterruptsSig))
							EXIImmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIDmaSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIDmaSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 11] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  55, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  64, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  76, length, &EXIClearInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  80, length, &__OSUnmaskInterruptsSigs[0]) &&
							get_immediate(data,  i +  88, i +  89, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  94, i +  95, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 102, i + 103, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 106, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 11] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  55, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  64, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  76, length, &EXIClearInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i +  80, length, &__OSUnmaskInterruptsSigs[0]) &&
							get_immediate(data,  i +  88, i +  89, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  94, i +  95, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 102, i + 103, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 106, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 11] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  55, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  64, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  76, length, &EXIClearInterruptsSigs[1]) &&
							findx_patterns(data, dataType, i +  80, length, &__OSUnmaskInterruptsSigs[0],
							                                                &__OSUnmaskInterruptsSigs[1], NULL) &&
							get_immediate (data,  i +  88, i +  89, &address) && address == 0xCC006800 &&
							get_immediate (data,  i +  94, i +  95, &address) && address == 0xCC006800 &&
							get_immediate (data,  i + 102, i + 103, &address) && address == 0xCC006800 &&
							findx_pattern (data, dataType, i + 106, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 6] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i +  39, i +  42, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  52, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 6] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i +  39, i +  42, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  52, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &EXIClearInterruptsSigs[3]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[1]) &&
							get_immediate(data,  i +  47, i +  48, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  54, i +  55, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  63, i +  64, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  67, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  33, length, &EXIClearInterruptsSigs[2]) &&
							findx_patterns(data, dataType, i +  37, length, &__OSUnmaskInterruptsSigs[1],
							                                                &__OSUnmaskInterruptsSigs[2], NULL) &&
							get_immediate (data,  i +  42, i +  43, &address) && address == 0xCC006800 &&
							findx_pattern (data, dataType, i +  52, length, &OSRestoreInterruptsSig))
							EXIDmaSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXISyncSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXISyncSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 5] & 0xFC00FFFF) == 0x1C000038 &&
							get_immediate(data,  i +  24, i +  25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  29, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  35, length, &CompleteTransferSigs[0]) &&
							get_immediate(data,  i +  44, i +  45, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  60, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  24, i +  25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  29, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  35, length, &CompleteTransferSigs[1]) &&
							get_immediate(data,  i +  44, i +  45, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  60, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  25, i +  26, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  30, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  36, length, &CompleteTransferSigs[2]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  53, i +  54, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  61, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  25, i +  26, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  30, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  36, length, &CompleteTransferSigs[2]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  53, i +  54, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  62, i +  63, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  71, i +  72, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  77, i +  78, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i +  83, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  25, i +  26, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  30, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  36, length, &CompleteTransferSigs[2]) &&
							get_immediate(data,  i +  50, i +  51, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  58, i +  59, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  67, i +  68, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  76, i +  77, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  82, i +  83, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 1] & 0xFC00FFFF) == 0x1C000038 &&
							get_immediate(data,  i +   5, i +  12, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  32, i +  34, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 103, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 1] & 0xFC00FFFF) == 0x1C000038 &&
							get_immediate(data,  i +   5, i +   9, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  31, i +  33, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 110, i + 111, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 119, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +   3, i +   8, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  31, i +  33, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 110, i + 111, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 119, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  14, i +  15, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  41, i +  42, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  65, i +  66, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  73, i +  74, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  81, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +   3, i +   8, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  31, i +  33, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 110, i + 111, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 125, i + 126, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i + 131, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if ((data[i + 4] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  10, i +  11, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  32, i +  33, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 110, i + 111, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 125, i + 126, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i + 131, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
					case 11:
						if ((data[i + 4] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  10, i +  11, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  32, i +  33, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 115, i + 116, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 130, i + 131, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i + 136, length, &OSRestoreInterruptsSig))
							EXISyncSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__EXIProbeSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__EXIProbeSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 5] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  24, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  28, i +  29, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  40, length, &EXIClearInterruptsSigs[0]) &&
							get_immediate(data,  i +  43, i +  44, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  50, i +  51, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  56, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  65, i +  66, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  71, i +  72, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  75, i +  76, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  85, i +  86, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  96, i +  97, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 101, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  24, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  28, i +  29, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  40, length, &EXIClearInterruptsSigs[0]) &&
							get_immediate(data,  i +  44, i +  45, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  51, i +  52, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  57, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  66, i +  67, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  72, i +  73, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  76, i +  77, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  87, i +  88, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  99, i + 100, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 104, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  25, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  29, i +  30, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  41, length, &EXIClearInterruptsSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  58, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  67, i +  68, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  73, i +  74, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  77, i +  78, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  88, i +  89, &address) && address == 0x800030C0 &&
							get_immediate(data,  i + 100, i + 101, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 105, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 6] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  17, i +  18, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  27, i +  30, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  37, i +  38, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  44, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  37, i +  52, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  65, i +  67, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  76, i +  78, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  83, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  17, i +  18, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  27, i +  31, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  38, i +  39, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  45, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  38, i +  53, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  67, i +  70, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  79, i +  82, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  86, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  18, i +  19, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  28, i +  29, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  36, i +  37, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  42, i +  43, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  49, i +  50, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  55, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  64, i +  65, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  70, i +  71, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  74, i +  75, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  85, i +  86, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  97, i +  98, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 102, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  17, i +  18, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  33, i +  34, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  38, i +  39, &address) && address == 0x800000F8 &&
							findx_pattern(data, dataType, i +  45, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  38, i +  54, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  69, i +  70, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  81, i +  82, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  86, length, &OSRestoreInterruptsSig))
							__EXIProbeSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIDetachSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIDetachSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 5] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 35, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 44, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[0]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 35, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 44, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[1]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i + 20, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 26, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 36, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 45, length, &__OSMaskInterruptsSigs[0],
							                                               &__OSMaskInterruptsSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 47, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[2]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 7] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[3]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[4]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 15, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 34, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 36, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[5]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXIAttachSigs[6]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		if (compare_pattern(&fp, &EXISelectSDSig)) {
			if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
				findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
				findx_pattern(data, dataType, i + 24, length, &__EXIProbeSigs[6]) &&
				findx_pattern(data, dataType, i + 33, length, &EXIGetIDSigs[8]) &&
				findx_pattern(data, dataType, i + 48, length, &OSRestoreInterruptsSig) &&
				get_immediate(data,   i + 55, i + 56, &address) && address == 0xCC006800 &&
				findx_pattern(data, dataType, i + 73, length, &__OSMaskInterruptsSigs[1]) &&
				findx_pattern(data, dataType, i + 76, length, &__OSMaskInterruptsSigs[1]) &&
				findx_pattern(data, dataType, i + 78, length, &OSRestoreInterruptsSig) &&
				find_pattern_after(data, length, &fp, &EXISelectSigs[6]))
				EXISelectSDSig.offsetFoundAt = i;
		}
		
		for (j = 0; j < sizeof(EXIDeselectSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIDeselectSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 5] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 33, i + 34, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 39, i + 40, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 52, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 55, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 57, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 63, length, &__EXIProbeSigs[0]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[0]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 33, i + 34, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 39, i + 40, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 52, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 55, length, &__OSUnmaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 57, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 63, length, &__EXIProbeSigs[1]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[1]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i + 20, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 26, length, &OSRestoreInterruptsSig) &&
							get_immediate (data,   i + 34, i + 35, &address) && address == 0xCC006800 &&
							get_immediate (data,   i + 40, i + 41, &address) && address == 0xCC006800 &&
							findx_patterns(data, dataType, i + 53, length, &__OSUnmaskInterruptsSigs[0],
							                                               &__OSUnmaskInterruptsSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 56, length, &__OSUnmaskInterruptsSigs[0],
							                                               &__OSUnmaskInterruptsSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 58, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 64, length, &__EXIProbeSigs[2]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[2]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 7] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 22, i + 25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 41, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 44, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 52, length, &__EXIProbeSigs[3]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[3]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 7] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 22, i + 25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 41, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 44, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 52, length, &__EXIProbeSigs[3]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[3]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 22, i + 25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 41, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 44, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 52, length, &__EXIProbeSigs[4]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[4]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 15, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 23, i + 24, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 29, i + 30, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 42, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 45, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 47, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 53, length, &__EXIProbeSigs[5]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[5]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							get_immediate (data,   i + 25, i + 26, &address) && address == 0xCC006800 &&
							findx_patterns(data, dataType, i + 41, length, &__OSUnmaskInterruptsSigs[1],
							                                               &__OSUnmaskInterruptsSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 44, length, &__OSUnmaskInterruptsSigs[1],
							                                               &__OSUnmaskInterruptsSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 52, length, &__EXIProbeSigs[6]) &&
							find_pattern_before(data, length, &fp, &EXISelectSigs[6]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_patterns(data, dataType, i +  5, length, &__OSMaskInterruptsSigs[0],
							                                               &__OSMaskInterruptsSigs[1], NULL) &&
							get_immediate (data,   i +  7, i +  8, &address) && address == 0xCC006800 &&
							get_immediate (data,   i + 10, i + 11, &address) && address == 0xCC006814 &&
							get_immediate (data,   i + 13, i + 14, &address) && address == 0xCC006828 &&
							get_immediate (data,   i + 16, i + 17, &address) && address == 0xCC006800 &&
							findi_patterns(data, dataType, i + 19, i + 20, length, &EXIIntrruptHandlerSigs[0],
							                                                       &EXIIntrruptHandlerSigs[1],
							                                                       &EXIIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 21, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 23, i + 24, length, &TCIntrruptHandlerSigs[0],
							                                                       &TCIntrruptHandlerSigs[1],
							                                                       &TCIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 25, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 27, i + 28, length, &EXTIntrruptHandlerSigs[0],
							                                                       &EXTIntrruptHandlerSigs[1],
							                                                       &EXTIntrruptHandlerSigs[2],
							                                                       &EXTIntrruptHandlerSigs[3], NULL) &&
							findx_patterns(data, dataType, i + 29, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 31, i + 32, length, &EXIIntrruptHandlerSigs[0],
							                                                       &EXIIntrruptHandlerSigs[1],
							                                                       &EXIIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 33, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 35, i + 36, length, &TCIntrruptHandlerSigs[0],
							                                                       &TCIntrruptHandlerSigs[1],
							                                                       &TCIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 37, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 39, i + 40, length, &EXTIntrruptHandlerSigs[0],
							                                                       &EXTIntrruptHandlerSigs[1],
							                                                       &EXTIntrruptHandlerSigs[2],
							                                                       &EXTIntrruptHandlerSigs[3], NULL) &&
							findx_patterns(data, dataType, i + 41, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 43, i + 44, length, &EXIIntrruptHandlerSigs[0],
							                                                       &EXIIntrruptHandlerSigs[1],
							                                                       &EXIIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_patterns(data, dataType, i + 47, i + 48, length, &TCIntrruptHandlerSigs[0],
							                                                       &TCIntrruptHandlerSigs[1],
							                                                       &TCIntrruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 49, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 53, length, &EXIProbeResetSigs[0],
							                                               &EXIProbeResetSigs[1], NULL))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  7, length, &__OSMaskInterruptsSigs[0]) &&
							get_immediate(data,   i +  9, i + 10, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 12, i + 13, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 15, i + 16, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 18, i + 19, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 21, i + 22, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 23, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 25, i + 26, length, &TCIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 27, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 29, i + 30, length, &EXTIntrruptHandlerSigs[3]) &&
							findx_pattern(data, dataType, i + 31, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 33, i + 34, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 35, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 37, i + 38, length, &TCIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 39, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 41, i + 42, length, &EXTIntrruptHandlerSigs[3]) &&
							findx_pattern(data, dataType, i + 43, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 45, i + 46, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 47, length, &__OSSetInterruptHandlerSigs[0]) &&
							findi_pattern(data, dataType, i + 49, i + 50, length, &TCIntrruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 51, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 55, length, &EXIProbeResetSigs[1]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (get_immediate (data,   i +  3, i +  4, &address) && address == 0xCC00680C &&
							get_immediate (data,   i +  8, i +  9, &address) && address == 0xCC006820 &&
							get_immediate (data,   i + 13, i + 14, &address) && address == 0xCC006834 &&
							findx_patterns(data, dataType, i + 20, length, &__OSMaskInterruptsSigs[0],
							                                               &__OSMaskInterruptsSigs[1], NULL) &&
							get_immediate (data,   i + 22, i + 23, &address) && address == 0xCC006800 &&
							get_immediate (data,   i + 25, i + 26, &address) && address == 0xCC006814 &&
							get_immediate (data,   i + 28, i + 29, &address) && address == 0xCC006828 &&
							get_immediate (data,   i + 31, i + 32, &address) && address == 0xCC006800 &&
							findi_pattern (data, dataType, i + 34, i + 35, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 36, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 38, i + 39, length, &TCIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 42, i + 43, length, &EXTIntrruptHandlerSigs[3]) &&
							findx_patterns(data, dataType, i + 44, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 46, i + 47, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 50, i + 51, length, &TCIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 52, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 54, i + 55, length, &EXTIntrruptHandlerSigs[3]) &&
							findx_patterns(data, dataType, i + 56, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 58, i + 59, length, &EXIIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 60, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findi_pattern (data, dataType, i + 62, i + 63, length, &TCIntrruptHandlerSigs[2]) &&
							findx_patterns(data, dataType, i + 64, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 68, length, &EXIGetIDSigs[3]) &&
							findx_pattern (data, dataType, i + 72, length, &EXIProbeResetSigs[1]) &&
							findx_pattern (data, dataType, i + 77, length, &EXIGetIDSigs[3]) &&
							findx_pattern (data, dataType, i + 91, length, &EXIGetIDSigs[3]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  9, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate(data,   i + 10, i + 12, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 10, i + 15, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 10, i + 18, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 10, i + 20, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 14, i + 16, length, &EXIIntrruptHandlerSigs[3]) &&
							findx_pattern(data, dataType, i + 21, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 22, i + 23, length, &TCIntrruptHandlerSigs[3]) &&
							findx_pattern(data, dataType, i + 26, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 27, i + 28, length, &EXTIntrruptHandlerSigs[4]) &&
							findx_pattern(data, dataType, i + 31, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 34, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 37, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 43, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 46, length, &__OSSetInterruptHandlerSigs[1]) &&
							get_immediate(data,   i + 50, i + 51, &address) && address == 0x800030C4 &&
							get_immediate(data,   i + 50, i + 53, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 54, length, &__EXIProbeSigs[3]) &&
							findx_pattern(data, dataType, i + 56, length, &__EXIProbeSigs[3]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern (data, dataType, i +  9, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate (data,   i + 10, i + 12, &address) && address == 0xCC006800 &&
							get_immediate (data,   i + 10, i + 15, &address) && address == 0xCC006814 &&
							get_immediate (data,   i + 10, i + 18, &address) && address == 0xCC006828 &&
							get_immediate (data,   i + 10, i + 20, &address) && address == 0xCC006800 &&
							findi_pattern (data, dataType, i + 14, i + 16, length, &EXIIntrruptHandlerSigs[4]) &&
							findx_pattern (data, dataType, i + 21, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern (data, dataType, i + 22, i + 23, length, &TCIntrruptHandlerSigs[4]) &&
							findx_pattern (data, dataType, i + 26, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_patterns(data, dataType, i + 27, i + 28, length, &EXTIntrruptHandlerSigs[5],
							                                                       &EXTIntrruptHandlerSigs[7], NULL) &&
							findx_pattern (data, dataType, i + 31, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 34, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 37, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 43, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 46, length, &__OSSetInterruptHandlerSigs[1]) &&
							get_immediate (data,   i + 50, i + 51, &address) && address == 0x800030C4 &&
							get_immediate (data,   i + 50, i + 54, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i + 58, length, &__EXIProbeSigs[4]) &&
							findx_pattern (data, dataType, i + 60, length, &__EXIProbeSigs[4]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  5, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate(data,   i +  7, i +  8, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 10, i + 11, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 13, i + 14, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 16, i + 17, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 19, i + 20, length, &EXIIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 21, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 23, i + 24, length, &TCIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 25, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 27, i + 28, length, &EXTIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 29, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 31, i + 32, length, &EXIIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 33, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 35, i + 36, length, &TCIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 37, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 39, i + 40, length, &EXTIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 41, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 43, i + 44, length, &EXIIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 47, i + 48, length, &TCIntrruptHandlerSigs[5]) &&
							findx_pattern(data, dataType, i + 49, length, &__OSSetInterruptHandlerSigs[1]) &&
							get_immediate(data,   i + 54, i + 55, &address) && address == 0x800030C4 &&
							get_immediate(data,   i + 56, i + 57, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 66, length, &__EXIProbeSigs[5]) &&
							findx_pattern(data, dataType, i + 68, length, &__EXIProbeSigs[5]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i + 11, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate(data,   i + 13, i + 14, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 13, i + 15, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 13, i + 16, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 13, i + 18, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 20, i + 21, length, &EXIIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 23, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 25, i + 26, length, &TCIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 28, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 30, i + 31, length, &EXTIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 33, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 36, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 39, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 42, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[1]) &&
							get_immediate(data,   i + 52, i + 53, &address) && address == 0x800030C4 &&
							get_immediate(data,   i + 52, i + 54, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 60, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i + 62, length, &__EXIProbeSigs[6]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (get_immediate (data,   i +  7, i +  8, &address) && address == 0xCC006800 &&
							findx_patterns(data, dataType, i + 23, length, &__OSMaskInterruptsSigs[1],
							                                               &__OSMaskInterruptsSigs[2], NULL) &&
							get_immediate (data,   i + 25, i + 26, &address) && address == 0xCC006800 &&
							get_immediate (data,   i + 25, i + 27, &address) && address == 0xCC006814 &&
							get_immediate (data,   i + 25, i + 28, &address) && address == 0xCC006828 &&
							get_immediate (data,   i + 25, i + 30, &address) && address == 0xCC006800 &&
							findi_pattern (data, dataType, i + 32, i + 33, length, &EXIIntrruptHandlerSigs[6]) &&
							findx_patterns(data, dataType, i + 35, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findi_pattern (data, dataType, i + 37, i + 38, length, &TCIntrruptHandlerSigs[6]) &&
							findx_patterns(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findi_pattern (data, dataType, i + 42, i + 43, length, &EXTIntrruptHandlerSigs[7]) &&
							findx_patterns(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 51, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 54, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 57, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							findx_patterns(data, dataType, i + 60, length, &__OSSetInterruptHandlerSigs[1],
							                                               &__OSSetInterruptHandlerSigs[2], NULL) &&
							get_immediate (data,   i + 64, i + 65, &address) && address == 0x800030C4 &&
							get_immediate (data,   i + 64, i + 66, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i + 72, length, &__EXIProbeSigs[6]) &&
							findx_pattern (data, dataType, i + 74, length, &__EXIProbeSigs[6]) &&
							findx_pattern (data, dataType, i + 78, length, &EXIGetIDSigs[8]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (get_immediate(data,   i +  7, i +  8, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 23, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate(data,   i + 25, i + 26, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 25, i + 27, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 25, i + 28, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 25, i + 30, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 32, i + 33, length, &EXIIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 35, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 37, i + 38, length, &TCIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 42, i + 43, length, &EXTIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 51, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 54, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 57, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 60, length, &__OSSetInterruptHandlerSigs[1]) &&
							get_immediate(data,   i + 64, i + 65, &address) && address == 0x800030C4 &&
							get_immediate(data,   i + 64, i + 66, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 72, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i + 74, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i + 78, length, &EXIGetIDSigs[8]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (get_immediate(data,   i +  7, i +  8, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 23, length, &__OSMaskInterruptsSigs[1]) &&
							get_immediate(data,   i + 25, i + 26, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 25, i + 27, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 25, i + 28, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 25, i + 30, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 32, i + 33, length, &EXIIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 35, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 37, i + 38, length, &TCIntrruptHandlerSigs[6]) &&
							findx_pattern(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[1]) &&
							findi_pattern(data, dataType, i + 42, i + 43, length, &EXTIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 45, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 51, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 54, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 57, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 60, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 64, length, &EXIGetIDSigs[8]) &&
							get_immediate(data,   i + 68, i + 69, &address) && address == 0x800030C4 &&
							get_immediate(data,   i + 68, i + 70, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 76, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i + 78, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i + 83, length, &EXIGetIDSigs[8]) &&
							findx_pattern(data, dataType, i + 97, length, &EXIGetIDSigs[8]))
							EXIInitSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIUnlockSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIUnlockSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 5] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 33, length, &SetExiInterruptMaskSigs[0]) &&
							findx_pattern(data, dataType, i + 53, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[0]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 33, length, &SetExiInterruptMaskSigs[1]) &&
							findx_pattern(data, dataType, i + 53, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[1]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 20, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 34, length, &SetExiInterruptMaskSigs[2]) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[2]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 8] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[3]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[3]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[4]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[4]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 15, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 23, length, &SetExiInterruptMaskSigs[5]) &&
							findx_pattern(data, dataType, i + 43, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[5]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[6]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &EXILockSigs[6]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(EXIGetIDSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &EXIGetIDSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  23, length, &EXIAttachSigs[0]) &&
							findx_pattern(data, dataType, i +  31, length, &EXILockSigs[0]) &&
							findx_pattern(data, dataType, i +  39, length, &EXISelectSigs[0]) &&
							findx_pattern(data, dataType, i +  51, length, &EXIImmSigs[0]) &&
							findx_pattern(data, dataType, i +  56, length, &EXISyncSigs[0]) &&
							findx_pattern(data, dataType, i +  65, length, &EXIImmSigs[0]) &&
							findx_pattern(data, dataType, i +  70, length, &EXISyncSigs[0]) &&
							findx_pattern(data, dataType, i +  75, length, &EXIDeselectSigs[0]) &&
							findx_pattern(data, dataType, i +  80, length, &EXIUnlockSigs[0]) &&
							findx_pattern(data, dataType, i +  86, length, &EXIDetachSigs[0]))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  26, length, &__EXIProbeSigs[1]) &&
							get_immediate(data,  i +  33, i +  34, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  44, length, &__EXIAttachSigs[0]) &&
							get_immediate(data,  i +  50, i +  51, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  63, length, &EXILockSigs[1]) &&
							findx_pattern(data, dataType, i +  71, length, &EXISelectSigs[1]) &&
							findx_pattern(data, dataType, i +  83, length, &EXIImmSigs[1]) &&
							findx_pattern(data, dataType, i +  88, length, &EXISyncSigs[1]) &&
							findx_pattern(data, dataType, i +  97, length, &EXIImmSigs[1]) &&
							findx_pattern(data, dataType, i + 102, length, &EXISyncSigs[1]) &&
							findx_pattern(data, dataType, i + 107, length, &EXIDeselectSigs[1]) &&
							findx_pattern(data, dataType, i + 112, length, &EXIUnlockSigs[1]) &&
							findx_pattern(data, dataType, i + 118, length, &EXIDetachSigs[1]) &&
							findx_pattern(data, dataType, i + 119, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i + 122, i + 123, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 135, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  27, length, &__EXIProbeSigs[2]) &&
							get_immediate (data,  i +  34, i +  35, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i +  45, length, &__EXIAttachSigs[1]) &&
							get_immediate (data,  i +  51, i +  52, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i +  64, length, &EXILockSigs[2]) &&
							findx_pattern (data, dataType, i +  72, length, &EXISelectSigs[2]) &&
							findx_pattern (data, dataType, i +  84, length, &EXIImmSigs[2]) &&
							findx_patterns(data, dataType, i +  89, length, &EXISyncSigs[2],
							                                                &EXISyncSigs[3], NULL) &&
							findx_pattern (data, dataType, i +  98, length, &EXIImmSigs[2]) &&
							findx_patterns(data, dataType, i + 103, length, &EXISyncSigs[2],
							                                                &EXISyncSigs[3], NULL) &&
							findx_pattern (data, dataType, i + 108, length, &EXIDeselectSigs[2]) &&
							findx_pattern (data, dataType, i + 113, length, &EXIUnlockSigs[2]) &&
							findx_pattern (data, dataType, i + 119, length, &EXIDetachSigs[2]) &&
							findx_pattern (data, dataType, i + 120, length, &OSDisableInterruptsSig) &&
							get_immediate (data,  i + 123, i + 124, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i + 136, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  38, length, &__EXIProbeSigs[2]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  56, length, &__EXIAttachSigs[1]) &&
							get_immediate(data,  i +  62, i +  63, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  65, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &EXILockSigs[2]) &&
							findx_pattern(data, dataType, i +  85, length, &EXISelectSigs[2]) &&
							findx_pattern(data, dataType, i +  97, length, &EXIImmSigs[2]) &&
							findx_pattern(data, dataType, i + 102, length, &EXISyncSigs[4]) &&
							findx_pattern(data, dataType, i + 111, length, &EXIImmSigs[2]) &&
							findx_pattern(data, dataType, i + 116, length, &EXISyncSigs[4]) &&
							findx_pattern(data, dataType, i + 121, length, &EXIDeselectSigs[2]) &&
							findx_pattern(data, dataType, i + 126, length, &EXIUnlockSigs[2]) &&
							findx_pattern(data, dataType, i + 128, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 134, length, &EXIDetachSigs[2]) &&
							findx_pattern(data, dataType, i + 135, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i + 138, i + 139, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 151, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 13] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  21, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  25, length, &__EXIProbeSigs[3]) &&
							findx_pattern(data, dataType, i +  29, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  36, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  42, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  47, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  56, length, &EXILockSigs[3]) &&
							findx_pattern(data, dataType, i +  64, length, &EXISelectSigs[3]) &&
							findx_pattern(data, dataType, i +  76, length, &EXIImmSigs[3]) &&
							findx_pattern(data, dataType, i +  81, length, &EXISyncSigs[6]) &&
							findx_pattern(data, dataType, i +  90, length, &EXIImmSigs[3]) &&
							findx_pattern(data, dataType, i +  95, length, &EXISyncSigs[6]) &&
							findx_pattern(data, dataType, i + 100, length, &EXIDeselectSigs[4]) &&
							(data[i + 104] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 106, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 112, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 119, length, &SetExiInterruptMaskSigs[3]) &&
							findx_pattern(data, dataType, i + 138, length, &OSRestoreInterruptsSig) &&
							(data[i + 143] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 145, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 151, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 160, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 168, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 170, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  16, length, &__EXIProbeSigs[4]) &&
							get_immediate (data,  i +  21, i +  24, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i +  33, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  39, length, &__EXIProbeSigs[4]) &&
							findx_pattern (data, dataType, i +  43, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  50, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern (data, dataType, i +  56, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern (data, dataType, i +  61, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  82, length, &EXILockSigs[4]) &&
							findx_pattern (data, dataType, i +  90, length, &EXISelectSigs[4]) &&
							findx_pattern (data, dataType, i + 102, length, &EXIImmSigs[4]) &&
							findx_patterns(data, dataType, i + 107, length, &EXISyncSigs[7],
							                                                &EXISyncSigs[9], NULL) &&
							findx_pattern (data, dataType, i + 116, length, &EXIImmSigs[4]) &&
							findx_patterns(data, dataType, i + 121, length, &EXISyncSigs[7],
							                                                &EXISyncSigs[9], NULL) &&
							findx_pattern (data, dataType, i + 126, length, &EXIDeselectSigs[5]) &&
							findx_pattern (data, dataType, i + 130, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 136, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 143, length, &SetExiInterruptMaskSigs[4]) &&
							findx_pattern (data, dataType, i + 162, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 167, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 173, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 182, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 190, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern (data, dataType, i + 192, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 193, length, &OSDisableInterruptsSig) &&
							get_immediate (data,  i + 194, i + 196, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i + 206, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  16, length, &__EXIProbeSigs[5]) &&
							get_immediate(data,  i +  23, i +  24, &address) && address == 0x800030C0 &&
							(data[i + 32] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  34, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  40, length, &__EXIProbeSigs[5]) &&
							findx_pattern(data, dataType, i +  44, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  51, length, &EXIClearInterruptsSigs[3]) &&
							findx_pattern(data, dataType, i +  57, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  62, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,  i +  69, i +  70, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  82, length, &EXILockSigs[5]) &&
							findx_pattern(data, dataType, i +  89, length, &EXISelectSigs[5]) &&
							findx_pattern(data, dataType, i + 100, length, &EXIImmSigs[5]) &&
							findx_pattern(data, dataType, i + 105, length, &EXISyncSigs[8]) &&
							findx_pattern(data, dataType, i + 114, length, &EXIImmSigs[5]) &&
							findx_pattern(data, dataType, i + 119, length, &EXISyncSigs[8]) &&
							findx_pattern(data, dataType, i + 124, length, &EXIDeselectSigs[6]) &&
							(data[i + 128] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 130, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 136, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 143, length, &SetExiInterruptMaskSigs[5]) &&
							findx_pattern(data, dataType, i + 163, length, &OSRestoreInterruptsSig) &&
							(data[i + 168] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 170, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 176, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 185, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 193, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 195, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 196, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i + 199, i + 200, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 211, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  16, length, &__EXIProbeSigs[6]) &&
							get_immediate(data,  i +  23, i +  24, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  33, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  39, length, &__EXIProbeSigs[6]) &&
							findx_pattern(data, dataType, i +  43, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  50, length, &EXIClearInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  56, length, &__OSUnmaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i +  61, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  82, length, &EXILockSigs[6]) &&
							findx_pattern(data, dataType, i +  90, length, &EXISelectSigs[6]) &&
							findx_pattern(data, dataType, i + 102, length, &EXIImmSigs[6]) &&
							findx_pattern(data, dataType, i + 107, length, &EXISyncSigs[10]) &&
							findx_pattern(data, dataType, i + 116, length, &EXIImmSigs[6]) &&
							findx_pattern(data, dataType, i + 121, length, &EXISyncSigs[10]) &&
							findx_pattern(data, dataType, i + 126, length, &EXIDeselectSigs[7]) &&
							findx_pattern(data, dataType, i + 130, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 136, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 143, length, &SetExiInterruptMaskSigs[6]) &&
							findx_pattern(data, dataType, i + 162, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 167, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 173, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 182, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 190, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 192, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 193, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i + 195, i + 196, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 206, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern (data, dataType, i +  25, length, &__EXIProbeSigs[6]) &&
							get_immediate (data,  i +  32, i +  33, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i +  42, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  48, length, &__EXIProbeSigs[6]) &&
							findx_pattern (data, dataType, i +  52, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  59, length, &EXIClearInterruptsSigs[2]) &&
							findx_patterns(data, dataType, i +  65, length, &__OSUnmaskInterruptsSigs[1],
							                                                &__OSUnmaskInterruptsSigs[2], NULL) &&
							findx_pattern (data, dataType, i +  70, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  93, length, &EXILockSigs[6]) &&
							findx_pattern (data, dataType, i + 101, length, &EXISelectSigs[6]) &&
							findx_pattern (data, dataType, i + 113, length, &EXIImmSigs[6]) &&
							findx_pattern (data, dataType, i + 118, length, &EXISyncSigs[11]) &&
							findx_pattern (data, dataType, i + 127, length, &EXIImmSigs[6]) &&
							findx_pattern (data, dataType, i + 132, length, &EXISyncSigs[11]) &&
							findx_pattern (data, dataType, i + 137, length, &EXIDeselectSigs[7]) &&
							findx_pattern (data, dataType, i + 141, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 147, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 154, length, &SetExiInterruptMaskSigs[6]) &&
							findx_pattern (data, dataType, i + 173, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 175, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 180, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 186, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 195, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 203, length, &__OSMaskInterruptsSigs[1],
							                                                &__OSMaskInterruptsSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 205, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 206, length, &OSDisableInterruptsSig) &&
							get_immediate (data,  i + 208, i + 209, &address) && address == 0x800030C0 &&
							findx_pattern (data, dataType, i + 219, length, &OSRestoreInterruptsSig))
							EXIGetIDSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__DVDInterruptHandlerSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__DVDInterruptHandlerSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  15, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  53, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i + 132, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 134, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 149, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 151, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  13, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i +  57, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i + 136, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 138, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 153, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 155, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_patterns(data, dataType, i +  11, length, &__OSGetSystemTimeSigs[0],
							                                                &__OSGetSystemTimeSigs[1], NULL) &&
							findx_patterns(data, dataType, i +  57, length, &__OSGetSystemTimeSigs[0],
							                                                &__OSGetSystemTimeSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 136, length, &OSClearContextSigs[0],
							                                                &OSClearContextSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 138, length, &OSSetCurrentContextSig) &&
							findx_patterns(data, dataType, i + 153, length, &OSClearContextSigs[0],
							                                                &OSClearContextSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 155, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  33, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  99, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 101, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 113, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  33, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  99, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 101, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 113, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 115, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  16, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  54, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 157, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 159, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 173, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 175, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  18, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  56, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 159, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 161, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 175, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 177, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  16, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 162, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 164, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 178, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 180, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  11, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i +  56, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 160, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 162, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 176, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 178, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i +  59, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 165, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 167, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 181, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 183, length, &OSSetCurrentContextSig))
							__DVDInterruptHandlerSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(ReadSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &ReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 12, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i + 41, length, &SetTimeoutAlarmSigs[0]) &&
							findx_pattern(data, dataType, i + 48, length, &SetTimeoutAlarmSigs[0]) &&
							find_pattern_before(data, length, &SetTimeoutAlarmSigs[0], &AlarmHandlerForTimeoutSigs[0]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[0],
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 43, length, &SetTimeoutAlarmSigs[0]) &&
							findx_pattern (data, dataType, i + 50, length, &SetTimeoutAlarmSigs[0]) &&
							find_pattern_before(data, length, &SetTimeoutAlarmSigs[0], &AlarmHandlerForTimeoutSigs[0]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findi_pattern(data, dataType, i + 30, i + 31, length, &AlarmHandlerForTimeoutSigs[1]) &&
							findi_pattern(data, dataType, i + 43, i + 44, length, &AlarmHandlerForTimeoutSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 15, length, &__OSGetSystemTimeSigs[1]) &&
							findi_pattern(data, dataType, i + 39, i + 40, length, &AlarmHandlerForTimeoutSigs[1]) &&
							findi_pattern(data, dataType, i + 52, i + 53, length, &AlarmHandlerForTimeoutSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 17, length, &__OSGetSystemTimeSigs[1]) &&
							findi_pattern(data, dataType, i + 41, i + 42, length, &AlarmHandlerForTimeoutSigs[1]) &&
							findi_pattern(data, dataType, i + 54, i + 55, length, &AlarmHandlerForTimeoutSigs[1]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[2]) &&
							findi_pattern(data, dataType, i + 39, i + 42, length, &AlarmHandlerForTimeoutSigs[2]) &&
							findi_pattern(data, dataType, i + 54, i + 57, length, &AlarmHandlerForTimeoutSigs[2]))
							ReadSigs[j].offsetFoundAt = i;
						break;
				}
			}
			else if (ReadSigs[j].offsetFoundAt) {
				for (k = 0; k < sizeof(DoJustReadSigs) / sizeof(FuncPattern); k++) {
					if (compare_pattern(&fp, &DoJustReadSigs[k])) {
						if (findx_pattern(data, dataType, i + 16, length, &ReadSigs[j]))
							DoJustReadSigs[k].offsetFoundAt = i;
					}
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDLowReadSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDLowReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  52, length, &DoJustReadSigs[0]) &&
							findx_pattern(data, dataType, i +  75, length, &DoJustReadSigs[0]) &&
							findx_pattern(data, dataType, i +  89, length, &__OSGetSystemTimeSigs[0]) &&
							findx_pattern(data, dataType, i + 112, length, &DoJustReadSigs[0])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, length, &DVDLowReadSigs[0], &DVDLowSeekSigs[0]) &&
								find_pattern_after(data, length, &DVDLowSeekSigs[0], &DVDLowWaitCoverCloseSigs[0]) &&
								find_pattern_after(data, length, &DVDLowWaitCoverCloseSigs[0], &DVDLowReadDiskIDSigs[0]) &&
								find_pattern_after(data, length, &DVDLowReadDiskIDSigs[0], &DVDLowStopMotorSigs[0]) &&
								find_pattern_after(data, length, &DVDLowStopMotorSigs[0], &DVDLowRequestErrorSigs[0]) &&
								find_pattern_after(data, length, &DVDLowRequestErrorSigs[0], &DVDLowInquirySigs[0]) &&
								find_pattern_after(data, length, &DVDLowInquirySigs[0], &DVDLowAudioStreamSigs[0]) &&
								find_pattern_after(data, length, &DVDLowAudioStreamSigs[0], &DVDLowRequestAudioStatusSigs[0]) &&
								find_pattern_after(data, length, &DVDLowRequestAudioStatusSigs[0], &DVDLowAudioBufferConfigSigs[0]) &&
								find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[0], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  52, length, &DoJustReadSigs[0]) &&
							findx_pattern (data, dataType, i +  75, length, &DoJustReadSigs[0]) &&
							findx_patterns(data, dataType, i +  89, length, &__OSGetSystemTimeSigs[0],
							                                                &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 112, length, &DoJustReadSigs[0])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, length, &DVDLowReadSigs[1], &DVDLowSeekSigs[1]) &&
								find_pattern_after(data, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]) &&
								find_pattern_after(data, length, &DVDLowWaitCoverCloseSigs[1], &DVDLowReadDiskIDSigs[1]) &&
								find_pattern_after(data, length, &DVDLowReadDiskIDSigs[1], &DVDLowStopMotorSigs[1]) &&
								find_pattern_after(data, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]) &&
								find_pattern_after(data, length, &DVDLowRequestErrorSigs[1], &DVDLowInquirySigs[1]) &&
								find_pattern_after(data, length, &DVDLowInquirySigs[1], &DVDLowAudioStreamSigs[1]) &&
								find_pattern_after(data, length, &DVDLowAudioStreamSigs[1], &DVDLowRequestAudioStatusSigs[1]) &&
								find_pattern_after(data, length, &DVDLowRequestAudioStatusSigs[1], &DVDLowAudioBufferConfigSigs[1]) &&
								find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 3:
						if (findx_patterns(data, dataType, i +  28, length, &ReadSigs[2], &ReadSigs[3], NULL) &&
							findx_patterns(data, dataType, i +  83, length, &ReadSigs[2], &ReadSigs[3], NULL) &&
							findx_pattern (data, dataType, i +  97, length, &__OSGetSystemTimeSigs[1]) &&
							findx_patterns(data, dataType, i + 125, length, &ReadSigs[2], &ReadSigs[3], NULL)) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, length, &DVDLowReadSigs[3], &DVDLowSeekSigs[3]) &&
								find_pattern_after(data, length, &DVDLowSeekSigs[3], &DVDLowWaitCoverCloseSigs[2]) &&
								find_pattern_after(data, length, &DVDLowWaitCoverCloseSigs[2], &DVDLowReadDiskIDSigs[3]) &&
								find_pattern_after(data, length, &DVDLowReadDiskIDSigs[3], &DVDLowStopMotorSigs[3]) &&
								find_pattern_after(data, length, &DVDLowStopMotorSigs[3], &DVDLowRequestErrorSigs[3]) &&
								find_pattern_after(data, length, &DVDLowRequestErrorSigs[3], &DVDLowInquirySigs[3]) &&
								find_pattern_after(data, length, &DVDLowInquirySigs[3], &DVDLowAudioStreamSigs[3]) &&
								find_pattern_after(data, length, &DVDLowAudioStreamSigs[3], &DVDLowRequestAudioStatusSigs[3]) &&
								find_pattern_after(data, length, &DVDLowRequestAudioStatusSigs[3], &DVDLowAudioBufferConfigSigs[3]) &&
								find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[3], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  28, length, &ReadSigs[4]) &&
							findx_pattern(data, dataType, i +  83, length, &ReadSigs[4]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 125, length, &ReadSigs[4])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, length, &DVDLowReadSigs[4], &DVDLowSeekSigs[4]) &&
								find_pattern_after(data, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]) &&
								find_pattern_after(data, length, &DVDLowWaitCoverCloseSigs[3], &DVDLowReadDiskIDSigs[4]) &&
								find_pattern_after(data, length, &DVDLowReadDiskIDSigs[4], &DVDLowStopMotorSigs[4]) &&
								find_pattern_after(data, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]) &&
								find_pattern_after(data, length, &DVDLowRequestErrorSigs[4], &DVDLowInquirySigs[4]) &&
								find_pattern_after(data, length, &DVDLowInquirySigs[4], &DVDLowAudioStreamSigs[4]) &&
								find_pattern_after(data, length, &DVDLowAudioStreamSigs[4], &DVDLowRequestAudioStatusSigs[4]) &&
								find_pattern_after(data, length, &DVDLowRequestAudioStatusSigs[4], &DVDLowAudioBufferConfigSigs[4]) &&
								find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  28, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 136, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 191, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 219, length, &__OSGetSystemTimeSigs[2])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, length, &DVDLowReadSigs[5], &DVDLowSeekSigs[5]) &&
								find_pattern_after(data, length, &DVDLowSeekSigs[5], &DVDLowWaitCoverCloseSigs[4]) &&
								find_pattern_after(data, length, &DVDLowWaitCoverCloseSigs[4], &DVDLowReadDiskIDSigs[5]) &&
								find_pattern_after(data, length, &DVDLowReadDiskIDSigs[5], &DVDLowStopMotorSigs[5]) &&
								find_pattern_after(data, length, &DVDLowStopMotorSigs[5], &DVDLowRequestErrorSigs[5]) &&
								find_pattern_after(data, length, &DVDLowRequestErrorSigs[5], &DVDLowInquirySigs[5]) &&
								find_pattern_after(data, length, &DVDLowInquirySigs[5], &DVDLowAudioStreamSigs[5]) &&
								find_pattern_after(data, length, &DVDLowAudioStreamSigs[5], &DVDLowRequestAudioStatusSigs[5]) &&
								find_pattern_after(data, length, &DVDLowRequestAudioStatusSigs[5], &DVDLowAudioBufferConfigSigs[5]))
								find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[5], &DVDLowResetSigs[2]);
						}
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDLowResetSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDLowResetSigs[j])) {
				switch (j) {
					case 0:
						if (findx_patterns(data, dataType, i + 13, length, &__OSGetSystemTimeSigs[0],
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 16, length, &__OSGetSystemTimeSigs[0],
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 41, length, &__OSGetSystemTimeSigs[0],
							                                               &__OSGetSystemTimeSigs[1], NULL))
							DVDLowResetSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 12, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 25, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 39, length, &__OSGetSystemTimeSigs[1]))
							DVDLowResetSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 12, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 19, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 40, length, &__OSGetSystemTimeSigs[2]))
							DVDLowResetSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDLowGetCoverStatusSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDLowGetCoverStatusSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 3, length, &__OSGetSystemTimeSigs[0]))
							DVDLowGetCoverStatusSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 3, length, &__OSGetSystemTimeSigs[1]))
							DVDLowGetCoverStatusSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 3, length, &__OSGetSystemTimeSigs[2]))
							DVDLowGetCoverStatusSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 13, length, &__DVDClearWaitingQueueSigs[0]) &&
							findx_pattern(data, dataType, i + 23, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 25, length, &__OSUnmaskInterruptsSigs[0]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i + 11, length, &__DVDClearWaitingQueueSigs[0]) &&
							findx_patterns(data, dataType, i + 21, length, &__OSSetInterruptHandlerSigs[0],
							                                               &__OSSetInterruptHandlerSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 23, length, &__OSUnmaskInterruptsSigs[0],
							                                               &__OSUnmaskInterruptsSigs[1], NULL))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 11, length, &__DVDClearWaitingQueueSigs[0]) &&
							findx_pattern(data, dataType, i + 23, length, &__OSSetInterruptHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 25, length, &__OSUnmaskInterruptsSigs[0]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 13, length, &__DVDClearWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i + 20, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 22, length, &__OSUnmaskInterruptsSigs[1]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 14, length, &__DVDClearWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i + 22, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 24, length, &__OSUnmaskInterruptsSigs[1]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 12, length, &__DVDClearWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i + 20, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 22, length, &__OSUnmaskInterruptsSigs[1]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i + 11, length, &__DVDClearWaitingQueueSigs[2]) &&
							findx_pattern(data, dataType, i + 19, length, &__OSSetInterruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 21, length, &__OSUnmaskInterruptsSigs[2]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i + 13, length, &__DVDClearWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i + 23, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern(data, dataType, i + 25, length, &__OSUnmaskInterruptsSigs[1]))
							DVDInitSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(stateGettingErrorSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &stateGettingErrorSigs[j])) {
				switch (j) {
					case 0:
						if (findi_patterns(data, dataType, i + 3, i + 4, length, &cbForStateGettingErrorSigs[0],
							                                                     &cbForStateGettingErrorSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 5, length, &DVDLowRequestErrorSigs[0],
							                                              &DVDLowRequestErrorSigs[1], NULL))
							stateGettingErrorSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findi_patterns(data, dataType, i + 1, i + 3, length, &cbForStateGettingErrorSigs[2],
							                                                     &cbForStateGettingErrorSigs[3],
							                                                     &cbForStateGettingErrorSigs[4],
							                                                     &cbForStateGettingErrorSigs[5],
							                                                     &cbForStateGettingErrorSigs[7], NULL) &&
							findx_patterns(data, dataType, i + 5, length, &DVDLowRequestErrorSigs[2],
							                                              &DVDLowRequestErrorSigs[3],
							                                              &DVDLowRequestErrorSigs[4], NULL))
							stateGettingErrorSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findi_pattern(data, dataType, i + 2, i + 4, length, &cbForStateGettingErrorSigs[6]) &&
							findx_pattern(data, dataType, i + 5, length, &DVDLowRequestErrorSigs[5]))
							stateGettingErrorSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(cbForUnrecoveredErrorSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &cbForUnrecoveredErrorSigs[j])) {
				switch (j) {
					case 0:
						if (findi_pattern (data, dataType, i + 24, i + 25, length, &cbForUnrecoveredErrorRetrySigs[0]) &&
							findx_patterns(data, dataType, i + 26, length, &DVDLowRequestErrorSigs[0],
							                                               &DVDLowRequestErrorSigs[1], NULL))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findi_pattern(data, dataType, i + 21, i + 22, length, &cbForUnrecoveredErrorRetrySigs[1]) &&
							findx_pattern(data, dataType, i + 23, length, &DVDLowRequestErrorSigs[1]))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findi_pattern(data, dataType, i + 16, i + 17, length, &cbForUnrecoveredErrorRetrySigs[2]) &&
							findx_pattern(data, dataType, i + 18, length, &DVDLowRequestErrorSigs[3]))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
					case 3:
						if (findi_pattern (data, dataType, i + 19, i + 20, length, &cbForUnrecoveredErrorRetrySigs[3]) &&
							findx_patterns(data, dataType, i + 21, length, &DVDLowRequestErrorSigs[3],
							                                               &DVDLowRequestErrorSigs[4], NULL))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findi_pattern(data, dataType, i + 14, i + 15, length, &cbForUnrecoveredErrorRetrySigs[4]) &&
							findx_pattern(data, dataType, i + 16, length, &DVDLowRequestErrorSigs[5]))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findi_pattern(data, dataType, i + 16, i + 17, length, &cbForUnrecoveredErrorRetrySigs[5]) &&
							findx_pattern(data, dataType, i + 18, length, &DVDLowRequestErrorSigs[4]))
							cbForUnrecoveredErrorSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(stateMotorStoppedSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &stateMotorStoppedSigs[j])) {
				switch (j) {
					case 0:
						if (findi_pattern (data, dataType, i + 3, i + 4, length, &cbForStateMotorStoppedSigs[0]) &&
							findx_patterns(data, dataType, i + 5, length, &DVDLowWaitCoverCloseSigs[0],
							                                              &DVDLowWaitCoverCloseSigs[1], NULL))
							stateMotorStoppedSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findi_patterns(data, dataType, i + 1, i + 3, length, &cbForStateMotorStoppedSigs[1],
							                                                     &cbForStateMotorStoppedSigs[2],
							                                                     &cbForStateMotorStoppedSigs[3],
							                                                     &cbForStateMotorStoppedSigs[5], NULL) &&
							findx_patterns(data, dataType, i + 5, length, &DVDLowWaitCoverCloseSigs[2],
							                                              &DVDLowWaitCoverCloseSigs[3], NULL))
							stateMotorStoppedSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findi_pattern(data, dataType, i + 2, i + 4, length, &cbForStateMotorStoppedSigs[4]) &&
							findx_pattern(data, dataType, i + 3, length, &DVDLowWaitCoverCloseSigs[4]))
							stateMotorStoppedSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(stateBusySigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &stateBusySigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  26, length, &DVDLowReadDiskIDSigs[0]) &&
							findx_pattern(data, dataType, i +  53, length, &DVDLowReadSigs[0]) &&
							findx_pattern(data, dataType, i +  62, length, &DVDLowSeekSigs[0]) &&
							findx_pattern(data, dataType, i +  66, length, &DVDLowStopMotorSigs[0]) &&
							findx_pattern(data, dataType, i +  70, length, &DVDLowStopMotorSigs[0]) &&
							findx_pattern(data, dataType, i +  85, length, &DVDLowRequestAudioStatusSigs[0]) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowAudioStreamSigs[0]) &&
							findx_pattern(data, dataType, i + 106, length, &DVDLowAudioStreamSigs[0]) &&
							findx_pattern(data, dataType, i + 119, length, &DVDLowAudioStreamSigs[0]) &&
							findx_pattern(data, dataType, i + 128, length, &DVDLowRequestAudioStatusSigs[0]) &&
							findx_pattern(data, dataType, i + 137, length, &DVDLowRequestAudioStatusSigs[0]) &&
							findx_pattern(data, dataType, i + 146, length, &DVDLowRequestAudioStatusSigs[0]) &&
							findx_pattern(data, dataType, i + 155, length, &DVDLowRequestAudioStatusSigs[0]) &&
							findx_pattern(data, dataType, i + 165, length, &DVDLowAudioBufferConfigSigs[0]) &&
							findx_pattern(data, dataType, i + 176, length, &DVDLowInquirySigs[0])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[0], &DVDLowWaitCoverCloseSigs[0]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[0], &DVDLowRequestErrorSigs[0]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[0], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  26, length, &DVDLowReadDiskIDSigs[1]) &&
							findx_pattern(data, dataType, i +  53, length, &DVDLowReadSigs[1]) &&
							findx_pattern(data, dataType, i +  62, length, &DVDLowSeekSigs[1]) &&
							findx_pattern(data, dataType, i +  66, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i +  70, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i +  85, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 106, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 119, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 128, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 137, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 146, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 155, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 165, length, &DVDLowAudioBufferConfigSigs[1]) &&
							findx_pattern(data, dataType, i + 176, length, &DVDLowInquirySigs[1])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  26, length, &DVDLowReadDiskIDSigs[1]) &&
							findx_pattern(data, dataType, i +  72, length, &DVDLowReadSigs[1]) &&
							findx_pattern(data, dataType, i +  81, length, &DVDLowSeekSigs[1]) &&
							findx_pattern(data, dataType, i +  85, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i +  89, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i + 104, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 125, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 138, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 147, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 156, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 165, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 174, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 184, length, &DVDLowAudioBufferConfigSigs[1]) &&
							findx_pattern(data, dataType, i + 195, length, &DVDLowInquirySigs[1])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  26, length, &DVDLowReadDiskIDSigs[1]) &&
							findx_pattern(data, dataType, i +  72, length, &DVDLowReadSigs[1]) &&
							findx_pattern(data, dataType, i +  81, length, &DVDLowSeekSigs[1]) &&
							findx_pattern(data, dataType, i +  85, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i +  89, length, &DVDLowStopMotorSigs[1]) &&
							findx_pattern(data, dataType, i + 104, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 125, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 138, length, &DVDLowAudioStreamSigs[1]) &&
							findx_pattern(data, dataType, i + 147, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 156, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 165, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 174, length, &DVDLowRequestAudioStatusSigs[1]) &&
							findx_pattern(data, dataType, i + 184, length, &DVDLowAudioBufferConfigSigs[1]) &&
							findx_pattern(data, dataType, i + 195, length, &DVDLowInquirySigs[1]) &&
							findx_pattern(data, dataType, i + 203, length, &DVDLowStopMotorSigs[1])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[2]) &&
							findx_pattern(data, dataType, i +  48, length, &DVDLowReadSigs[2]) &&
							findx_pattern(data, dataType, i +  57, length, &DVDLowSeekSigs[2]) &&
							findx_pattern(data, dataType, i +  61, length, &DVDLowStopMotorSigs[2]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowStopMotorSigs[2]) &&
							findx_pattern(data, dataType, i +  80, length, &DVDLowRequestAudioStatusSigs[2]) &&
							findx_pattern(data, dataType, i +  90, length, &DVDLowAudioStreamSigs[2]) &&
							findx_pattern(data, dataType, i + 101, length, &DVDLowAudioStreamSigs[2]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[2]) &&
							findx_pattern(data, dataType, i + 123, length, &DVDLowRequestAudioStatusSigs[2]) &&
							findx_pattern(data, dataType, i + 132, length, &DVDLowRequestAudioStatusSigs[2]) &&
							findx_pattern(data, dataType, i + 141, length, &DVDLowRequestAudioStatusSigs[2]) &&
							findx_pattern(data, dataType, i + 150, length, &DVDLowRequestAudioStatusSigs[2]) &&
							findx_pattern(data, dataType, i + 160, length, &DVDLowAudioBufferConfigSigs[2]) &&
							findx_pattern(data, dataType, i + 171, length, &DVDLowInquirySigs[2])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[2], &DVDLowWaitCoverCloseSigs[2]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[2], &DVDLowRequestErrorSigs[2]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[2], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[3]) &&
							findx_pattern(data, dataType, i +  48, length, &DVDLowReadSigs[3]) &&
							findx_pattern(data, dataType, i +  57, length, &DVDLowSeekSigs[3]) &&
							findx_pattern(data, dataType, i +  61, length, &DVDLowStopMotorSigs[3]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowStopMotorSigs[3]) &&
							findx_pattern(data, dataType, i +  80, length, &DVDLowRequestAudioStatusSigs[3]) &&
							findx_pattern(data, dataType, i +  90, length, &DVDLowAudioStreamSigs[3]) &&
							findx_pattern(data, dataType, i + 101, length, &DVDLowAudioStreamSigs[3]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[3]) &&
							findx_pattern(data, dataType, i + 123, length, &DVDLowRequestAudioStatusSigs[3]) &&
							findx_pattern(data, dataType, i + 132, length, &DVDLowRequestAudioStatusSigs[3]) &&
							findx_pattern(data, dataType, i + 141, length, &DVDLowRequestAudioStatusSigs[3]) &&
							findx_pattern(data, dataType, i + 150, length, &DVDLowRequestAudioStatusSigs[3]) &&
							findx_pattern(data, dataType, i + 160, length, &DVDLowAudioBufferConfigSigs[3]) &&
							findx_pattern(data, dataType, i + 171, length, &DVDLowInquirySigs[3])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[3], &DVDLowWaitCoverCloseSigs[2]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[3], &DVDLowRequestErrorSigs[3]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[3], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[4]) &&
							findx_pattern(data, dataType, i +  48, length, &DVDLowReadSigs[4]) &&
							findx_pattern(data, dataType, i +  57, length, &DVDLowSeekSigs[4]) &&
							findx_pattern(data, dataType, i +  61, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  80, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i +  90, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 101, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 123, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 132, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 141, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 150, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 160, length, &DVDLowAudioBufferConfigSigs[4]) &&
							findx_pattern(data, dataType, i + 171, length, &DVDLowInquirySigs[4])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[4]) &&
							findx_pattern(data, dataType, i +  48, length, &DVDLowReadSigs[4]) &&
							findx_pattern(data, dataType, i +  57, length, &DVDLowSeekSigs[4]) &&
							findx_pattern(data, dataType, i +  61, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  80, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i +  90, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 101, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 114, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 123, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 132, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 141, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 150, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 160, length, &DVDLowAudioBufferConfigSigs[4]) &&
							findx_pattern(data, dataType, i + 171, length, &DVDLowInquirySigs[4])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[4]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowReadSigs[4]) &&
							findx_pattern(data, dataType, i +  74, length, &DVDLowSeekSigs[4]) &&
							findx_pattern(data, dataType, i +  78, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  82, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  97, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 107, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 118, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 131, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 140, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 149, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 158, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 167, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 177, length, &DVDLowAudioBufferConfigSigs[4]) &&
							findx_pattern(data, dataType, i + 188, length, &DVDLowInquirySigs[4])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  24, length, &DVDLowReadDiskIDSigs[5]) &&
							findx_pattern(data, dataType, i +  62, length, &DVDLowReadSigs[5]) &&
							findx_pattern(data, dataType, i +  70, length, &DVDLowSeekSigs[5]) &&
							findx_pattern(data, dataType, i +  74, length, &DVDLowStopMotorSigs[5]) &&
							findx_pattern(data, dataType, i +  78, length, &DVDLowStopMotorSigs[5]) &&
							findx_pattern(data, dataType, i +  92, length, &DVDLowRequestAudioStatusSigs[5]) &&
							findx_pattern(data, dataType, i + 102, length, &DVDLowAudioStreamSigs[5]) &&
							findx_pattern(data, dataType, i + 112, length, &DVDLowAudioStreamSigs[5]) &&
							findx_pattern(data, dataType, i + 124, length, &DVDLowAudioStreamSigs[5]) &&
							findx_pattern(data, dataType, i + 132, length, &DVDLowRequestAudioStatusSigs[5]) &&
							findx_pattern(data, dataType, i + 140, length, &DVDLowRequestAudioStatusSigs[5]) &&
							findx_pattern(data, dataType, i + 148, length, &DVDLowRequestAudioStatusSigs[5]) &&
							findx_pattern(data, dataType, i + 156, length, &DVDLowRequestAudioStatusSigs[5]) &&
							findx_pattern(data, dataType, i + 165, length, &DVDLowAudioBufferConfigSigs[5]) &&
							findx_pattern(data, dataType, i + 175, length, &DVDLowInquirySigs[5])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[5], &DVDLowWaitCoverCloseSigs[4]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[5], &DVDLowRequestErrorSigs[5]);
							find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[5], &DVDLowResetSigs[2]);
						}
						break;
					case 10:
						if (findx_pattern(data, dataType, i +  25, length, &DVDLowReadDiskIDSigs[4]) &&
							findx_pattern(data, dataType, i +  65, length, &DVDLowReadSigs[4]) &&
							findx_pattern(data, dataType, i +  74, length, &DVDLowSeekSigs[4]) &&
							findx_pattern(data, dataType, i +  78, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  82, length, &DVDLowStopMotorSigs[4]) &&
							findx_pattern(data, dataType, i +  97, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 107, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 118, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 131, length, &DVDLowAudioStreamSigs[4]) &&
							findx_pattern(data, dataType, i + 140, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 149, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 158, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 167, length, &DVDLowRequestAudioStatusSigs[4]) &&
							findx_pattern(data, dataType, i + 177, length, &DVDLowAudioBufferConfigSigs[4]) &&
							findx_pattern(data, dataType, i + 188, length, &DVDLowInquirySigs[4]) &&
							findx_pattern(data, dataType, i + 196, length, &DVDLowStopMotorSigs[4])) {
							stateBusySigs[j].offsetFoundAt = i;
							
							find_pattern_after(data, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
				}
			}
			else if (stateBusySigs[j].offsetFoundAt) {
				for (k = 0; k < sizeof(cbForStateBusySigs) / sizeof(FuncPattern); k++) {
					if (compare_pattern(&fp, &cbForStateBusySigs[k])) {
						switch (k) {
							case 0:
								if (findx_pattern(data, dataType, i + 143, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 1:
								if (findx_pattern(data, dataType, i + 127, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 2:
								if (findx_pattern(data, dataType, i + 121, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 3:
								if (findx_pattern(data, dataType, i + 141, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 4:
								if (findx_pattern(data, dataType, i + 150, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 5:
								if (findx_pattern(data, dataType, i + 161, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 6:
								if (findx_pattern(data, dataType, i + 175, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 7:
								if (findx_pattern(data, dataType, i + 178, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 8:
								if (findx_pattern(data, dataType, i + 190, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 9:
								if (findx_pattern(data, dataType, i + 182, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 10:
								if (findx_pattern(data, dataType, i + 184, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
							case 11:
								if (findx_pattern(data, dataType, i + 201, length, &stateBusySigs[j]))
									cbForStateBusySigs[k].offsetFoundAt = i;
								break;
						}
					}
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDResetSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDResetSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 3, length, &DVDLowResetSigs[0]))
							DVDResetSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 3, length, &DVDLowResetSigs[1]))
							DVDResetSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 3, length, &DVDLowResetSigs[2]))
							DVDResetSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDCancelAsyncSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDCancelAsyncSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  30, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &DVDLowBreakSigs[0]) &&
							findx_pattern(data, dataType, i +  45, length, &__DVDDequeueWaitingQueueSigs[0]) &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[0]) &&
							findx_pattern(data, dataType, i + 112, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 159, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[0], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
						}
						break;
					case 1:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  30, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &DVDLowBreakSigs[1]) &&
							findx_pattern(data, dataType, i +  45, length, &__DVDDequeueWaitingQueueSigs[0]) &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[0]) &&
							findx_pattern(data, dataType, i + 112, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 159, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[1], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
						}
						break;
					case 2:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  30, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &DVDLowBreakSigs[1]) &&
							findx_pattern(data, dataType, i +  45, length, &__DVDDequeueWaitingQueueSigs[0]) &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[1]) &&
							findx_pattern(data, dataType, i + 112, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 164, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[1], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
						}
						break;
					case 3:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  35, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  47, length, &DVDLowBreakSigs[2]) &&
							findx_pattern(data, dataType, i +  50, length, &__DVDDequeueWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i +  89, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 112, length, &DVDLowClearCallbackSigs[2]) &&
							findx_pattern(data, dataType, i + 162, length, &OSRestoreInterruptsSig))
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  45, length, &DVDLowBreakSigs[2]) &&
							findx_pattern(data, dataType, i +  48, length, &__DVDDequeueWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i +  90, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  97, length, &DVDLowClearCallbackSigs[2]) &&
							findx_pattern(data, dataType, i + 147, length, &OSRestoreInterruptsSig))
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  32, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  43, length, &DVDLowBreakSigs[3]) &&
							findx_pattern(data, dataType, i +  46, length, &__DVDDequeueWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[2]) &&
							findx_pattern(data, dataType, i + 101, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 147, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[3], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
						}
						break;
					case 6:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  32, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  43, length, &DVDLowBreakSigs[4]) &&
							findx_pattern(data, dataType, i +  46, length, &__DVDDequeueWaitingQueueSigs[1]) &&
							findx_pattern(data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[2]) &&
							findx_pattern(data, dataType, i + 101, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 147, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[4], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
						}
						break;
					case 7:
						if (findx_pattern (data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  32, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  43, length, &DVDLowBreakSigs[4]) &&
							findx_pattern (data, dataType, i +  46, length, &__DVDDequeueWaitingQueueSigs[1]) &&
							findx_pattern (data, dataType, i +  88, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i +  95, length, &DVDLowClearCallbackSigs[2],
							                                                &DVDLowClearCallbackSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 101, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 150, length, &OSRestoreInterruptsSig)) {
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
							
							if (find_pattern_before(data, length, &DVDLowBreakSigs[4], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
						}
						break;
					case 8:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  31, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  42, length, &DVDLowBreakSigs[5]) &&
							findx_pattern(data, dataType, i +  45, length, &__DVDDequeueWaitingQueueSigs[2]) &&
							findx_pattern(data, dataType, i +  87, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  94, length, &DVDLowClearCallbackSigs[3]) &&
							findx_pattern(data, dataType, i + 100, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 149, length, &OSRestoreInterruptsSig))
							DVDCancelAsyncSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DVDCheckDiskSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DVDCheckDiskSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &DVDGetCurrentDiskIDSigs[0]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 59, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &DVDGetCurrentDiskIDSigs[0]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 50, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 55, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__DVDTestAlarmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__DVDTestAlarmSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 12, length, &__DVDLowTestAlarmSig))
							__DVDTestAlarmSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  9, length, &__DVDLowTestAlarmSig))
							__DVDTestAlarmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(AIInitDMASigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &AIInitDMASigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  6, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i +  8, i +  9, &address) && address == 0xCC005030 &&
							get_immediate(data,   i + 13, i + 14, &address) && address == 0xCC005030 &&
							get_immediate(data,   i + 15, i + 16, &address) && address == 0xCC005032 &&
							get_immediate(data,   i + 20, i + 21, &address) && address == 0xCC005032 &&
							get_immediate(data,   i + 30, i + 31, &address) && address == 0xCC005036 &&
							get_immediate(data,   i + 35, i + 36, &address) && address == 0xCC005036 &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig))
							AIInitDMASigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i +  8, i +  9, &address) && address == 0xCC005030 &&
							get_immediate(data,   i +  8, i + 10, &address) && address == 0xCC005000 &&
							get_immediate(data,   i +  8, i + 11, &address) && address == 0xCC005000 &&
							get_immediate(data,   i +  8, i + 12, &address) && address == 0xCC005000 &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig))
							AIInitDMASigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__VMBASESetupExceptionHandlersSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__VMBASESetupExceptionHandlersSigs[j])) {
				switch (j) {
					case 0:
						if (findi_pattern(data, dataType, i +  7, i +  8, length, &__VMBASEDSIExceptionHandlerSig) &&
							findx_pattern(data, dataType, i + 17, length, &DCFlushRangeNoSyncSigs[0]) &&
							findx_pattern(data, dataType, i + 29, length, &DCFlushRangeNoSyncSigs[0]) &&
							findx_pattern(data, dataType, i + 44, length, &DCFlushRangeNoSyncSigs[0]) &&
							findi_pattern(data, dataType, i + 49, i + 50, length, &__VMBASEISIExceptionHandlerSig) &&
							findx_pattern(data, dataType, i + 57, length, &DCFlushRangeNoSyncSigs[0]) &&
							findx_pattern(data, dataType, i + 68, length, &DCFlushRangeNoSyncSigs[0]) &&
							findx_pattern(data, dataType, i + 83, length, &DCFlushRangeNoSyncSigs[0]))
							__VMBASESetupExceptionHandlersSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findi_pattern(data, dataType, i +  2, i +  5, length, &__VMBASEDSIExceptionHandlerSig) &&
							findx_pattern(data, dataType, i + 15, length, &DCFlushRangeNoSyncSigs[1]) &&
							findx_pattern(data, dataType, i + 27, length, &DCFlushRangeNoSyncSigs[1]) &&
							findx_pattern(data, dataType, i + 42, length, &DCFlushRangeNoSyncSigs[1]) &&
							findi_pattern(data, dataType, i + 47, i + 49, length, &__VMBASEISIExceptionHandlerSig) &&
							findx_pattern(data, dataType, i + 57, length, &DCFlushRangeNoSyncSigs[1]) &&
							findx_pattern(data, dataType, i + 69, length, &DCFlushRangeNoSyncSigs[1]) &&
							findx_pattern(data, dataType, i + 84, length, &DCFlushRangeNoSyncSigs[1]))
							__VMBASESetupExceptionHandlersSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		i += fp.Length - 1;
	}
	
	if ((i = SystemCallVectorSig.offsetFoundAt))
		for (j = 0; j < SystemCallVectorSig.Length; j++)
			data[i + j] = 0;
	
	for (j = 0; j < sizeof(PrepareExecSigs) / sizeof(FuncPattern); j++)
	if ((i = PrepareExecSigs[j].offsetFoundAt)) {
		u32 *PrepareExec = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PrepareExec) {
			switch (j) {
				case 0:
					data[i + 42] = branchAndLink(MASK_IRQ,   PrepareExec + 42);
					data[i + 44] = branchAndLink(UNMASK_IRQ, PrepareExec + 44);
					break;
				case 1:
					data[i + 46] = branchAndLink(MASK_IRQ,   PrepareExec + 46);
					data[i + 48] = branchAndLink(UNMASK_IRQ, PrepareExec + 48);
					break;
			}
			print_gecko("Found:[%s] @ %08X\n", PrepareExecSigs[j].Name, PrepareExec);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(OSExceptionInitSigs) / sizeof(FuncPattern); j++)
	if ((i = OSExceptionInitSigs[j].offsetFoundAt)) {
		u32 *OSExceptionInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSExceptionInit) {
			switch (j) {
				case 0:
					data[i + 140] = 0x28000009;	// cmplwi	r0, 9
					data[i + 153] = 0x28000009;	// cmplwi	r0, 9
					break;
				case 1:
				case 2:
					data[i + 133] = 0x28000009;	// cmplwi	r0, 9
					data[i + 149] = 0x28000009;	// cmplwi	r0, 9
					break;
				case 3:
					data[i + 125] = 0x28000009;	// cmplwi	r0, 9
					data[i + 139] = 0x28000009;	// cmplwi	r0, 9
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", OSExceptionInitSigs[j].Name, j, OSExceptionInit);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(OSSetAlarmSigs) / sizeof(FuncPattern); j++)
	if ((i = OSSetAlarmSigs[j].offsetFoundAt)) {
		u32 *OSSetAlarm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSSetAlarm) {
			if (j == 0) {
				data[i + 10] = 0x38A0FFFF;	// li		r5, -1
				data[i + 11] = 0x3800FFFF;	// li		r0, -1
			}
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 0] = (u32)OSSetAlarm;
			
			print_gecko("Found:[%s$%i] @ %08X\n", OSSetAlarmSigs[j].Name, j, OSSetAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(OSCancelAlarmSigs) / sizeof(FuncPattern); j++)
	if ((i = OSCancelAlarmSigs[j].offsetFoundAt)) {
		u32 *OSCancelAlarm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSCancelAlarm) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 1] = (u32)OSCancelAlarm;
			
			print_gecko("Found:[%s$%i] @ %08X\n", OSCancelAlarmSigs[j].Name, j, OSCancelAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSUnhandledExceptionSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSUnhandledExceptionSigs[j].offsetFoundAt)) {
		u32 *__OSUnhandledException = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSUnhandledException) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 2] = (u32)__OSUnhandledException;
			
			print_gecko("Found:[%s$%i] @ %08X\n", __OSUnhandledExceptionSigs[j].Name, j, __OSUnhandledException);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSBootDolSimpleSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSBootDolSimpleSigs[j].offsetFoundAt)) {
		u32 *__OSBootDolSimple = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSBootDolSimple) {
			switch (j) {
				case 0:
					data[i + 43] = branchAndLink(MASK_IRQ,   __OSBootDolSimple + 43);
					data[i + 45] = branchAndLink(UNMASK_IRQ, __OSBootDolSimple + 45);
					break;
				case 1:
				case 2:
					data[i + 42] = branchAndLink(MASK_IRQ,   __OSBootDolSimple + 42);
					data[i + 44] = branchAndLink(UNMASK_IRQ, __OSBootDolSimple + 44);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __OSBootDolSimpleSigs[j].Name, j, __OSBootDolSimple);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SetInterruptMaskSigs) / sizeof(FuncPattern); j++)
	if ((i = SetInterruptMaskSigs[j].offsetFoundAt)) {
		u32 *SetInterruptMask = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 **jumpTable;
		
		if (SetInterruptMask) {
			switch (j) {
				case 0:
				case 1:
					jumpTable = loadResolve(data, dataType, i + 5, i + 6);
					jumpTable[4] = SetInterruptMask + 11;
					break;
				case 3:
				case 4:
					jumpTable = loadResolve(data, dataType, i + 3, i + 4);
					jumpTable[4] = SetInterruptMask +  9;
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", SetInterruptMaskSigs[j].Name, j, SetInterruptMask);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSMaskInterruptsSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSMaskInterruptsSigs[j].offsetFoundAt)) {
		u32 *__OSMaskInterrupts = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSMaskInterrupts) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 3] = (u32)__OSMaskInterrupts;
			
			print_gecko("Found:[%s$%i] @ %08X\n", __OSMaskInterruptsSigs[j].Name, j, __OSMaskInterrupts);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSUnmaskInterruptsSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSUnmaskInterruptsSigs[j].offsetFoundAt)) {
		u32 *__OSUnmaskInterrupts = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSUnmaskInterrupts) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 4] = (u32)__OSUnmaskInterrupts;
			
			print_gecko("Found:[%s$%i] @ %08X\n", __OSUnmaskInterruptsSigs[j].Name, j, __OSUnmaskInterrupts);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSDispatchInterruptSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSDispatchInterruptSigs[j].offsetFoundAt)) {
		u32 *__OSDispatchInterrupt = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSDispatchInterrupt) {
			switch (j) {
				case 0:
					data[i + 161] = 0x4800008C;	// b		+35
					break;
				case 1:
				case 2:
					data[i + 165] = 0x4800008C;	// b		+35
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __OSDispatchInterruptSigs[j].Name, j, __OSDispatchInterrupt);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSRebootSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSRebootSigs[j].offsetFoundAt)) {
		u32 *__OSReboot = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSReboot) {
			switch (j) {
				case 0:
					data[i + 41] = branchAndLink(MASK_IRQ,   __OSReboot + 41);
					data[i + 43] = branchAndLink(UNMASK_IRQ, __OSReboot + 43);
					break;
				case 1:
					data[i + 49] = branchAndLink(MASK_IRQ,   __OSReboot + 49);
					data[i + 51] = branchAndLink(UNMASK_IRQ, __OSReboot + 51);
					break;
				case 2:
				case 3:
					data[i + 48] = branchAndLink(MASK_IRQ,   __OSReboot + 48);
					data[i + 50] = branchAndLink(UNMASK_IRQ, __OSReboot + 50);
					break;
				case 4:
					data[i + 45] = branchAndLink(MASK_IRQ,   __OSReboot + 45);
					data[i + 47] = branchAndLink(UNMASK_IRQ, __OSReboot + 47);
					break;
				case 6:
					data[i + 31] = branchAndLink(MASK_IRQ,   __OSReboot + 31);
					data[i + 33] = branchAndLink(UNMASK_IRQ, __OSReboot + 33);
					break;
				case 7:
					data[i + 38] = branchAndLink(MASK_IRQ,   __OSReboot + 38);
					data[i + 40] = branchAndLink(UNMASK_IRQ, __OSReboot + 40);
					break;
				case 8:
				case 9:
					data[i + 36] = branchAndLink(MASK_IRQ,   __OSReboot + 36);
					data[i + 38] = branchAndLink(UNMASK_IRQ, __OSReboot + 38);
					break;
				case 10:
				case 11:
					data[i + 32] = branchAndLink(MASK_IRQ,   __OSReboot + 32);
					data[i + 34] = branchAndLink(UNMASK_IRQ, __OSReboot + 34);
					break;
				case 12:
					data[i + 31] = branchAndLink(MASK_IRQ,   __OSReboot + 31);
					data[i + 33] = branchAndLink(UNMASK_IRQ, __OSReboot + 33);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __OSRebootSigs[j].Name, j, __OSReboot);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSDoHotResetSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSDoHotResetSigs[j].offsetFoundAt)) {
		u32 *__OSDoHotReset = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSDoHotReset) {
			switch (j) {
				case 0: data[i + 4] = branchAndLink(FINI, __OSDoHotReset + 4); break;
				case 1:
				case 2: data[i + 5] = branchAndLink(FINI, __OSDoHotReset + 5); break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __OSDoHotResetSigs[j].Name, j, __OSDoHotReset);
			patched++;
		}
		
		if ((i = PPCHaltSig.offsetFoundAt)) {
			u32 *PPCHalt = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			
			if (PPCHalt) {
				if (__OSDoHotReset && swissSettings.igrType != IGR_OFF)
					data[i + 4] = branch(__OSDoHotReset, PPCHalt + 4);
				
				print_gecko("Found:[%s] @ %08X\n", PPCHaltSig.Name, PPCHalt);
				patched++;
			}
		}
	}
	
	for (j = 0; j < sizeof(OSResetSystemSigs) / sizeof(FuncPattern); j++)
	if ((i = OSResetSystemSigs[j].offsetFoundAt)) {
		u32 *OSResetSystem = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSResetSystem) {
			switch (j) {
				case  7: data[i + 72] = branchAndLink(FINI, OSResetSystem + 72); break;
				case  8:
				case  9: data[i + 73] = branchAndLink(FINI, OSResetSystem + 73); break;
				case 10: data[i + 72] = branchAndLink(FINI, OSResetSystem + 72); break;
				case 11:
				case 12: data[i + 77] = branchAndLink(FINI, OSResetSystem + 77); break;
				case 13: data[i + 66] = branchAndLink(FINI, OSResetSystem + 66); break;
				case 14: data[i + 81] = branchAndLink(FINI, OSResetSystem + 81); break;
				case 15: data[i + 86] = branchAndLink(FINI, OSResetSystem + 86); break;
				case 16: data[i + 87] = branchAndLink(FINI, OSResetSystem + 87); break;
				case 17: data[i + 74] = branchAndLink(FINI, OSResetSystem + 74); break;
			}
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 5] = (u32)OSResetSystem;
			
			print_gecko("Found:[%s$%i] @ %08X\n", OSResetSystemSigs[j].Name, j, OSResetSystem);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSInitSystemCallSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSInitSystemCallSigs[j].offsetFoundAt)) {
		u32 *__OSInitSystemCall = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 __OSArenaLo, __OSArenaHi;
		
		if (__OSInitSystemCall) {
			switch (j) {
				case 0:
					data[i +  4] = 0x38600000 | ((u32)VAR_JUMP_TABLE & 0xFFFF);
					data[i + 12] = 0x3CA00000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i + 13] = 0x38050000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i + 17] = 0x38800020;	// li		r4, 32
					
					if (get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 20] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 20] = 0x38600000;
					
					if (get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 21] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 21] = 0x38800000;
					
					data[i + 22] = branchAndLink(INIT, __OSInitSystemCall + 22);
					break;
				case 1:
					data[i +  4] = 0x3CA00000 | ((u32)VAR_JUMP_TABLE + 0x8000) >> 16;
					data[i +  6] = 0x3C600000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i +  7] = 0x3BE50000 | ((u32)VAR_JUMP_TABLE & 0xFFFF);
					data[i +  8] = 0x38030000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i + 14] = 0x38800020;	// li		r4, 32
					
					if (get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 17] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 17] = 0x38600000;
					
					if (get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 18] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 18] = 0x38800000;
					
					data[i + 19] = branchAndLink(INIT, __OSInitSystemCall + 19);
					break;
				case 2:
					data[i +  3] = 0x3C600000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i +  6] = 0x3CA00000 | ((u32)VAR_JUMP_TABLE + 0x8000) >> 16;
					data[i +  7] = 0x38030000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i +  8] = 0x38650000 | ((u32)VAR_JUMP_TABLE & 0xFFFF);
					data[i + 11] = 0x3C600000 | ((u32)VAR_JUMP_TABLE + 0x8000) >> 16;
					data[i + 12] = 0x38800020;	// li		r4, 32
					data[i + 13] = 0x38630000 | ((u32)VAR_JUMP_TABLE & 0xFFFF);
					data[i + 16] = 0x60000000;	// nop
					
					if (get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 17] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 17] = 0x38600000;
					
					if (get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 18] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 18] = 0x38800000;
					
					data[i + 19] = branchAndLink(INIT, __OSInitSystemCall + 19);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __OSInitSystemCallSigs[j].Name, j, __OSInitSystemCall);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SelectThreadSigs) / sizeof(FuncPattern); j++)
	if ((i = SelectThreadSigs[j].offsetFoundAt)) {
		u32 *SelectThread = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SelectThread) {
			switch (j) {
				case 0:
					data[i + 58] = branchAndLink(IDLE_THREAD, SelectThread + 58);
					data[i + 61] = 0x4182FFF4;	// beq		-3
					break;
				case 1:
					data[i + 57] = branchAndLink(IDLE_THREAD, SelectThread + 57);
					data[i + 60] = 0x4182FFF4;	// beq		-3
					break;
				case 2:
					data[i + 77] = branchAndLink(IDLE_THREAD, SelectThread + 77);
					data[i + 80] = 0x4182FFF4;	// beq		-3
					break;
				case 3:
					data[i + 82] = branchAndLink(IDLE_THREAD, SelectThread + 82);
					data[i + 85] = 0x4182FFF4;	// beq		-3
					break;
				case 4:
					data[i + 83] = branchAndLink(IDLE_THREAD, SelectThread + 83);
					data[i + 86] = 0x4182FFF4;	// beq		-3
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", SelectThreadSigs[j].Name, j, SelectThread);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SetExiInterruptMaskSigs) / sizeof(FuncPattern); j++)
	if ((i = SetExiInterruptMaskSigs[j].offsetFoundAt)) {
		u32 *SetExiInterruptMask = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SetExiInterruptMask) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i + 29] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 29);
						data[i + 32] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 32);
						data[i + 41] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 41);
						data[i + 44] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 44);
						break;
					case 3:
					case 4:
						data[i + 27] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 27);
						data[i + 30] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 30);
						data[i + 39] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 39);
						data[i + 42] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 42);
						break;
					case 5:
						data[i + 29] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 29);
						data[i + 32] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 32);
						data[i + 41] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 41);
						data[i + 44] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 44);
						break;
					case 6:
						data[i + 27] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 27);
						data[i + 30] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 30);
						data[i + 39] = branchAndLink(MASK_IRQ,   SetExiInterruptMask + 39);
						data[i + 42] = branchAndLink(UNMASK_IRQ, SetExiInterruptMask + 42);
						break;
				}
			}
			if (devices[DEVICE_CUR] == &__device_fsp) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i + 28] = 0x3C600040;	// lis		r3, 0x0040
						data[i + 31] = 0x3C600040;	// lis		r3, 0x0040
						break;
					case 3:
					case 4:
						data[i + 26] = 0x3C600040;	// lis		r3, 0x0040
						data[i + 29] = 0x3C600040;	// lis		r3, 0x0040
						break;
					case 5:
						data[i + 28] = 0x3C600040;	// lis		r3, 0x0040
						data[i + 31] = 0x3C600040;	// lis		r3, 0x0040
						break;
					case 6:
						data[i + 26] = 0x3C600040;	// lis		r3, 0x0040
						data[i + 29] = 0x3C600040;	// lis		r3, 0x0040
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", SetExiInterruptMaskSigs[j].Name, j, SetExiInterruptMask);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(CompleteTransferSigs) / sizeof(FuncPattern); j++)
	if ((i = CompleteTransferSigs[j].offsetFoundAt)) {
		u32 *CompleteTransfer = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (CompleteTransfer) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i + 33] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", CompleteTransferSigs[j].Name, j, CompleteTransfer);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIImmSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIImmSigs[j].offsetFoundAt)) {
		u32 *EXIImm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIImm) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  73] = branchAndLink(UNMASK_IRQ, EXIImm + 73);
						data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 111] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i +  37] = branchAndLink(UNMASK_IRQ, EXIImm + 37);
						data[i + 119] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 133] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +  38] = branchAndLink(UNMASK_IRQ, EXIImm + 38);
						data[i +  58] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i +  37] = branchAndLink(UNMASK_IRQ, EXIImm + 37);
						data[i + 118] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 135] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIImmSigs[j].Name, j, EXIImm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIDmaSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIDmaSigs[j].offsetFoundAt)) {
		u32 *EXIDma = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIDma) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  80] = branchAndLink(UNMASK_IRQ, EXIDma + 80);
						data[i +  88] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  94] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 102] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i +  37] = branchAndLink(UNMASK_IRQ, EXIDma + 37);
						data[i +  39] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +  38] = branchAndLink(UNMASK_IRQ, EXIDma + 38);
						data[i +  47] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  54] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  63] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i +  37] = branchAndLink(UNMASK_IRQ, EXIDma + 37);
						data[i +  42] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIDmaSigs[j].Name, j, EXIDma);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXISyncSigs) / sizeof(FuncPattern); j++)
	if ((i = EXISyncSigs[j].offsetFoundAt)) {
		u32 *EXISync = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISync) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i +  24] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  44] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  52] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i +  25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  45] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  53] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
						data[i +  25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  45] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  53] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  62] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  71] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 4:
						data[i +  25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  50] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  58] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  67] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +   5] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  32] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 6:
						data[i +   5] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  31] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i +   3] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  31] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 8:
						data[i +  14] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  41] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  65] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  73] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 9:
						data[i +   3] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  31] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 113] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 117] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 121] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						break;
					case 10:
						data[i +  10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  32] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 113] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 117] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 121] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						break;
					case 11:
						data[i +  10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  32] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 115] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 118] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 122] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						data[i + 126] = 0x80640010;	// lwz		r3, 0x0010 (r4)
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXISyncSigs[j].Name, j, EXISync);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIClearInterruptsSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIClearInterruptsSigs[j].offsetFoundAt)) {
		u32 *EXIClearInterrupts = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIClearInterrupts) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
						data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 39] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 1:
						data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 40] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i +  1] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
						data[i +  4] = 0x3CE00C00;	// lis		r7, 0x0C00
						data[i + 20] = 0x3CE00C00;	// lis		r7, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIClearInterruptsSigs[j].Name, j, EXIClearInterrupts);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__EXIProbeSigs) / sizeof(FuncPattern); j++)
	if ((i = __EXIProbeSigs[j].offsetFoundAt)) {
		u32 *__EXIProbe = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__EXIProbe) {
			switch (j) {
				case 0:
				case 1:
					data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
				case 4:
					data[i + 17] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 36] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __EXIProbeSigs[j].Name, j, __EXIProbe);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__EXIAttachSigs) / sizeof(FuncPattern); j++)
	if ((i = __EXIAttachSigs[j].offsetFoundAt)) {
		u32 *__EXIAttach = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__EXIAttach) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
						data[i + 43] = branchAndLink(UNMASK_IRQ, __EXIAttach + 43);
						break;
					case 1:
						data[i + 44] = branchAndLink(UNMASK_IRQ, __EXIAttach + 44);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __EXIAttachSigs[j].Name, j, __EXIAttach);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIAttachSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIAttachSigs[j].offsetFoundAt)) {
		u32 *EXIAttach = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIAttach) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
						data[i + 47] = branchAndLink(UNMASK_IRQ, EXIAttach + 47);
						break;
					case 3:
						data[i + 42] = branchAndLink(UNMASK_IRQ, EXIAttach + 42);
						break;
					case 4:
						data[i + 52] = branchAndLink(UNMASK_IRQ, EXIAttach + 52);
						break;
					case 5:
						data[i + 58] = branchAndLink(UNMASK_IRQ, EXIAttach + 58);
						break;
					case 6:
						data[i + 52] = branchAndLink(UNMASK_IRQ, EXIAttach + 52);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIAttachSigs[j].Name, j, EXIAttach);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIDetachSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIDetachSigs[j].offsetFoundAt)) {
		u32 *EXIDetach = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIDetach) {
			switch (j) {
				case 0:
				case 1:
				case 2:
					data[i + 11] = 0x2C1D0003;	// cmpwi	r29, 3
					break;
			}
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i + 44] = branchAndLink(MASK_IRQ, EXIDetach + 44);
						break;
					case 2:
						data[i + 45] = branchAndLink(MASK_IRQ, EXIDetach + 45);
						break;
					case 3:
					case 4:
						data[i + 36] = branchAndLink(MASK_IRQ, EXIDetach + 36);
						break;
					case 5:
						data[i + 34] = branchAndLink(MASK_IRQ, EXIDetach + 34);
						break;
					case 6:
						data[i + 36] = branchAndLink(MASK_IRQ, EXIDetach + 36);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIDetachSigs[j].Name, j, EXIDetach);
			patched++;
		}
	}
	
	if ((i = EXISelectSDSig.offsetFoundAt)) {
		u32 *EXISelectSD = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISelectSD) {
			data[i + 55] = 0x3C600C00;	// lis		r3, 0x0C00
			
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				data[i + 73] = branchAndLink(MASK_IRQ, EXISelectSD + 73);
				data[i + 76] = branchAndLink(MASK_IRQ, EXISelectSD + 76);
			}
			print_gecko("Found:[%s] @ %08X\n", EXISelectSDSig.Name, EXISelectSD);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXISelectSigs) / sizeof(FuncPattern); j++)
	if ((i = EXISelectSigs[j].offsetFoundAt)) {
		u32 *EXISelect = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISelect) {
			switch (j) {
				case 0:
				case 1:
				case 2:
					data[i +  79] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  91] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
				case 4:
					data[i + 38] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 42] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 55] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i + 41] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i + 104] = branchAndLink(MASK_IRQ, EXISelect + 104);
						data[i + 107] = branchAndLink(MASK_IRQ, EXISelect + 107);
						break;
					case 3:
					case 4:
						data[i + 63] = branchAndLink(MASK_IRQ, EXISelect + 63);
						data[i + 66] = branchAndLink(MASK_IRQ, EXISelect + 66);
						break;
					case 5:
						data[i + 68] = branchAndLink(MASK_IRQ, EXISelect + 68);
						data[i + 71] = branchAndLink(MASK_IRQ, EXISelect + 71);
						break;
					case 6:
						data[i + 63] = branchAndLink(MASK_IRQ, EXISelect + 63);
						data[i + 66] = branchAndLink(MASK_IRQ, EXISelect + 66);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXISelectSigs[j].Name, j, EXISelect);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIDeselectSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIDeselectSigs[j].offsetFoundAt)) {
		u32 *EXIDeselect = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIDeselect) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i + 33] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 39] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 52] = branchAndLink(UNMASK_IRQ, EXIDeselect + 52);
						data[i + 55] = branchAndLink(UNMASK_IRQ, EXIDeselect + 55);
						break;
					case 2:
						data[i + 34] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 40] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 53] = branchAndLink(UNMASK_IRQ, EXIDeselect + 53);
						data[i + 56] = branchAndLink(UNMASK_IRQ, EXIDeselect + 56);
						break;
					case 3:
					case 4:
					case 5:
						data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 41] = branchAndLink(UNMASK_IRQ, EXIDeselect + 41);
						data[i + 44] = branchAndLink(UNMASK_IRQ, EXIDeselect + 44);
						break;
					case 6:
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 42] = branchAndLink(UNMASK_IRQ, EXIDeselect + 42);
						data[i + 45] = branchAndLink(UNMASK_IRQ, EXIDeselect + 45);
						break;
					case 7:
						data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 41] = branchAndLink(UNMASK_IRQ, EXIDeselect + 41);
						data[i + 44] = branchAndLink(UNMASK_IRQ, EXIDeselect + 44);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIDeselectSigs[j].Name, j, EXIDeselect);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *EXIIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 3:
						data[i + 10] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 4:
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIIntrruptHandlerSigs[j].Name, j, EXIIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(TCIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = TCIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *TCIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (TCIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i + 27] = branchAndLink(MASK_IRQ, TCIntrruptHandler + 27);
						break;
					case 2:
						data[i + 28] = branchAndLink(MASK_IRQ, TCIntrruptHandler + 28);
						break;
					case 3:
					case 4:
						data[i + 21] = branchAndLink(MASK_IRQ, TCIntrruptHandler + 21);
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i + 17] = branchAndLink(MASK_IRQ, TCIntrruptHandler + 17);
						data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 53] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 21] = branchAndLink(MASK_IRQ, TCIntrruptHandler + 21);
						data[i + 23] = 0x3CC00C00;	// lis		r6, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", TCIntrruptHandlerSigs[j].Name, j, TCIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXTIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = EXTIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *EXTIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXTIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
					case 1:
						data[i + 23] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 23);
						data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i + 24] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 24);
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
						data[i + 24] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 24);
						break;
					case 4:
						data[i + 16] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 16);
						data[i + 18] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 5:
						data[i + 15] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 15);
						data[i + 17] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
					case 6:
						data[i + 13] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 13);
						data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i + 18] = branchAndLink(MASK_IRQ, EXTIntrruptHandler + 18);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXTIntrruptHandlerSigs[j].Name, j, EXTIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIInitSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIInitSigs[j].offsetFoundAt)) {
		u32 *EXIInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIInit) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 0:
						data[i +  5] = branchAndLink(MASK_IRQ, EXIInit + 5);
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 21] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 21);
						data[i + 25] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 25);
						data[i + 29] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 29);
						data[i + 33] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 33);
						data[i + 37] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 37);
						data[i + 41] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 41);
						data[i + 45] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 45);
						data[i + 49] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 49);
						break;
					case 1:
						data[i +  7] = branchAndLink(MASK_IRQ, EXIInit + 7);
						data[i +  9] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 23] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 23);
						data[i + 27] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 27);
						data[i + 31] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 31);
						data[i + 35] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 35);
						data[i + 39] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 39);
						data[i + 43] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 43);
						data[i + 47] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 47);
						data[i + 51] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 51);
						break;
					case 2:
						data[i +  3] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  8] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 20] = branchAndLink(MASK_IRQ, EXIInit + 20);
						data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 31] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 36] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 36);
						data[i + 40] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 40);
						data[i + 44] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 44);
						data[i + 48] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 48);
						data[i + 52] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 52);
						data[i + 56] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 56);
						data[i + 60] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 60);
						data[i + 64] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 64);
						break;
					case 3:
					case 4:
						data[i +  9] = branchAndLink(MASK_IRQ, EXIInit + 9);
						data[i + 10] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 21] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 21);
						data[i + 26] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 26);
						data[i + 31] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 31);
						data[i + 34] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 34);
						data[i + 37] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 37);
						data[i + 40] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 40);
						data[i + 43] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 43);
						data[i + 46] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 46);
						break;
					case 5:
						data[i +  5] = branchAndLink(MASK_IRQ, EXIInit + 5);
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 21] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 21);
						data[i + 25] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 25);
						data[i + 29] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 29);
						data[i + 33] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 33);
						data[i + 37] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 37);
						data[i + 41] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 41);
						data[i + 45] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 45);
						data[i + 49] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 49);
						break;
					case 6:
						data[i + 11] = branchAndLink(MASK_IRQ, EXIInit + 11);
						data[i + 13] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 23] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 23);
						data[i + 28] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 28);
						data[i + 33] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 33);
						data[i + 36] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 36);
						data[i + 39] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 39);
						data[i + 42] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 42);
						data[i + 45] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 45);
						data[i + 48] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 48);
						break;
					case 7:
					case 8:
					case 9:
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 23] = branchAndLink(MASK_IRQ, EXIInit + 23);
						data[i + 25] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 35] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 35);
						data[i + 40] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 40);
						data[i + 45] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 45);
						data[i + 48] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 48);
						data[i + 51] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 51);
						data[i + 54] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 54);
						data[i + 57] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 57);
						data[i + 60] = branchAndLink(SET_IRQ_HANDLER, EXIInit + 60);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIInitSigs[j].Name, j, EXIInit);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXILockSigs) / sizeof(FuncPattern); j++)
	if ((i = EXILockSigs[j].offsetFoundAt)) {
		u32 *EXILock = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXILock) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 6] = (u32)EXILock;
			
			print_gecko("Found:[%s$%i] @ %08X\n", EXILockSigs[j].Name, j, EXILock);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIUnlockSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIUnlockSigs[j].offsetFoundAt)) {
		u32 *EXIUnlock = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIUnlock) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 7] = (u32)EXIUnlock;
			
			print_gecko("Found:[%s$%i] @ %08X\n", EXIUnlockSigs[j].Name, j, EXIUnlock);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIGetIDSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIGetIDSigs[j].offsetFoundAt)) {
		u32 *EXIGetID = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIGetID) {
			if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
				switch (j) {
					case 4:
						data[i +  42] = branchAndLink(UNMASK_IRQ, EXIGetID +  42);
						data[i + 168] = branchAndLink(MASK_IRQ,   EXIGetID + 168);
						break;
					case 5:
						data[i +  56] = branchAndLink(UNMASK_IRQ, EXIGetID +  56);
						data[i + 190] = branchAndLink(MASK_IRQ,   EXIGetID + 190);
						break;
					case 6:
						data[i +  57] = branchAndLink(UNMASK_IRQ, EXIGetID +  57);
						data[i + 193] = branchAndLink(MASK_IRQ,   EXIGetID + 193);
						break;
					case 7:
						data[i +  56] = branchAndLink(UNMASK_IRQ, EXIGetID +  56);
						data[i + 190] = branchAndLink(MASK_IRQ,   EXIGetID + 190);
						break;
					case 8:
						data[i +  65] = branchAndLink(UNMASK_IRQ, EXIGetID +  65);
						data[i + 203] = branchAndLink(MASK_IRQ,   EXIGetID + 203);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", EXIGetIDSigs[j].Name, j, EXIGetID);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__DVDInterruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = __DVDInterruptHandlerSigs[j].offsetFoundAt)) {
		u32 *__DVDInterruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__DVDInterruptHandler) {
			switch (j) {
				case 0:
					data[i +  28] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  48] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  72] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  89] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  91] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  97] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 107] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 113] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i +  32] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  52] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  95] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 101] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 111] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 117] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  30] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  52] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  95] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 101] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 111] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 117] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
				case 4:
					data[i +   1] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  28] = 0x3FE00C00;	// lis		r31, 0x0C00
					data[i +  54] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  68] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  69] = 0x901F6004;	// stw		r0, 0x6004 (r31)
					data[i +  74] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  76] = 0x80050004;	// lwz		r0, 0x0004 (r5)
					data[i +  85] = 0x90050004;	// stw		r0, 0x0004 (r5)
					data[i +  89] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i +  29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  49] = 0x3FE00C00;	// lis		r31, 0x0C00
					data[i +  75] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  89] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  90] = 0x901F6004;	// stw		r0, 0x6004 (r31)
					data[i +  95] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  97] = 0x80050004;	// lwz		r0, 0x0004 (r5)
					data[i + 106] = 0x90050004;	// stw		r0, 0x0004 (r5)
					data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i +  31] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  51] = 0x3FE00C00;	// lis		r31, 0x0C00
					data[i +  77] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  91] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  92] = 0x901F6004;	// stw		r0, 0x6004 (r31)
					data[i +  97] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  99] = 0x80050004;	// lwz		r0, 0x0004 (r5)
					data[i + 108] = 0x90050004;	// stw		r0, 0x0004 (r5)
					data[i + 112] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 7:
					data[i +  33] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  54] = 0x3FE00C00;	// lis		r31, 0x0C00
					data[i +  80] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  94] = 0x801F6004;	// lwz		r0, 0x6004 (r31)
					data[i +  95] = 0x901F6004;	// stw		r0, 0x6004 (r31)
					data[i + 100] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 102] = 0x80050004;	// lwz		r0, 0x0004 (r5)
					data[i + 111] = 0x90050004;	// stw		r0, 0x0004 (r5)
					data[i + 115] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 8:
					data[i +  28] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  51] = 0x3FE00C00;	// lis		r31, 0x0C00
					data[i +  78] = 0x801B0004;	// lwz		r0, 0x0004 (r27)
					data[i +  92] = 0x801B0004;	// lwz		r0, 0x0004 (r27)
					data[i +  93] = 0x901B0004;	// stw		r0, 0x0004 (r27)
					data[i +  98] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 100] = 0x80050004;	// lwz		r0, 0x0004 (r5)
					data[i + 109] = 0x90050004;	// stw		r0, 0x0004 (r5)
					data[i + 113] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 9:
					data[i +  30] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  54] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  80] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  95] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 102] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 112] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 118] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __DVDInterruptHandlerSigs[j].Name, j, __DVDInterruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(AlarmHandlerForTimeoutSigs) / sizeof(FuncPattern); j++)
	if ((i = AlarmHandlerForTimeoutSigs[j].offsetFoundAt)) {
		u32 *AlarmHandlerForTimeout = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (AlarmHandlerForTimeout) {
			switch (j) {
				case 0:
				case 1:
				case 2:
					data[i + 6] = branchAndLink(MASK_IRQ, AlarmHandlerForTimeout + 6);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", AlarmHandlerForTimeoutSigs[j].Name, j, AlarmHandlerForTimeout);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(ReadSigs) / sizeof(FuncPattern); j++)
	if ((i = ReadSigs[j].offsetFoundAt)) {
		u32 *Read = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (Read) {
			switch (j) {
				case 0:
					data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 31] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 33] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  8] = 0x3CC00C00;	// lis		r6, 0x0C00
					break;
				case 3:
					data[i + 17] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 4:
					data[i + 19] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i + 18] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", ReadSigs[j].Name, j, Read);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowReadSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowReadSigs[j].offsetFoundAt)) {
		u32 *DVDLowRead = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowRead) {
			switch (j) {
				case 0:
				case 1:
					data[i +  40] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
				case 3:
				case 4:
					data[i +   1] = 0x3CE00C00;	// lis		r7, 0x0C00
					break;
				case 5:
					data[i +   7] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i +  32] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 140] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 223] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowReadSigs[j].Name, j, DVDLowRead);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowSeekSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowSeekSigs[j].offsetFoundAt)) {
		u32 *DVDLowSeek = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowSeek) {
			switch (j) {
				case 0:
					data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 26] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
				case 3:
					data[i +  1] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 4:
					data[i +  7] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i +  3] = 0x3CE00C00;	// lis		r7, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowSeekSigs[j].Name, j, DVDLowSeek);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowWaitCoverCloseSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowWaitCoverCloseSigs[j].offsetFoundAt)) {
		u32 *DVDLowWaitCoverClose = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowWaitCoverClose) {
			switch (j) {
				case 0:
					data[i + 4] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 1:
					data[i + 6] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 2:
				case 3:
					data[i + 2] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 4] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowWaitCoverCloseSigs[j].Name, j, DVDLowWaitCoverClose);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowReadDiskIDSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowReadDiskIDSigs[j].offsetFoundAt)) {
		u32 *DVDLowReadDiskID = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowReadDiskID) {
			switch (j) {
				case 0:
					data[i + 19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 30] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 33] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 35] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  2] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 3:
					data[i + 10] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 4:
					data[i + 11] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i +  6] = 0x3D000C00;	// lis		r8, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowReadDiskIDSigs[j].Name, j, DVDLowReadDiskID);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowStopMotorSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowStopMotorSigs[j].offsetFoundAt)) {
		u32 *DVDLowStopMotor = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowStopMotor) {
			switch (j) {
				case 0:
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i +  9] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  1] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i +  1] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 4:
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i +  3] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowStopMotorSigs[j].Name, j, DVDLowStopMotor);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowRequestErrorSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowRequestErrorSigs[j].offsetFoundAt)) {
		u32 *DVDLowRequestError = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowRequestError) {
			switch (j) {
				case 0:
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i +  9] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  1] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i +  1] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 4:
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i +  3] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowRequestErrorSigs[j].Name, j, DVDLowRequestError);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowInquirySigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowInquirySigs[j].offsetFoundAt)) {
		u32 *DVDLowInquiry = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowInquiry) {
			switch (j) {
				case 0:
					data[i +  8] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 11] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 14] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  0] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 3:
					data[i +  1] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 4:
					data[i +  9] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i +  3] = 0x3D000C00;	// lis		r8, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowInquirySigs[j].Name, j, DVDLowInquiry);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowAudioStreamSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowAudioStreamSigs[j].offsetFoundAt)) {
		u32 *DVDLowAudioStream = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowAudioStream) {
			switch (j) {
				case 0:
					data[i + 11] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  2] = 0x3CE00C00;	// lis		r7, 0x0C00
					break;
				case 3:
					data[i +  1] = 0x3CE00C00;	// lis		r7, 0x0C00
					break;
				case 4:
					data[i +  7] = 0x3CC00C00;	// lis		r6, 0x0C00
					break;
				case 5:
					data[i +  6] = 0x3D000C00;	// lis		r8, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowAudioStreamSigs[j].Name, j, DVDLowAudioStream);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowRequestAudioStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowRequestAudioStatusSigs[j].offsetFoundAt)) {
		u32 *DVDLowRequestAudioStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowRequestAudioStatus) {
			switch (j) {
				case 0:
					data[i +  9] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 11] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 14] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  2] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 3:
					data[i +  1] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 4:
					data[i +  7] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i +  6] = 0x3CC00C00;	// lis		r6, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowRequestAudioStatusSigs[j].Name, j, DVDLowRequestAudioStatus);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowAudioBufferConfigSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowAudioBufferConfigSigs[j].offsetFoundAt)) {
		u32 *DVDLowAudioBufferConfig = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowAudioBufferConfig) {
			switch (j) {
				case 0:
					data[i + 35] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 38] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 37] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 40] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
				case 4:
					data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 14] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowAudioBufferConfigSigs[j].Name, j, DVDLowAudioBufferConfig);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowResetSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowResetSigs[j].offsetFoundAt)) {
		u32 *DVDLowReset = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowReset) {
			switch (j) {
				case 0:
					data[i +  5] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 11] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 37] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i +  1] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 11] = 0x901F0024;	// stw		r0, 0x0024 (r31)
					data[i + 36] = 0x901F0024;	// stw		r0, 0x0024 (r31)
					break;
				case 2:
					data[i +  6] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 36] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowResetSigs[j].Name, j, DVDLowReset);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DoBreakSigs) / sizeof(FuncPattern); j++)
	if ((i = DoBreakSigs[j].offsetFoundAt)) {
		u32 *DoBreak = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DoBreak) {
			switch (j) {
				case 0:
					data[i + 2] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 5] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 0] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DoBreakSigs[j].Name, j, DoBreak);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(AlarmHandlerForBreakSigs) / sizeof(FuncPattern); j++)
	if ((i = AlarmHandlerForBreakSigs[j].offsetFoundAt)) {
		u32 *AlarmHandlerForBreak = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (AlarmHandlerForBreak) {
			switch (j) {
				case 0:
					data[i + 3] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 1] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", AlarmHandlerForBreakSigs[j].Name, j, AlarmHandlerForBreak);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowBreakSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowBreakSigs[j].offsetFoundAt)) {
		u32 *DVDLowBreak = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowBreak) {
			switch (j) {
				case 0:
					data[i + 3] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 0] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 3:
					data[i + 1] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowBreakSigs[j].Name, j, DVDLowBreak);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowClearCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowClearCallbackSigs[j].offsetFoundAt)) {
		u32 *DVDLowClearCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowClearCallback) {
			switch (j) {
				case 0:
				case 1:
					data[i + 3] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 0] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i + 1] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 0] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowClearCallbackSigs[j].Name, j, DVDLowClearCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDLowGetCoverStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDLowGetCoverStatusSigs[j].offsetFoundAt)) {
		u32 *DVDLowGetCoverStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDLowGetCoverStatus) {
			switch (j) {
				case 0:
					data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
				case 2:
					data[i + 26] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDLowGetCoverStatusSigs[j].Name, j, DVDLowGetCoverStatus);
			patched++;
		}
	}
	
	if ((i = __DVDLowTestAlarmSig.offsetFoundAt)) {
		u32 *__DVDLowTestAlarm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *__DVDLowTestAlarmHook;
		
		if (__DVDLowTestAlarm) {
			__DVDLowTestAlarmHook = getPatchAddr(DVD_LOWTESTALARMHOOK);
			
			data[i + 12] = 0x38000000;	// li		r0, 0
			data[i + 13] = branch(__DVDLowTestAlarmHook, __DVDLowTestAlarm + 13);
			
			print_gecko("Found:[%s] @ %08X\n", __DVDLowTestAlarmSig.Name, __DVDLowTestAlarm);
			patched++;
		}
	}
	
	if ((i = DVDGetTransferredSizeSig.offsetFoundAt)) {
		u32 *DVDGetTransferredSize = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDGetTransferredSize) {
			data[i + 20] = 0x3C800C00;	// lis		r4, 0x0C00
			
			print_gecko("Found:[%s] @ %08X\n", DVDGetTransferredSizeSig.Name, DVDGetTransferredSize);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDInitSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDInitSigs[j].offsetFoundAt)) {
		u32 *DVDInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDInit) {
			switch (j) {
				case 0:
					data[i + 23] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 23);
					data[i + 25] = branchAndLink(UNMASK_IRQ,      DVDInit + 25);
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 21] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 21);
					data[i + 23] = branchAndLink(UNMASK_IRQ,      DVDInit + 23);
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 30] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 23] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 23);
					data[i + 25] = branchAndLink(UNMASK_IRQ,      DVDInit + 25);
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i + 20] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 20);
					data[i + 22] = branchAndLink(UNMASK_IRQ,      DVDInit + 22);
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 22] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 22);
					data[i + 24] = branchAndLink(UNMASK_IRQ,      DVDInit + 24);
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 20] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 20);
					data[i + 22] = branchAndLink(UNMASK_IRQ,      DVDInit + 22);
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i + 19] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 19);
					data[i + 21] = branchAndLink(UNMASK_IRQ,      DVDInit + 21);
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 7:
					data[i + 23] = branchAndLink(SET_IRQ_HANDLER, DVDInit + 23);
					data[i + 25] = branchAndLink(UNMASK_IRQ,      DVDInit + 25);
					data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDInitSigs[j].Name, j, DVDInit);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(cbForStateGettingErrorSigs) / sizeof(FuncPattern); j++)
	if ((i = cbForStateGettingErrorSigs[j].offsetFoundAt)) {
		u32 *cbForStateGettingError = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (cbForStateGettingError) {
			switch (j) {
				case 0:
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 31] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i + 26] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 7:
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", cbForStateGettingErrorSigs[j].Name, j, cbForStateGettingError);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(cbForUnrecoveredErrorRetrySigs) / sizeof(FuncPattern); j++)
	if ((i = cbForUnrecoveredErrorRetrySigs[j].offsetFoundAt)) {
		u32 *cbForUnrecoveredErrorRetry = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (cbForUnrecoveredErrorRetry) {
			switch (j) {
				case 0:
					data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", cbForUnrecoveredErrorRetrySigs[j].Name, j, cbForUnrecoveredErrorRetry);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(cbForStateMotorStoppedSigs) / sizeof(FuncPattern); j++)
	if ((i = cbForStateMotorStoppedSigs[j].offsetFoundAt)) {
		u32 *cbForStateMotorStopped = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (cbForStateMotorStopped) {
			switch (j) {
				case 0:
					data[i + 14] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
				case 2:
				case 3:
					data[i +  1] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i +  2] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i +  1] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", cbForStateMotorStoppedSigs[j].Name, j, cbForStateMotorStopped);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(stateBusySigs) / sizeof(FuncPattern); j++)
	if ((i = stateBusySigs[j].offsetFoundAt)) {
		u32 *stateBusy = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (stateBusy) {
			switch (j) {
				case 0:
				case 1:
					data[i +  17] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  28] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  30] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  35] = 0x3C000008;	// lis		r0, 8
					data[i +  38] = 0x3C000008;	// lis		r0, 8
					data[i +  55] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  57] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  72] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  74] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  97] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  99] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 108] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 110] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 121] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 123] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 130] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 132] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 139] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 141] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 148] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 150] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 157] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 159] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 167] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 169] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  17] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  47] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  49] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  74] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  91] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 116] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 118] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 127] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 129] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 140] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 142] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 149] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 151] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 158] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 160] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 167] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 169] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 176] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 178] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 186] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 188] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i +  17] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  47] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  49] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  74] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  91] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 116] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 118] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 127] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 129] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 140] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 142] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 149] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 151] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 158] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 160] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 167] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 169] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 176] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 178] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 186] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 188] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 197] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 199] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
				case 5:
				case 6:
				case 7:
					data[i +  16] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  30] = 0x3C800008;	// lis		r4, 8
					data[i +  50] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  67] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  92] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 103] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 116] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 125] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 134] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 143] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 152] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 162] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 8:
					data[i +  16] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  44] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  67] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  84] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 109] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 120] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 133] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 142] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 151] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 160] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 169] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 179] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 9:
					data[i +  16] = 0x3CC00C00;	// lis		r6, 0x0C00
					data[i +  43] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  64] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i +  80] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 104] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 114] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 126] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 134] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 142] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 150] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 158] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 167] = 0x3CC00C00;	// lis		r6, 0x0C00
					break;
				case 10:
					data[i +  16] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  44] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  67] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  84] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 109] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 120] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 133] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 142] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 151] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 160] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 169] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 179] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 190] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", stateBusySigs[j].Name, j, stateBusy);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(cbForStateBusySigs) / sizeof(FuncPattern); j++)
	if ((i = cbForStateBusySigs[j].offsetFoundAt)) {
		u32 *cbForStateBusy = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (cbForStateBusy) {
			switch (j) {
				case 0:
					data[i +  75] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 178] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 182] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 206] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i +  67] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 154] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 158] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 182] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i +  61] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 148] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 152] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 176] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i +  61] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 168] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 172] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 196] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i +  75] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 181] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 185] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 207] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i +  82] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 194] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 198] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 221] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i +  93] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 208] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 212] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 235] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 7:
					data[i +  96] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 211] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 215] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 238] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 8:
					data[i + 102] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 236] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 240] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 263] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 9:
					data[i +  94] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 230] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 234] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 256] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 10:
					data[i +  96] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 230] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 234] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 257] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 11:
					data[i +  96] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 247] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 251] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 274] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", cbForStateBusySigs[j].Name, j, cbForStateBusy);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDResetSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDResetSigs[j].offsetFoundAt)) {
		u32 *DVDReset = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDReset) {
			switch (j) {
				case 0:
					data[i + 5] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 9] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 4] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 5] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDResetSigs[j].Name, j, DVDReset);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDCheckDiskSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDCheckDiskSigs[j].offsetFoundAt)) {
		u32 *DVDCheckDisk = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDCheckDisk) {
			switch (j) {
				case 0:
				case 1:
					data[i + 44] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
				case 3:
				case 4:
					data[i + 40] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DVDCheckDiskSigs[j].Name, j, DVDCheckDisk);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(AIInitDMASigs) / sizeof(FuncPattern); j++)
	if ((i = AIInitDMASigs[j].offsetFoundAt)) {
		u32 *AIInitDMA = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (AIInitDMA) {
			if (devices[DEVICE_CUR]->emulated() & EMU_AUDIO_STREAMING) {
				switch (j) {
					case 0:
						data[i +  8] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 30] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 35] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 1:
						data[i +  8] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", AIInitDMASigs[j].Name, j, AIInitDMA);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(AIGetDMAStartAddrSigs) / sizeof(FuncPattern); j++)
	if ((i = AIGetDMAStartAddrSigs[j].offsetFoundAt)) {
		u32 *AIGetDMAStartAddr = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (AIGetDMAStartAddr) {
			if (devices[DEVICE_CUR]->emulated() & EMU_AUDIO_STREAMING) {
				switch (j) {
					case 0:
						data[i + 0] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 2] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 1:
						data[i + 0] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_gecko("Found:[%s] @ %08X\n", AIGetDMAStartAddrSigs[j].Name, AIGetDMAStartAddr);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(GXPeekZSigs) / sizeof(FuncPattern); j++)
	if ((i = GXPeekZSigs[j].offsetFoundAt)) {
		u32 *GXPeekZ = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXPeekZ) {
			memcpy(data + i, _gxpeekz_c, sizeof(_gxpeekz_c));
			
			data[i + 1] = 0x3C600800;	// lis		r3, 0x0800
			
			print_gecko("Found:[%s$%i] @ %08X\n", GXPeekZSigs[j].Name, j, GXPeekZ);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__VMBASESetupExceptionHandlersSigs) / sizeof(FuncPattern); j++)
	if ((i = __VMBASESetupExceptionHandlersSigs[j].offsetFoundAt)) {
		u32 *__VMBASESetupExceptionHandlers = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__VMBASESetupExceptionHandlers) {
			if (j == 0) {
				data[i + 22] = 0x7C1E07AC;	// icbi		r30, r0
				data[i + 32] = 0x7C1EE7AC;	// icbi		r30, r28
				data[i + 47] = 0x7C1EE7AC;	// icbi		r30, r28
				data[i + 61] = 0x7C1E07AC;	// icbi		r30, r0
				data[i + 71] = 0x7C1EEFAC;	// icbi		r30, r29
				data[i + 86] = 0x7C1EE7AC;	// icbi		r30, r28
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __VMBASESetupExceptionHandlersSigs[j].Name, j, __VMBASESetupExceptionHandlers);
			patched++;
		}
	}
	
	if ((i = __VMBASEISIExceptionHandlerSig.offsetFoundAt)) {
		u32 *__VMBASEISIExceptionHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__VMBASEISIExceptionHandler) {
			data[i +  5] = 0x7CBA02A6;	// mfsrr0	r5
			data[i + 10] = 0x7CBA02A6;	// mfsrr0	r5
			
			print_gecko("Found:[%s] @ %08X\n", __VMBASEISIExceptionHandlerSig.Name, __VMBASEISIExceptionHandler);
			patched++;
		}
	}
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

extern GXRModeObj *newmode;

void Patch_VideoMode(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	u32 address;
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
		{ 54, 21, 6, 3, 3, 12, NULL, 0, "VISetRegsD" },
		{ 58, 21, 7, 3, 3, 12, NULL, 0, "VISetRegsD" }
	};
	FuncPattern __VIRetraceHandlerSigs[8] = {
		{ 120, 41,  7, 10, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 121, 41,  7, 11, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 132, 39,  7, 10, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 137, 42,  8, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 138, 42,  9, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 140, 43, 10, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 147, 46, 13, 10, 15, 15, NULL, 0, "__VIRetraceHandler" },	// SN Systems ProDG
		{ 157, 50, 10, 15, 16, 13, NULL, 0, "__VIRetraceHandler" }
	};
	FuncPattern getTimingSigs[8] = {
		{  30,  12,  2,  0,  7,  2,           NULL,                     0, "getTimingD" },
		{  40,  16,  2,  0, 12,  2, getTimingPatch, getTimingPatch_length, "getTimingD" },
		{  26,  11,  0,  0,  0,  3,           NULL,                     0, "getTiming" },
		{  30,  13,  0,  0,  0,  3,           NULL,                     0, "getTiming" },
		{  36,  15,  0,  0,  0,  4, getTimingPatch, getTimingPatch_length, "getTiming" },
		{  40,  19,  0,  0,  0,  2,           NULL,                     0, "getTiming" },
		{ 559, 112, 44, 14, 53, 48,           NULL,                     0, "getTiming" },	// SN Systems ProDG
		{  42,  20,  0,  0,  0,  2,           NULL,                     0, "getTiming" }
	};
	FuncPattern __VIInitSigs[7] = {
		{ 159, 44, 5, 4,  4, 16, NULL, 0, "__VIInitD" },
		{ 161, 44, 5, 4,  5, 16, NULL, 0, "__VIInitD" },
		{ 122, 21, 8, 1,  5,  5, NULL, 0, "__VIInit" },
		{ 126, 23, 8, 1,  6,  5, NULL, 0, "__VIInit" },
		{ 128, 23, 8, 1,  7,  5, NULL, 0, "__VIInit" },
		{ 176, 56, 4, 0, 21,  3, NULL, 0, "__VIInit" },	// SN Systems ProDG
		{ 129, 24, 7, 1,  8,  5, NULL, 0, "__VIInit" }
	};
	FuncPattern AdjustPositionSig = 
		{ 135, 9, 1, 0, 17, 47, NULL, 0, "AdjustPositionD" };
	FuncPattern ImportAdjustingValuesSig = 
		{ 26, 9, 3, 3, 0, 4, NULL, 0, "ImportAdjustingValuesD" };
	FuncPattern VIInitSigs[8] = {
		{ 208, 60, 18, 8,  1, 24, NULL, 0, "VIInitD" },
		{ 218, 64, 21, 8,  1, 22, NULL, 0, "VIInitD" },
		{ 269, 37, 18, 7, 18, 57, NULL, 0, "VIInit" },
		{ 270, 37, 19, 7, 18, 57, NULL, 0, "VIInit" },
		{ 283, 40, 24, 7, 18, 49, NULL, 0, "VIInit" },
		{ 286, 41, 25, 7, 18, 49, NULL, 0, "VIInit" },
		{ 300, 48, 27, 8, 17, 48, NULL, 0, "VIInit" },
		{ 335, 85, 23, 9, 17, 42, NULL, 0, "VIInit" }	// SN Systems ProDG
	};
	FuncPattern VIWaitForRetraceSigs[2] = {
		{ 19, 5, 2, 3, 1, 4, NULL, 0, "VIWaitForRetraceD" },
		{ 21, 7, 4, 3, 1, 4, NULL, 0, "VIWaitForRetrace" }
	};
	FuncPattern setFbbRegsSigs[3] = {
		{ 167, 62, 22, 2,  8, 24, NULL, 0, "setFbbRegsD" },
		{ 181, 54, 34, 0, 10, 16, NULL, 0, "setFbbRegs" },
		{ 177, 51, 34, 0, 10, 16, NULL, 0, "setFbbRegs" }	// SN Systems ProDG
	};
	FuncPattern setVerticalRegsSigs[4] = {
		{ 118, 17, 11, 0, 4, 23, NULL, 0, "setVerticalRegsD" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs" },
		{ 114, 19, 13, 0, 4, 25, NULL, 0, "setVerticalRegs" }	// SN Systems ProDG
	};
	FuncPattern VIConfigureSigs[11] = {
		{ 280,  74, 15, 20, 21, 20, NULL, 0, "VIConfigureD" },
		{ 314,  86, 15, 21, 27, 20, NULL, 0, "VIConfigureD" },
		{ 428,  90, 43,  6, 32, 60, NULL, 0, "VIConfigure" },
		{ 420,  87, 41,  6, 31, 60, NULL, 0, "VIConfigure" },
		{ 423,  88, 41,  6, 31, 61, NULL, 0, "VIConfigure" },
		{ 464, 100, 43, 13, 34, 61, NULL, 0, "VIConfigure" },
		{ 462,  99, 43, 12, 34, 61, NULL, 0, "VIConfigure" },
		{ 487, 105, 44, 12, 38, 63, NULL, 0, "VIConfigure" },
		{ 522, 111, 44, 13, 53, 64, NULL, 0, "VIConfigure" },
		{ 559, 112, 44, 14, 53, 48, NULL, 0, "VIConfigure" },	// SN Systems ProDG
		{ 514, 110, 44, 13, 49, 63, NULL, 0, "VIConfigure" }
	};
	FuncPattern VIConfigurePanSigs[2] = {
		{ 100, 26,  3, 9,  6,  4, NULL, 0, "VIConfigurePanD" },
		{ 229, 40, 11, 4, 25, 35, NULL, 0, "VIConfigurePan" }
	};
	FuncPattern VISetBlackSigs[3] = {
		{ 30, 6, 5, 3, 0, 3, NULL, 0, "VISetBlackD" },
		{ 31, 7, 6, 3, 0, 3, NULL, 0, "VISetBlack" },
		{ 30, 7, 6, 3, 0, 4, NULL, 0, "VISetBlack" }	// SN Systems ProDG
	};
	FuncPattern VIGetRetraceCountSig = 
		{ 2, 1, 0, 0, 0, 0, NULL, 0, "VIGetRetraceCount" };
	FuncPattern GetCurrentDisplayPositionSig = 
		{ 15, 3, 2, 0, 0, 1, NULL, 0, "GetCurrentDisplayPosition" };
	FuncPattern getCurrentHalfLineSigs[2] = {
		{ 26, 9, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD" },
		{ 24, 7, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD" }
	};
	FuncPattern getCurrentFieldEvenOddSigs[5] = {
		{ 33,  7, 2, 3, 4, 5, NULL, 0, "getCurrentFieldEvenOddD" },
		{ 15,  5, 2, 1, 2, 3, NULL, 0, "getCurrentFieldEvenOddD" },
		{ 47, 14, 2, 2, 4, 8, NULL, 0, "getCurrentFieldEvenOdd" },
		{ 26,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd" },
		{ 26,  8, 0, 0, 1, 5, NULL, 0, "getCurrentFieldEvenOdd" }	// SN Systems ProDG
	};
	FuncPattern VIGetNextFieldSigs[4] = {
		{ 20,  4, 2, 3, 0, 3, NULL, 0, "VIGetNextFieldD" },
		{ 61, 16, 4, 4, 4, 9, NULL, 0, "VIGetNextField" },
		{ 42, 10, 3, 2, 2, 8, NULL, 0, "VIGetNextField" },
		{ 39, 13, 4, 3, 2, 6, NULL, 0, "VIGetNextField" }
	};
	FuncPattern VIGetDTVStatusSigs[2] = {
		{ 17, 3, 2, 2, 0, 3, NULL, 0, "VIGetDTVStatusD" },
		{ 15, 4, 3, 2, 0, 2, NULL, 0, "VIGetDTVStatus" }
	};
	FuncPattern __GXInitGXSigs[9] = {
		{ 1130, 567, 66, 133, 46, 46, NULL, 0, "__GXInitGXD" },
		{  544, 319, 33, 109, 18,  5, NULL, 0, "__GXInitGXD" },
		{  976, 454, 81, 119, 43, 36, NULL, 0, "__GXInitGX" },
		{  530, 307, 35, 107, 18, 10, NULL, 0, "__GXInitGX" },
		{  545, 310, 35, 108, 24, 11, NULL, 0, "__GXInitGX" },
		{  561, 313, 36, 110, 28, 11, NULL, 0, "__GXInitGX" },
		{  546, 293, 37, 110,  7,  9, NULL, 0, "__GXInitGX" },	// SN Systems ProDG
		{  549, 289, 38, 110,  7,  9, NULL, 0, "__GXInitGX" },	// SN Systems ProDG
		{  590, 333, 34, 119, 28, 11, NULL, 0, "__GXInitGX" }
	};
	FuncPattern GXAdjustForOverscanSigs[4] = {
		{ 57,  6,  4, 0, 3, 11, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscanD" },
		{ 72, 17, 15, 0, 3,  5, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan" },
		{ 63, 11,  8, 1, 2, 10, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan" },	// SN Systems ProDG
		{ 81, 17, 15, 0, 3,  7, GXAdjustForOverscanPatch, GXAdjustForOverscanPatch_length, "GXAdjustForOverscan" }
	};
	FuncPattern GXSetDispCopySrcSigs[5] = {
		{ 104, 44, 10, 5, 5, 6, NULL, 0, "GXSetDispCopySrcD" },
		{  48, 19,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc" },
		{  36,  9,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc" },
		{  14,  2,  2, 0, 0, 2, NULL, 0, "GXSetDispCopySrc" },	// SN Systems ProDG
		{  31, 11,  8, 0, 0, 0, NULL, 0, "GXSetDispCopySrc" }
	};
	FuncPattern GXSetDispCopyYScaleSigs[7] = {
		{ 100, 33, 8, 8, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD" },
		{  85, 32, 4, 6, 4, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScaleD" },
		{  47, 15, 8, 2, 0, 4, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale" },
		{  53, 17, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale" },
		{  50, 14, 4, 1, 5, 8, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale" },
		{  44,  8, 4, 1, 2, 3, GXSetDispCopyYScalePatch2, GXSetDispCopyYScalePatch2_length, "GXSetDispCopyYScale" },	// SN Systems ProDG
		{  51, 16, 4, 1, 5, 7, GXSetDispCopyYScalePatch1, GXSetDispCopyYScalePatch1_length, "GXSetDispCopyYScale" }
	};
	FuncPattern GXSetCopyFilterSigs[5] = {
		{ 567, 183, 44, 32, 36, 38, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilterD" },
		{ 138,  15,  7,  0,  4,  5, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter" },
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter" },	// SN Systems ProDG
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter" },	// SN Systems ProDG
		{ 130,  25,  7,  0,  4,  0, GXSetCopyFilterPatch, GXSetCopyFilterPatch_length, "GXSetCopyFilter" }
	};
	FuncPattern GXSetDispCopyGammaSigs[4] = {
		{ 34, 12, 3, 2, 2, 3, NULL, 0, "GXSetDispCopyGammaD" },
		{  7,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma" },
		{  6,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma" },	// SN Systems ProDG
		{  5,  2, 1, 0, 0, 0, NULL, 0, "GXSetDispCopyGamma" }
	};
	FuncPattern GXCopyDispSigs[5] = {
		{ 149, 62,  3, 14, 14, 3, NULL, 0, "GXCopyDispD" },
		{  92, 34, 14,  0,  3, 1, NULL, 0, "GXCopyDisp" },
		{  87, 29, 14,  0,  3, 1, NULL, 0, "GXCopyDisp" },
		{  69, 15, 12,  0,  1, 1, NULL, 0, "GXCopyDisp" },	// SN Systems ProDG
		{  90, 35, 14,  0,  3, 0, NULL, 0, "GXCopyDisp" }
	};
	FuncPattern GXSetBlendModeSigs[5] = {
		{ 154, 66, 10, 7, 9, 17, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendModeD" },
		{  65, 20,  8, 0, 2,  6, GXSetBlendModePatch1, GXSetBlendModePatch1_length, "GXSetBlendMode" },
		{  21,  6,  2, 0, 0,  2, GXSetBlendModePatch2, GXSetBlendModePatch2_length, "GXSetBlendMode" },
		{  36,  2,  2, 0, 0,  6, GXSetBlendModePatch3, GXSetBlendModePatch3_length, "GXSetBlendMode" },	// SN Systems ProDG
		{  38,  2,  2, 0, 0,  8, GXSetBlendModePatch3, GXSetBlendModePatch3_length, "GXSetBlendMode" }	// SN Systems ProDG
	};
	FuncPattern __GXSetViewportSig = 
		{ 36, 15, 7, 0, 0, 0, GXSetViewportPatch, GXSetViewportPatch_length, "__GXSetViewport" };
	FuncPattern GXSetViewportJitterSigs[5] = {
		{ 193, 76, 22, 4, 15, 22, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitterD" },
		{  71, 20, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter" },
		{  65, 14, 15, 1,  1,  3, GXSetViewportJitterPatch, GXSetViewportJitterPatch_length, "GXSetViewportJitter" },
		{  31,  6, 10, 1,  0,  4,                     NULL,                               0, "GXSetViewportJitter" },	// SN Systems ProDG
		{  22,  6,  8, 1,  0,  2,                     NULL,                               0, "GXSetViewportJitter" }
	};
	FuncPattern GXSetViewportSigs[5] = {
		{ 21, 9, 8, 1, 0, 2, NULL, 0, "GXSetViewportD" },
		{  9, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport" },
		{  9, 3, 2, 1, 0, 2, NULL, 0, "GXSetViewport" },
		{  2, 1, 0, 0, 1, 0, NULL, 0, "GXSetViewport" },	// SN Systems ProDG
		{ 18, 5, 8, 1, 0, 2, NULL, 0, "GXSetViewport" }
	};
	_SDA2_BASE_ = _SDA_BASE_ = 0;
	
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
		memcpy(VAR_VFILTER, vertical_reduction[swissSettings.forceVFilter], 7);
	else
		memcpy(VAR_VFILTER, vertical_filters[swissSettings.forceVFilter], 7);
	
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
			*(u16 *)VAR_SAR_WIDTH = 656;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 6:
			*(u16 *)VAR_SAR_WIDTH = 672;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 7:
			*(u16 *)VAR_SAR_WIDTH = 704;
			*(u8 *)VAR_SAR_HEIGHT = 0;
			break;
		case 8:
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
				get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
				get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
				i += 4;
			}
			continue;
		}
		if ((data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6) ||
			(data[i - 1] != 0x4E800020 &&
			(data[i + 0] == 0x60000000 || data[i - 1] != 0x4C000064) &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
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
			}
			else if (VIInitSigs[j].offsetFoundAt && i == VIInitSigs[j].offsetFoundAt + VIInitSigs[j].Length) {
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
					}
				}
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
			}
			else if (VIConfigureSigs[j].offsetFoundAt && i == VIConfigureSigs[j].offsetFoundAt + VIConfigureSigs[j].Length) {
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
					}
				}
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
					}
				}
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
			}
			else if (getCurrentFieldEvenOddSigs[j].offsetFoundAt && i == getCurrentFieldEvenOddSigs[j].offsetFoundAt + getCurrentFieldEvenOddSigs[j].Length) {
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
					}
				}
			}
		}
		
		for (j = 0; j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern); j++) {
			if (!VIGetDTVStatusSigs[j].offsetFoundAt && compare_pattern(&fp, &VIGetDTVStatusSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i +  6, i +  7, &address) && address == 0xCC00206E &&
							findx_pattern(data, dataType, i + 10, length, &OSRestoreInterruptsSig))
							VIGetDTVStatusSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i +  5, i +  6, &address) && address == 0xCC00206E &&
							findx_pattern(data, dataType, i +  8, length, &OSRestoreInterruptsSig))
							VIGetDTVStatusSigs[j].offsetFoundAt = i;
						break;
				}
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
			if (swissSettings.gameVMode >= 8 && swissSettings.gameVMode <= 14) {
				switch (k) {
					case 0:
					case 2:
						data[i + 8] = 0x38600006;	// li		r3, 6
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", getCurrentFieldEvenOddSigs[k].Name, k, getCurrentFieldEvenOdd);
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
							data[i +  7] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i +  8] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 1:
							data[i +  6] = branchAndLink(getCurrentFieldEvenOdd, VISetRegs + 6);
							data[i +  7] = 0x2C030000;	// cmpwi	r3, 0
							data[i +  8] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i +  9] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i + 10] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
							break;
					}
				}
				print_gecko("Found:[%s$%i] @ %08X\n", VISetRegsSigs[j].Name, j, VISetRegs);
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
							data[i + 60] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i + 61] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 4:
						case 5:
							data[i + 60] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 60);
							data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 62] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 63] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i + 64] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 6:
							data[i + 65] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 65);
							data[i + 66] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 67] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 68] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i + 69] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
							break;
						case 7:
							data[i + 77] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 77);
							data[i + 78] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 79] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 80] = 0x3C800000 | ((u32)VAR_AREA >> 16);
							data[i + 81] = 0x98640000 | ((u32)VAR_CURRENT_FIELD & 0xFFFF);
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
				print_gecko("Found:[%s$%i] @ %08X\n", __VIRetraceHandlerSigs[j].Name, j, __VIRetraceHandler);
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
			}
			else if (swissSettings.gameVMode >= 8 && swissSettings.gameVMode <= 14) {
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
			print_gecko("Found:[%s$%i] @ %08X\n", getTimingSigs[j].Name, j, getTiming);
		}
	}
	
	for (j = 0; j < sizeof(__VIInitSigs) / sizeof(FuncPattern); j++)
		if (__VIInitSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(__VIInitSigs) / sizeof(FuncPattern) && (i = __VIInitSigs[j].offsetFoundAt)) {
		u32 *__VIInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__VIInit) {
			switch (j) {
				case 0:
					data[i +  10] = 0x579B07BE;	// clrlwi	r27, r28, 30
					data[i + 135] = 0x281B0002;	// cmplwi	r27, 2
					data[i + 137] = 0x5760177A;	// rlwinm	r0, r27, 2, 29, 29
					break;
				case 1:
					data[i +  10] = 0x57BB07BE;	// clrlwi	r27, r29, 30
					data[i + 135] = 0x281B0002;	// cmplwi	r27, 2
					data[i + 137] = 0x281B0003;	// cmplwi	r27, 3
					data[i + 139] = 0x5760177A;	// rlwinm	r0, r27, 2, 29, 29
					break;
				case 2:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 3:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x281E0003;	// cmplwi	r30, 3
					data[i + 106] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 4:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x281E0003;	// cmplwi	r30, 3
					data[i + 106] = 0x281E0001;	// cmplwi	r30, 1
					data[i + 107] = 0x41810020;	// bgt		+8
					data[i + 108] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 5:
					data[i +   5] = 0x546607BE;	// clrlwi	r6, r3, 30
					data[i +  90] = 0x28060002;	// cmplwi	r6, 2
					data[i + 157] = 0x28060003;	// cmplwi	r6, 3
					data[i + 159] = 0x28060001;	// cmplwi	r6, 1
					data[i + 160] = 0x41810020;	// bgt		+8
					data[i + 161] = 0x54C5177A;	// rlwinm	r5, r6, 2, 29, 29
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __VIInitSigs[j].Name, j, __VIInit);
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
			switch (k) {
				case 0:
					data[i +  14] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					break;
				case 1:
					data[i +  14] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					data[i + 143] = 0x38000000 | (swissSettings.sramVideo & 0xFFFF);
					break;
				case 2:
				case 3:
					data[i +  15] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					break;
				case 4:
					data[i +  18] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					break;
				case 5:
					data[i +  18] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					data[i + 138] = 0x38800000 | (swissSettings.sramVideo & 0xFFFF);
					break;
				case 6:
					data[i +  24] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					data[i + 152] = 0x38800000 | (swissSettings.sramVideo & 0xFFFF);
					break;
				case 7:
					data[i +  17] = 0x38600000 | (newmode->viTVMode & 0xFFFF);
					data[i + 201] = 0x39200000 | (swissSettings.sramVideo & 0xFFFF);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", VIInitSigs[k].Name, k, VIInit);
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
				print_gecko("Found:[%s$%i] @ %08X\n", VIWaitForRetraceSigs[j].Name, j, VIWaitForRetrace);
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
			print_gecko("Found:[%s$%i] @ %08X\n", setFbbRegsSigs[j].Name, j, setFbbRegs);
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
			if (swissSettings.forceVideoActive) {
				switch (j) {
					case 0: data[i + 51] = 0x38000000; break;	// li		r0, 0
					case 1:
					case 2: data[i +  4] = 0x3BE00000; break;	// li		r31, 0
					case 3: data[i +  4] = 0x39800000; break;	// li		r12, 0
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", setVerticalRegsSigs[j].Name, j, setVerticalRegs);
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
					data[i + 317] = 0x28000003;	// cmplwi	r0, 3
					data[i + 319] = 0x28000001;	// cmplwi	r0, 1
					data[i + 320] = 0x40810010;	// ble		+4
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
					data[i + 343] = 0x280A0003;	// cmplwi	r10, 3
					data[i + 345] = 0x280A0001;	// cmplwi	r10, 1
					data[i + 346] = 0x40810010;	// ble		+4
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
			print_gecko("Found:[%s$%i] @ %08X\n", VIConfigureSigs[j].Name, j, VIConfigure);
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
			print_gecko("Found:[%s$%i] @ %08X\n", VIConfigurePanSigs[j].Name, j, VIConfigurePan);
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
			print_gecko("Found:[%s$%i] @ %08X\n", VISetBlackSigs[j].Name, j, VISetBlack);
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
			else if (swissSettings.gameVMode >= 8 && swissSettings.gameVMode <= 14) {
				if (j == 1)
					data[i + 12] = 0x38600006;	// li		r3, 6
			}
			print_gecko("Found:[%s$%i] @ %08X\n", VIGetNextFieldSigs[j].Name, j, VIGetNextField);
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
				
				print_gecko("Found:[%s$%i] @ %08X\n", VIGetDTVStatusSigs[j].Name, j, VIGetDTVStatus);
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
			print_gecko("Found:[%s$%i] @ %08X\n", __GXInitGXSigs[j].Name, j, __GXInitGX);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXAdjustForOverscanSigs[j].Name, j, GXAdjustForOverscan);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXSetDispCopyYScaleSigs[j].Name, j, GXSetDispCopyYScale);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXSetCopyFilterSigs[j].Name, j, GXSetCopyFilter);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXSetBlendModeSigs[j].Name, j, GXSetBlendMode);
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
				
				print_gecko("Found:[%s$%i] @ %08X\n", GXCopyDispSigs[j].Name, j, GXCopyDisp);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXSetViewportSigs[j].Name, j, GXSetViewport);
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
				print_gecko("Found:[%s$%i] @ %08X\n", GXSetViewportJitterSigs[j].Name, j, GXSetViewportJitter);
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
		{ 44, 19, 6, 0, 0, 6, NULL, 0, "GXSetScissor" },
		{ 36, 12, 6, 0, 0, 6, NULL, 0, "GXSetScissor" },
		{ 30, 13, 6, 0, 0, 1, NULL, 0, "GXSetScissor" }
	};
	FuncPattern GXSetProjectionSigs[3] = {
		{ 53, 30, 17, 0, 1, 0, NULL, 0, "GXSetProjection" },
		{ 45, 22, 17, 0, 1, 0, NULL, 0, "GXSetProjection" },
		{ 41, 18, 11, 0, 1, 0, NULL, 0, "GXSetProjection" }
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
						print_gecko("Found:[%s$%i] @ %08X\n", GXSetScissorSigs[j].Name, j, GXSetScissor);
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
						print_gecko("Found:[%s$%i] @ %08X\n", GXSetProjectionSigs[j].Name, j, GXSetProjection);
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
		{ 101, 29, 11, 0, 11, 11, NULL, 0, "GXInitTexObjLOD" },
		{  89, 16,  4, 0,  8,  6, NULL, 0, "GXInitTexObjLOD" },	// SN Systems ProDG
		{  89, 30, 11, 0, 11,  6, NULL, 0, "GXInitTexObjLOD" }
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
					print_gecko("Found:[%s$%i] @ %08X\n", GXInitTexObjLODSigs[j].Name, j, GXInitTexObjLOD);
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

u32 _fontencoded[] = {
	0x3C60CC00,	// lis		r3, 0xCC00
	0xA003206E,	// lhz		r0, 0x206E (r3)
	0x540007BD,	// rlwinm.	r0, r0, 0, 30, 30
	0x4182000C,	// beq		+3
	0x38600001,	// li		r3, 1
	0x48000008,	// b		+2
	0x38600000	// li		r3, 0
};

u32 _fontencode_a[] = {
	0x3C60CC00,	// lis		r3, 0xCC00
	0xA003206E,	// lhz		r0, 0x206E (r3)
	0x540007BD,	// rlwinm.	r0, r0, 0, 30, 30
	0x4182000C,	// beq		+3
	0x38000001,	// li		r0, 1
	0x48000008,	// b		+2
	0x38000000	// li		r0, 0
};

u32 _fontencode_b[] = {
	0x3C80CC00,	// lis		r4, 0xCC00
	0xA004206E,	// lhz		r0, 0x206E (r4)
	0x540007BD,	// rlwinm.	r0, r0, 0, 30, 30
	0x4182000C,	// beq		+3
	0x38000001,	// li		r0, 1
	0x48000008,	// b		+2
	0x38000000	// li		r0, 0
};

u32 _fontencode_c[] = {
	0x3C60CC00,	// lis		r3, 0xCC00
	0x38632000,	// addi		r3, r3, 0x2000
	0xA063006E,	// lhz		r3, 0x006E (r3)
	0x546307BD,	// rlwinm.	r3, r3, 0, 30, 30
	0x4182000C,	// beq		+3
	0x38600001,	// li		r3, 1
	0x48000008,	// b		+2
	0x38600000	// li		r3, 0
};

int Patch_FontEncode(u32 *data, u32 length)
{
	int i;
	int patched = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!memcmp(data + i, _fontencoded, sizeof(_fontencoded))) {
			data[i + 4] = 0x38600000 | (swissSettings.fontEncode & 0xFFFF);
			data[i + 6] = 0x38600000 | (swissSettings.fontEncode & 0xFFFF);
			patched++;
		} else if (!memcmp(data + i, _fontencode_a, sizeof(_fontencode_a)) || !memcmp(data + i, _fontencode_b, sizeof(_fontencode_b))) {
			data[i + 4] = 0x38000000 | (swissSettings.fontEncode & 0xFFFF);
			data[i + 6] = 0x38000000 | (swissSettings.fontEncode & 0xFFFF);
			patched++;
		} else if (!memcmp(data + i, _fontencode_c, sizeof(_fontencode_c))) {
			data[i + 5] = 0x38600000 | (swissSettings.fontEncode & 0xFFFF);
			data[i + 7] = 0x38600000 | (swissSettings.fontEncode & 0xFFFF);
			patched++;
		}
	}
	print_gecko("Patch:[fontEncode] applied %d times\n", patched);
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

u8 BS2Ntsc448IntAa[] = {
	0x00,0x00,0x00,0x00,
	0x02,0x50,0x00,0xE2,
	0x01,0xC0,0x00,0x28,
	0x00,0x10,0x02,0x80,
	0x01,0xC0,0x00,0x00,
	0x00,0x00,0x00,0x01,
	0x00,0x01,0x03,0x02,
	0x09,0x06,0x03,0x0A,
	0x03,0x02,0x09,0x06,
	0x03,0x0A,0x09,0x02,
	0x03,0x06,0x09,0x0A,
	0x09,0x02,0x03,0x06,
	0x09,0x0A,0x08,0x08,
	0x0A,0x0C,0x0A,0x08,
	0x08,0x00,0x00,0x00
};

u8 BS2Pal520IntAa[] = {
	0x00,0x00,0x00,0x04,
	0x02,0x50,0x01,0x06,
	0x02,0x08,0x00,0x28,
	0x00,0x1B,0x02,0x80,
	0x02,0x08,0x00,0x00,
	0x00,0x00,0x00,0x01,
	0x00,0x01,0x03,0x02,
	0x09,0x06,0x03,0x0A,
	0x03,0x02,0x09,0x06,
	0x03,0x0A,0x09,0x02,
	0x03,0x06,0x09,0x0A,
	0x09,0x02,0x03,0x06,
	0x09,0x0A,0x08,0x08,
	0x0A,0x0C,0x0A,0x08,
	0x08,0x00,0x00,0x00
};

// Apply specific per-game hacks/workarounds
int Patch_GameSpecific(void *data, u32 length, const char *gameID, int dataType)
{
	void *addr;
	int patched = 0;
	
	// Fix Zelda WW on Wii (__GXSetVAT patch)
	if (!is_gamecube() && (!strncmp(gameID, "GZLP01", 6) || !strncmp(gameID, "GZLE01", 6) || !strncmp(gameID, "GZLJ01", 6)) && dataType == PATCH_DOL) {
		int mode;
		if(!strncmp(gameID, "GZLP01", 6))
			mode = 1;	//PAL
		else 
			mode = 2;	//NTSC-U,NTSC-J
		void *addr_start = data;
		void *addr_end = data+length;
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
	} else if (dataType == PATCH_BS2) {
		switch (length) {
			case 1435168:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(s16 *)(data + 0x81300712 - 0x81300000) = newmode->viTVMode & ~0x3;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300E8A - 0x81300000) = 1;
				*(s16 *)(data + 0x81300EA2 - 0x81300000) = 1;
				*(s16 *)(data + 0x81300EAA - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813016A0 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302F00 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B3E4 - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B40A - 0x81300000) = 10;
				*(s16 *)(data + 0x8130B412 - 0x81300000) = 38;
				*(s16 *)(data + 0x8130B416 - 0x81300000) = 39;
				*(s16 *)(data + 0x8130B422 - 0x81300000) = 15;
				*(s16 *)(data + 0x8130B42E - 0x81300000) = 7;
				*(s16 *)(data + 0x8130B43A - 0x81300000) = 1;
				*(s16 *)(data + 0x8130B446 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130B44E - 0x81300000) = 45;
				*(s16 *)(data + 0x8130B452 - 0x81300000) = 46;
				*(s16 *)(data + 0x8130B45A - 0x81300000) = 42;
				*(s16 *)(data + 0x8130B45E - 0x81300000) = 40;
				*(s16 *)(data + 0x8130B466 - 0x81300000) = 43;
				*(s16 *)(data + 0x8130B476 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130B47E - 0x81300000) = 29;
				*(s16 *)(data + 0x8130B482 - 0x81300000) = 30;
				*(s16 *)(data + 0x8130B48E - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130E8B2 - 0x81300000) = 8;
					*(s16 *)(data + 0x8130E8B6 - 0x81300000) = 15;
					*(s16 *)(data + 0x8130E8C2 - 0x81300000) = 6;
					*(s16 *)(data + 0x8130E8C6 - 0x81300000) = 5;
					*(s16 *)(data + 0x8130E8CA - 0x81300000) = 4;
					*(s16 *)(data + 0x8130E8CE - 0x81300000) = 13;
					*(s16 *)(data + 0x8130E8D2 - 0x81300000) = 255;
					*(s16 *)(data + 0x8130E8D6 - 0x81300000) = 17;
					*(s16 *)(data + 0x8130E8DE - 0x81300000) = 33;
					*(s16 *)(data + 0x8130E8E2 - 0x81300000) = 50;
					*(u32 *)(data + 0x8130E91C - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x8130E924 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x8130E928 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x8130E92C - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x8130E930 - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8135DDE0 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1448256:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(u32 *)(data + 0x81300710 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x81300714 - 0x81300000) = 0x38600000 | (newmode->viTVMode & 0xFFFC);
				
				// Accept any region code.
				*(s16 *)(data + 0x8130092E - 0x81300000) = 1;
				*(s16 *)(data + 0x81300946 - 0x81300000) = 1;
				*(s16 *)(data + 0x8130094E - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81302E60 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813046C0 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130CBA4 - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130CBCA - 0x81300000) = 10;
				*(s16 *)(data + 0x8130CBD2 - 0x81300000) = 38;
				*(s16 *)(data + 0x8130CBD6 - 0x81300000) = 39;
				*(s16 *)(data + 0x8130CBE2 - 0x81300000) = 15;
				*(s16 *)(data + 0x8130CBEE - 0x81300000) = 7;
				*(s16 *)(data + 0x8130CBFA - 0x81300000) = 1;
				*(s16 *)(data + 0x8130CC06 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130CC0E - 0x81300000) = 45;
				*(s16 *)(data + 0x8130CC12 - 0x81300000) = 46;
				*(s16 *)(data + 0x8130CC1A - 0x81300000) = 42;
				*(s16 *)(data + 0x8130CC1E - 0x81300000) = 40;
				*(s16 *)(data + 0x8130CC26 - 0x81300000) = 43;
				*(s16 *)(data + 0x8130CC36 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130CC3E - 0x81300000) = 29;
				*(s16 *)(data + 0x8130CC42 - 0x81300000) = 30;
				*(s16 *)(data + 0x8130CC4E - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x81310072 - 0x81300000) = 8;
					*(s16 *)(data + 0x81310076 - 0x81300000) = 15;
					*(s16 *)(data + 0x81310082 - 0x81300000) = 6;
					*(s16 *)(data + 0x81310086 - 0x81300000) = 5;
					*(s16 *)(data + 0x8131008A - 0x81300000) = 4;
					*(s16 *)(data + 0x8131008E - 0x81300000) = 13;
					*(s16 *)(data + 0x81310092 - 0x81300000) = 255;
					*(s16 *)(data + 0x81310096 - 0x81300000) = 17;
					*(s16 *)(data + 0x8131009E - 0x81300000) = 33;
					*(s16 *)(data + 0x813100A2 - 0x81300000) = 50;
					*(u32 *)(data + 0x813100DC - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x813100E4 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x813100E8 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x813100EC - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x813100F0 - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8135FD00 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1449824:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// Force 24MB physical memory size.
				*(s16 *)(data + 0x813004AE - 0x81300000) = 0x1800000 >> 16;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(u32 *)(data + 0x813009BC - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x813009C0 - 0x81300000) = 0x38600000 | (newmode->viTVMode & 0xFFFC);
				
				// Force production mode.
				*(s16 *)(data + 0x81300A4E - 0x81300000) = 1;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130121E - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813031A0 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x8130497C - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130CE60 - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130CE86 - 0x81300000) = 10;
				*(s16 *)(data + 0x8130CE8E - 0x81300000) = 38;
				*(s16 *)(data + 0x8130CE92 - 0x81300000) = 39;
				*(s16 *)(data + 0x8130CE9E - 0x81300000) = 15;
				*(s16 *)(data + 0x8130CEAA - 0x81300000) = 7;
				*(s16 *)(data + 0x8130CEB6 - 0x81300000) = 1;
				*(s16 *)(data + 0x8130CEC2 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130CECA - 0x81300000) = 45;
				*(s16 *)(data + 0x8130CECE - 0x81300000) = 46;
				*(s16 *)(data + 0x8130CED6 - 0x81300000) = 42;
				*(s16 *)(data + 0x8130CEDA - 0x81300000) = 40;
				*(s16 *)(data + 0x8130CEE2 - 0x81300000) = 43;
				*(s16 *)(data + 0x8130CEF2 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130CEFA - 0x81300000) = 29;
				*(s16 *)(data + 0x8130CEFE - 0x81300000) = 30;
				*(s16 *)(data + 0x8130CF0A - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8131032E - 0x81300000) = 8;
					*(s16 *)(data + 0x81310332 - 0x81300000) = 15;
					*(s16 *)(data + 0x8131033E - 0x81300000) = 6;
					*(s16 *)(data + 0x81310342 - 0x81300000) = 5;
					*(s16 *)(data + 0x81310346 - 0x81300000) = 4;
					*(s16 *)(data + 0x8131034A - 0x81300000) = 13;
					*(s16 *)(data + 0x8131034E - 0x81300000) = 255;
					*(s16 *)(data + 0x81310352 - 0x81300000) = 17;
					*(s16 *)(data + 0x8131035A - 0x81300000) = 33;
					*(s16 *)(data + 0x8131035E - 0x81300000) = 50;
					*(u32 *)(data + 0x81310398 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x813103A0 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x813103A4 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x813103A8 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x813103AC - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x81360160 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "DEV  Revision 1.0");
				patched++;
				break;
			case 1583072:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B55C - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B592 - 0x81300000) = 38;
				*(s16 *)(data + 0x8130B5B2 - 0x81300000) = 10;
				*(s16 *)(data + 0x8130B5BA - 0x81300000) = 39;
				*(s16 *)(data + 0x8130B5BE - 0x81300000) = 15;
				*(s16 *)(data + 0x8130B5C6 - 0x81300000) = 7;
				*(s16 *)(data + 0x8130B5CA - 0x81300000) = 1;
				*(s16 *)(data + 0x8130B5D2 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130B5D6 - 0x81300000) = 45;
				*(s16 *)(data + 0x8130B5DE - 0x81300000) = 46;
				*(s16 *)(data + 0x8130B5E2 - 0x81300000) = 42;
				*(s16 *)(data + 0x8130B5EA - 0x81300000) = 40;
				*(s16 *)(data + 0x8130B5EE - 0x81300000) = 43;
				*(s16 *)(data + 0x8130B602 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130B606 - 0x81300000) = 29;
				*(s16 *)(data + 0x8130B60E - 0x81300000) = 30;
				*(s16 *)(data + 0x8130B61A - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EAAA - 0x81300000) = 8;
					*(s16 *)(data + 0x8130EAAE - 0x81300000) = 15;
					*(s16 *)(data + 0x8130EABA - 0x81300000) = 6;
					*(s16 *)(data + 0x8130EABE - 0x81300000) = 5;
					*(s16 *)(data + 0x8130EAC2 - 0x81300000) = 4;
					*(s16 *)(data + 0x8130EAC6 - 0x81300000) = 13;
					*(s16 *)(data + 0x8130EACA - 0x81300000) = 255;
					*(s16 *)(data + 0x8130EACE - 0x81300000) = 17;
					*(s16 *)(data + 0x8130EAD6 - 0x81300000) = 33;
					*(s16 *)(data + 0x8130EADA - 0x81300000) = 50;
					*(u32 *)(data + 0x8130EB14 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x8130EB1C - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x8130EB20 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EB24 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x8130EB28 - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8137D9F0 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.1");
				patched++;
				break;
			case 1763040:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (newmode->viTVMode >> 2 != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130F1C6 - 0x81300000) = 10;
					*(s16 *)(data + 0x8130F1CA - 0x81300000) = 255;
					*(s16 *)(data + 0x8130F1D6 - 0x81300000) = 7;
					*(s16 *)(data + 0x8130F1DA - 0x81300000) = 6;
					*(s16 *)(data + 0x8130F1DE - 0x81300000) = 5;
					*(s16 *)(data + 0x8130F1E2 - 0x81300000) = 16;
					*(s16 *)(data + 0x8130F1E6 - 0x81300000) = 18;
					*(s16 *)(data + 0x8130F1EA - 0x81300000) = 20;
					*(s16 *)(data + 0x8130F1F2 - 0x81300000) = 40;
					*(s16 *)(data + 0x8130F1F6 - 0x81300000) = 60;
					*(u32 *)(data + 0x8130F230 - 0x81300000) = 0xB3E30016;
					*(u32 *)(data + 0x8130F238 - 0x81300000) = 0x9963000B;
					*(u32 *)(data + 0x8130F23C - 0x81300000) = 0x9BC3001C;
					*(u32 *)(data + 0x8130F240 - 0x81300000) = 0x9BA30037;
					*(u32 *)(data + 0x8130F244 - 0x81300000) = 0x99430035;
					
					memcpy(data + 0x81380FD0 - 0x81300000, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1760128:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300458 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130046C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(u32 *)(data + 0x81300764 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x81300768 - 0x81300000) = 0x38600000 | (newmode->viTVMode & 0xFFFF);
				
				// Accept any region code.
				*(s16 *)(data + 0x813009D6 - 0x81300000) = 1;
				*(s16 *)(data + 0x813009FA - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81302AEC - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813043EC - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (newmode->viTVMode >> 2 != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130FDDA - 0x81300000) = 10;
					*(s16 *)(data + 0x8130FDDE - 0x81300000) = 255;
					*(s16 *)(data + 0x8130FDEA - 0x81300000) = 7;
					*(s16 *)(data + 0x8130FDEE - 0x81300000) = 6;
					*(s16 *)(data + 0x8130FDF2 - 0x81300000) = 5;
					*(s16 *)(data + 0x8130FDF6 - 0x81300000) = 16;
					*(s16 *)(data + 0x8130FDFA - 0x81300000) = 18;
					*(s16 *)(data + 0x8130FDFE - 0x81300000) = 20;
					*(s16 *)(data + 0x8130FE06 - 0x81300000) = 40;
					*(s16 *)(data + 0x8130FE0A - 0x81300000) = 60;
					*(u32 *)(data + 0x8130FE44 - 0x81300000) = 0xB3E30016;
					*(u32 *)(data + 0x8130FE4C - 0x81300000) = 0x9963000B;
					*(u32 *)(data + 0x8130FE50 - 0x81300000) = 0x9BC3001C;
					*(u32 *)(data + 0x8130FE54 - 0x81300000) = 0x9BA30037;
					*(u32 *)(data + 0x8130FE58 - 0x81300000) = 0x99430035;
					
					memcpy(data + 0x81380384 - 0x81300000, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1561760:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (newmode->viTVMode >> 2 != VI_PAL) {
					memcpy(data + 0x8137D910 - 0x81300000, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				} else {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130E9D6 - 0x81300000) = 8;
					*(s16 *)(data + 0x8130E9DA - 0x81300000) = 15;
					*(s16 *)(data + 0x8130E9E6 - 0x81300000) = 6;
					*(s16 *)(data + 0x8130E9EA - 0x81300000) = 5;
					*(s16 *)(data + 0x8130E9EE - 0x81300000) = 4;
					*(s16 *)(data + 0x8130E9F2 - 0x81300000) = 13;
					*(s16 *)(data + 0x8130E9F6 - 0x81300000) = 255;
					*(s16 *)(data + 0x8130E9FA - 0x81300000) = 17;
					*(s16 *)(data + 0x8130EA02 - 0x81300000) = 33;
					*(s16 *)(data + 0x8130EA06 - 0x81300000) = 50;
					*(u32 *)(data + 0x8130EA40 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x8130EA48 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x8130EA4C - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EA50 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x8130EA54 - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8137D910 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "MPAL Revision 1.1");
				patched++;
				break;
			case 1607552:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000) = 0x60000000;
				
				// Force 24MB physical memory size.
				*(s16 *)(data + 0x81300496 - 0x81300000) = 0x1800000 >> 16;
				
				// __VIInit(newmode->viTVMode);
				*(u32 *)(data + 0x813007F0 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x813007F4 - 0x81300000) = 0x38600000 | (newmode->viTVMode & 0xFFFF);
				
				// Force production mode.
				*(s16 *)(data + 0x813008A6 - 0x81300000) = 1;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300C02 - 0x81300000) = 1;
				*(s16 *)(data + 0x81300C26 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81303BF4 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813054B0 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130DBFC - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130DC32 - 0x81300000) = 38;
				*(s16 *)(data + 0x8130DC52 - 0x81300000) = 10;
				*(s16 *)(data + 0x8130DC5A - 0x81300000) = 39;
				*(s16 *)(data + 0x8130DC5E - 0x81300000) = 15;
				*(s16 *)(data + 0x8130DC66 - 0x81300000) = 7;
				*(s16 *)(data + 0x8130DC6A - 0x81300000) = 1;
				*(s16 *)(data + 0x8130DC72 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130DC76 - 0x81300000) = 45;
				*(s16 *)(data + 0x8130DC7E - 0x81300000) = 46;
				*(s16 *)(data + 0x8130DC82 - 0x81300000) = 42;
				*(s16 *)(data + 0x8130DC8A - 0x81300000) = 40;
				*(s16 *)(data + 0x8130DC8E - 0x81300000) = 43;
				*(s16 *)(data + 0x8130DCA2 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130DCA6 - 0x81300000) = 29;
				*(s16 *)(data + 0x8130DCAE - 0x81300000) = 30;
				*(s16 *)(data + 0x8130DCBA - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8131114A - 0x81300000) = 8;
					*(s16 *)(data + 0x8131114E - 0x81300000) = 15;
					*(s16 *)(data + 0x8131115A - 0x81300000) = 6;
					*(s16 *)(data + 0x8131115E - 0x81300000) = 5;
					*(s16 *)(data + 0x81311162 - 0x81300000) = 4;
					*(s16 *)(data + 0x81311166 - 0x81300000) = 13;
					*(s16 *)(data + 0x8131116A - 0x81300000) = 255;
					*(s16 *)(data + 0x8131116E - 0x81300000) = 17;
					*(s16 *)(data + 0x81311176 - 0x81300000) = 33;
					*(s16 *)(data + 0x8131117A - 0x81300000) = 50;
					*(u32 *)(data + 0x813111B4 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x813111BC - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x813111C0 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x813111C4 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x813111C8 - 0x81300000) = 0x99630035;
				}
				print_gecko("Patched:[%s]\n", "TDEV Revision 1.1");
				patched++;
				break;
			case 1586336:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300876 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300ACE - 0x81300000) = 1;
				*(s16 *)(data + 0x81300AF2 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x8130185C - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81303184 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B8D0 - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B906 - 0x81300000) = 38;
				*(s16 *)(data + 0x8130B926 - 0x81300000) = 10;
				*(s16 *)(data + 0x8130B92E - 0x81300000) = 39;
				*(s16 *)(data + 0x8130B932 - 0x81300000) = 15;
				*(s16 *)(data + 0x8130B93A - 0x81300000) = 7;
				*(s16 *)(data + 0x8130B93E - 0x81300000) = 1;
				*(s16 *)(data + 0x8130B946 - 0x81300000) = 4;
				*(s16 *)(data + 0x8130B94A - 0x81300000) = 45;
				*(s16 *)(data + 0x8130B952 - 0x81300000) = 46;
				*(s16 *)(data + 0x8130B956 - 0x81300000) = 42;
				*(s16 *)(data + 0x8130B95E - 0x81300000) = 40;
				*(s16 *)(data + 0x8130B962 - 0x81300000) = 43;
				*(s16 *)(data + 0x8130B976 - 0x81300000) = 31;
				*(s16 *)(data + 0x8130B97A - 0x81300000) = 29;
				*(s16 *)(data + 0x8130B982 - 0x81300000) = 30;
				*(s16 *)(data + 0x8130B98E - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EE1E - 0x81300000) = 8;
					*(s16 *)(data + 0x8130EE22 - 0x81300000) = 15;
					*(s16 *)(data + 0x8130EE2E - 0x81300000) = 6;
					*(s16 *)(data + 0x8130EE32 - 0x81300000) = 5;
					*(s16 *)(data + 0x8130EE36 - 0x81300000) = 4;
					*(s16 *)(data + 0x8130EE3A - 0x81300000) = 13;
					*(s16 *)(data + 0x8130EE3E - 0x81300000) = 255;
					*(s16 *)(data + 0x8130EE42 - 0x81300000) = 17;
					*(s16 *)(data + 0x8130EE4A - 0x81300000) = 33;
					*(s16 *)(data + 0x8130EE4E - 0x81300000) = 50;
					*(u32 *)(data + 0x8130EE88 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x8130EE90 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x8130EE94 - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EE98 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x8130EE9C - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8137ECB8 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1587488:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300876 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300ACE - 0x81300000) = 1;
				*(s16 *)(data + 0x81300AF2 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81301874 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x8130319C - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B8E8 - 0x81300000) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B91E - 0x81300000) = 38;
				*(s16 *)(data + 0x8130B93E - 0x81300000) = 10;
				*(s16 *)(data + 0x8130B946 - 0x81300000) = 39;
				*(s16 *)(data + 0x8130B94A - 0x81300000) = 15;
				*(s16 *)(data + 0x8130B952 - 0x81300000) = 7;
				*(s16 *)(data + 0x8130B956 - 0x81300000) = 1;
				*(s16 *)(data + 0x8130B95E - 0x81300000) = 4;
				*(s16 *)(data + 0x8130B962 - 0x81300000) = 45;
				*(s16 *)(data + 0x8130B96A - 0x81300000) = 46;
				*(s16 *)(data + 0x8130B96E - 0x81300000) = 42;
				*(s16 *)(data + 0x8130B976 - 0x81300000) = 40;
				*(s16 *)(data + 0x8130B97A - 0x81300000) = 43;
				*(s16 *)(data + 0x8130B98E - 0x81300000) = 31;
				*(s16 *)(data + 0x8130B992 - 0x81300000) = 29;
				*(s16 *)(data + 0x8130B99A - 0x81300000) = 30;
				*(s16 *)(data + 0x8130B9A6 - 0x81300000) = 80;
				
				if (newmode->viTVMode >> 2 == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EE36 - 0x81300000) = 8;
					*(s16 *)(data + 0x8130EE3A - 0x81300000) = 15;
					*(s16 *)(data + 0x8130EE46 - 0x81300000) = 6;
					*(s16 *)(data + 0x8130EE4A - 0x81300000) = 5;
					*(s16 *)(data + 0x8130EE4E - 0x81300000) = 4;
					*(s16 *)(data + 0x8130EE52 - 0x81300000) = 13;
					*(s16 *)(data + 0x8130EE56 - 0x81300000) = 255;
					*(s16 *)(data + 0x8130EE5A - 0x81300000) = 17;
					*(s16 *)(data + 0x8130EE62 - 0x81300000) = 33;
					*(s16 *)(data + 0x8130EE66 - 0x81300000) = 50;
					*(u32 *)(data + 0x8130EEA0 - 0x81300000) = 0xB1630016;
					*(u32 *)(data + 0x8130EEA8 - 0x81300000) = 0x9943000B;
					*(u32 *)(data + 0x8130EEAC - 0x81300000) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EEB0 - 0x81300000) = 0x9B830037;
					*(u32 *)(data + 0x8130EEB4 - 0x81300000) = 0x99630035;
					
					memcpy(data + 0x8137F138 - 0x81300000, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1766752:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300612 - 0x81300000) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300882 - 0x81300000) = 1;
				*(s16 *)(data + 0x813008A6 - 0x81300000) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81301628 - 0x81300000) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302F50 - 0x81300000) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (newmode->viTVMode >> 2 != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130F306 - 0x81300000) = 10;
					*(s16 *)(data + 0x8130F30A - 0x81300000) = 255;
					*(s16 *)(data + 0x8130F316 - 0x81300000) = 7;
					*(s16 *)(data + 0x8130F31A - 0x81300000) = 6;
					*(s16 *)(data + 0x8130F31E - 0x81300000) = 5;
					*(s16 *)(data + 0x8130F322 - 0x81300000) = 16;
					*(s16 *)(data + 0x8130F326 - 0x81300000) = 18;
					*(s16 *)(data + 0x8130F32A - 0x81300000) = 20;
					*(s16 *)(data + 0x8130F332 - 0x81300000) = 40;
					*(s16 *)(data + 0x8130F336 - 0x81300000) = 60;
					*(u32 *)(data + 0x8130F370 - 0x81300000) = 0xB3E30016;
					*(u32 *)(data + 0x8130F378 - 0x81300000) = 0x9963000B;
					*(u32 *)(data + 0x8130F37C - 0x81300000) = 0x9BC3001C;
					*(u32 *)(data + 0x8130F380 - 0x81300000) = 0x9BA30037;
					*(u32 *)(data + 0x8130F384 - 0x81300000) = 0x99430035;
					
					memcpy(data + 0x81382470 - 0x81300000, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.2");
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GC6E01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3779808:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005C24 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005D2C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005D50 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80036598 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80036688 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80036740 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GC6J01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3699680:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005CA4 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GC6P01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4096576:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005D1C - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005E24 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005E48 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x800387E4 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x800388D4 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x8003898C - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDKEA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3009280:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD)
					*(s16 *)(data + 0x801F1FF2 - 0x800056C0 + 0x2600) = 128;
				else
					*(s16 *)(data + 0x801F1FF2 - 0x800056C0 + 0x2600) = 64;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDKJA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3075488:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD)
					*(s16 *)(data + 0x801FF53E - 0x800056C0 + 0x2600) = 128;
				else
					*(s16 *)(data + 0x801FF53E - 0x800056C0 + 0x2600) = 64;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDXEA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2039904:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x800EFF3A - 0x800129E0 + 0x2520) = 128;
					
					*(s16 *)(data + 0x800F0082 - 0x800129E0 + 0x2520) = 128;
					
					*(s16 *)(data + 0x800F0502 - 0x800129E0 + 0x2520) = 128;
				} else {
					*(s16 *)(data + 0x800EFF3A - 0x800129E0 + 0x2520) = 64;
					
					*(s16 *)(data + 0x800F0082 - 0x800129E0 + 0x2520) = 64;
					
					*(s16 *)(data + 0x800F0502 - 0x800129E0 + 0x2520) = 64;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDXJA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1853024:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x800E65EE - 0x80012260 + 0x2520) = 128;
					
					*(s16 *)(data + 0x800E6736 - 0x80012260 + 0x2520) = 128;
					
					*(s16 *)(data + 0x800E6BB6 - 0x80012260 + 0x2520) = 128;
				} else {
					*(s16 *)(data + 0x800E65EE - 0x80012260 + 0x2520) = 64;
					
					*(s16 *)(data + 0x800E6736 - 0x80012260 + 0x2520) = 64;
					
					*(s16 *)(data + 0x800E6BB6 - 0x80012260 + 0x2520) = 64;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GFZE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1414848:
				// Skip exception stubbing.
				*(u32 *)(data + 0x80005608 - 0x800055E0 + 0x25E0) = 0x48000038;
				*(u32 *)(data + 0x8000560C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005610 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005618 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000561C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005620 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005624 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005628 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000562C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005630 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005634 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005638 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000563C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005640 - 0x800055E0 + 0x25E0) = 0x38000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GFZJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1412928:
				// Skip exception stubbing and force Japanese language.
				*(u32 *)(data + 0x80005608 - 0x800055E0 + 0x25E0) = 0x48000050;
				*(u32 *)(data + 0x8000560C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005610 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005618 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000561C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005620 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005624 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005628 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000562C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005630 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005634 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005638 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000563C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005640 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005644 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005648 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000564C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005650 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005654 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005658 - 0x800055E0 + 0x25E0) = 0x38000005;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GFZP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1425760:
				// Skip exception stubbing.
				*(u32 *)(data + 0x80005608 - 0x800055E0 + 0x25E0) = 0x48000038;
				*(u32 *)(data + 0x8000560C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005610 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005618 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000561C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005620 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005624 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005628 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000562C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005630 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005634 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005638 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x8000563C - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005640 - 0x800055E0 + 0x25E0) = 0x38000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GGIJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 205696:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x8001501C - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 266324:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x807023F0 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GH9P52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3420832:
				// Use debug monitor address instead of 0x81800000/0x83000000.
				*(u32 *)(data + 0x800142C8 - 0x800034A0 + 0x4A0) = 0x3D608000;
				*(u32 *)(data + 0x800142CC - 0x800034A0 + 0x4A0) = 0x816B00EC;
				
				*(u32 *)(data + 0x80014598 - 0x800034A0 + 0x4A0) = 0x3D208000;
				*(u32 *)(data + 0x8001459C - 0x800034A0 + 0x4A0) = 0x812900EC;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GOND69", 6) || !strncmp(gameID, "GONE69", 6) || !strncmp(gameID, "GONF69", 6) || !strncmp(gameID, "GONP69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 277472:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80014C54 - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 339500:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80701C38 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GONJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 276416:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80014C54 - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 338376:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80701C38 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GOYD69", 6) || !strncmp(gameID, "GOYE69", 6) || !strncmp(gameID, "GOYF69", 6) || !strncmp(gameID, "GOYP69", 6) || !strncmp(gameID, "GOYS69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 207232:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x8001501C - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 268372:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x807023F0 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPOE8P", 6) && (dataType == PATCH_DOL || dataType == PATCH_DOL_PRS)) {
		switch (length) {
			case 204480:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x806C7A76 - 0x806C7500 + 0x2600) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7A82 - 0x806C7500 + 0x2600) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x806C7E2A - 0x806C7500 + 0x2600) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7E32 - 0x806C7500 + 0x2600) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 213952:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F2BE - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F2C6 - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F566 - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F56E - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 5233088:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8036F5C6 - 0x8000F740 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8036F5CE - 0x8000F740 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPOJ8P", 6) && (dataType == PATCH_DOL || dataType == PATCH_DOL_PRS)) {
		switch (length) {
			case 211360:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x806C7B36 - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7B42 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x806C7EEA - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7EF2 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 214880:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F2BE - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F2C6 - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F566 - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F56E - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 5234880:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8036F1FA - 0x8000F740 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8036F202 - 0x8000F740 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPSE8P", 6) && (dataType == PATCH_DOL || dataType == PATCH_DOL_PRS)) {
		switch (length) {
			case 211648:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x806C7C92 - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7C9E - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x806C805E - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C8066 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 220800:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F742 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F74A - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4785536:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x800590F6 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x800590FE - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x80315B92 - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80315B9A - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPSJ8P", 6) && (dataType == PATCH_DOL || dataType == PATCH_DOL_PRS)) {
		switch (length) {
			case 212928:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x806C7B36 - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7B42 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x806C7EEA - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7EF2 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 222048:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F606 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F60E - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4781856:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x800591A2 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x800591AA - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x80314B3A - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80314B42 - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPSP8P", 6) && (dataType == PATCH_DOL || dataType == PATCH_DOL_PRS)) {
		switch (length) {
			case 212000:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x806C7C92 - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C7C9E - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x806C805E - 0x806C75C0 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x806C8066 - 0x806C75C0 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 221152:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F742 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F74A - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4794720:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8005925E - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x80059266 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x803167F2 - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x803167FA - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPXE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2065728:
				// Move structures from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x80005B56 - 0x800056C0 + 0x2600) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B5A - 0x800056C0 + 0x2600) = ((u32)addr & 0xFFFF);
				*(s16 *)(data + 0x80005B72 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B76 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1940480:
				// Move structures from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x80005B56 - 0x800056C0 + 0x2600) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B5A - 0x800056C0 + 0x2600) = ((u32)addr & 0xFFFF);
				*(s16 *)(data + 0x80005B72 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B76 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPXP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2076160:
				// Move structures from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x80005B56 - 0x800056C0 + 0x2600) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B5A - 0x800056C0 + 0x2600) = ((u32)addr & 0xFFFF);
				*(s16 *)(data + 0x80005B72 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) + 0x8000) >> 16;
				*(s16 *)(data + 0x80005B76 - 0x800056C0 + 0x2600) = (((u32)addr + 0xE00) & 0xFFFF);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GR8D69", 6) || !strncmp(gameID, "GR8E69", 6) || !strncmp(gameID, "GR8F69", 6) || !strncmp(gameID, "GR8P69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 206016:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x4A0) = 0x387D2720;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 260164:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x500) = 0x387D2720;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GRZJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 206240:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x4A0) = 0x387D2800;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 260852:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x500) = 0x387D2800;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GS2D78", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2308384:
				// Fix race condition with DVD callback.
				*(u32 *)(data + 0x80155A08 - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80155A0C - 0x80014140 + 0x2620) = 0x3C800000 | (0x80155AD8 + 0x8000) >> 16;
				*(u32 *)(data + 0x80155A10 - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x80155A14 - 0x80014140 + 0x2620) = 0x7C7D1B78;
				*(u32 *)(data + 0x80155A18 - 0x80014140 + 0x2620) = 0x38E40000 | (0x80155AD8 & 0xFFFF);
				*(u32 *)(data + 0x80155A1C - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80155A20 - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155A24 - 0x80014140 + 0x2620) = 0x7FA4EB78;
				*(u32 *)(data + 0x80155A28 - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155A2C - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155A30 - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x801996C8, (u32 *)0x80155A30);
				*(u32 *)(data + 0x80155A70 - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80155A74 - 0x80014140 + 0x2620) = 0x3C600000 | (0x80155AD8 + 0x8000) >> 16;
				*(u32 *)(data + 0x80155A78 - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x80155A7C - 0x80014140 + 0x2620) = 0x7F84E378;
				*(u32 *)(data + 0x80155A80 - 0x80014140 + 0x2620) = 0x38E30000 | (0x80155AD8 & 0xFFFF);
				*(u32 *)(data + 0x80155A84 - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80155A88 - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155A8C - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155A90 - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155A94 - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x801996C8, (u32 *)0x80155A94);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GS2E78", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2275008:
				// Fix race condition with DVD callback.
				*(u32 *)(data + 0x80151A50 - 0x80014180 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80151A54 - 0x80014180 + 0x2620) = 0x3C800000 | (0x80151B20 + 0x8000) >> 16;
				*(u32 *)(data + 0x80151A58 - 0x80014180 + 0x2620) = 0x980D9978;
				*(u32 *)(data + 0x80151A5C - 0x80014180 + 0x2620) = 0x7C7D1B78;
				*(u32 *)(data + 0x80151A60 - 0x80014180 + 0x2620) = 0x38E40000 | (0x80151B20 & 0xFFFF);
				*(u32 *)(data + 0x80151A64 - 0x80014180 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80151A68 - 0x80014180 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80151A6C - 0x80014180 + 0x2620) = 0x7FA4EB78;
				*(u32 *)(data + 0x80151A70 - 0x80014180 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80151A74 - 0x80014180 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80151A78 - 0x80014180 + 0x2620) = branchAndLink((u32 *)0x8019414C, (u32 *)0x80151A78);
				*(u32 *)(data + 0x80151AB8 - 0x80014180 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80151ABC - 0x80014180 + 0x2620) = 0x3C600000 | (0x80151B20 + 0x8000) >> 16;
				*(u32 *)(data + 0x80151AC0 - 0x80014180 + 0x2620) = 0x980D9978;
				*(u32 *)(data + 0x80151AC4 - 0x80014180 + 0x2620) = 0x7F84E378;
				*(u32 *)(data + 0x80151AC8 - 0x80014180 + 0x2620) = 0x38E30000 | (0x80151B20 & 0xFFFF);
				*(u32 *)(data + 0x80151ACC - 0x80014180 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80151AD0 - 0x80014180 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80151AD4 - 0x80014180 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80151AD8 - 0x80014180 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80151ADC - 0x80014180 + 0x2620) = branchAndLink((u32 *)0x8019414C, (u32 *)0x80151ADC);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GS2F78", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2306400:
				// Fix race condition with DVD callback.
				*(u32 *)(data + 0x80155A4C - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80155A50 - 0x80014140 + 0x2620) = 0x3C800000 | (0x80155B1C + 0x8000) >> 16;
				*(u32 *)(data + 0x80155A54 - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x80155A58 - 0x80014140 + 0x2620) = 0x7C7D1B78;
				*(u32 *)(data + 0x80155A5C - 0x80014140 + 0x2620) = 0x38E40000 | (0x80155B1C & 0xFFFF);
				*(u32 *)(data + 0x80155A60 - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80155A64 - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155A68 - 0x80014140 + 0x2620) = 0x7FA4EB78;
				*(u32 *)(data + 0x80155A6C - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155A70 - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155A74 - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x80199784, (u32 *)0x80155A74);
				*(u32 *)(data + 0x80155AB4 - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80155AB8 - 0x80014140 + 0x2620) = 0x3C600000 | (0x80155B1C + 0x8000) >> 16;
				*(u32 *)(data + 0x80155ABC - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x80155AC0 - 0x80014140 + 0x2620) = 0x7F84E378;
				*(u32 *)(data + 0x80155AC4 - 0x80014140 + 0x2620) = 0x38E30000 | (0x80155B1C & 0xFFFF);
				*(u32 *)(data + 0x80155AC8 - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80155ACC - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155AD0 - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155AD4 - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155AD8 - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x80199784, (u32 *)0x80155AD8);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GS2P78", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2303168:
				// Fix race condition with DVD callback.
				*(u32 *)(data + 0x801559E4 - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x801559E8 - 0x80014140 + 0x2620) = 0x3C800000 | (0x80155AB4 + 0x8000) >> 16;
				*(u32 *)(data + 0x801559EC - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x801559F0 - 0x80014140 + 0x2620) = 0x7C7D1B78;
				*(u32 *)(data + 0x801559F4 - 0x80014140 + 0x2620) = 0x38E40000 | (0x80155AB4 & 0xFFFF);
				*(u32 *)(data + 0x801559F8 - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x801559FC - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155A00 - 0x80014140 + 0x2620) = 0x7FA4EB78;
				*(u32 *)(data + 0x80155A04 - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155A08 - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155A0C - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x801996B8, (u32 *)0x80155A0C);
				*(u32 *)(data + 0x80155A4C - 0x80014140 + 0x2620) = 0x38000001;
				*(u32 *)(data + 0x80155A50 - 0x80014140 + 0x2620) = 0x3C600000 | (0x80155AB4 + 0x8000) >> 16;
				*(u32 *)(data + 0x80155A54 - 0x80014140 + 0x2620) = 0x980D99F8;
				*(u32 *)(data + 0x80155A58 - 0x80014140 + 0x2620) = 0x7F84E378;
				*(u32 *)(data + 0x80155A5C - 0x80014140 + 0x2620) = 0x38E30000 | (0x80155AB4 & 0xFFFF);
				*(u32 *)(data + 0x80155A60 - 0x80014140 + 0x2620) = 0x7FC5F378;
				*(u32 *)(data + 0x80155A64 - 0x80014140 + 0x2620) = 0x38610008;
				*(u32 *)(data + 0x80155A68 - 0x80014140 + 0x2620) = 0x38C00000;
				*(u32 *)(data + 0x80155A6C - 0x80014140 + 0x2620) = 0x39000002;
				*(u32 *)(data + 0x80155A70 - 0x80014140 + 0x2620) = branchAndLink((u32 *)0x801996B8, (u32 *)0x80155A70);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GUMD52", 6) || !strncmp(gameID, "GUMP52", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 3314144:
				// Use debug monitor address instead of 0x81800000/0x83000000.
				*(u32 *)(data + 0x8000DD4C - 0x800034A0 + 0x4A0) = 0x3D608000;
				*(u32 *)(data + 0x8000DD50 - 0x800034A0 + 0x4A0) = 0x816B00EC;
				
				*(u32 *)(data + 0x8000DFD4 - 0x800034A0 + 0x4A0) = 0x3C608000;
				*(u32 *)(data + 0x8000DFD8 - 0x800034A0 + 0x4A0) = 0x806300EC;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GUME52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3314080:
				// Use debug monitor address instead of 0x81800000/0x83000000.
				*(u32 *)(data + 0x8000DD4C - 0x800034A0 + 0x4A0) = 0x3D608000;
				*(u32 *)(data + 0x8000DD50 - 0x800034A0 + 0x4A0) = 0x816B00EC;
				
				*(u32 *)(data + 0x8000DFD4 - 0x800034A0 + 0x4A0) = 0x3C608000;
				*(u32 *)(data + 0x8000DFD8 - 0x800034A0 + 0x4A0) = 0x806300EC;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWJE52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3420512:
				// Use debug monitor address instead of 0x81800000/0x83000000.
				*(u32 *)(data + 0x800142C8 - 0x800034A0 + 0x4A0) = 0x3D608000;
				*(u32 *)(data + 0x800142CC - 0x800034A0 + 0x4A0) = 0x816B00EC;
				
				*(u32 *)(data + 0x80014598 - 0x800034A0 + 0x4A0) = 0x3D208000;
				*(u32 *)(data + 0x8001459C - 0x800034A0 + 0x4A0) = 0x812900EC;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTEA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 839584:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD)
					*(s16 *)(data + 0x8003758E - 0x8000AEA0 + 0x2520) = 128;
				else
					*(s16 *)(data + 0x8003758E - 0x8000AEA0 + 0x2520) = 64;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTJA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 804544:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD)
					*(s16 *)(data + 0x8003740A - 0x8000AE60 + 0x2520) = 128;
				else
					*(s16 *)(data + 0x8003740A - 0x8000AE60 + 0x2520) = 64;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTPA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 834912:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD)
					*(s16 *)(data + 0x800374D6 - 0x8000AE60 + 0x2520) = 128;
				else
					*(s16 *)(data + 0x800374D6 - 0x8000AE60 + 0x2520) = 64;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4333056:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F614 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005C8AC - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005C960 - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005C984 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086930 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086A34 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800875BC - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4191264:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F50C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005AF90 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005AFF8 - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005B01C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800842F8 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800843FC - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80084E70 - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4573216:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F11C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CC60 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CD14 - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005CD38 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80087AD4 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80087BD8 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80088574 - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "P2ME01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 508384:
				// Move persistent structure from 0x80001798 to 0x80000198.
				*(s16 *)(data + 0x800056FA - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80005702 - 0x800056C0 + 0x2600) = 0x01A0;
				*(s16 *)(data + 0x8000571A - 0x800056C0 + 0x2600) = 0x01C7;
				*(s16 *)(data + 0x8000572E - 0x800056C0 + 0x2600) = 0x01C8;
				*(s16 *)(data + 0x80005742 - 0x800056C0 + 0x2600) = 0x01C8;
				*(s16 *)(data + 0x80005772 - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80005906 - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x800059A2 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005A12 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005CE2 - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80005D3A - 0x800056C0 + 0x2600) = 0x019A;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 832960:
				// Move persistent structure from 0x80001798 to 0x80000198.
				*(s16 *)(data + 0x800056FA - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x8000585E - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000586E - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x800058A2 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x800059DE - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005F4A - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000604E - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80006052 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80006266 - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006282 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x800062FA - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006316 - 0x800056C0 + 0x2600) = 0x0198;
				
				*(s16 *)(data + 0x8000680E - 0x800056C0 + 0x2600) = 0x0198;
				
				*(s16 *)(data + 0x800068B6 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000697E - 0x800056C0 + 0x2600) = 0x01C8;
				
				*(s16 *)(data + 0x80006A4A - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006A52 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80006A62 - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80006A7E - 0x800056C0 + 0x2600) = 0x01A0;
				*(s16 *)(data + 0x80006A92 - 0x800056C0 + 0x2600) = 0x01C7;
				*(s16 *)(data + 0x80006AAE - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006AFE - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006B2A - 0x800056C0 + 0x2600) = 0x0199;
				
				*(s16 *)(data + 0x80006EF2 - 0x800056C0 + 0x2600) = 0x0199;
				
				*(s16 *)(data + 0x80007126 - 0x800056C0 + 0x2600) = 0x0199;
				
				*(s16 *)(data + 0x80007132 - 0x800056C0 + 0x2600) = 0x0199;
				
				*(s16 *)(data + 0x80007142 - 0x800056C0 + 0x2600) = 0x0199;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PC6E01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 508672:
				// Move persistent structure from 0x80001798 to 0x80000198.
				*(s16 *)(data + 0x800056FA - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80005702 - 0x800056C0 + 0x2600) = 0x01A0;
				*(s16 *)(data + 0x8000571A - 0x800056C0 + 0x2600) = 0x01C7;
				*(s16 *)(data + 0x8000572E - 0x800056C0 + 0x2600) = 0x01C8;
				*(s16 *)(data + 0x80005742 - 0x800056C0 + 0x2600) = 0x01C8;
				*(s16 *)(data + 0x8000592A - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000592E - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x80005996 - 0x800056C0 + 0x2600) = 0x0198;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 771104:
				// Move persistent structure from 0x80001798 to 0x80000198.
				*(s16 *)(data + 0x800056F6 - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x8000579A - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x800057AA - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x800057E2 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005AFE - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005C86 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x80005CDE - 0x800056C0 + 0x2600) = 0x0198;
				
				*(s16 *)(data + 0x8000613E - 0x800056C0 + 0x2600) = 0x0198;
				
				*(s16 *)(data + 0x800061E6 - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000629E - 0x800056C0 + 0x2600) = 0x01C8;
				
				*(s16 *)(data + 0x80006376 - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x8000637E - 0x800056C0 + 0x2600) = 0x0198;
				*(s16 *)(data + 0x8000638E - 0x800056C0 + 0x2600) = 0x019A;
				*(s16 *)(data + 0x800063AA - 0x800056C0 + 0x2600) = 0x01A0;
				*(s16 *)(data + 0x800063BE - 0x800056C0 + 0x2600) = 0x01C7;
				*(s16 *)(data + 0x800063DA - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x80006422 - 0x800056C0 + 0x2600) = 0x0199;
				*(s16 *)(data + 0x8000644A - 0x800056C0 + 0x2600) = 0x0199;
				
				*(s16 *)(data + 0x800068CA - 0x800056C0 + 0x2600) = 0x0199;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PCKJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1181312:
				// Move persistent structure from 0x800017C0 to 0x800001C0.
				*(s16 *)(data + 0x800062DA - 0x80005C80 + 0x2600) = 0x01C0;
				*(s16 *)(data + 0x8000635E - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x80006926 - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x800069EA - 0x80005C80 + 0x2600) = 0x01C0;
				*(s16 *)(data + 0x800069EE - 0x80005C80 + 0x2600) = 0x01C1;
				*(s16 *)(data + 0x80006A1A - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x800076E2 - 0x80005C80 + 0x2600) = 0x01C1;
				*(s16 *)(data + 0x8000783E - 0x80005C80 + 0x2600) = 0x01C1;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 3771872:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005DD0 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PCSJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1177728:
				// Move persistent structure from 0x800017C0 to 0x800001C0.
				*(s16 *)(data + 0x800062DA - 0x80005C80 + 0x2600) = 0x01C0;
				*(s16 *)(data + 0x8000635E - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x80006926 - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x800069EA - 0x80005C80 + 0x2600) = 0x01C0;
				*(s16 *)(data + 0x800069EE - 0x80005C80 + 0x2600) = 0x01C1;
				*(s16 *)(data + 0x80006A1A - 0x80005C80 + 0x2600) = 0x01C0;
				
				*(s16 *)(data + 0x800076E2 - 0x80005C80 + 0x2600) = 0x01C1;
				*(s16 *)(data + 0x8000783E - 0x80005C80 + 0x2600) = 0x01C1;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 3771872:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005DD0 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	}
	return patched;
}

// Overwrite game specific file content
int Patch_GameSpecificFile(void *data, u32 length, const char *gameID, const char *fileName)
{
	void *addr;
	int patched = 0;
	
	if (!strncmp(gameID, "DPOJ8P", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x4) = 0x1800000;
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GCCE01", 6) || !strncmp(gameID, "GCCP01", 6)) {
		if (!strcasecmp(fileName, "ffcc_cli.bin")) {
			*(u16 *)(data + 0x2504) = __builtin_bswap16(0x8004);
			*(u16 *)(data + 0x2506) = __builtin_bswap16(0x20FF);
			*(u16 *)(data + 0x2508) = __builtin_bswap16(0x7018);
			*(u16 *)(data + 0x250A) = __builtin_bswap16(0x705C);
			*(u16 *)(data + 0x250C) = __builtin_bswap16(0x4904);
			*(u16 *)(data + 0x250E) = __builtin_bswap16(0x2001);
			*(u16 *)(data + 0x2510) = __builtin_bswap16(0x7008);
			*(u16 *)(data + 0x2512) = __builtin_bswap16(0x4901);
			*(u16 *)(data + 0x2514) = __builtin_bswap16(0x600C);
			*(u16 *)(data + 0x2516) = __builtin_bswap16(0xE009);
			
			*(u32 *)(data + 0x2518) = __builtin_bswap32(0x03000190);
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GCCJGC", 6)) {
		if (!strcasecmp(fileName, "ffcc_cli.bin")) {
			*(u16 *)(data + 0x132C) = __builtin_bswap16(0x8004);
			*(u16 *)(data + 0x132E) = __builtin_bswap16(0x20FF);
			*(u16 *)(data + 0x1330) = __builtin_bswap16(0x7018);
			*(u16 *)(data + 0x1332) = __builtin_bswap16(0x705C);
			*(u16 *)(data + 0x1334) = __builtin_bswap16(0x4904);
			*(u16 *)(data + 0x1336) = __builtin_bswap16(0x2001);
			*(u16 *)(data + 0x1338) = __builtin_bswap16(0x7008);
			*(u16 *)(data + 0x133A) = __builtin_bswap16(0x4901);
			*(u16 *)(data + 0x133C) = __builtin_bswap16(0x600C);
			*(u16 *)(data + 0x133E) = __builtin_bswap16(0xE009);
			
			*(u32 *)(data + 0x1340) = __builtin_bswap32(0x03000138);
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GHQE7D", 6) || !strncmp(gameID, "GHQP7D", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x24) = 5;
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GMRE70", 6) || !strncmp(gameID, "GMRP70", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x24) = 5;
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GS8P7D", 6)) {
		if (!strcasecmp(fileName, "SPYROCFG_NGC.CFG")) {
			if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
				addr = strnstr(data, "\tHeight:\t\t\t496\r\n", length);
				if (addr) memcpy(addr, "\tHeight:\t\t\t448\r\n", 16);
			}
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXD64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FC8;
			
			*(u32 *)(data + 0x1FC8) = 4;
			*(u32 *)(data + 0x1FCC) = 0x1FE0;
			*(u32 *)(data + 0x1FD0) = 0x1FEE;
			*(u32 *)(data + 0x1FD4) = 0x1FF3;
			*(u32 *)(data + 0x1FD8) = 0x1FFD;
			*(u32 *)(data + 0x1FDC) = 0;
			
			strcpy(data + 0x1FE0, "Clonewars.elf");
			strcpy(data + 0x1FEE, "/pal");
			strcpy(data + 0x1FF3, "/language");
			strcpy(data + 0x1FFD, "1");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXE64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FE4;
			
			*(u32 *)(data + 0x1FE4) = 1;
			*(u32 *)(data + 0x1FE8) = 0x1FF1;
			*(u32 *)(data + 0x1FEC) = 0;
			
			strcpy(data + 0x1FF1, "Clonewars.elf");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXF64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FC8;
			
			*(u32 *)(data + 0x1FC8) = 4;
			*(u32 *)(data + 0x1FCC) = 0x1FE0;
			*(u32 *)(data + 0x1FD0) = 0x1FEE;
			*(u32 *)(data + 0x1FD4) = 0x1FF3;
			*(u32 *)(data + 0x1FD8) = 0x1FFD;
			*(u32 *)(data + 0x1FDC) = 0;
			
			strcpy(data + 0x1FE0, "Clonewars.elf");
			strcpy(data + 0x1FEE, "/pal");
			strcpy(data + 0x1FF3, "/language");
			strcpy(data + 0x1FFD, "2");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXI64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FC8;
			
			*(u32 *)(data + 0x1FC8) = 4;
			*(u32 *)(data + 0x1FCC) = 0x1FE0;
			*(u32 *)(data + 0x1FD0) = 0x1FEE;
			*(u32 *)(data + 0x1FD4) = 0x1FF3;
			*(u32 *)(data + 0x1FD8) = 0x1FFD;
			*(u32 *)(data + 0x1FDC) = 0;
			
			strcpy(data + 0x1FE0, "Clonewars.elf");
			strcpy(data + 0x1FEE, "/pal");
			strcpy(data + 0x1FF3, "/language");
			strcpy(data + 0x1FFD, "4");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXJ13", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FC0;
			
			*(u32 *)(data + 0x1FC0) = 4;
			*(u32 *)(data + 0x1FC4) = 0x1FDB;
			*(u32 *)(data + 0x1FC8) = 0x1FE9;
			*(u32 *)(data + 0x1FCC) = 0x1FF3;
			*(u32 *)(data + 0x1FD0) = 0x1FF5;
			*(u32 *)(data + 0x1FD4) = 0;
			
			strcpy(data + 0x1FDB, "Clonewars.elf");
			strcpy(data + 0x1FE9, "/language");
			strcpy(data + 0x1FF3, "5");
			strcpy(data + 0x1FF5, "/japanese");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXP64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FDC;
			
			*(u32 *)(data + 0x1FDC) = 2;
			*(u32 *)(data + 0x1FE0) = 0x1FEC;
			*(u32 *)(data + 0x1FE4) = 0x1FFA;
			*(u32 *)(data + 0x1FE8) = 0;
			
			strcpy(data + 0x1FEC, "Clonewars.elf");
			strcpy(data + 0x1FFA, "/pal");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GSXS64", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			u32 args = *(u32 *)(data + 0x8);
			if (args) memset(data + args, 0, length - args);
			
			*(u32 *)(data + 0x8) = 0x1FC8;
			
			*(u32 *)(data + 0x1FC8) = 4;
			*(u32 *)(data + 0x1FCC) = 0x1FE0;
			*(u32 *)(data + 0x1FD0) = 0x1FEE;
			*(u32 *)(data + 0x1FD4) = 0x1FF3;
			*(u32 *)(data + 0x1FD8) = 0x1FFD;
			*(u32 *)(data + 0x1FDC) = 0;
			
			strcpy(data + 0x1FE0, "Clonewars.elf");
			strcpy(data + 0x1FEE, "/pal");
			strcpy(data + 0x1FF3, "/language");
			strcpy(data + 0x1FFD, "3");
			
			print_gecko("Patched:[%s]\n", fileName);
			patched++;
		}
	}
	return patched;
}

int Patch_GameSpecificHypervisor(void *data, u32 length, const char *gameID, int dataType)
{
	int patched = 0;
	
	if (dataType == PATCH_BS2) {
		switch (length) {
			case 1435168:
				*(s16 *)(data + 0x813007C2 - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1448256:
				*(s16 *)(data + 0x813007DA - 0x81300000) = 0x0C00;
				
				*(u32 *)(data + 0x813011D0 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x813011D0);
				*(u32 *)(data + 0x813011DC - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x813011DC);
				*(u32 *)(data + 0x81301258 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81301258);
				*(u32 *)(data + 0x81301260 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x81301260);
				
				*(s16 *)(data + 0x81301422 - 0x81300000) = 0x0C00;
				*(s16 *)(data + 0x81301476 - 0x81300000) = 0x0C00;
				*(u32 *)(data + 0x81301484 - 0x81300000) = 0x80050004;
				*(u32 *)(data + 0x813014A8 - 0x81300000) = 0x90050004;
				
				*(s16 *)(data + 0x8130170E - 0x81300000) = 0x0C00;
				
				*(s16 *)(data + 0x813019AE - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1449824:
				*(s16 *)(data + 0x81300B4E - 0x81300000) = 0x0C00;
				
				*(u32 *)(data + 0x81301510 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81301510);
				*(u32 *)(data + 0x8130151C - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x8130151C);
				*(u32 *)(data + 0x81301598 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81301598);
				*(u32 *)(data + 0x813015A0 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x813015A0);
				
				*(s16 *)(data + 0x81301762 - 0x81300000) = 0x0C00;
				*(s16 *)(data + 0x813017B6 - 0x81300000) = 0x0C00;
				*(u32 *)(data + 0x813017C4 - 0x81300000) = 0x80050004;
				*(u32 *)(data + 0x813017E8 - 0x81300000) = 0x90050004;
				
				*(s16 *)(data + 0x81301A4E - 0x81300000) = 0x0C00;
				
				*(s16 *)(data + 0x81301CEE - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "DEV  Revision 1.0");
				patched++;
				break;
			case 1583072:
				*(s16 *)(data + 0x813005EA - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.1");
				patched++;
				break;
			case 1763040:
				*(s16 *)(data + 0x813005EA - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1760128:
				*(s16 *)(data + 0x8130082E - 0x81300000) = 0x0C00;
				
				*(u32 *)(data + 0x81301258 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81301258);
				*(u32 *)(data + 0x81301264 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x81301264);
				*(u32 *)(data + 0x813012D8 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x813012D8);
				*(u32 *)(data + 0x813012E0 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x813012E0);
				
				*(s16 *)(data + 0x81301496 - 0x81300000) = 0x0C00;
				*(s16 *)(data + 0x813014E6 - 0x81300000) = 0x0C00;
				*(u32 *)(data + 0x813014F4 - 0x81300000) = 0x80050004;
				*(u32 *)(data + 0x81301518 - 0x81300000) = 0x90050004;
				
				*(s16 *)(data + 0x81301772 - 0x81300000) = 0x0C00;
				
				*(s16 *)(data + 0x813018A6 - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1561760:
				*(s16 *)(data + 0x813005EA - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "MPAL Revision 1.1");
				patched++;
				break;
			case 1607552:
				*(s16 *)(data + 0x81300A5A - 0x81300000) = 0x0C00;
				
				*(u32 *)(data + 0x81301FC8 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81301FC8);
				*(u32 *)(data + 0x81301FD4 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x81301FD4);
				*(u32 *)(data + 0x81302050 - 0x81300000) = branchAndLink(SET_IRQ_HANDLER, (u32 *)0x81302050);
				*(u32 *)(data + 0x81302058 - 0x81300000) = branchAndLink(UNMASK_IRQ,      (u32 *)0x81302058);
				
				*(s16 *)(data + 0x8130221A - 0x81300000) = 0x0C00;
				*(s16 *)(data + 0x8130226E - 0x81300000) = 0x0C00;
				*(u32 *)(data + 0x8130227C - 0x81300000) = 0x80050004;
				*(u32 *)(data + 0x813022A0 - 0x81300000) = 0x90050004;
				
				*(s16 *)(data + 0x81302506 - 0x81300000) = 0x0C00;
				
				*(s16 *)(data + 0x813027A6 - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "TDEV Revision 1.1");
				patched++;
				break;
			case 1586336:
				*(s16 *)(data + 0x81300926 - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1587488:
				*(s16 *)(data + 0x81300926 - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1766752:
				*(s16 *)(data + 0x813006DA - 0x81300000) = 0x0C00;
				
				print_gecko("Patched:[%s]\n", "PAL  Revision 1.2");
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "D56J01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3055616:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800AF1B4 - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "G3FD69", 6) || !strncmp(gameID, "G3FE69", 6) || !strncmp(gameID, "G3FF69", 6) || !strncmp(gameID, "G3FP69", 6) || !strncmp(gameID, "G3FS69", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4880320:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80184792 - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x8018479A - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x801847AA - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x801847B6 - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x801847C2 - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GHBE7D", 6) || !strncmp(gameID, "GHBP7D", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 647104:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x80004556 - 0x80003100 + 0x100) =  0x0304;
				*(s16 *)(data + 0x80004562 - 0x80003100 + 0x100) = -0x0304;
				*(s16 *)(data + 0x80004566 - 0x80003100 + 0x100) =  0x0304;
				
				// Fix ISI exception check.
				*(u32 *)(data + 0x80005D84 - 0x80003100 + 0x100) = 0x7CBA02A6;
				*(u32 *)(data + 0x80005D98 - 0x80003100 + 0x100) = 0x7CBA02A6;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GISE36", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3266176:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x8010A74A - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x8010A752 - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x8010A762 - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x8010A76E - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x8010A77A - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GISP36", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3268768:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x8010AEE2 - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x8010AEEA - 0x800055E0 + 0x25E0) = 0x0304;
				*(s16 *)(data + 0x8010AEFA - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x8010AF06 - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				*(s16 *)(data + 0x8010AF12 - 0x800055E0 + 0x25E0) = 56 * sizeof(u32);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GMIE70", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4147712:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x801EB86A - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB872 - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB876 - 0x80009080 + 0x25E0) = -0x0304;
				*(s16 *)(data + 0x801EB886 - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB8CE - 0x80009080 + 0x25E0) = -0x0308;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GMIJ70", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4196672:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x801EB51E - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB526 - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB52A - 0x80009080 + 0x25E0) = -0x0304;
				*(s16 *)(data + 0x801EB53A - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB582 - 0x80009080 + 0x25E0) = -0x0308;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GMIP70", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4147776:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x801EB8A2 - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB8AA - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB8AE - 0x80009080 + 0x25E0) = -0x0304;
				*(s16 *)(data + 0x801EB8BE - 0x80009080 + 0x25E0) =  0x0304;
				*(s16 *)(data + 0x801EB906 - 0x80009080 + 0x25E0) = -0x0308;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPAE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2825472:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800B09EC - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPAJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3055584:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800AF1C0 - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPAP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2745728:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800B538C - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GPAU01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2720352:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800B5304 - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GSXD64", 6) || !strncmp(gameID, "GSXF64", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 3728640:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80004ACE - 0x80003100 + 0x100) = 13;
				*(s16 *)(data + 0x80004ADA - 0x80003100 + 0x100) = 0x0304;
				*(s16 *)(data + 0x80004B02 - 0x80003100 + 0x100) = 13 * sizeof(u32);
				*(s16 *)(data + 0x80004B06 - 0x80003100 + 0x100) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GSXE64", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3728544:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80004ACE - 0x80003100 + 0x100) = 13;
				*(s16 *)(data + 0x80004ADA - 0x80003100 + 0x100) = 0x0304;
				*(s16 *)(data + 0x80004B02 - 0x80003100 + 0x100) = 13 * sizeof(u32);
				*(s16 *)(data + 0x80004B06 - 0x80003100 + 0x100) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GSXI64", 6) || !strncmp(gameID, "GSXS64", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 3728640:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80004ACE - 0x80003100 + 0x100) = 13;
				*(s16 *)(data + 0x80004ADA - 0x80003100 + 0x100) = 0x0304;
				*(s16 *)(data + 0x80004B02 - 0x80003100 + 0x100) = 13 * sizeof(u32);
				*(s16 *)(data + 0x80004B06 - 0x80003100 + 0x100) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GSXJ13", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3730336:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80004AEA - 0x80003100 + 0x100) = 13;
				*(s16 *)(data + 0x80004AF6 - 0x80003100 + 0x100) = 0x0304;
				*(s16 *)(data + 0x80004B1E - 0x80003100 + 0x100) = 13 * sizeof(u32);
				*(s16 *)(data + 0x80004B22 - 0x80003100 + 0x100) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GSXP64", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3728640:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x80004ACE - 0x80003100 + 0x100) = 13;
				*(s16 *)(data + 0x80004ADA - 0x80003100 + 0x100) = 0x0304;
				*(s16 *)(data + 0x80004B02 - 0x80003100 + 0x100) = 13 * sizeof(u32);
				*(s16 *)(data + 0x80004B06 - 0x80003100 + 0x100) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GT6E70", 6) || !strncmp(gameID, "GT6P70", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4248480:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x801FA75A - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA762 - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA766 - 0x800056C0 + 0x2600) = -0x0304;
				*(s16 *)(data + 0x801FA776 - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA7BE - 0x800056C0 + 0x2600) = -0x0308;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GT6J70", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4255968:
				// Move up jump to DSI exception handler.
				*(s16 *)(data + 0x801FA78A - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA792 - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA796 - 0x800056C0 + 0x2600) = -0x0304;
				*(s16 *)(data + 0x801FA7A6 - 0x800056C0 + 0x2600) =  0x0304;
				*(s16 *)(data + 0x801FA7EE - 0x800056C0 + 0x2600) = -0x0308;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4333056:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AE0F2 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4191264:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802A8B1A - 0x800056A0 + 0x2600) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4573216:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802B0046 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PCKJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3093408:
				// Skip second EXIInit call.
				*(u32 *)(data + 0x800B0464 - 0x80005840 + 0x26C0) = 0x60000000;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	}
	return patched;
}

void Patch_GameSpecificVideo(void *data, u32 length, const char *gameID, int dataType)
{
#define SET_VI_WIDTH(rmodeobj, width) \
{ \
	GXRModeObj *rmode = (GXRModeObj *) (rmodeobj); \
	rmode->viXOrigin = (u16) ((720 - (width)) / 2); \
	rmode->viWidth = (u16) (width); \
}
	if (!strncmp(gameID, "GAEJ01", 6) && dataType == PATCH_DOL) {
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
				
				SET_VI_WIDTH(data + 0x800D6DB0 - 0x800D67E0 + 0xD37E0, 640);
				SET_VI_WIDTH(data + 0x800D6DEC - 0x800D67E0 + 0xD37E0, 640);
				SET_VI_WIDTH(data + 0x800D6E28 - 0x800D67E0 + 0xD37E0, 640);
				SET_VI_WIDTH(data + 0x800D6E64 - 0x800D67E0 + 0xD37E0, 704);
				SET_VI_WIDTH(data + 0x800D6EA0 - 0x800D67E0 + 0xD37E0, 704);
				
				SET_VI_WIDTH(data + 0x80105DE0 - 0x800D67E0 + 0xD37E0, 704);
				SET_VI_WIDTH(data + 0x80105E1C - 0x800D67E0 + 0xD37E0, 704);
				SET_VI_WIDTH(data + 0x80105E58 - 0x800D67E0 + 0xD37E0, 704);
				SET_VI_WIDTH(data + 0x80105ED0 - 0x800D67E0 + 0xD37E0, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800D8978 - 0x800D83A0 + 0xD53A0, 640);
				SET_VI_WIDTH(data + 0x800D89B4 - 0x800D83A0 + 0xD53A0, 640);
				SET_VI_WIDTH(data + 0x800D89F0 - 0x800D83A0 + 0xD53A0, 640);
				SET_VI_WIDTH(data + 0x800D8A2C - 0x800D83A0 + 0xD53A0, 704);
				SET_VI_WIDTH(data + 0x800D8A68 - 0x800D83A0 + 0xD53A0, 704);
				
				SET_VI_WIDTH(data + 0x80107BA0 - 0x800D83A0 + 0xD53A0, 704);
				SET_VI_WIDTH(data + 0x80107BDC - 0x800D83A0 + 0xD53A0, 704);
				SET_VI_WIDTH(data + 0x80107C18 - 0x800D83A0 + 0xD53A0, 704);
				SET_VI_WIDTH(data + 0x80107C90 - 0x800D83A0 + 0xD53A0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFE01", 6) && dataType == PATCH_DOL) {
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
				
				SET_VI_WIDTH(data + 0x800AFE58 - 0x800AF860 + 0xAC860, 640);
				SET_VI_WIDTH(data + 0x800AFE94 - 0x800AF860 + 0xAC860, 640);
				SET_VI_WIDTH(data + 0x800AFED0 - 0x800AF860 + 0xAC860, 640);
				SET_VI_WIDTH(data + 0x800AFF0C - 0x800AF860 + 0xAC860, 640);
				SET_VI_WIDTH(data + 0x800AFF48 - 0x800AF860 + 0xAC860, 704);
				SET_VI_WIDTH(data + 0x800AFF84 - 0x800AF860 + 0xAC860, 704);
				
				SET_VI_WIDTH(data + 0x800E15B0 - 0x800AF860 + 0xAC860, 704);
				SET_VI_WIDTH(data + 0x800E15EC - 0x800AF860 + 0xAC860, 704);
				SET_VI_WIDTH(data + 0x800E1628 - 0x800AF860 + 0xAC860, 704);
				SET_VI_WIDTH(data + 0x800E16A0 - 0x800AF860 + 0xAC860, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFJ01", 6) && dataType == PATCH_DOL) {
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
				
				SET_VI_WIDTH(data + 0x800DA9A4 - 0x800AC1C0 + 0xA91C0, 704);
				SET_VI_WIDTH(data + 0x800DA9E0 - 0x800AC1C0 + 0xA91C0, 704);
				SET_VI_WIDTH(data + 0x800DAA1C - 0x800AC1C0 + 0xA91C0, 704);
				SET_VI_WIDTH(data + 0x800DAA58 - 0x800AC1C0 + 0xA91C0, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800DAEE4 - 0x800AC4E0 + 0xA94E0, 704);
				SET_VI_WIDTH(data + 0x800DAF20 - 0x800AC4E0 + 0xA94E0, 704);
				SET_VI_WIDTH(data + 0x800DAF5C - 0x800AC4E0 + 0xA94E0, 704);
				SET_VI_WIDTH(data + 0x800DAF98 - 0x800AC4E0 + 0xA94E0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFP01", 6) && dataType == PATCH_DOL) {
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
				
				SET_VI_WIDTH(data + 0x800BB658 - 0x800BAFC0 + 0xB7FC0, 640);
				SET_VI_WIDTH(data + 0x800BB694 - 0x800BAFC0 + 0xB7FC0, 640);
				SET_VI_WIDTH(data + 0x800BB6D0 - 0x800BAFC0 + 0xB7FC0, 640);
				SET_VI_WIDTH(data + 0x800BB70C - 0x800BAFC0 + 0xB7FC0, 640);
				
				*(u16 *)(data + 0x800BB714 - 0x800BAFC0 + 0xB7FC0) = 574;
				*(u16 *)(data + 0x800BB718 - 0x800BAFC0 + 0xB7FC0) = 0;
				*(u16 *)(data + 0x800BB71C - 0x800BAFC0 + 0xB7FC0) = 574;
				
				*(u16 *)(data + 0x800BB750 - 0x800BAFC0 + 0xB7FC0) = 574;
				*(u16 *)(data + 0x800BB754 - 0x800BAFC0 + 0xB7FC0) = 0;
				*(u16 *)(data + 0x800BB758 - 0x800BAFC0 + 0xB7FC0) = 574;
				
				SET_VI_WIDTH(data + 0x800BB748 - 0x800BAFC0 + 0xB7FC0, 704);
				SET_VI_WIDTH(data + 0x800BB784 - 0x800BAFC0 + 0xB7FC0, 704);
				
				SET_VI_WIDTH(data + 0x800EE1C0 - 0x800BAFC0 + 0xB7FC0, 704);
				SET_VI_WIDTH(data + 0x800EE1FC - 0x800BAFC0 + 0xB7FC0, 704);
				SET_VI_WIDTH(data + 0x800EE238 - 0x800BAFC0 + 0xB7FC0, 704);
				SET_VI_WIDTH(data + 0x800EE2B0 - 0x800BAFC0 + 0xB7FC0, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800BB3F8 - 0x800BAD60 + 0xB7D60, 640);
				SET_VI_WIDTH(data + 0x800BB434 - 0x800BAD60 + 0xB7D60, 640);
				SET_VI_WIDTH(data + 0x800BB470 - 0x800BAD60 + 0xB7D60, 640);
				SET_VI_WIDTH(data + 0x800BB4AC - 0x800BAD60 + 0xB7D60, 640);
				
				*(u16 *)(data + 0x800BB4B4 - 0x800BAD60 + 0xB7D60) = 574;
				*(u16 *)(data + 0x800BB4B8 - 0x800BAD60 + 0xB7D60) = 0;
				*(u16 *)(data + 0x800BB4BC - 0x800BAD60 + 0xB7D60) = 574;
				
				*(u16 *)(data + 0x800BB4F0 - 0x800BAD60 + 0xB7D60) = 574;
				*(u16 *)(data + 0x800BB4F4 - 0x800BAD60 + 0xB7D60) = 0;
				*(u16 *)(data + 0x800BB4F8 - 0x800BAD60 + 0xB7D60) = 574;
				
				SET_VI_WIDTH(data + 0x800BB4E8 - 0x800BAD60 + 0xB7D60, 704);
				SET_VI_WIDTH(data + 0x800BB524 - 0x800BAD60 + 0xB7D60, 704);
				
				SET_VI_WIDTH(data + 0x800EEBE0 - 0x800BAD60 + 0xB7D60, 704);
				SET_VI_WIDTH(data + 0x800EEC1C - 0x800BAD60 + 0xB7D60, 704);
				SET_VI_WIDTH(data + 0x800EEC58 - 0x800BAD60 + 0xB7D60, 704);
				SET_VI_WIDTH(data + 0x800EECD0 - 0x800BAD60 + 0xB7D60, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800BB518 - 0x800BAE80 + 0xB7E80, 640);
				SET_VI_WIDTH(data + 0x800BB554 - 0x800BAE80 + 0xB7E80, 640);
				SET_VI_WIDTH(data + 0x800BB590 - 0x800BAE80 + 0xB7E80, 640);
				SET_VI_WIDTH(data + 0x800BB5CC - 0x800BAE80 + 0xB7E80, 640);
				
				*(u16 *)(data + 0x800BB5D4 - 0x800BAE80 + 0xB7E80) = 574;
				*(u16 *)(data + 0x800BB5D8 - 0x800BAE80 + 0xB7E80) = 0;
				*(u16 *)(data + 0x800BB5DC - 0x800BAE80 + 0xB7E80) = 574;
				
				*(u16 *)(data + 0x800BB610 - 0x800BAE80 + 0xB7E80) = 574;
				*(u16 *)(data + 0x800BB614 - 0x800BAE80 + 0xB7E80) = 0;
				*(u16 *)(data + 0x800BB618 - 0x800BAE80 + 0xB7E80) = 574;
				
				SET_VI_WIDTH(data + 0x800BB608 - 0x800BAE80 + 0xB7E80, 704);
				SET_VI_WIDTH(data + 0x800BB644 - 0x800BAE80 + 0xB7E80, 704);
				
				SET_VI_WIDTH(data + 0x800EFAA0 - 0x800BAE80 + 0xB7E80, 704);
				SET_VI_WIDTH(data + 0x800EFADC - 0x800BAE80 + 0xB7E80, 704);
				SET_VI_WIDTH(data + 0x800EFB18 - 0x800BAE80 + 0xB7E80, 704);
				SET_VI_WIDTH(data + 0x800EFB90 - 0x800BAE80 + 0xB7E80, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800BB278 - 0x800BABE0 + 0xB7BE0, 640);
				SET_VI_WIDTH(data + 0x800BB2B4 - 0x800BABE0 + 0xB7BE0, 640);
				SET_VI_WIDTH(data + 0x800BB2F0 - 0x800BABE0 + 0xB7BE0, 640);
				SET_VI_WIDTH(data + 0x800BB32C - 0x800BABE0 + 0xB7BE0, 640);
				
				*(u16 *)(data + 0x800BB334 - 0x800BABE0 + 0xB7BE0) = 574;
				*(u16 *)(data + 0x800BB338 - 0x800BABE0 + 0xB7BE0) = 0;
				*(u16 *)(data + 0x800BB33C - 0x800BABE0 + 0xB7BE0) = 574;
				
				*(u16 *)(data + 0x800BB370 - 0x800BABE0 + 0xB7BE0) = 574;
				*(u16 *)(data + 0x800BB374 - 0x800BABE0 + 0xB7BE0) = 0;
				*(u16 *)(data + 0x800BB378 - 0x800BABE0 + 0xB7BE0) = 574;
				
				SET_VI_WIDTH(data + 0x800BB368 - 0x800BABE0 + 0xB7BE0, 704);
				SET_VI_WIDTH(data + 0x800BB3A4 - 0x800BABE0 + 0xB7BE0, 704);
				
				SET_VI_WIDTH(data + 0x800EEA60 - 0x800BABE0 + 0xB7BE0, 704);
				SET_VI_WIDTH(data + 0x800EEA9C - 0x800BABE0 + 0xB7BE0, 704);
				SET_VI_WIDTH(data + 0x800EEAD8 - 0x800BABE0 + 0xB7BE0, 704);
				SET_VI_WIDTH(data + 0x800EEB50 - 0x800BABE0 + 0xB7BE0, 704);
				
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
				
				SET_VI_WIDTH(data + 0x800BB398 - 0x800BAD00 + 0xB7D00, 640);
				SET_VI_WIDTH(data + 0x800BB3D4 - 0x800BAD00 + 0xB7D00, 640);
				SET_VI_WIDTH(data + 0x800BB410 - 0x800BAD00 + 0xB7D00, 640);
				SET_VI_WIDTH(data + 0x800BB44C - 0x800BAD00 + 0xB7D00, 640);
				
				*(u16 *)(data + 0x800BB454 - 0x800BAD00 + 0xB7D00) = 574;
				*(u16 *)(data + 0x800BB458 - 0x800BAD00 + 0xB7D00) = 0;
				*(u16 *)(data + 0x800BB45E - 0x800BAD00 + 0xB7D00) = 574;
				
				*(u16 *)(data + 0x800BB490 - 0x800BAD00 + 0xB7D00) = 574;
				*(u16 *)(data + 0x800BB494 - 0x800BAD00 + 0xB7D00) = 0;
				*(u16 *)(data + 0x800BB498 - 0x800BAD00 + 0xB7D00) = 574;
				
				SET_VI_WIDTH(data + 0x800BB488 - 0x800BAD00 + 0xB7D00, 704);
				SET_VI_WIDTH(data + 0x800BB4C4 - 0x800BAD00 + 0xB7D00, 704);
				
				SET_VI_WIDTH(data + 0x800EF960 - 0x800BAD00 + 0xB7D00, 704);
				SET_VI_WIDTH(data + 0x800EF99C - 0x800BAD00 + 0xB7D00, 704);
				SET_VI_WIDTH(data + 0x800EF9D8 - 0x800BAD00 + 0xB7D00, 704);
				SET_VI_WIDTH(data + 0x800EFA50 - 0x800BAD00 + 0xB7D00, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFU01", 6) && dataType == PATCH_DOL) {
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
				
				SET_VI_WIDTH(data + 0x800BA398 - 0x800B9DA0 + 0xB6DA0, 640);
				SET_VI_WIDTH(data + 0x800BA3D4 - 0x800B9DA0 + 0xB6DA0, 640);
				SET_VI_WIDTH(data + 0x800BA410 - 0x800B9DA0 + 0xB6DA0, 640);
				SET_VI_WIDTH(data + 0x800BA44C - 0x800B9DA0 + 0xB6DA0, 640);
				
				*(u16 *)(data + 0x800BA454 - 0x800B9DA0 + 0xB6DA0) = 574;
				*(u16 *)(data + 0x800BA458 - 0x800B9DA0 + 0xB6DA0) = 0;
				*(u16 *)(data + 0x800BA45C - 0x800B9DA0 + 0xB6DA0) = 574;
				
				*(u16 *)(data + 0x800BA490 - 0x800B9DA0 + 0xB6DA0) = 574;
				*(u16 *)(data + 0x800BA494 - 0x800B9DA0 + 0xB6DA0) = 0;
				*(u16 *)(data + 0x800BA498 - 0x800B9DA0 + 0xB6DA0) = 574;
				
				SET_VI_WIDTH(data + 0x800BA488 - 0x800B9DA0 + 0xB6DA0, 704);
				SET_VI_WIDTH(data + 0x800BA4C4 - 0x800B9DA0 + 0xB6DA0, 704);
				
				SET_VI_WIDTH(data + 0x800EC440 - 0x800B9DA0 + 0xB6DA0, 704);
				SET_VI_WIDTH(data + 0x800EC47C - 0x800B9DA0 + 0xB6DA0, 704);
				SET_VI_WIDTH(data + 0x800EC4B8 - 0x800B9DA0 + 0xB6DA0, 704);
				SET_VI_WIDTH(data + 0x800EC530 - 0x800B9DA0 + 0xB6DA0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GB4E51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2013952:
				SET_VI_WIDTH(data + 0x801E42B0 - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E42EC - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E4328 - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E43A0 - 0x80199FA0 + 0x196F40, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GB4P51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2013600:
				SET_VI_WIDTH(data + 0x801E4170 - 0x80199F20 + 0x196EE0, 704);
				SET_VI_WIDTH(data + 0x801E41AC - 0x80199F20 + 0x196EE0, 704);
				SET_VI_WIDTH(data + 0x801E4224 - 0x80199F20 + 0x196EE0, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7)
					*(s16 *)(data + 0x80008BD2 - 0x80005800 + 0x2620) = 0;
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GBOE51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1934336:
				SET_VI_WIDTH(data + 0x801D3588 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D35C4 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D3600 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D3678 - 0x80189060 + 0x186040, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GBOP51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1934688:
				SET_VI_WIDTH(data + 0x801D3708 - 0x80189180 + 0x186180, 704);
				SET_VI_WIDTH(data + 0x801D3744 - 0x80189180 + 0x186180, 704);
				SET_VI_WIDTH(data + 0x801D37BC - 0x80189180 + 0x186180, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8000AA0E - 0x800057E0 + 0x2600) = 0;
					
					*(s16 *)(data + 0x800ADF46 - 0x800057E0 + 0x2600) = (0x801D37BC + 0x8000) >> 16;
					*(s16 *)(data + 0x800ADF4E - 0x800057E0 + 0x2600) = (0x801D37BC & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEDE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3156384:
				SET_VI_WIDTH(data + 0x802FC500 - 0x8023B940 + 0x238940, 704);
				SET_VI_WIDTH(data + 0x802FC608 - 0x8023B940 + 0x238940, 704);
				
				SET_VI_WIDTH(data + 0x802FEF98 - 0x8023B940 + 0x238940, 704);
				SET_VI_WIDTH(data + 0x802FEFD4 - 0x8023B940 + 0x238940, 704);
				SET_VI_WIDTH(data + 0x802FF04C - 0x8023B940 + 0x238940, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEDJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3106464:
				SET_VI_WIDTH(data + 0x802EE640 - 0x8023C240 + 0x239240, 704);
				SET_VI_WIDTH(data + 0x802EE748 - 0x8023C240 + 0x239240, 704);
				
				SET_VI_WIDTH(data + 0x802F10D8 - 0x8023C240 + 0x239240, 704);
				SET_VI_WIDTH(data + 0x802F1114 - 0x8023C240 + 0x239240, 704);
				SET_VI_WIDTH(data + 0x802F118C - 0x8023C240 + 0x239240, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEDP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3183200:
				SET_VI_WIDTH(data + 0x80301140 - 0x8023B920 + 0x238920, 704);
				
				SET_VI_WIDTH(data + 0x80303BD8 - 0x8023B920 + 0x238920, 704);
				SET_VI_WIDTH(data + 0x80303C14 - 0x8023B920 + 0x238920, 704);
				SET_VI_WIDTH(data + 0x80303C8C - 0x8023B920 + 0x238920, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80018436 - 0x800068E0 + 0x2600) = (0x80301140 + 0x8000) >> 16;
					*(s16 *)(data + 0x80018442 - 0x800068E0 + 0x2600) = (0x80301140 & 0xFFFF);
					
					*(u32 *)(data + 0x8061EB08 - 0x8061D420 + 0x303FA0) = 0;
					
					*(u32 *)(data + 0x8061EB18 - 0x8061D420 + 0x303FA0) = 0x80301140;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEDW01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3101184:
				SET_VI_WIDTH(data + 0x802E9260 - 0x8023B700 + 0x238700, 704);
				SET_VI_WIDTH(data + 0x802E9368 - 0x8023B700 + 0x238700, 704);
				
				SET_VI_WIDTH(data + 0x802EBCF8 - 0x8023B700 + 0x238700, 704);
				SET_VI_WIDTH(data + 0x802EBD34 - 0x8023B700 + 0x238700, 704);
				SET_VI_WIDTH(data + 0x802EBDAC - 0x8023B700 + 0x238700, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEME7F", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1483520:
				SET_VI_WIDTH(data + 0x80167190 - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x801671CC - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x80167208 - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x80167280 - 0x800945C0 + 0x915C0, 704);
				
				if (swissSettings.gameVMode > 0) {
					*(s16 *)(data + 0x800331BE - 0x80005740 + 0x2520) = 448;
					*(u32 *)(data + 0x800331FC - 0x80005740 + 0x2520) = 0x60000000;
					
					*(u32 *)(data + 0x806D0898 - 0x806D06C0 + 0x169120) = 0x801671CC;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEMJ28", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1483264:
				SET_VI_WIDTH(data + 0x80167068 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x801670A4 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x801670E0 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x80167158 - 0x80094500 + 0x91500, 704);
				
				if (swissSettings.gameVMode > 0) {
					*(s16 *)(data + 0x80033062 - 0x80005740 + 0x2520) = 448;
					*(u32 *)(data + 0x800330A0 - 0x80005740 + 0x2520) = 0x60000000;
					
					*(u32 *)(data + 0x806D0660 - 0x806D0480 + 0x169000) = 0x801670A4;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEMP7F", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1485472:
				*(u16 *)(data + 0x8016791E - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x80167920 - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x80167924 - 0x80094B80 + 0x91B80) = 16;
				*(u16 *)(data + 0x80167928 - 0x80094B80 + 0x91B80) = 448;
				
				*(u16 *)(data + 0x8016795A - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x8016795C - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x80167960 - 0x80094B80 + 0x91B80) = 16;
				*(u16 *)(data + 0x80167964 - 0x80094B80 + 0x91B80) = 448;
				
				*(u16 *)(data + 0x80167996 - 0x80094B80 + 0x91B80) = 269;
				*(u16 *)(data + 0x80167998 - 0x80094B80 + 0x91B80) = 269;
				*(u16 *)(data + 0x8016799C - 0x80094B80 + 0x91B80) = 18;
				*(u16 *)(data + 0x801679A0 - 0x80094B80 + 0x91B80) = 538;
				
				*(u16 *)(data + 0x801679D2 - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x801679D4 - 0x80094B80 + 0x91B80) = 538;
				*(u16 *)(data + 0x801679D8 - 0x80094B80 + 0x91B80) = 18;
				*(u16 *)(data + 0x801679DC - 0x80094B80 + 0x91B80) = 538;
				
				*(u16 *)(data + 0x80167A0E - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x80167A10 - 0x80094B80 + 0x91B80) = 448;
				*(u16 *)(data + 0x80167A14 - 0x80094B80 + 0x91B80) = 16;
				*(u16 *)(data + 0x80167A18 - 0x80094B80 + 0x91B80) = 448;
				
				SET_VI_WIDTH(data + 0x80167918 - 0x80094B80 + 0x91B80, 704);
				SET_VI_WIDTH(data + 0x80167954 - 0x80094B80 + 0x91B80, 704);
				SET_VI_WIDTH(data + 0x80167990 - 0x80094B80 + 0x91B80, 704);
				SET_VI_WIDTH(data + 0x801679CC - 0x80094B80 + 0x91B80, 704);
				SET_VI_WIDTH(data + 0x80167A08 - 0x80094B80 + 0x91B80, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80061BE2 - 0x80005740 + 0x2520) = (0x80167990 + 0x8000) >> 16;
					*(s16 *)(data + 0x80061BE6 - 0x80005740 + 0x2520) = (0x80167990 & 0xFFFF);
					
					*(s16 *)(data + 0x80078FB6 - 0x80005740 + 0x2520) = (0x80167990 + 0x8000) >> 16;
					*(s16 *)(data + 0x80078FBA - 0x80005740 + 0x2520) = (0x80167990 & 0xFFFF);
					
					memmove(data + 0x80167990 - 0x80094B80 + 0x91B80,
					        data + 0x801679CC - 0x80094B80 + 0x91B80, 0x80167A44 - 0x801679CC);
					
					*(u16 *)(data + 0x801679D2 - 0x80094B80 + 0x91B80) = 224;
					*(u16 *)(data + 0x801679D4 - 0x80094B80 + 0x91B80) = 224;
					*(u32 *)(data + 0x801679E0 - 0x80094B80 + 0x91B80) = VI_XFBMODE_SF;
					*(u8 *)(data + 0x801679E4 - 0x80094B80 + 0x91B80) = GX_TRUE;
					memcpy(data + 0x801679FE - 0x80094B80 + 0x91B80, (u8[]){0, 0, 21, 22, 21, 0, 0}, 7);
					
					*(u32 *)(data + 0x806D0F58 - 0x806D0D80 + 0x1698C0) = 0x801679CC;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7E08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2866304:
				SET_VI_WIDTH(data + 0x802B0FF8 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B1034 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B1070 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B10E8 - 0x80269E80 + 0x266E80, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7J08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2891072:
				SET_VI_WIDTH(data + 0x802B7078 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B70B4 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B70F0 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B7168 - 0x8026C680 + 0x269680, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7P08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2961056:
				SET_VI_WIDTH(data + 0x8027D278 - 0x8027D200 + 0x27A200, 704);
				
				SET_VI_WIDTH(data + 0x802C81B8 - 0x8027D200 + 0x27A200, 704);
				SET_VI_WIDTH(data + 0x802C81F4 - 0x8027D200 + 0x27A200, 704);
				SET_VI_WIDTH(data + 0x802C826C - 0x8027D200 + 0x27A200, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GKDP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1067296:
				*(s16 *)(data + 0x80015E36 - 0x8000BEA0 + 0x2520) = 704;
				*(s16 *)(data + 0x80015E3E - 0x8000BEA0 + 0x2520) = 8;
				
				SET_VI_WIDTH(data + 0x800F3944 - 0x800ECE80 + 0xE9E80, 704);
				
				SET_VI_WIDTH(data + 0x801019A0 - 0x800ECE80 + 0xE9E80, 704);
				SET_VI_WIDTH(data + 0x801019DC - 0x800ECE80 + 0xE9E80, 704);
				SET_VI_WIDTH(data + 0x80101A18 - 0x800ECE80 + 0xE9E80, 704);
				SET_VI_WIDTH(data + 0x80101A90 - 0x800ECE80 + 0xE9E80, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GLME01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3799584:
				SET_VI_WIDTH(data + 0x802181D0 - 0x80218100 + 0x215100, 704);
				
				SET_VI_WIDTH(data + 0x80395C40 - 0x80218100 + 0x215100, 704);
				SET_VI_WIDTH(data + 0x80395C7C - 0x80218100 + 0x215100, 704);
				SET_VI_WIDTH(data + 0x80395CB8 - 0x80218100 + 0x215100, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GLMJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3754304:
				SET_VI_WIDTH(data + 0x8020D4B0 - 0x8020D3E0 + 0x20A3E0, 704);
				
				SET_VI_WIDTH(data + 0x8038ABA0 - 0x8020D3E0 + 0x20A3E0, 704);
				SET_VI_WIDTH(data + 0x8038ABDC - 0x8020D3E0 + 0x20A3E0, 704);
				SET_VI_WIDTH(data + 0x8038AC18 - 0x8020D3E0 + 0x20A3E0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GLMP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3739680:
				*(s16 *)(data + 0x8000739E - 0x800057C0 + 0x2520) = 40;
				*(s16 *)(data + 0x800073A6 - 0x800057C0 + 0x2520) = 640;
				
				SET_VI_WIDTH(data + 0x803870A8 - 0x8021D240 + 0x21A240, 704);
				SET_VI_WIDTH(data + 0x803870E4 - 0x8021D240 + 0x21A240, 704);
				SET_VI_WIDTH(data + 0x80387120 - 0x8021D240 + 0x21A240, 704);
				SET_VI_WIDTH(data + 0x80387198 - 0x8021D240 + 0x21A240, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80007596 - 0x800057C0 + 0x2520) = 116;
					
					*(u32 *)(data + 0x804BD630 - 0x804BD620 + 0x384E80) = 0x803908D4;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPIE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3102432:
				SET_VI_WIDTH(data + 0x802A5664 - 0x80222D20 + 0x21FD20, 704);
				SET_VI_WIDTH(data + 0x802A56A0 - 0x80222D20 + 0x21FD20, 704);
				
				SET_VI_WIDTH(data + 0x802E8D28 - 0x80222D20 + 0x21FD20, 704);
				SET_VI_WIDTH(data + 0x802E8D64 - 0x80222D20 + 0x21FD20, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 3102592:
				SET_VI_WIDTH(data + 0x802A5704 - 0x80222DC0 + 0x21FDC0, 704);
				SET_VI_WIDTH(data + 0x802A5740 - 0x80222DC0 + 0x21FDC0, 704);
				
				SET_VI_WIDTH(data + 0x802E8DC8 - 0x80222DC0 + 0x21FDC0, 704);
				SET_VI_WIDTH(data + 0x802E8E04 - 0x80222DC0 + 0x21FDC0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPIJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3127584:
				SET_VI_WIDTH(data + 0x802AABCC - 0x80227EA0 + 0x224EA0, 704);
				SET_VI_WIDTH(data + 0x802AAC08 - 0x80227EA0 + 0x224EA0, 704);
				
				SET_VI_WIDTH(data + 0x802EEF08 - 0x80227EA0 + 0x224EA0, 704);
				SET_VI_WIDTH(data + 0x802EEF44 - 0x80227EA0 + 0x224EA0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 3128832:
				SET_VI_WIDTH(data + 0x802AB0AC - 0x80228380 + 0x225380, 704);
				SET_VI_WIDTH(data + 0x802AB0E8 - 0x80228380 + 0x225380, 704);
				
				SET_VI_WIDTH(data + 0x802EF3E8 - 0x80228380 + 0x225380, 704);
				SET_VI_WIDTH(data + 0x802EF424 - 0x80228380 + 0x225380, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPIP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3113952:
				*(u32 *)(data + 0x802A7CB4 - 0x80224BC0 + 0x221BC0) = VI_TVMODE_EURGB60_INT;
				*(u16 *)(data + 0x802A7CC0 - 0x80224BC0 + 0x221BC0) = 0;
				*(u32 *)(data + 0x802A7CC8 - 0x80224BC0 + 0x221BC0) = VI_XFBMODE_DF;
				memcpy(data + 0x802A7CE6 - 0x80224BC0 + 0x221BC0, (u8[]){5, 6, 14, 14, 14, 6, 5}, 7);
				
				SET_VI_WIDTH(data + 0x802A7CB4 - 0x80224BC0 + 0x221BC0, 704);
				SET_VI_WIDTH(data + 0x802A7CF0 - 0x80224BC0 + 0x221BC0, 640);
				
				SET_VI_WIDTH(data + 0x802EB9F0 - 0x80224BC0 + 0x221BC0, 704);
				SET_VI_WIDTH(data + 0x802EBA2C - 0x80224BC0 + 0x221BC0, 704);
				SET_VI_WIDTH(data + 0x802EBAA4 - 0x80224BC0 + 0x221BC0, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(u32 *)(data + 0x803E24F0 - 0x803E1C60 + 0x2E9380) = 0x802A7CB4;
					
					*(f32 *)(data + 0x803ED5B0 - 0x803ED120 + 0x2F3DE0) = 1.0f;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPTP41", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4017536:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(u32 *)(data + 0x801B25C4 - 0x8002B240 + 0x2600) = 
					*(u32 *)(data + 0x801B25C0 - 0x8002B240 + 0x2600);
					*(u32 *)(data + 0x801B25C0 - 0x8002B240 + 0x2600) = 
					*(u32 *)(data + 0x801B25B0 - 0x8002B240 + 0x2600);
					
					*(s16 *)(data + 0x801B2592 - 0x8002B240 + 0x2600) = (0x803B5010 + 0x8000) >> 16;
					*(s16 *)(data + 0x801B2596 - 0x8002B240 + 0x2600) = (0x803B5010 & 0xFFFF);
					*(s16 *)(data + 0x801B25AA - 0x8002B240 + 0x2600) = 16;
					*(s16 *)(data + 0x801B25B2 - 0x8002B240 + 0x2600) = 640;
					*(s16 *)(data + 0x801B25C2 - 0x8002B240 + 0x2600) = 448;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 639584:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8002E65E - 0x8000B400 + 0x2600) = (0x80098534 + 0x8000) >> 16;
					*(s16 *)(data + 0x8002E666 - 0x8000B400 + 0x2600) = (0x80098534 & 0xFFFF);
					*(s16 *)(data + 0x8002E676 - 0x8000B400 + 0x2600) = 40;
					*(s16 *)(data + 0x8002E67E - 0x8000B400 + 0x2600) = 40;
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GSAE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3398976:
				memset(data + 0x80049504 - 0x800066E0 + 0x2620, 0, 0x80049550 - 0x80049504);
				*(u32 *)(data + 0x80049504 - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049508 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x8004950C - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x80049510 - 0x800066E0 + 0x2620) = 0x806D9B10;
				*(u32 *)(data + 0x80049514 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024CDB8, (u32 *)0x80049514);
				*(u32 *)(data + 0x80049518 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D554, (u32 *)0x80049518);
				*(u32 *)(data + 0x8004951C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024C8F0, (u32 *)0x8004951C);
				*(u32 *)(data + 0x80049520 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024C8F0, (u32 *)0x80049520);
				*(u32 *)(data + 0x80049524 - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x80049528 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x8004952C - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x80049530 - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032E620 - 0x802C2D60 + 0x2BFD60, 704);
				SET_VI_WIDTH(data + 0x8032E65C - 0x802C2D60 + 0x2BFD60, 704);
				SET_VI_WIDTH(data + 0x8032E698 - 0x802C2D60 + 0x2BFD60, 704);
				SET_VI_WIDTH(data + 0x8032E710 - 0x802C2D60 + 0x2BFD60, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 3402176:
				memset(data + 0x80049680 - 0x800066E0 + 0x2620, 0, 0x800496CC - 0x80049680);
				*(u32 *)(data + 0x80049680 - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049684 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x80049688 - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x8004968C - 0x800066E0 + 0x2620) = 0x806D9B30;
				*(u32 *)(data + 0x80049690 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D51C, (u32 *)0x80049690);
				*(u32 *)(data + 0x80049694 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024DCB8, (u32 *)0x80049694);
				*(u32 *)(data + 0x80049698 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D054, (u32 *)0x80049698);
				*(u32 *)(data + 0x8004969C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D054, (u32 *)0x8004969C);
				*(u32 *)(data + 0x800496A0 - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x800496A4 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x800496A8 - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x800496AC - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032F278 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F2B4 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F2F0 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F368 - 0x802C34E0 + 0x2C04E0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GSAJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3399264:
				memset(data + 0x80049524 - 0x800066E0 + 0x2620, 0, 0x80049570 - 0x80049524);
				*(u32 *)(data + 0x80049524 - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049528 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x8004952C - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x80049530 - 0x800066E0 + 0x2620) = 0x806D9B10;
				*(u32 *)(data + 0x80049534 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024CEA8, (u32 *)0x80049534);
				*(u32 *)(data + 0x80049538 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D644, (u32 *)0x80049538);
				*(u32 *)(data + 0x8004953C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024C9E0, (u32 *)0x8004953C);
				*(u32 *)(data + 0x80049540 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024C9E0, (u32 *)0x80049540);
				*(u32 *)(data + 0x80049544 - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x80049548 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x8004954C - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x80049550 - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032E740 - 0x802C2E60 + 0x2BFE60, 704);
				SET_VI_WIDTH(data + 0x8032E77C - 0x802C2E60 + 0x2BFE60, 704);
				SET_VI_WIDTH(data + 0x8032E7B8 - 0x802C2E60 + 0x2BFE60, 704);
				SET_VI_WIDTH(data + 0x8032E830 - 0x802C2E60 + 0x2BFE60, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 3402176:
				memset(data + 0x80049680 - 0x800066E0 + 0x2620, 0, 0x800496CC - 0x80049680);
				*(u32 *)(data + 0x80049680 - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049684 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x80049688 - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x8004968C - 0x800066E0 + 0x2620) = 0x806D9B30;
				*(u32 *)(data + 0x80049690 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D51C, (u32 *)0x80049690);
				*(u32 *)(data + 0x80049694 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024DCB8, (u32 *)0x80049694);
				*(u32 *)(data + 0x80049698 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D054, (u32 *)0x80049698);
				*(u32 *)(data + 0x8004969C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D054, (u32 *)0x8004969C);
				*(u32 *)(data + 0x800496A0 - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x800496A4 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x800496A8 - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x800496AC - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032F278 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F2B4 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F2F0 - 0x802C34E0 + 0x2C04E0, 704);
				SET_VI_WIDTH(data + 0x8032F368 - 0x802C34E0 + 0x2C04E0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GSAP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3405152:
				memset(data + 0x8004971C - 0x800066E0 + 0x2620, 0, 0x80049768 - 0x8004971C);
				*(u32 *)(data + 0x8004971C - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049720 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x80049724 - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x80049728 - 0x800066E0 + 0x2620) = 0x806D9B68;
				*(u32 *)(data + 0x8004972C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D62C, (u32 *)0x8004972C);
				*(u32 *)(data + 0x80049730 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024DDC8, (u32 *)0x80049730);
				*(u32 *)(data + 0x80049734 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D164, (u32 *)0x80049734);
				*(u32 *)(data + 0x80049738 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D164, (u32 *)0x80049738);
				*(u32 *)(data + 0x8004973C - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x80049740 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x80049744 - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x80049748 - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032FDF8 - 0x802C35A0 + 0x2C05A0, 704);
				SET_VI_WIDTH(data + 0x8032FE34 - 0x802C35A0 + 0x2C05A0, 704);
				SET_VI_WIDTH(data + 0x8032FEAC - 0x802C35A0 + 0x2C05A0, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80020FFA - 0x800066E0 + 0x2620) = (0x8032FEAC + 0x8000) >> 16;
					*(s16 *)(data + 0x80020FFE - 0x800066E0 + 0x2620) = (0x8032FEAC & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
			case 3405600:
				memset(data + 0x8004971C - 0x800066E0 + 0x2620, 0, 0x80049768 - 0x8004971C);
				*(u32 *)(data + 0x8004971C - 0x800066E0 + 0x2620) = 0x9421FFF0;
				*(u32 *)(data + 0x80049720 - 0x800066E0 + 0x2620) = 0x7C0802A6;
				*(u32 *)(data + 0x80049724 - 0x800066E0 + 0x2620) = 0x90010014;
				*(u32 *)(data + 0x80049728 - 0x800066E0 + 0x2620) = 0x806D9B68;
				*(u32 *)(data + 0x8004972C - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D764, (u32 *)0x8004972C);
				*(u32 *)(data + 0x80049730 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024DF00, (u32 *)0x80049730);
				*(u32 *)(data + 0x80049734 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D29C, (u32 *)0x80049734);
				*(u32 *)(data + 0x80049738 - 0x800066E0 + 0x2620) = branchAndLink((u32 *)0x8024D29C, (u32 *)0x80049738);
				*(u32 *)(data + 0x8004973C - 0x800066E0 + 0x2620) = 0x80010014;
				*(u32 *)(data + 0x80049740 - 0x800066E0 + 0x2620) = 0x7C0803A6;
				*(u32 *)(data + 0x80049744 - 0x800066E0 + 0x2620) = 0x38210010;
				*(u32 *)(data + 0x80049748 - 0x800066E0 + 0x2620) = 0x4E800020;
				
				SET_VI_WIDTH(data + 0x8032FFB8 - 0x802C36E0 + 0x2C06E0, 704);
				SET_VI_WIDTH(data + 0x8032FFF4 - 0x802C36E0 + 0x2C06E0, 704);
				SET_VI_WIDTH(data + 0x8033006C - 0x802C36E0 + 0x2C06E0, 704);
				
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80020FFA - 0x800066E0 + 0x2620) = (0x8033006C + 0x8000) >> 16;
					*(s16 *)(data + 0x80020FFE - 0x800066E0 + 0x2620) = (0x8033006C & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3431456:
				SET_VI_WIDTH(data + 0x80341420 - 0x80173DA0 + 0x170DA0, 704);
				SET_VI_WIDTH(data + 0x8034145C - 0x80173DA0 + 0x170DA0, 704);
				SET_VI_WIDTH(data + 0x80341498 - 0x80173DA0 + 0x170DA0, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3418464:
				SET_VI_WIDTH(data + 0x8033E1E0 - 0x80170880 + 0x16D880, 704);
				SET_VI_WIDTH(data + 0x8033E21C - 0x80170880 + 0x16D880, 704);
				SET_VI_WIDTH(data + 0x8033E258 - 0x80170880 + 0x16D880, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3442880:
				SET_VI_WIDTH(data + 0x80252AC0 - 0x80176760 + 0x173760, 704);
				
				SET_VI_WIDTH(data + 0x80343FC0 - 0x80176760 + 0x173760, 704);
				SET_VI_WIDTH(data + 0x80343FFC - 0x80176760 + 0x173760, 704);
				SET_VI_WIDTH(data + 0x80344038 - 0x80176760 + 0x173760, 704);
				
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if ((!strncmp(gameID, "GX2D52", 6) || !strncmp(gameID, "GX2P52", 6) || !strncmp(gameID, "GX2S52", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4055712:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x80010A1A - 0x80005760 + 0x2540) = (0x80368D5C + 0x8000) >> 16;
					*(s16 *)(data + 0x80010A1E - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x80010A22 - 0x80005760 + 0x2540) = (0x80368D5C & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLP52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4182304:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x80384784 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x80384784 & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLX52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4182624:
				if (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x803848C4 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x803848C4 & 0xFFFF);
				}
				print_gecko("Patched:[%.6s]\n", gameID);
				break;
		}
	}
#undef SET_VI_WIDTH
}

int Patch_Miscellaneous(u32 *data, u32 length, int dataType)
{
	int i, j;
	int patched = 0;
	u32 address, constant;
	FuncPattern OSGetConsoleTypeSigs[] = {
		{ 13, 7, 0, 0, 2, 0, NULL, 0, "OSGetConsoleTypeD" },
		{ 10, 4, 0, 0, 2, 0, NULL, 0, "OSGetConsoleType" },
		{  9, 4, 0, 0, 1, 0, NULL, 0, "OSGetConsoleType" }	// SN Systems ProDG
	};
	FuncPattern OSSetAlarmSigs[3] = {
		{ 58, 18, 4, 8, 0, 9, NULL, 0, "OSSetAlarmD" },
		{ 26,  5, 4, 4, 0, 5, NULL, 0, "OSSetAlarm" },
		{ 28,  3, 4, 6, 0, 9, NULL, 0, "OSSetAlarm" }	// SN Systems ProDG
	};
	FuncPattern OSCancelAlarmSigs[3] = {
		{ 52, 15,  7, 6, 5,  4, NULL, 0, "OSCancelAlarmD" },
		{ 71, 18, 10, 7, 9, 12, NULL, 0, "OSCancelAlarm" },
		{ 70, 18, 10, 7, 9, 13, NULL, 0, "OSCancelAlarm" }	// SN Systems ProDG
	};
	FuncPattern OSGetFontEncodeSigs[2] = {
		{ 31, 8, 2, 1, 7, 2, NULL, 0, "OSGetFontEncodeD" },
		{ 22, 6, 0, 0, 6, 0, NULL, 0, "OSGetFontEncode" }
	};
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern __OSLockSramSigs[3] = {
		{  9, 3, 2, 1, 0, 2, NULL, 0, "__OSLockSramD" },
		{ 23, 7, 5, 2, 2, 2, NULL, 0, "__OSLockSram" },
		{ 20, 7, 4, 2, 2, 3, NULL, 0, "__OSLockSram" }	// SN Systems ProDG
	};
	FuncPattern __OSLockSramExSigs[3] = {
		{  9, 3, 2, 1, 0, 2, NULL, 0, "__OSLockSramExD" },
		{ 23, 9, 5, 2, 2, 2, NULL, 0, "__OSLockSramEx" },
		{ 20, 8, 4, 2, 2, 2, NULL, 0, "__OSLockSramEx" }	// SN Systems ProDG
	};
	FuncPattern __OSUnlockSramSigs[3] = {
		{  11,  4,  3, 1, 0,  2, NULL, 0, "__OSUnlockSramD" },
		{   9,  3,  2, 1, 0,  2, NULL, 0, "__OSUnlockSram" },
		{ 173, 53, 11, 9, 9, 25, NULL, 0, "__OSUnlockSram" }	// SN Systems ProDG
	};
	FuncPattern __OSUnlockSramExSigs[3] = {
		{ 11,  4,  3, 1, 0, 2, NULL, 0, "__OSUnlockSramExD" },
		{  9,  3,  2, 1, 0, 2, NULL, 0, "__OSUnlockSramEx" },
		{ 98, 43, 11, 9, 5, 6, NULL, 0, "__OSUnlockSramEx" }	// SN Systems ProDG
	};
	FuncPattern InitializeUARTSigs[5] = {
		{ 18,  7, 4, 1, 1, 2, NULL, 0, "InitializeUARTD" },
		{ 28, 12, 6, 1, 2, 2, NULL, 0, "InitializeUARTD" },
		{ 16,  6, 3, 1, 1, 2, NULL, 0, "InitializeUART" },
		{ 18,  7, 4, 1, 1, 2, NULL, 0, "InitializeUART" },
		{ 28, 12, 6, 1, 2, 2, NULL, 0, "InitializeUART" }
	};
	FuncPattern QueueLengthSig = 
		{ 38, 21, 3, 6, 1, 2, NULL, 0, "QueueLengthD" };
	FuncPattern WriteUARTNSigs[5] = {
		{  99, 28, 3,  9, 17, 8, WriteUARTN_bin, WriteUARTN_bin_size, "WriteUARTND" },
		{ 105, 28, 3, 12, 17, 9, WriteUARTN_bin, WriteUARTN_bin_size, "WriteUARTND" },
		{ 128, 48, 4, 14, 18, 7, WriteUARTN_bin, WriteUARTN_bin_size, "WriteUARTN" },
		{ 128, 47, 4, 14, 18, 7, WriteUARTN_bin, WriteUARTN_bin_size, "WriteUARTN" },
		{ 135, 47, 4, 17, 18, 9, WriteUARTN_bin, WriteUARTN_bin_size, "WriteUARTN" }
	};
	FuncPattern SIGetResponseSigs[6] = {
		{ 36, 13, 4, 1, 2,  4, NULL, 0, "SIGetResponseD" },
		{ 48, 12, 5, 4, 3,  7, NULL, 0, "SIGetResponseD" },
		{  9,  4, 2, 0, 0,  1, NULL, 0, "SIGetResponse" },
		{ 52, 13, 8, 3, 2, 10, NULL, 0, "SIGetResponse" },
		{ 49, 13, 8, 3, 2,  7, NULL, 0, "SIGetResponse" },
		{ 74, 26, 9, 4, 3, 14, NULL, 0, "SIGetResponse" }	// SN Systems ProDG
	};
	FuncPattern UpdateOriginSigs[5] = {
		{ 105, 15, 1, 0, 20, 4, NULL, 0, "UpdateOriginD" },
		{ 107, 13, 2, 1, 20, 6, NULL, 0, "UpdateOriginD" },
		{  81,  7, 0, 0, 18, 1, NULL, 0, "UpdateOrigin" },
		{ 101, 14, 0, 0, 18, 5, NULL, 0, "UpdateOrigin" },
		{ 105, 14, 3, 1, 20, 5, NULL, 0, "UpdateOrigin" }
	};
	FuncPattern PADOriginCallbackSigs[6] = {
		{  39, 17,  4, 5,  3,  3, NULL, 0, "PADOriginCallbackD" },
		{  69, 28,  9, 6,  2,  9, NULL, 0, "PADOriginCallback" },
		{  70, 28, 10, 6,  2,  9, NULL, 0, "PADOriginCallback" },
		{  71, 27, 10, 6,  2,  9, NULL, 0, "PADOriginCallback" },
		{  49, 21,  6, 6,  1,  8, NULL, 0, "PADOriginCallback" },
		{ 143, 30,  6, 6, 21, 11, NULL, 0, "PADOriginCallback" }	// SN Systems ProDG
	};
	FuncPattern PADOriginUpdateCallbackSigs[7] = {
		{  31, 10,  4, 2,  3,  5, NULL, 0, "PADOriginUpdateCallbackD" },
		{  34,  8,  2, 3,  4,  4, NULL, 0, "PADOriginUpdateCallbackD" },
		{  10,  2,  2, 1,  0,  2, NULL, 0, "PADOriginUpdateCallback" },
		{  15,  4,  2, 1,  1,  4, NULL, 0, "PADOriginUpdateCallback" },
		{  48, 13,  9, 5,  2,  9, NULL, 0, "PADOriginUpdateCallback" },
		{ 143, 23, 10, 5, 22, 13, NULL, 0, "PADOriginUpdateCallback" },	// SN Systems ProDG
		{  51, 14, 10, 5,  2, 10, NULL, 0, "PADOriginUpdateCallback" }
	};
	FuncPattern PADInitSigs[15] = {
		{  88, 37,  4,  8, 3, 11, NULL, 0, "PADInitD" },
		{  91, 38,  5,  8, 6, 11, NULL, 0, "PADInitD" },
		{  90, 37,  5,  8, 6, 11, NULL, 0, "PADInitD" },
		{  92, 38,  5,  9, 6, 11, NULL, 0, "PADInitD" },
		{  84, 25,  7, 10, 1, 11, NULL, 0, "PADInit" },
		{  91, 27,  7, 10, 1, 17, NULL, 0, "PADInit" },
		{  94, 29,  7, 11, 1, 17, NULL, 0, "PADInit" },
		{ 113, 31, 11, 11, 1, 17, NULL, 0, "PADInit" },
		{ 129, 42, 13, 12, 2, 18, NULL, 0, "PADInit" },
		{ 132, 43, 14, 12, 5, 18, NULL, 0, "PADInit" },
		{ 133, 43, 14, 12, 5, 18, NULL, 0, "PADInit" },
		{ 132, 42, 14, 12, 5, 18, NULL, 0, "PADInit" },
		{ 134, 43, 14, 13, 5, 18, NULL, 0, "PADInit" },
		{ 123, 38, 16, 10, 5, 27, NULL, 0, "PADInit" },	// SN Systems ProDG
		{  84, 24,  8,  9, 4,  9, NULL, 0, "PADInit" }
	};
	FuncPattern PADReadSigs[9] = {
		{ 172, 65,  3, 15, 16, 18, NULL, 0, "PADReadD" },
		{ 171, 66,  4, 20, 17, 14, NULL, 0, "PADReadD" },
		{ 128, 49,  4, 10, 11, 11, NULL, 0, "PADRead" },
		{ 200, 75,  9, 20, 17, 18, NULL, 0, "PADRead" },
		{ 206, 78,  7, 20, 17, 19, NULL, 0, "PADRead" },
		{ 237, 87, 13, 27, 17, 25, NULL, 0, "PADRead" },
		{ 235, 86, 13, 27, 17, 24, NULL, 0, "PADRead" },
		{ 233, 71, 13, 29, 17, 27, NULL, 0, "PADRead" },	// SN Systems ProDG
		{ 192, 73,  8, 23, 16, 15, NULL, 0, "PADRead" }
	};
	FuncPattern PADSetSpecSigs[2] = {
		{ 42, 15, 8, 1, 9, 3, NULL, 0, "PADSetSpecD" },
		{ 24,  7, 5, 0, 8, 0, NULL, 0, "PADSetSpec" }
	};
	FuncPattern SPEC0_MakeStatusSigs[3] = {
		{ 96, 28, 0, 0, 12, 9, NULL, 0, "SPEC0_MakeStatusD" },
		{ 93, 26, 0, 0, 11, 9, NULL, 0, "SPEC0_MakeStatus" },
		{ 85, 18, 0, 0,  2, 8, NULL, 0, "SPEC0_MakeStatus" }	// SN Systems ProDG
	};
	FuncPattern SPEC1_MakeStatusSigs[3] = {
		{ 96, 28, 0, 0, 12, 9, NULL, 0, "SPEC1_MakeStatusD" },
		{ 93, 26, 0, 0, 11, 9, NULL, 0, "SPEC1_MakeStatus" },
		{ 85, 18, 0, 0,  2, 8, NULL, 0, "SPEC1_MakeStatus" }	// SN Systems ProDG
	};
	FuncPattern ClampS8Sig = 
		{ 23, 1, 1, 0, 5, 7, NULL, 0, "ClampS8D" };
	FuncPattern ClampU8Sig = 
		{ 7, 0, 0, 0, 1, 3, NULL, 0, "ClampU8" };
	FuncPattern SPEC2_MakeStatusSigs[5] = {
		{ 186, 43, 3, 6, 20, 14, NULL, 0, "SPEC2_MakeStatusD" },
		{ 218, 54, 4, 6, 22, 19, NULL, 0, "SPEC2_MakeStatusD" },
		{ 254, 46, 0, 0, 42, 71, NULL, 0, "SPEC2_MakeStatus" },
		{ 234, 46, 0, 0, 42, 51, NULL, 0, "SPEC2_MakeStatus" },	// SN Systems ProDG
		{ 284, 55, 2, 0, 43, 76, NULL, 0, "SPEC2_MakeStatus" }
	};
	FuncPattern TimeoutHandlerSigs[3] = {
		{ 50, 18, 4, 2, 7, 3, NULL, 0, "TimeoutHandlerD" },
		{ 38, 15, 5, 1, 3, 5, NULL, 0, "TimeoutHandler" },
		{ 41, 16, 5, 1, 4, 5, NULL, 0, "TimeoutHandler" }
	};
	FuncPattern SetupTimeoutAlarmSigs[4] = {
		{ 61, 20, 3, 3,  8, 17, NULL, 0, "SetupTimeoutAlarmD" },
		{ 91, 28, 3, 4, 10, 30, NULL, 0, "SetupTimeoutAlarmD" },
		{ 62, 20, 3, 3,  8, 14, NULL, 0, "SetupTimeoutAlarm" },
		{ 91, 27, 3, 4, 10, 25, NULL, 0, "SetupTimeoutAlarm" }
	};
	FuncPattern RetrySigs[5] = {
		{  98, 34, 2, 15,  8,  3, NULL, 0, "RetryD" },
		{  98, 33, 2, 15,  8,  3, NULL, 0, "RetryD" },
		{ 123, 40, 4, 15, 11, 13, NULL, 0, "Retry" },
		{ 139, 48, 4, 16, 14, 14, NULL, 0, "Retry" },
		{ 168, 54, 4, 17, 16, 25, NULL, 0, "Retry" }
	};
	FuncPattern __CARDStartSigs[6] = {
		{  65, 24, 6, 5,  7,  2, NULL, 0, "__CARDStartD" },
		{  70, 20, 6, 7,  7,  3, NULL, 0, "__CARDStartD" },
		{  88, 30, 8, 5, 10, 13, NULL, 0, "__CARDStart" },
		{ 104, 38, 8, 6, 13, 14, NULL, 0, "__CARDStart" },
		{ 109, 32, 6, 8, 13, 14, NULL, 0, "__CARDStart" },
		{ 137, 37, 6, 9, 15, 25, NULL, 0, "__CARDStart" }
	};
	FuncPattern __CARDGetFontEncodeSig = 
		{ 2, 0, 0, 0, 0, 0, NULL, 0, "__CARDGetFontEncode" };
	FuncPattern __CARDGetControlBlockSigs[4] = {
		{ 46, 11, 6, 2, 5, 4, NULL, 0, "__CARDGetControlBlockD" },
		{ 46, 11, 6, 2, 5, 4, NULL, 0, "__CARDGetControlBlockD" },
		{ 44, 12, 7, 2, 5, 4, NULL, 0, "__CARDGetControlBlock" },
		{ 46, 13, 8, 2, 5, 2, NULL, 0, "__CARDGetControlBlock" }
	};
	FuncPattern __CARDPutControlBlockSigs[4] = {
		{ 29, 8, 3, 3, 1, 3, NULL, 0, "__CARDPutControlBlockD" },
		{ 34, 9, 4, 3, 2, 3, NULL, 0, "__CARDPutControlBlockD" },
		{ 20, 5, 5, 2, 1, 2, NULL, 0, "__CARDPutControlBlock" },
		{ 25, 6, 6, 2, 2, 2, NULL, 0, "__CARDPutControlBlock" }
	};
	FuncPattern CARDGetEncodingSigs[2] = {
		{ 27,  9, 4, 2, 2, 3, NULL, 0, "CARDGetEncodingD" },
		{ 34, 12, 5, 3, 4, 2, NULL, 0, "CARDGetEncoding" }
	};
	FuncPattern VerifyIDSigs[8] = {
		{ 107, 33, 2, 7, 10, 33, NULL, 0, "VerifyIDD" },
		{ 107, 33, 2, 7, 10, 33, NULL, 0, "VerifyIDD" },
		{ 107, 33, 2, 7, 10, 33, NULL, 0, "VerifyIDD" },
		{ 176, 32, 2, 7, 16, 63, NULL, 0, "VerifyID" },
		{ 161, 28, 2, 6, 12, 63, NULL, 0, "VerifyID" },
		{ 161, 28, 2, 6, 12, 63, NULL, 0, "VerifyID" },
		{ 161, 28, 2, 6, 12, 63, NULL, 0, "VerifyID" },
		{ 161, 28, 2, 6, 12, 63, NULL, 0, "VerifyID" }
	};
	FuncPattern DoMountSigs[11] = {
		{ 181, 48,  6, 18, 20, 26, NULL, 0, "DoMountD" },
		{ 222, 57,  7, 20, 25, 27, NULL, 0, "DoMountD" },
		{ 244, 57,  7, 20, 37, 27, NULL, 0, "DoMountD" },
		{ 229, 63,  7, 24, 24, 27, NULL, 0, "DoMountD" },
		{ 247, 67,  7, 26, 27, 29, NULL, 0, "DoMountD" },
		{ 231, 59, 12, 17, 20, 33, NULL, 0, "DoMount" },
		{ 238, 60, 10, 17, 21, 33, NULL, 0, "DoMount" },
		{ 276, 68, 11, 19, 26, 33, NULL, 0, "DoMount" },
		{ 297, 67, 11, 19, 38, 33, NULL, 0, "DoMount" },
		{ 260, 66, 11, 20, 24, 33, NULL, 0, "DoMount" },
		{ 277, 69, 11, 22, 27, 35, NULL, 0, "DoMount" }
	};
	_SDA2_BASE_ = _SDA_BASE_ = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if (!_SDA2_BASE_ && !_SDA_BASE_) {
			if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
				(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
				(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
				(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
				get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
				get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
				i += 4;
			}
			continue;
		}
		if ((data[i + 0] != 0x7C0802A6 && data[i + 1] != 0x7C0802A6) ||
			(data[i - 1] != 0x4E800020 &&
			(data[i + 0] == 0x60000000 || data[i - 1] != 0x4C000064) &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
			continue;
		
		FuncPattern fp;
		make_pattern(data, i, length, &fp);
		
		for (j = 0; j < sizeof(InitializeUARTSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &InitializeUARTSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  3, length, &OSGetConsoleTypeSigs[0]) &&
							get_immediate(data,   i + 10, i + 11, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i +  9, length, &OSGetConsoleTypeSigs[0],
							                                               &OSGetConsoleTypeSigs[1], NULL) &&
							get_immediate (data,   i + 20, i + 21, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  3, length, &OSGetConsoleTypeSigs[1]) &&
							get_immediate(data,   i +  8, i +  9, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  3, length, &OSGetConsoleTypeSigs[1]) &&
							get_immediate(data,   i + 10, i + 11, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_patterns(data, dataType, i +  9, length, &OSGetConsoleTypeSigs[1],
							                                               &OSGetConsoleTypeSigs[2], NULL) &&
							get_immediate (data,   i + 16, i + 17, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(WriteUARTNSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &WriteUARTNSigs[j])) {
				switch (j) {
					case 0:
						if (get_immediate(data,  i +   7, i +   8, &constant) && constant == 0xA5FF005A &&
							findx_pattern(data, dataType, i +  36, length, &QueueLengthSig))
							WriteUARTNSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (get_immediate(data,  i +   7, i +   8, &constant) && constant == 0xA5FF005A &&
							findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  40, length, &QueueLengthSig) &&
							findx_pattern(data, dataType, i +  98, length, &OSRestoreInterruptsSig))
							WriteUARTNSigs[j].offsetFoundAt = i;
						break;
					case 2:
					case 3:
						if (get_immediate(data,  i +   7, i +   8, &constant) && constant == 0xA5FF005A)
							WriteUARTNSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (get_immediate(data,  i +   7, i +   8, &constant) && constant == 0xA5FF005A &&
							findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  22, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 128, length, &OSRestoreInterruptsSig))
							WriteUARTNSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &PADOriginCallbackSigs[j])) {
				switch (j) {
					case 0:
						if (findx_patterns(data, dataType, i +  31, length, &UpdateOriginSigs[0],
							                                                &UpdateOriginSigs[1], NULL))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +   7, length, &UpdateOriginSigs[2]) &&
							findx_pattern(data, dataType, i +  16, length, &SIGetResponseSigs[2]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +   7, length, &UpdateOriginSigs[2]) &&
							findx_pattern(data, dataType, i +  16, length, &SIGetResponseSigs[2]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  10, length, &UpdateOriginSigs[3]) &&
							findx_pattern(data, dataType, i +  19, length, &SIGetResponseSigs[2]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern (data, dataType, i +   7, length, &UpdateOriginSigs[4]) &&
							findx_patterns(data, dataType, i +  16, length, &SIGetResponseSigs[1],
							                                                &SIGetResponseSigs[3],
							                                                &SIGetResponseSigs[4], NULL))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 111, length, &SIGetResponseSigs[5]))
							PADOriginCallbackSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &PADOriginUpdateCallbackSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  25, length, &UpdateOriginSigs[0]))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  24, length, &UpdateOriginSigs[1]))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +   5, length, &UpdateOriginSigs[2]))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_patterns(data, dataType, i +  10, length, &UpdateOriginSigs[2],
							                                                &UpdateOriginSigs[3], NULL))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  16, length, &UpdateOriginSigs[4]) &&
							findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  40, length, &OSRestoreInterruptsSig))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 113, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 134, length, &OSRestoreInterruptsSig))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  16, length, &UpdateOriginSigs[4]) &&
							findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  43, length, &OSRestoreInterruptsSig))
							PADOriginUpdateCallbackSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(PADInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &PADInitSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  11, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  56, i +  57, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  62, i +  63, &address) && address == 0x800030E0)
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  13, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  57, i +  58, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  65, i +  66, &address) && address == 0x800030E0)
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  13, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  59, i +  60, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  67, i +  68, &address) && address == 0x800030E0)
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  15, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  61, i +  62, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  69, i +  70, &address) && address == 0x800030E0)
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  53, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  56, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  87, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  12, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  47, i +  48, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  50, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  56, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  60, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  64, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  75, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 106, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  12, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  47, i +  48, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  50, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  56, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  60, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  64, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  74, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 122, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  50, i +  51, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  59, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  63, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  67, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 125, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if (findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  50, i +  51, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  59, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  63, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  52, i +  67, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  77, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 126, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 11:
						if (findx_pattern(data, dataType, i +  14, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  52, i +  53, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  54, i +  55, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  54, i +  59, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  54, i +  63, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  54, i +  67, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  76, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 125, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 12:
						if (findx_pattern(data, dataType, i +  16, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  54, i +  55, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  57, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  61, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  65, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  69, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  78, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 127, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 13:
						if (findx_pattern(data, dataType, i +  15, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  43, i +  47, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  51, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  67, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 115, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 14:
						if (findx_pattern(data, dataType, i +  16, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  54, i +  55, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  57, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  61, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  65, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  56, i +  69, &address) && address == 0x800030E0)
							PADInitSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(PADReadSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &PADReadSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  55, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  75, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 108, length, &SIGetResponseSigs[0])) {
							findp_pattern(data, dataType, i + 132, 13, length, &SPEC2_MakeStatusSigs[0]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 1:
						if (findx_pattern (data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  68, length, &SIGetResponseSigs[1]) &&
							findx_pattern (data, dataType, i + 105, length, &SIGetResponseSigs[1]) &&
							findx_pattern (data, dataType, i + 164, length, &OSRestoreInterruptsSig)) {
							findp_patterns(data, dataType, i + 128, 13, length, &SPEC2_MakeStatusSigs[0],
							                                                    &SPEC2_MakeStatusSigs[1], NULL);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  81, length, &SIGetResponseSigs[2])) {
							findp_pattern(data, dataType, i +  96, 13, length, &SPEC2_MakeStatusSigs[2]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  64, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  82, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 115, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 136, length, &SIGetResponseSigs[2])) {
							findp_pattern(data, dataType, i + 157, 13, length, &SPEC2_MakeStatusSigs[2]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  64, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  81, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  83, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 114, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 142, length, &SIGetResponseSigs[2])) {
							findp_pattern(data, dataType, i + 163, 13, length, &SPEC2_MakeStatusSigs[2]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 5:
						if (findx_pattern (data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  26, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 119, length, &SIGetResponseSigs[3],
							                                                &SIGetResponseSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 159, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 174, length, &SIGetResponseSigs[3],
							                                                &SIGetResponseSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 230, length, &OSRestoreInterruptsSig)) {
							findp_pattern (data, dataType, i + 194, 13, length, &SPEC2_MakeStatusSigs[2]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 6:
						if (findx_pattern (data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  24, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 117, length, &SIGetResponseSigs[1],
							                                                &SIGetResponseSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 157, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i + 172, length, &SIGetResponseSigs[1],
							                                                &SIGetResponseSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 228, length, &OSRestoreInterruptsSig)) {
							findp_pattern (data, dataType, i + 192, 13, length, &SPEC2_MakeStatusSigs[2]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 7:
						if (findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 112, length, &SIGetResponseSigs[5]) &&
							findx_pattern(data, dataType, i + 168, length, &SIGetResponseSigs[5]) &&
							findx_pattern(data, dataType, i + 225, length, &OSRestoreInterruptsSig)) {
							findp_pattern(data, dataType, i + 188, 13, length, &SPEC2_MakeStatusSigs[3]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
					case 8:
						if (findx_pattern(data, dataType, i +   5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  71, length, &SIGetResponseSigs[4]) &&
							findx_pattern(data, dataType, i +  90, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 114, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 129, length, &SIGetResponseSigs[4]) &&
							findx_pattern(data, dataType, i + 185, length, &OSRestoreInterruptsSig)) {
							findp_pattern(data, dataType, i + 149, 13, length, &SPEC2_MakeStatusSigs[4]);
							PADReadSigs[j].offsetFoundAt = i;
						}
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(SPEC2_MakeStatusSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &SPEC2_MakeStatusSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 159, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 163, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 167, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 171, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 175, length, &ClampU8Sig) &&
							findx_pattern(data, dataType, i + 179, length, &ClampU8Sig))
							SPEC2_MakeStatusSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 191, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 195, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 199, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 203, length, &ClampS8Sig) &&
							findx_pattern(data, dataType, i + 207, length, &ClampU8Sig) &&
							findx_pattern(data, dataType, i + 211, length, &ClampU8Sig))
							SPEC2_MakeStatusSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(SetupTimeoutAlarmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &SetupTimeoutAlarmSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  6, length, &OSCancelAlarmSigs[0]) &&
							get_immediate(data,   i + 19, i + 20, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 26, i + 27, length, &TimeoutHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 28, length, &OSSetAlarmSigs[0]) &&
							get_immediate(data,   i + 35, i + 36, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 53, i + 54, length, &TimeoutHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 55, length, &OSSetAlarmSigs[0]))
							SetupTimeoutAlarmSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  6, length, &OSCancelAlarmSigs[0]) &&
							get_immediate(data,   i + 19, i + 20, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 26, i + 27, length, &TimeoutHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 28, length, &OSSetAlarmSigs[0]) &&
							get_immediate(data,   i + 38, i + 39, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 56, i + 57, length, &TimeoutHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 58, length, &OSSetAlarmSigs[0]) &&
							get_immediate(data,   i + 65, i + 66, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 83, i + 84, length, &TimeoutHandlerSigs[0]) &&
							findx_pattern(data, dataType, i + 85, length, &OSSetAlarmSigs[0]))
							SetupTimeoutAlarmSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  6, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,   i + 18, i + 19, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 21, i + 27, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 30, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,   i + 32, i + 34, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 35, i + 49, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 56, length, &OSSetAlarmSigs[1]))
							SetupTimeoutAlarmSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  6, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,   i + 18, i + 19, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 21, i + 27, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 30, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,   i + 35, i + 37, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 38, i + 52, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 59, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,   i + 61, i + 63, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 64, i + 78, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 85, length, &OSSetAlarmSigs[1]))
							SetupTimeoutAlarmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(RetrySigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &RetrySigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  31, length, &SetupTimeoutAlarmSigs[0]))
							RetrySigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  31, length, &SetupTimeoutAlarmSigs[1]))
							RetrySigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  21, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  31, i +  33, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  34, i +  48, length, &TimeoutHandlerSigs[1]) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetAlarmSigs[1]))
							RetrySigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  21, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  33, i +  34, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  36, i +  42, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  45, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  47, i +  49, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  50, i +  64, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  71, length, &OSSetAlarmSigs[1]))
							RetrySigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  21, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  33, i +  34, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  36, i +  42, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  45, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  50, i +  52, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  53, i +  67, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  74, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  76, i +  78, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  79, i +  93, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 100, length, &OSSetAlarmSigs[1]))
							RetrySigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__CARDStartSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__CARDStartSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  58, length, &SetupTimeoutAlarmSigs[0]))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i +  60, length, &SetupTimeoutAlarmSigs[0],
							                                                &SetupTimeoutAlarmSigs[1], NULL) &&
							findx_pattern (data, dataType, i +  63, length, &OSRestoreInterruptsSig))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  46, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  56, i +  58, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  59, i +  73, length, &TimeoutHandlerSigs[1]) &&
							findx_pattern(data, dataType, i +  80, length, &OSSetAlarmSigs[1]))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  46, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  58, i +  59, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  61, i +  67, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  70, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  72, i +  74, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  75, i +  89, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  96, length, &OSSetAlarmSigs[1]))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  61, i +  62, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  64, i +  70, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  73, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  75, i +  77, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  78, i +  92, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  99, length, &OSSetAlarmSigs[1]) &&
							findx_pattern(data, dataType, i + 102, length, &OSRestoreInterruptsSig))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  49, length, &OSCancelAlarmSigs[1]) &&
							get_immediate(data,  i +  61, i +  62, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  64, i +  70, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i +  73, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i +  78, i +  80, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  93, i +  94, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 101, length, &OSSetAlarmSigs[1]) &&
							get_immediate(data,  i + 103, i + 105, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i + 106, i + 120, length, &TimeoutHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 127, length, &OSSetAlarmSigs[1]) &&
							findx_pattern(data, dataType, i + 130, length, &OSRestoreInterruptsSig))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(CARDGetEncodingSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &CARDGetEncodingSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  8, length, &__CARDGetControlBlockSigs[1]) &&
							findx_pattern(data, dataType, i + 21, length, &__CARDPutControlBlockSigs[1]))
							CARDGetEncodingSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i +  6, length, &__CARDGetControlBlockSigs[2],
							                                               &__CARDGetControlBlockSigs[3], NULL) &&
							findx_pattern (data, dataType, i + 15, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 27, length, &OSRestoreInterruptsSig))
							CARDGetEncodingSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(VerifyIDSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &VerifyIDSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  30, length, &OSGetFontEncodeSigs[0]) &&
							findx_pattern(data, dataType, i +  39, length, &__OSLockSramExSigs[0]) &&
							get_immediate(data,  i +  44, i +  46, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  53,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  64,       0, &constant) && constant == 0x0108 &&
							findx_pattern(data, dataType, i +  74, length, &__OSUnlockSramExSigs[0]) &&
							get_immediate(data,  i +  78, i +  80, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  87,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  92,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 100, length, &__OSUnlockSramExSigs[0]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  30, length, &OSGetFontEncodeSigs[0]) &&
							findx_pattern(data, dataType, i +  39, length, &__OSLockSramExSigs[0]) &&
							get_immediate(data,  i +  44, i +  46, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  53,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  64,       0, &constant) && constant == 0x0110 &&
							findx_pattern(data, dataType, i +  74, length, &__OSUnlockSramExSigs[0]) &&
							get_immediate(data,  i +  78, i +  80, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  87,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  92,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 100, length, &__OSUnlockSramExSigs[0]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  32, length, &__OSLockSramExSigs[0]) &&
							get_immediate(data,  i +  37, i +  39, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  46,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  57,       0, &constant) && constant == 0x0110 &&
							findx_pattern(data, dataType, i +  67, length, &__OSUnlockSramExSigs[0]) &&
							get_immediate(data,  i +  71, i +  73, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i +  80,       0, &constant) && constant == 0x3039 &&
							get_immediate(data,  i +  85,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i +  93, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i +  94, length, &__CARDGetFontEncodeSig))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  88, length, &__OSLockSramSigs[1]) &&
							get_immediate(data,  i +  94, i +  95, &address) && address == 0xCC00206E &&
							findx_pattern(data, dataType, i + 103, length, &__OSUnlockSramSigs[1]) &&
							findx_pattern(data, dataType, i + 112, length, &__OSLockSramExSigs[1]) &&
							get_immediate(data,  i + 115, i + 117, &constant) && constant == 0x2E8BA2E9 &&
							get_immediate(data,  i + 123, i + 126, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i + 134,       0, &constant) && constant == 0x3039 &&
							findx_pattern(data, dataType, i + 147, length, &__OSUnlockSramExSigs[1]) &&
							get_immediate(data,  i + 162,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 169, length, &__OSUnlockSramExSigs[1]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  88, length, &OSGetFontEncodeSigs[1]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSLockSramExSigs[1]) &&
							get_immediate(data,  i + 100, i + 102, &constant) && constant == 0x2E8BA2E9 &&
							get_immediate(data,  i + 108, i + 111, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i + 119,       0, &constant) && constant == 0x3039 &&
							findx_pattern(data, dataType, i + 132, length, &__OSUnlockSramExSigs[1]) &&
							get_immediate(data,  i + 147,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 154, length, &__OSUnlockSramExSigs[1]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  88, length, &OSGetFontEncodeSigs[1]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSLockSramExSigs[1]) &&
							get_immediate(data,  i + 100, i + 102, &constant) && constant == 0x3E0F83E1 &&
							get_immediate(data,  i + 108, i + 111, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i + 119,       0, &constant) && constant == 0x3039 &&
							findx_pattern(data, dataType, i + 132, length, &__OSUnlockSramExSigs[1]) &&
							get_immediate(data,  i + 147,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 154, length, &__OSUnlockSramExSigs[1]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  88, length, &OSGetFontEncodeSigs[1]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSLockSramExSigs[1]) &&
							get_immediate(data,  i + 100, i + 102, &constant) && constant == 0x78787879 &&
							get_immediate(data,  i + 108, i + 111, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i + 119,       0, &constant) && constant == 0x3039 &&
							findx_pattern(data, dataType, i + 132, length, &__OSUnlockSramExSigs[1]) &&
							get_immediate(data,  i + 147,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 154, length, &__OSUnlockSramExSigs[1]))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  90, length, &__OSLockSramExSigs[1]) &&
							get_immediate(data,  i +  93, i +  95, &constant) && constant == 0x78787879 &&
							get_immediate(data,  i + 101, i + 104, &constant) && constant == 0x41C64E6D &&
							get_immediate(data,  i + 112,       0, &constant) && constant == 0x3039 &&
							findx_pattern(data, dataType, i + 125, length, &__OSUnlockSramExSigs[1]) &&
							get_immediate(data,  i + 140,       0, &constant) && constant == 0x7FFF &&
							findx_pattern(data, dataType, i + 147, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 148, length, &__CARDGetFontEncodeSig))
							VerifyIDSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(DoMountSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DoMountSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  83, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 102, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 108, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 120, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 167, length, &__CARDPutControlBlockSigs[0]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 105, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 124, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 130, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 142, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 158, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 163, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 208, length, &__CARDPutControlBlockSigs[1]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 127, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 146, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 152, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 164, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 180, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 185, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 230, length, &__CARDPutControlBlockSigs[1]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 112, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 131, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 137, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 149, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 165, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 170, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 215, length, &__CARDPutControlBlockSigs[1]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 130, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 149, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 155, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 167, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 183, length, &__OSLockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 188, length, &__OSUnlockSramExSigs[0]) &&
							findx_pattern(data, dataType, i + 233, length, &__CARDPutControlBlockSigs[1]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  70, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 130, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 132, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 162, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 206, length, &__CARDPutControlBlockSigs[2]) &&
							findx_pattern(data, dataType, i + 217, length, &__CARDPutControlBlockSigs[2]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern (data, dataType, i +  76, length, &__OSLockSramExSigs[1]) &&
							findx_pattern (data, dataType, i + 136, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern (data, dataType, i + 141, length, &__OSLockSramExSigs[1]) &&
							findx_pattern (data, dataType, i + 171, length, &__OSUnlockSramExSigs[1]) &&
							findx_patterns(data, dataType, i + 216, length, &__CARDPutControlBlockSigs[2],
							                                                &__CARDPutControlBlockSigs[3], NULL))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  98, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 158, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 163, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 193, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 209, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 213, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 254, length, &__CARDPutControlBlockSigs[3]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i + 119, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 179, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 184, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 214, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 230, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 234, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 275, length, &__CARDPutControlBlockSigs[3]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  82, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 142, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 147, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 177, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 193, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 197, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 238, length, &__CARDPutControlBlockSigs[3]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 10:
						if (findx_pattern(data, dataType, i +  99, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 159, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 164, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 194, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 210, length, &__OSLockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 214, length, &__OSUnlockSramExSigs[1]) &&
							findx_pattern(data, dataType, i + 255, length, &__CARDPutControlBlockSigs[3]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		i += fp.Length - 1;
	}
	
	for (j = 0; j < sizeof(InitializeUARTSigs) / sizeof(FuncPattern); j++)
	if ((i = InitializeUARTSigs[j].offsetFoundAt)) {
		u32 *InitializeUART = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (InitializeUART) {
			if ((devices[DEVICE_CUR]->features & FEAT_HYPERVISOR) && swissSettings.debugUSB && !swissSettings.wiirdDebug) {
				memset(data + i, 0, InitializeUARTSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000;	// li		r3, 0
				data[i + 1] = 0x4E800020;	// blr
			}
			print_gecko("Found:[%s$%i] @ %08X\n", InitializeUARTSigs[j].Name, j, InitializeUART);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(WriteUARTNSigs) / sizeof(FuncPattern); j++)
	if ((i = WriteUARTNSigs[j].offsetFoundAt)) {
		u32 *WriteUARTN = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (WriteUARTN) {
			if ((devices[DEVICE_CUR]->features & FEAT_HYPERVISOR) && swissSettings.debugUSB && !swissSettings.wiirdDebug) {
				memset(data + i, 0, WriteUARTNSigs[j].Length * sizeof(u32));
				memcpy(data + i, WriteUARTNSigs[j].Patch, WriteUARTNSigs[j].PatchLength);
			}
			print_gecko("Found:[%s$%i] @ %08X\n", WriteUARTNSigs[j].Name, j, WriteUARTN);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(UpdateOriginSigs) / sizeof(FuncPattern); j++)
	if ((i = UpdateOriginSigs[j].offsetFoundAt)) {
		u32 *UpdateOrigin = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (UpdateOrigin) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0: data[i + 79] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 1: data[i + 82] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 75] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 3: data[i + 77] = 0x20A5007F; break;	// subfic	r5, r5, 127
					case 4: data[i + 81] = 0x2084007F; break;	// subfic	r4, r4, 127
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0: data[i + 82] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 1: data[i + 85] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 78] = 0x2004007F; break;	// subfic	r0, r4, 127
					case 3: data[i + 80] = 0x20A5007F; break;	// subfic	r5, r5, 127
					case 4: data[i + 84] = 0x2084007F; break;	// subfic	r4, r4, 127
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", UpdateOriginSigs[j].Name, j, UpdateOrigin);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = PADOriginCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginCallback) {
			if (j == 5) {
				if (swissSettings.invertCStick & 1)
					data[i + 86] = 0x2004007F;	// subfic	r0, r4, 127
				if (swissSettings.invertCStick & 2)
					data[i + 89] = 0x2004007F;	// subfic	r0, r4, 127
			}
			print_gecko("Found:[%s$%i] @ %08X\n", PADOriginCallbackSigs[j].Name, j, PADOriginCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = PADOriginUpdateCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginUpdateCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginUpdateCallback) {
			if (j == 5) {
				if (swissSettings.invertCStick & 1)
					data[i + 93] = 0x2003007F;	// subfic	r0, r3, 127
				if (swissSettings.invertCStick & 2)
					data[i + 96] = 0x2003007F;	// subfic	r0, r3, 127
			}
			print_gecko("Found:[%s$%i] @ %08X\n", PADOriginUpdateCallbackSigs[j].Name, j, PADOriginUpdateCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADReadSigs) / sizeof(FuncPattern); j++)
	if ((i = PADReadSigs[j].offsetFoundAt)) {
		u32 *PADRead = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADRead) {
			if (devices[DEVICE_CUR]->features & FEAT_HYPERVISOR) {
				switch (j) {
					case 0:
						data[i + 159] = 0x387E0000;	// addi		r3, r30, 0
						data[i + 160] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 161] = branchAndLink(CHECK_PAD, PADRead + 161);
						break;
					case 1:
						data[i + 156] = 0x387E0000;	// addi		r3, r30, 0
						data[i + 157] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 158] = branchAndLink(CHECK_PAD, PADRead + 158);
						break;
					case 2:
						data[i + 100] = 0x387D0000;	// addi		r3, r29, 0
						data[i + 101] = 0x389B0000;	// addi		r4, r27, 0
						data[i + 102] = branchAndLink(CHECK_PAD, PADRead + 102);
						break;
					case 3:
						data[i + 185] = 0x38760000;	// addi		r3, r22, 0
						data[i + 186] = 0x38950000;	// addi		r4, r21, 0
						data[i + 187] = branchAndLink(CHECK_PAD, PADRead + 187);
						break;
					case 4:
						data[i + 190] = 0x38770000;	// addi		r3, r23, 0
						data[i + 191] = 0x38950000;	// addi		r4, r21, 0
						data[i + 192] = branchAndLink(CHECK_PAD, PADRead + 192);
						break;
					case 5:
						data[i + 221] = 0x38750000;	// addi		r3, r21, 0
						data[i + 222] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 223] = branchAndLink(CHECK_PAD, PADRead + 223);
						break;
					case 6:
						data[i + 219] = 0x38750000;	// addi		r3, r21, 0
						data[i + 220] = 0x389F0000;	// addi		r4, r31, 0
						data[i + 221] = branchAndLink(CHECK_PAD, PADRead + 221);
						break;
					case 7:
						data[i + 216] = 0x7F63DB78;	// mr		r3, r27
						data[i + 217] = 0x7F24CB78;	// mr		r4, r25
						data[i + 218] = branchAndLink(CHECK_PAD, PADRead + 218);
						break;
					case 8:
						data[i + 176] = 0x38790000;	// addi		r3, r25, 0
						data[i + 177] = 0x38970000;	// addi		r4, r23, 0
						data[i + 178] = branchAndLink(CHECK_PAD, PADRead + 178);
						break;
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", PADReadSigs[j].Name, j, PADRead);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADSetSpecSigs) / sizeof(FuncPattern); j++)
	if ((i = PADSetSpecSigs[j].offsetFoundAt)) {
		u32 *PADSetSpec = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADSetSpec) {
			switch (j) {
				case 0:
					findi_pattern (data, dataType, i + 25, i + 26, length, &SPEC0_MakeStatusSigs[0]);
					findi_pattern (data, dataType, i + 29, i + 30, length, &SPEC1_MakeStatusSigs[0]);
					findi_patterns(data, dataType, i + 33, i + 34, length, &SPEC2_MakeStatusSigs[0],
					                                                       &SPEC2_MakeStatusSigs[1], NULL);
					break;
				case 1:
					findi_patterns(data, dataType, i + 11, i + 12, length, &SPEC0_MakeStatusSigs[1],
					                                                       &SPEC0_MakeStatusSigs[2], NULL);
					findi_patterns(data, dataType, i + 15, i + 16, length, &SPEC1_MakeStatusSigs[1],
					                                                       &SPEC1_MakeStatusSigs[2], NULL);
					findi_patterns(data, dataType, i + 19, i + 20, length, &SPEC2_MakeStatusSigs[2],
					                                                       &SPEC2_MakeStatusSigs[3],
					                                                       &SPEC2_MakeStatusSigs[4], NULL);
					break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", PADSetSpecSigs[j].Name, j, PADSetSpec);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SPEC0_MakeStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = SPEC0_MakeStatusSigs[j].offsetFoundAt)) {
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
			print_gecko("Found:[%s$%i] @ %08X\n", SPEC0_MakeStatusSigs[j].Name, j, SPEC0_MakeStatus);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SPEC1_MakeStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = SPEC1_MakeStatusSigs[j].offsetFoundAt)) {
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
			print_gecko("Found:[%s$%i] @ %08X\n", SPEC1_MakeStatusSigs[j].Name, j, SPEC1_MakeStatus);
			patched++;
		}
	}
	
	if ((i = ClampS8Sig.offsetFoundAt)) {
		u32 *ClampS8 = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (ClampS8) {
			data[i + 3] = 0x41800020;	// blt		+8
			data[i + 4] = 0x3BE4FF81;	// subi		r31, r4, 127
			
			print_gecko("Found:[%s] @ %08X\n", ClampS8Sig.Name, ClampS8);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SPEC2_MakeStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = SPEC2_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC2_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC2_MakeStatus) {
			switch (j) {
				case 2:
					data[i + 150] = 0x41800024;	// blt		+9
					data[i + 152] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 173] = 0x41800024;	// blt		+9
					data[i + 175] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 196] = 0x41800024;	// blt		+9
					data[i + 198] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 219] = 0x41800024;	// blt		+9
					data[i + 221] = 0x3805FF81;	// subi		r0, r5, 127
					break;
				case 3:
					data[i + 138] = 0x41800020;	// blt		+8
					data[i + 139] = 0x38A7FF81;	// subi		r5, r7, 127
					data[i + 159] = 0x41800020;	// blt		+8
					data[i + 160] = 0x38A7FF81;	// subi		r5, r7, 127
					data[i + 180] = 0x41800020;	// blt		+8
					data[i + 181] = 0x38A7FF81;	// subi		r5, r7, 127
					data[i + 201] = 0x41800020;	// blt		+8
					data[i + 202] = 0x38A7FF81;	// subi		r5, r7, 127
					break;
				case 4:
					data[i + 180] = 0x41800024;	// blt		+9
					data[i + 182] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 203] = 0x41800024;	// blt		+9
					data[i + 205] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 226] = 0x41800024;	// blt		+9
					data[i + 228] = 0x3805FF81;	// subi		r0, r5, 127
					data[i + 249] = 0x41800024;	// blt		+9
					data[i + 251] = 0x3805FF81;	// subi		r0, r5, 127
					break;
			}
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0:
					case 1: data[i + 147] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 142] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 3: data[i + 130] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 4: data[i + 142] = 0x2006007F; break;	// subfic	r0, r6, 127
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0:
					case 1: data[i + 150] = 0x2003007F; break;	// subfic	r0, r3, 127
					case 2: data[i + 145] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 3: data[i + 133] = 0x2005007F; break;	// subfic	r0, r5, 127
					case 4: data[i + 145] = 0x2006007F; break;	// subfic	r0, r6, 127
				}
			}
			print_gecko("Found:[%s$%i] @ %08X\n", SPEC2_MakeStatusSigs[j].Name, j, SPEC2_MakeStatus);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SetupTimeoutAlarmSigs) / sizeof(FuncPattern); j++)
	if ((i = SetupTimeoutAlarmSigs[j].offsetFoundAt)) {
		u32 *SetupTimeoutAlarm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SetupTimeoutAlarm) {
			switch (j) {
				case 0:
				case 1: data[i + 24] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 2:
				case 3: data[i + 26] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
			}
			print_gecko("Found:[%s$%i] @ %08X\n", SetupTimeoutAlarmSigs[j].Name, j, SetupTimeoutAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(RetrySigs) / sizeof(FuncPattern); j++)
	if ((i = RetrySigs[j].offsetFoundAt)) {
		u32 *Retry = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (Retry) {
			switch (j) {
				case 3:
				case 4: data[i + 41] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
			}
			print_gecko("Found:[%s$%i] @ %08X\n", RetrySigs[j].Name, j, Retry);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__CARDStartSigs) / sizeof(FuncPattern); j++)
	if ((i = __CARDStartSigs[j].offsetFoundAt)) {
		u32 *__CARDStart = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__CARDStart) {
			switch (j) {
				case 3: data[i + 66] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 4:
				case 5: data[i + 69] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
			}
			print_gecko("Found:[%s$%i] @ %08X\n", __CARDStartSigs[j].Name, j, __CARDStart);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(CARDGetEncodingSigs) / sizeof(FuncPattern); j++)
	if ((i = CARDGetEncodingSigs[j].offsetFoundAt)) {
		u32 *CARDGetEncoding = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (CARDGetEncoding) {
			switch (j) {
				case 0: data[i + 16] = 0x38000000 | (swissSettings.fontEncode & 0xFFFF); break;
				case 1: data[i + 12] = 0x38000000 | (swissSettings.fontEncode & 0xFFFF); break;
			}
			print_gecko("Found:[%s$%i] @ %08X\n", CARDGetEncodingSigs[j].Name, j, CARDGetEncoding);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(VerifyIDSigs) / sizeof(FuncPattern); j++)
	if ((i = VerifyIDSigs[j].offsetFoundAt)) {
		u32 *VerifyID = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VerifyID) {
			memset(data + i, 0, VerifyIDSigs[j].Length * sizeof(u32));
			
			data[i + 0] = 0x38600000;	// li		r3, 0
			data[i + 1] = 0x4E800020;	// blr
			
			print_gecko("Found:[%s$%i] @ %08X\n", VerifyIDSigs[j].Name, j, VerifyID);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DoMountSigs) / sizeof(FuncPattern); j++)
	if ((i = DoMountSigs[j].offsetFoundAt)) {
		u32 *DoMount = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DoMount) {
			switch (j) {
				case  0: data[i + 128] = 0x48000004; break;	// b		+1
				case  1: data[i + 150] = 0x48000004; break;	// b		+1
				case  2: data[i + 172] = 0x48000004; break;	// b		+1
				case  3: data[i + 157] = 0x48000004; break;	// b		+1
				case  4: data[i + 175] = 0x48000004; break;	// b		+1
				case  5: data[i + 170] = 0x48000004; break;	// b		+1
				case  6: data[i + 179] = 0x48000004; break;	// b		+1
				case  7: data[i + 201] = 0x48000004; break;	// b		+1
				case  8: data[i + 222] = 0x48000004; break;	// b		+1
				case  9: data[i + 185] = 0x48000004; break;	// b		+1
				case 10: data[i + 202] = 0x48000004; break;	// b		+1
			}
			print_gecko("Found:[%s$%i] @ %08X\n", DoMountSigs[j].Name, j, DoMount);
			patched++;
		}
	}
	return patched;
}

void *Calc_ProperAddress(void *data, int dataType, u32 offsetFoundAt) {
	if(dataType == PATCH_DOL || dataType == PATCH_DOL_PRS) {
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
				if(offsetFoundAt >= hdr->textOffset[i] && offsetFoundAt < hdr->textOffset[i] + hdr->textLength[i]) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return (void*)(offsetFoundAt+hdr->textAddress[i]-hdr->textOffset[i]);
				}
			}
		}

		// Inspect data sections (shouldn't really need to unless someone was sneaky..)
		for (i = 0; i < MAXDATASECTION; i++) {
			if (hdr->dataAddress[i] && hdr->dataLength[i]) {
				// Does what we found lie in this section?
				if(offsetFoundAt >= hdr->dataOffset[i] && offsetFoundAt < hdr->dataOffset[i] + hdr->dataLength[i]) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return (void*)(offsetFoundAt+hdr->dataAddress[i]-hdr->dataOffset[i]);
				}
			}
		}
	
	}
	else if(dataType == PATCH_ELF) {
		if(!valid_elf_image(data))
			return NULL;

		Elf32_Ehdr *ehdr = (Elf32_Ehdr *) data;
		Elf32_Phdr *phdr = (Elf32_Phdr *)(data + ehdr->e_phoff);
		int i;

		// Inspect loadable segments to see if what we found lies in here
		for (i = 0; i < ehdr->e_phnum; i++) {
			if (phdr[i].p_type == PT_LOAD) {
				// Does what we found lie in this segment?
				if(offsetFoundAt >= phdr[i].p_offset && offsetFoundAt < phdr[i].p_offset + phdr[i].p_filesz) {
					// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
					return (void*)(offsetFoundAt+phdr[i].p_vaddr-phdr[i].p_offset);
				}
			}
		}
	}
	else if(dataType == PATCH_APPLOADER) {
		u32 offset = sizeof(ApploaderHeader);
		ApploaderHeader *apploaderHeader = (ApploaderHeader *) data;

		if(offsetFoundAt >= offset && offsetFoundAt < offset + apploaderHeader->size) {
			return (void*)(offsetFoundAt+0x81200000-offset);
		}
		else if(apploaderHeader->rebootSize) {
			offset += apploaderHeader->size;

			if(strncmp(apploaderHeader->date, "2004/02/01", 10) <= 0) {
				if(offsetFoundAt >= offset && offsetFoundAt < offset + apploaderHeader->rebootSize)
					return (void*)(offsetFoundAt+0x81300000-offset);
			}
			else {
				return Calc_ProperAddress(data + offset, PATCH_DOL, offsetFoundAt - offset);
			}
		}
	}
	else if(dataType == PATCH_BS2) {
		if(offsetFoundAt < 0x1AF6E0)
			return (void*)(offsetFoundAt+0x81300000);
	}
	return NULL;
}

void *Calc_Address(void *data, int dataType, u32 properAddress) {
	if(dataType == PATCH_DOL || dataType == PATCH_DOL_PRS) {
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
				if(properAddress >= hdr->textAddress[i] && properAddress < hdr->textAddress[i] + hdr->textLength[i]) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-hdr->textAddress[i]+hdr->textOffset[i];
				}
			}
		}

		// Inspect data sections (shouldn't really need to unless someone was sneaky..)
		for (i = 0; i < MAXDATASECTION; i++) {
			if (hdr->dataAddress[i] && hdr->dataLength[i]) {
				// Does what we found lie in this section?
				if(properAddress >= hdr->dataAddress[i] && properAddress < hdr->dataAddress[i] + hdr->dataLength[i]) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-hdr->dataAddress[i]+hdr->dataOffset[i];
				}
			}
		}
	
	}
	else if(dataType == PATCH_ELF) {
		if(!valid_elf_image(data))
			return NULL;

		Elf32_Ehdr *ehdr = (Elf32_Ehdr *) data;
		Elf32_Phdr *phdr = (Elf32_Phdr *)(data + ehdr->e_phoff);
		int i;

		// Inspect loadable segments to see if what we found lies in here
		for (i = 0; i < ehdr->e_phnum; i++) {
			if (phdr[i].p_type == PT_LOAD) {
				// Does what we found lie in this segment?
				if(properAddress >= phdr[i].p_vaddr && properAddress < phdr[i].p_vaddr + phdr[i].p_filesz) {
					// Yes it does, return the properAddress - load address as that's where it'll end up!
					return data+properAddress-phdr[i].p_vaddr+phdr[i].p_offset;
				}
			}
		}	
	}
	else if(dataType == PATCH_APPLOADER) {
		u32 offset = sizeof(ApploaderHeader);
		ApploaderHeader *apploaderHeader = (ApploaderHeader *) data;

		if(properAddress >= 0x81200000 && properAddress < 0x81200000 + apploaderHeader->size) {
			return data+properAddress-0x81200000+offset;
		}
		else if(apploaderHeader->rebootSize) {
			offset += apploaderHeader->size;

			if(strncmp(apploaderHeader->date, "2004/02/01", 10) <= 0) {
				if(properAddress >= 0x81300000 && properAddress < 0x81300000 + apploaderHeader->rebootSize)
					return data+properAddress-0x81300000+offset;
			}
			else {
				return Calc_Address(data + offset, PATCH_DOL, properAddress);
			}
		}
	}
	else if(dataType == PATCH_BS2) {
		if(properAddress >= 0x81300000 && properAddress < 0x814AF6E0)
			return data+properAddress-0x81300000;
	}
	return NULL;
}

// Ocarina cheat engine hook - Patch OSSleepThread
int Patch_CheatsHook(u8 *data, u32 length, u32 type) {
	if(type == PATCH_APPLOADER || type == PATCH_BS2)
		return 0;

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


