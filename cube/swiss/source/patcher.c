/* DOL Patching code by emu_kidid */


#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/machine/processor.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <libdeflate.h>
#include <zlib.h>
#include "swiss.h"
#include "main.h"
#include "patcher.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"
#include "sidestep.h"
#include "elf.h"
#include "ata.h"
#include "bba.h"
#include "cheats.h"

char VAR_AREA[0x3100];

static u32 top_addr;

static void *patch_locations[PATCHES_MAX];

// Returns where the ASM patch has been copied to
void *installPatch(int patchId) {
	void *patchLocation = NULL;
	const void *patch = NULL; u32 patchSize = 0;
	switch(patchId) {
		case BACKWARDS_MEMCPY:
			patch = backwards_memcpy_bin; patchSize = backwards_memcpy_bin_size; break;
		case DVD_LOWTESTALARMHOOK:
			patch = DVDLowTestAlarmHook; patchSize = DVDLowTestAlarmHook_size; break;
		case GX_COPYDISPHOOK:
			patch = GXCopyDispHook; patchSize = GXCopyDispHook_size; break;
		case GX_INITTEXOBJLODHOOK:
			patch = GXInitTexObjLODHook; patchSize = GXInitTexObjLODHook_size; break;
		case GX_SETPROJECTIONHOOK:
			patch = GXSetProjectionHook; patchSize = GXSetProjectionHook_size; break;
		case GX_SETSCISSORHOOK:
			patch = GXSetScissorHook; patchSize = GXSetScissorHook_size; break;
		case MTX_FRUSTUMHOOK:
			patch = MTXFrustumHook; patchSize = MTXFrustumHook_size; break;
		case MTX_LIGHTFRUSTUMHOOK:
			patch = MTXLightFrustumHook; patchSize = MTXLightFrustumHook_size; break;
		case MTX_LIGHTPERSPECTIVEHOOK:
			patch = MTXLightPerspectiveHook; patchSize = MTXLightPerspectiveHook_size; break;
		case MTX_ORTHOHOOK:
			patch = MTXOrthoHook; patchSize = MTXOrthoHook_size; break;
		case MTX_PERSPECTIVEHOOK:
			patch = MTXPerspectiveHook; patchSize = MTXPerspectiveHook_size; break;
		case OS_CALLALARMHANDLER:
			patch = CallAlarmHandler_bin; patchSize = CallAlarmHandler_bin_size; break;
		case OS_RESERVED:
			patchSize = 0x1800; break;
		case PAD_CHECKSTATUS:
			patch = CheckStatus_bin; patchSize = CheckStatus_bin_size; break;
		case PAD_CHECKSTATUS_GCDIGITAL:
			patch = CheckStatusGCDigital_bin; patchSize = CheckStatusGCDigital_bin_size; break;
		case VI_CONFIGURE240P:
			patch = VIConfigure240p; patchSize = VIConfigure240p_size; break;
		case VI_CONFIGURE288P:
			patch = VIConfigure288p; patchSize = VIConfigure288p_size; break;
		case VI_CONFIGURE480I:
			patch = VIConfigure480i; patchSize = VIConfigure480i_size; break;
		case VI_CONFIGURE480P:
			patch = VIConfigure480p; patchSize = VIConfigure480p_size; break;
		case VI_CONFIGURE540P50:
			patch = VIConfigure540p50; patchSize = VIConfigure540p50_size; break;
		case VI_CONFIGURE540P60:
			patch = VIConfigure540p60; patchSize = VIConfigure540p60_size; break;
		case VI_CONFIGURE576I:
			patch = VIConfigure576i; patchSize = VIConfigure576i_size; break;
		case VI_CONFIGURE576P:
			patch = VIConfigure576p; patchSize = VIConfigure576p_size; break;
		case VI_CONFIGURE960I:
			patch = VIConfigure960i; patchSize = VIConfigure960i_size; break;
		case VI_CONFIGURE1080I50:
			patch = VIConfigure1080i50; patchSize = VIConfigure1080i50_size; break;
		case VI_CONFIGURE1080I60:
			patch = VIConfigure1080i60; patchSize = VIConfigure1080i60_size; break;
		case VI_CONFIGURE1152I:
			patch = VIConfigure1152i; patchSize = VIConfigure1152i_size; break;
		case VI_CONFIGUREAUTOP:
			patch = VIConfigureAutop; patchSize = VIConfigureAutop_size; break;
		case VI_CONFIGUREFIELDMODE:
			patch = VIConfigureFieldMode; patchSize = VIConfigureFieldMode_size; break;
		case VI_CONFIGUREHOOK1:
			patch = VIConfigureHook1; patchSize = VIConfigureHook1_size; break;
		case VI_CONFIGUREHOOK1_GCVIDEO:
			patch = VIConfigureHook1GCVideo; patchSize = VIConfigureHook1GCVideo_size; break;
		case VI_CONFIGUREHOOK1_RT4K:
			patch = VIConfigureHook1RT4K; patchSize = VIConfigureHook1RT4K_size; break;
		case VI_CONFIGUREHOOK2:
			patch = VIConfigureHook2; patchSize = VIConfigureHook2_size; break;
		case VI_CONFIGURENOYSCALE:
			patch = VIConfigureNoYScale; patchSize = VIConfigureNoYScale_size; break;
		case VI_CONFIGUREPANHOOK:
			patch = VIConfigurePanHook; patchSize = VIConfigurePanHook_size; break;
		case VI_CONFIGUREPANHOOKD:
			patch = VIConfigurePanHookD; patchSize = VIConfigurePanHookD_size; break;
		case VI_GETRETRACECOUNTHOOK:
			patch = VIGetRetraceCountHook; patchSize = VIGetRetraceCountHook_size; break;
		case VI_RETRACEHANDLERHOOK:
			patch = VIRetraceHandlerHook; patchSize = VIRetraceHandlerHook_size; break;
		default:
			break;
	}
	patchLocation = installPatch2(patch, patchSize);
	print_debug("Installed patch %i to %08X\n", patchId, patchLocation);
	return patchLocation;
}

void *installPatch2(const void *patch, u32 patchSize) {
	void *patchLocation = NULL;
	void *addr;
	if (!top_addr) {
		addr = SYS_AllocArenaMemHi(patchSize, patch ? 4 : 32);
	} else {
		if (top_addr == 0x81800000)
			top_addr -= 8;
		top_addr -= patchSize;
		top_addr &= patch ? ~3 : ~31;
		addr = (void *)top_addr;
	}
	patchLocation = Calc_Address(NULL, PATCH_OTHER, (u32)addr);
	if (patch) memcpy(patchLocation, patch, patchSize);
	else memset(patchLocation, 0, patchSize);
	DCFlushRange(patchLocation, patchSize);
	ICInvalidateRange(patchLocation, patchSize);
	return addr;
}

// See patchIds enum in patcher.h
void *getPatchAddr(int patchId) {
	if(patchId >= PATCHES_MAX || patchId < 0) {
		print_debug("Invalid Patch location requested\n");
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
		if (patch_locations[patchId] < (void *)top_addr)
			patch_locations[patchId] = NULL;
}

u32 getTopAddr() {
	return top_addr & ~31;
}

int install_code(int final)
{
	void *patchLocation = NULL;
	const void *patch = NULL; u32 patchSize = 0;
	
	// Reload Stub
	if (!top_addr) {
		u32 size = SYS_GetPhysicalMemSize();
		*(u32 *)0x80000034 = (u32)MEM_PHYSICAL_TO_K0(size);
		*(u32 *)0x800000F0 = size;
		return 1;
	}
	// IDE-EXI
	else if(devices[DEVICE_CUR] == &__device_ata_a || devices[DEVICE_CUR] == &__device_ata_b || devices[DEVICE_CUR] == &__device_ata_c) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ | EMU_BUS_ARBITER:
			case EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER:
				patch     = !_ideexi_version ? ideexi_v1_bin      : ideexi_v2_bin;
				patchSize = !_ideexi_version ? ideexi_v1_bin_size : ideexi_v2_bin_size;
				break;
			case EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER:
				patch     = !_ideexi_version ? ideexi_v1_dtk_bin      : ideexi_v2_dtk_bin;
				patchSize = !_ideexi_version ? ideexi_v1_dtk_bin_size : ideexi_v2_dtk_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
				patch     = !_ideexi_version ? ideexi_v1_card_bin      : ideexi_v2_card_bin;
				patchSize = !_ideexi_version ? ideexi_v1_card_bin_size : ideexi_v2_card_bin_size;
				break;
			case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
				if (!strcmp(bba_device_str, "ENC28J60")) {
					patch     = !_ideexi_version ? ideexi_v1_enc28j60_eth_bin      : ideexi_v2_enc28j60_eth_bin;
					patchSize = !_ideexi_version ? ideexi_v1_enc28j60_eth_bin_size : ideexi_v2_enc28j60_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W5500")) {
					patch     = !_ideexi_version ? ideexi_v1_w5500_eth_bin      : ideexi_v2_w5500_eth_bin;
					patchSize = !_ideexi_version ? ideexi_v1_w5500_eth_bin_size : ideexi_v2_w5500_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W6100")) {
					patch     = !_ideexi_version ? ideexi_v1_w6100_eth_bin      : ideexi_v2_w6100_eth_bin;
					patchSize = !_ideexi_version ? ideexi_v1_w6100_eth_bin_size : ideexi_v2_w6100_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W6300")) {
					patch     = !_ideexi_version ? ideexi_v1_w6300_eth_bin      : ideexi_v2_w6300_eth_bin;
					patchSize = !_ideexi_version ? ideexi_v1_w6300_eth_bin_size : ideexi_v2_w6300_eth_bin_size;
				} else
					return 0;
				break;
			default:
				return 0;
		}
		print_debug("Installing Patch for IDE-EXI\n");
	}
	// SD Card over EXI
	else if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
		if (strcmp(devices[DEVICE_CUR]->hwName, "Semi-Passive SD Card Adapter")) {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_READ | EMU_BUS_ARBITER:
				case EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER:
					patch     = sd_v1_bin;
					patchSize = sd_v1_bin_size;
					break;
				case EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER:
					patch     = sd_v1_dtk_bin;
					patchSize = sd_v1_dtk_bin_size;
					break;
				case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
					patch     = sd_v1_card_bin;
					patchSize = sd_v1_card_bin_size;
					break;
				case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					if (!strcmp(bba_device_str, "ENC28J60")) {
						patch     = sd_v1_enc28j60_eth_bin;
						patchSize = sd_v1_enc28j60_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W5500")) {
						patch     = sd_v1_w5500_eth_bin;
						patchSize = sd_v1_w5500_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6100")) {
						patch     = sd_v1_w6100_eth_bin;
						patchSize = sd_v1_w6100_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6300")) {
						patch     = sd_v1_w6300_eth_bin;
						patchSize = sd_v1_w6300_eth_bin_size;
					} else
						return 0;
					break;
				default:
					return 0;
			}
		} else {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_READ | EMU_BUS_ARBITER:
				case EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER:
					patch     = sd_v2_bin;
					patchSize = sd_v2_bin_size;
					break;
				case EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER:
					patch     = sd_v2_dtk_bin;
					patchSize = sd_v2_dtk_bin_size;
					break;
				case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
					patch     = sd_v2_card_bin;
					patchSize = sd_v2_card_bin_size;
					break;
				case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					if (!strcmp(bba_device_str, "ENC28J60")) {
						patch     = sd_v2_enc28j60_eth_bin;
						patchSize = sd_v2_enc28j60_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W5500")) {
						patch     = sd_v2_w5500_eth_bin;
						patchSize = sd_v2_w5500_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6100")) {
						patch     = sd_v2_w6100_eth_bin;
						patchSize = sd_v2_w6100_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6300")) {
						patch     = sd_v2_w6300_eth_bin;
						patchSize = sd_v2_w6300_eth_bin_size;
					} else
						return 0;
					break;
				default:
					return 0;
			}
		}
		print_debug("Installing Patch for SD Card over EXI\n");
	}
	// DVD
	else if(devices[DEVICE_CUR] == &__device_dvd || devices[DEVICE_CUR] == &__device_wode) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_NONE:
				break;
			case EMU_READ:
			case EMU_READ | EMU_BUS_ARBITER:
				patch     = dvd_bin;
				patchSize = dvd_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
				patch     = dvd_card_bin;
				patchSize = dvd_card_bin_size;
				break;
			default:
				return 0;
		}
		print_debug("Installing Patch for DVD\n");
	}
	// USB Gecko
	else if(devices[DEVICE_CUR] == &__device_usbgecko) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_READ | EMU_BUS_ARBITER:
				patch     = usbgecko_bin;
				patchSize = usbgecko_bin_size;
				break;
			default:
				return 0;
		}
		print_debug("Installing Patch for USB Gecko\n");
	}
	// Wiikey Fusion
	else if(devices[DEVICE_CUR] == &__device_wkf) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_NONE:
				break;
			case EMU_READ:
			case EMU_READ | EMU_BUS_ARBITER:
				patch     = wkf_bin;
				patchSize = wkf_bin_size;
				break;
			case EMU_READ | EMU_AUDIO_STREAMING:
			case EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER:
				patch     = wkf_dtk_bin;
				patchSize = wkf_dtk_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
				patch     = wkf_card_bin;
				patchSize = wkf_card_bin_size;
				break;
			default:
				return 0;
		}
		print_debug("Installing Patch for WKF\n");
	}
	// Broadband Adapter
	else if(devices[DEVICE_CUR] == &__device_fsp) {
		if (devices[DEVICE_PATCHES] != &__device_fsp) {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_READ | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					patch     = fsp_bin;
					patchSize = fsp_bin_size;
					break;
				default:
					return 0;
			}
		} else {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_READ | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					patch     = fsp_bin;
					patchSize = fsp_bin_size;
					break;
				case EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					patch     = fsp_dtk_bin;
					patchSize = fsp_dtk_bin_size;
					break;
				case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					patch     = fsp_eth_bin;
					patchSize = fsp_eth_bin_size;
					break;
				default:
					return 0;
			}
		}
		print_debug("Installing Patch for File Service Protocol\n");
	}
	// GC Loader
	else if(devices[DEVICE_CUR] == &__device_gcloader) {
		if (devices[DEVICE_PATCHES] != &__device_gcloader) {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_NONE:
					break;
				case EMU_READ:
				case EMU_READ | EMU_BUS_ARBITER:
				case EMU_READ | EMU_READ_SPEED:
				case EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER:
					patch     = gcloader_v1_bin;
					patchSize = gcloader_v1_bin_size;
					break;
				case EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER:
					patch     = gcloader_v1_card_bin;
					patchSize = gcloader_v1_card_bin_size;
					break;
				default:
					return 0;
			}
		} else {
			switch (devices[DEVICE_CUR]->emulated()) {
				case EMU_NONE:
					break;
				case EMU_READ:
				case EMU_READ | EMU_READ_SPEED:
					patch     = gcloader_v2_bin;
					patchSize = gcloader_v2_bin_size;
					break;
				case EMU_READ | EMU_AUDIO_STREAMING:
					patch     = gcloader_v2_dtk_bin;
					patchSize = gcloader_v2_dtk_bin_size;
					break;
				case EMU_READ | EMU_MEMCARD:
					patch     = gcloader_v2_card_bin;
					patchSize = gcloader_v2_card_bin_size;
					break;
				case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
					if (!strcmp(bba_device_str, "ENC28J60")) {
						patch     = gcloader_v2_enc28j60_eth_bin;
						patchSize = gcloader_v2_enc28j60_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W5500")) {
						patch     = gcloader_v2_w5500_eth_bin;
						patchSize = gcloader_v2_w5500_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6100")) {
						patch     = gcloader_v2_w6100_eth_bin;
						patchSize = gcloader_v2_w6100_eth_bin_size;
					} else if (!strcmp(bba_device_str, "WIZnet W6300")) {
						patch     = gcloader_v2_w6300_eth_bin;
						patchSize = gcloader_v2_w6300_eth_bin_size;
					} else
						return 0;
					break;
				default:
					return 0;
			}
		}
		print_debug("Installing Patch for GC Loader\n");
	}
	// FlippyDrive
	else if(devices[DEVICE_CUR] == &__device_flippy || devices[DEVICE_CUR] == &__device_flippyflash) {
		switch (devices[DEVICE_CUR]->emulated()) {
			case EMU_NONE:
				break;
			case EMU_READ:
			case EMU_READ | EMU_READ_SPEED:
				patch     = flippy_bin;
				patchSize = flippy_bin_size;
				break;
			case EMU_READ | EMU_MEMCARD:
				patch     = flippy_card_bin;
				patchSize = flippy_card_bin_size;
				break;
			case EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING:
				if (!strcmp(bba_device_str, "ENC28J60")) {
					patch     = flippy_enc28j60_eth_bin;
					patchSize = flippy_enc28j60_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W5500")) {
					patch     = flippy_w5500_eth_bin;
					patchSize = flippy_w5500_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W6100")) {
					patch     = flippy_w6100_eth_bin;
					patchSize = flippy_w6100_eth_bin_size;
				} else if (!strcmp(bba_device_str, "WIZnet W6300")) {
					patch     = flippy_w6300_eth_bin;
					patchSize = flippy_w6300_eth_bin_size;
				} else
					return 0;
				break;
			default:
				return 0;
		}
		print_debug("Installing Patch for FlippyDrive\n");
	}
	if (!final) {
		print_debug("Space for patch remaining: %i\n", top_addr - LO_RESERVE);
		print_debug("Space taken by vars/video patches: %i\n", HI_RESERVE - top_addr);
		if (top_addr < LO_RESERVE + patchSize ||
			top_addr < 0x80001800)
			return 0;
		patchLocation = Calc_Address(NULL, PATCH_OTHER, LO_RESERVE);
		memcpy(patchLocation, patch, patchSize);
		DCFlushRange(patchLocation, patchSize);
		ICInvalidateRange(patchLocation, patchSize);
	} else {
		for (u32 i = 0x100; i <= 0x900; i += 0x100) {
			if (i == 0x300) {
				((u32 *)&VAR_AREA[i])[0] = 0x48000002 | ((u32)DSI_EXCEPTION_VECTOR & 0x03FFFFFC);
				((u32 *)&VAR_AREA[i])[1] = 0x4C000064;
			} else
				*(u32 *)&VAR_AREA[i] = 0x4C000064;
		}
		memcpy((void *)0x80000000, VAR_AREA, sizeof(VAR_AREA));
		DCFlushRangeNoSync((void *)0x80000000, sizeof(VAR_AREA));
		ICInvalidateRange((void *)0x80000000, sizeof(VAR_AREA));
		_sync();
		if (top_addr != 0x81800000)
			mtspr(DABR, 0x800000E8 | 0b110);
		mtspr(EAR, 0x8000000C);
	}
	return 1;
}

void make_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
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
	functionPattern->properAddress = Calc_ProperAddress(data, dataType, offsetFoundAt * sizeof(u32));
}

bool compare_pattern(FuncPattern *FPatA, FuncPattern *FPatB)
{
	return memcmp(FPatA, FPatB, sizeof(u32) * 6) == 0;
}

bool find_pattern(u32 *data, int dataType, u32 offsetFoundAt, u32 length, FuncPattern *functionPattern)
{
	FuncPattern FP;
	
	make_pattern(data, dataType, offsetFoundAt, length, &FP);
	
	if (functionPattern && compare_pattern(&FP, functionPattern)) {
		functionPattern->offsetFoundAt = FP.offsetFoundAt;
		functionPattern->properAddress = FP.properAddress;
		return true;
	}
	
	if (!functionPattern) {
		print_debug("Length: %d\n", FP.Length);
		print_debug("Loads: %d\n", FP.Loads);
		print_debug("Stores: %d\n", FP.Stores);
		print_debug("FCalls: %d\n", FP.FCalls);
		print_debug("Branch: %d\n", FP.Branch);
		print_debug("Moves: %d\n", FP.Moves);
	}
	
	return false;
}

bool find_pattern_before(u32 *data, int dataType, u32 length, FuncPattern *FPatA, FuncPattern *FPatB)
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
	
	return find_pattern(data, dataType, offsetFoundAt, length, FPatB);
}

bool find_pattern_after(u32 *data, int dataType, u32 length, FuncPattern *FPatA, FuncPattern *FPatB)
{
	u32 offsetFoundAt = FPatA->offsetFoundAt + FPatA->Length;
	
	if (offsetFoundAt == FPatB->offsetFoundAt)
		return true;
	
	if (offsetFoundAt >= length / sizeof(u32))
		return false;
	
	return find_pattern(data, dataType, offsetFoundAt, length, FPatB);
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
	
	return address && find_pattern(data, dataType, address - data, length, functionPattern);
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
	
	return address && find_pattern(data, dataType, address - data, length, functionPattern);
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
	
	return offsetFoundAt && find_pattern(data, dataType, offsetFoundAt, length, functionPattern);
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

u32 _memcpy[] = {
	0x7C041840,	// cmplw	r4, r3
	0x41800028,	// blt		+10
	0x3884FFFF,	// subi		r4, r4, 1
	0x38C3FFFF,	// subi		r6, r3, 1
	0x38A50001,	// addi		r5, r5, 1
	0x4800000C,	// b		+3
	0x8C040001,	// lbzu		r0, 1 (r4)
	0x9C060001,	// stbu		r0, 1 (r6)
	0x34A5FFFF,	// subic.	r5, r5, 1
	0x4082FFF4,	// bne		-3
	0x4E800020,	// blr
	0x7C842A14,	// add		r4, r4, r5
	0x7CC32A14,	// add		r6, r3, r5
	0x38A50001,	// addi		r5, r5, 1
	0x4800000C,	// b		+3
	0x8C04FFFF,	// lbzu		r0, -1 (r4)
	0x9C06FFFF,	// stbu		r0, -1 (r6)
	0x34A5FFFF,	// subic.	r5, r5, 1
	0x4082FFF4,	// bne		-3
	0x4E800020	// blr
};

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

u32 _gxpeekargb_a[] = {
	0x546013BA,	// clrlslwi	r0, r3, 16, 2
	0x6400C800,	// oris		r0, r0, 0xC800
	0x54030512,	// rlwinm	r3, r0, 0, 20, 9
	0x54806126,	// clrlslwi	r0, r4, 16, 12
	0x7C600378,	// or		r0, r3, r0
	0x5403028E,	// rlwinm	r0, r0, 0, 10, 7
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

u32 _gxpeekargb_b[] = {
	0x5463103A,	// slwi		r3, r3, 2
	0x54846026,	// slwi		r4, r4, 12
	0x6463C800,	// oris		r3, r3, 0xC800
	0x54600512,	// rlwinm	r0, r3, 0, 20, 9
	0x7C032378,	// or		r3, r0, r4
	0x5463028E,	// rlwinm	r3, r3, 0, 10, 7
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

u32 _gxpeekargb_c[] = {
	0x5460043E,	// clrlwi	r0, r3, 16
	0x3C60C800,	// lis		r3, 0xC800
	0x5003153A,	// insrwi	r3, r0, 10, 20
	0x38000000,	// li		r0, 0
	0x508362A6,	// insrwi	r3, r4, 10, 10
	0x5003B212,	// insrwi	r3, r0, 2, 8
	0x80030000,	// lwz		r0, 0 (r3)
	0x90050000,	// stw		r0, 0 (r5)
	0x4E800020	// blr
};

u32 _gxpokeargb_a[] = {
	0x546013BA,	// clrlslwi	r0, r3, 16, 2
	0x6400C800,	// oris		r0, r0, 0xC800
	0x54030512,	// rlwinm	r3, r0, 0, 20, 9
	0x54806126,	// clrlslwi	r0, r4, 16, 12
	0x7C600378,	// or		r0, r3, r0
	0x5403028E,	// rlwinm	r0, r0, 0, 10, 7
	0x90A30000,	// stw		r5, 0 (r3)
	0x4E800020	// blr
};

u32 _gxpokeargb_b[] = {
	0x5463103A,	// slwi		r3, r3, 2
	0x54846026,	// slwi		r4, r4, 12
	0x6463C800,	// oris		r3, r3, 0xC800
	0x54600512,	// rlwinm	r0, r3, 0, 20, 9
	0x7C032378,	// or		r3, r0, r4
	0x5463028E,	// rlwinm	r3, r3, 0, 10, 7
	0x90A30000,	// stw		r5, 0 (r3)
	0x4E800020	// blr
};

u32 _gxpokeargb_c[] = {
	0x5460043E,	// clrlwi	r0, r3, 16
	0x3C60C800,	// lis		r3, 0xC800
	0x5003153A,	// insrwi	r3, r0, 10, 20
	0x38000000,	// li		r0, 0
	0x508362A6,	// insrwi	r3, r4, 10, 10
	0x5003B212,	// insrwi	r3, r0, 2, 8
	0x90A30000,	// stw		r5, 0 (r3)
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

u32 _gxpokez_a[] = {
	0x546013BA,	// clrlslwi	r0, r3, 16, 2
	0x6400C800,	// oris		r0, r0, 0xC800
	0x54030512,	// rlwinm	r3, r0, 0, 20, 9
	0x54806126,	// clrlslwi	r0, r4, 16, 12
	0x7C600378,	// or		r0, r3, r0
	0x5400028E,	// rlwinm	r0, r0, 0, 10, 7
	0x64030040,	// oris		r3, r0, 0x0040
	0x90A30000,	// stw		r5, 0 (r3)
	0x4E800020	// blr
};

u32 _gxpokez_b[] = {
	0x5463103A,	// slwi		r3, r3, 2
	0x54846026,	// slwi		r4, r4, 12
	0x6463C800,	// oris		r3, r3, 0xC800
	0x54600512,	// rlwinm	r0, r3, 0, 20, 9
	0x7C032378,	// or		r3, r0, r4
	0x5460028E,	// rlwinm	r0, r3, 0, 10, 7
	0x64030040,	// oris		r3, r0, 0x0040
	0x90A30000,	// stw		r5, 0 (r3)
	0x4E800020	// blr
};

u32 _gxpokez_c[] = {
	0x5460043E,	// clrlwi	r0, r3, 16
	0x3C60C800,	// lis		r3, 0xC800
	0x5003153A,	// insrwi	r3, r0, 10, 20
	0x38000001,	// li		r0, 1
	0x508362A6,	// insrwi	r3, r4, 10, 10
	0x5003B212,	// insrwi	r3, r0, 2, 8
	0x90A30000,	// stw		r5, 0 (r3)
	0x4E800020	// blr
};

int Patch_Hypervisor(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	int patched = 0;
	u32 address;
	FuncPattern memcpySig = 
		{ 11, 3, 0, 0, 2, 1, memcpyPatch, memcpyPatchLength, "memcpy" };
	FuncPattern PPCHaltSig = 
		{ 5, 1, 0, 0, 1, 1, NULL, 0, "PPCHalt" };
	FuncPattern ClearArenaSigs[5] = {
		{ 79, 19, 2, 22, 7, 9, NULL, 0, "ClearArenaD" },
		{ 92, 30, 7, 22, 7, 9, NULL, 0, "ClearArenaD" },
		{ 89, 25, 2, 22, 8, 9, NULL, 0, "ClearArenaD" },
		{ 67, 13, 4, 21, 6, 9, NULL, 0, "ClearArena" },
		{ 74, 17, 7, 21, 6, 9, NULL, 0, "ClearArena" }
	};
	FuncPattern OSInitSigs[30] = {
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
		{ 302,  99, 20, 70, 35, 17, NULL, 0, "OSInit" },	// SN Systems ProDG
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
	FuncPattern InsertAlarmSigs[4] = {
		{ 123, 38, 17,  6, 11, 24, NULL, 0, "InsertAlarmD" },
		{ 148, 30, 16, 10, 17, 45, NULL, 0, "InsertAlarm" },
		{ 154, 31, 16, 12, 17, 51, NULL, 0, "InsertAlarm" },	// SN Systems ProDG
		{ 154, 32, 16, 12, 17, 51, NULL, 0, "InsertAlarm" }		// SN Systems ProDG
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
	FuncPattern DecrementerExceptionCallbackSigs[5] = {
		{  93, 30,  6, 14,  4, 10, NULL, 0, "DecrementerExceptionCallbackD" },
		{ 101, 33,  6, 18,  4, 10, NULL, 0, "DecrementerExceptionCallbackD" },
		{ 132, 36, 10, 16, 12, 30, NULL, 0, "DecrementerExceptionCallback" },
		{ 140, 39, 10, 20, 12, 30, NULL, 0, "DecrementerExceptionCallback" },
		{ 140, 35, 10, 20, 12, 32, NULL, 0, "DecrementerExceptionCallback" }	// SN Systems ProDG
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
	FuncPattern ExternalInterruptHandlerSigs[3] = {
		{  5, 0,  3, 0, 1, 0, NULL, 0, "ExternalInterruptHandler" },
		{ 19, 0, 10, 0, 1, 7, NULL, 0, "ExternalInterruptHandler" },
		{ 20, 0, 11, 0, 1, 7, NULL, 0, "ExternalInterruptHandler" }	// SN Systems ProDG
	};
	FuncPattern __OSDoHotResetSigs[3] = {
		{ 17, 6, 3, 3, 0, 2, NULL, 0, "__OSDoHotResetD" },
		{ 18, 6, 3, 3, 0, 3, NULL, 0, "__OSDoHotReset" },
		{ 17, 5, 3, 3, 0, 3, NULL, 0, "__OSDoHotReset" }	// SN Systems ProDG
	};
	FuncPattern OSResetSystemSigs[19] = {
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
		{ 152, 44, 2, 26, 20, 9, NULL, 0, "OSResetSystem" },	// SN Systems ProDG
		{ 174, 44, 3, 25, 37, 9, NULL, 0, "OSResetSystem" },
		{ 175, 44, 3, 25, 37, 9, NULL, 0, "OSResetSystem" },
		{ 128, 38, 6, 31, 16, 6, NULL, 0, "OSResetSystem" }
	};
	FuncPattern OSGetResetCodeSigs[6] = {
		{ 17,  7, 2, 0, 2, 0, NULL, 0, "OSGetResetCodeD" },
		{ 21, 11, 2, 0, 2, 0, NULL, 0, "OSGetResetCodeD" },
		{ 12,  5, 0, 0, 2, 0, NULL, 0, "OSGetResetCode" },
		{ 13,  5, 0, 0, 2, 0, NULL, 0, "OSGetResetCode" },
		{ 10,  4, 0, 0, 1, 0, NULL, 0, "OSGetResetCode" },	// SN Systems ProDG
		{ 14,  7, 0, 0, 2, 0, NULL, 0, "OSGetResetCode" }
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
	FuncPattern EXIImmSigs[8] = {
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 122, 38, 7, 9, 12,  9, NULL, 0, "EXIImmD" },
		{ 151, 27, 8, 5, 12, 17, NULL, 0, "EXIImm" },
		{ 151, 27, 8, 5, 12, 17, NULL, 0, "EXIImm" },
		{  87, 24, 7, 5,  7,  9, NULL, 0, "EXIImm" },
		{ 151, 36, 8, 5, 12, 32, NULL, 0, "EXIImm" },
		{ 150, 34, 8, 7,  7, 39, NULL, 0, "EXIImm" }	// SN Systems ProDG
	};
	FuncPattern EXIDmaSigs[8] = {
		{ 113, 42, 5, 10, 8,  6, NULL, 0, "EXIDmaD" },
		{ 113, 42, 5, 10, 8,  6, NULL, 0, "EXIDmaD" },
		{ 113, 42, 5, 10, 8,  6, NULL, 0, "EXIDmaD" },
		{  59, 17, 7,  5, 2,  4, NULL, 0, "EXIDma" },
		{  59, 17, 7,  5, 2,  4, NULL, 0, "EXIDma" },
		{  74, 28, 8,  5, 2,  8, NULL, 0, "EXIDma" },
		{  59, 17, 7,  5, 2,  5, NULL, 0, "EXIDma" },
		{  61, 16, 7,  7, 2, 10, NULL, 0, "EXIDma" }	// SN Systems ProDG
	};
	FuncPattern EXISyncSigs[13] = {
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
		{ 147, 40, 3, 4, 13, 19, NULL, 0, "EXISync" },
		{ 152, 41, 3, 6, 13, 23, NULL, 0, "EXISync" }	// SN Systems ProDG
	};
	FuncPattern EXIClearInterruptsSigs[5] = {
		{ 48, 13, 5, 1, 5, 3, NULL, 0, "EXIClearInterruptsD" },
		{ 49, 14, 5, 1, 5, 3, NULL, 0, "EXIClearInterruptsD" },
		{ 18,  3, 1, 0, 3, 2, NULL, 0, "EXIClearInterrupts" },
		{ 27,  5, 1, 0, 3, 0, NULL, 0, "EXIClearInterrupts" },
		{ 21,  4, 0, 0, 3, 3, NULL, 0, "EXIClearInterrupts" }	// SN Systems ProDG
	};
	FuncPattern EXIProbeResetSigs[3] = {
		{ 16,  7, 4, 2, 0, 2, NULL, 0, "EXIProbeResetD" },
		{ 23, 12, 6, 2, 0, 2, NULL, 0, "EXIProbeResetD" },
		{ 19,  8, 6, 2, 0, 2, NULL, 0, "EXIProbeReset" }	// SN Systems ProDG
	};
	FuncPattern __EXIProbeSigs[8] = {
		{ 108, 38, 2, 7, 10, 10, NULL, 0, "EXIProbeD" },
		{ 111, 38, 5, 7, 10, 10, NULL, 0, "__EXIProbeD" },
		{ 112, 39, 5, 7, 10, 10, NULL, 0, "__EXIProbeD" },
		{  90, 30, 4, 5,  8, 10, NULL, 0, "EXIProbe" },
		{  93, 30, 7, 5,  8, 10, NULL, 0, "__EXIProbe" },
		{ 109, 34, 6, 5,  8,  8, NULL, 0, "__EXIProbe" },
		{  93, 30, 7, 5,  8,  9, NULL, 0, "__EXIProbe" },
		{  97, 31, 9, 5,  8, 15, NULL, 0, "__EXIProbe" }	// SN Systems ProDG
	};
	FuncPattern EXIAttachSigs[8] = {
		{  59, 19,  5,  8,  5,  5, NULL, 0, "EXIAttachD" },
		{  43, 11,  3,  6,  3,  5, NULL, 0, "EXIAttachD" },
		{  44, 12,  3,  6,  3,  5, NULL, 0, "EXIAttachD" },
		{  57, 18,  9,  6,  3,  4, NULL, 0, "EXIAttach" },
		{  67, 19,  4, 11,  3,  3, NULL, 0, "EXIAttach" },
		{  74, 19,  5, 11,  3,  7, NULL, 0, "EXIAttach" },
		{  67, 18,  4, 11,  3,  5, NULL, 0, "EXIAttach" },
		{ 214, 54, 10, 20, 19, 29, NULL, 0, "EXIAttach" }	// SN Systems ProDG
	};
	FuncPattern EXIDetachSigs[8] = {
		{ 53, 16, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 53, 16, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 54, 17, 3, 6, 5, 5, NULL, 0, "EXIDetachD" },
		{ 47, 15, 6, 5, 3, 3, NULL, 0, "EXIDetach" },
		{ 47, 15, 6, 5, 3, 3, NULL, 0, "EXIDetach" },
		{ 43, 12, 3, 5, 3, 4, NULL, 0, "EXIDetach" },
		{ 47, 15, 6, 5, 3, 4, NULL, 0, "EXIDetach" },
		{ 45, 15, 6, 5, 3, 5, NULL, 0, "EXIDetach" }	// SN Systems ProDG
	};
	FuncPattern EXISelectSDSig = 
		{ 85, 23, 4, 7, 14, 6, NULL, 0, "EXISelectSD" };
	FuncPattern EXISelectSigs[8] = {
		{ 116, 33, 3, 10, 17,  6, NULL, 0, "EXISelectD" },
		{ 116, 33, 3, 10, 17,  6, NULL, 0, "EXISelectD" },
		{ 116, 33, 3, 10, 17,  6, NULL, 0, "EXISelectD" },
		{  75, 18, 4,  6, 11,  7, NULL, 0, "EXISelect" },
		{  75, 18, 4,  6, 11,  7, NULL, 0, "EXISelect" },
		{  80, 20, 4,  6, 11,  6, NULL, 0, "EXISelect" },
		{  75, 18, 4,  6, 11,  8, NULL, 0, "EXISelect" },
		{ 148, 37, 6, 12, 19, 22, NULL, 0, "EXISelect" }	// SN Systems ProDG
	};
	FuncPattern EXIDeselectSigs[9] = {
		{  76, 21,  3,  7, 14,  5, NULL, 0, "EXIDeselectD" },
		{  76, 21,  3,  7, 14,  5, NULL, 0, "EXIDeselectD" },
		{  77, 22,  3,  7, 14,  5, NULL, 0, "EXIDeselectD" },
		{  68, 20,  8,  6, 11,  3, NULL, 0, "EXIDeselect" },
		{  68, 20,  8,  6, 12,  3, NULL, 0, "EXIDeselect" },
		{  68, 20,  8,  6, 12,  3, NULL, 0, "EXIDeselect" },
		{  66, 17,  3,  6, 12,  4, NULL, 0, "EXIDeselect" },
		{  68, 20,  8,  6, 12,  4, NULL, 0, "EXIDeselect" },
		{ 136, 37, 10, 10, 18, 17, NULL, 0, "EXIDeselect" }	// SN Systems ProDG
	};
	FuncPattern EXIIntrruptHandlerSigs[8] = {
		{ 42, 16, 3, 2, 3, 2, NULL, 0, "EXIIntrruptHandlerD" },
		{ 50, 20, 2, 6, 3, 3, NULL, 0, "EXIIntrruptHandlerD" },
		{ 51, 21, 2, 6, 3, 3, NULL, 0, "EXIIntrruptHandlerD" },
		{ 32, 10, 3, 0, 1, 7, NULL, 0, "EXIIntrruptHandler" },
		{ 50, 19, 6, 4, 1, 8, NULL, 0, "EXIIntrruptHandler" },
		{ 48, 16, 3, 4, 1, 3, NULL, 0, "EXIIntrruptHandler" },
		{ 50, 19, 6, 4, 1, 7, NULL, 0, "EXIIntrruptHandler" },
		{ 47, 15, 5, 4, 1, 7, NULL, 0, "EXIIntrruptHandler" }	// SN Systems ProDG
	};
	FuncPattern TCIntrruptHandlerSigs[8] = {
		{  50, 18, 4, 4, 3,  4, NULL, 0, "TCIntrruptHandlerD" },
		{  58, 22, 3, 8, 3,  3, NULL, 0, "TCIntrruptHandlerD" },
		{  59, 23, 3, 8, 3,  3, NULL, 0, "TCIntrruptHandlerD" },
		{ 125, 34, 9, 1, 8, 21, NULL, 0, "TCIntrruptHandler" },
		{ 134, 37, 9, 5, 8, 21, NULL, 0, "TCIntrruptHandler" },
		{  87, 28, 5, 5, 6,  4, NULL, 0, "TCIntrruptHandler" },
		{ 134, 41, 9, 5, 8, 22, NULL, 0, "TCIntrruptHandler" },
		{ 131, 37, 8, 5, 8, 23, NULL, 0, "TCIntrruptHandler" }	// SN Systems ProDG
	};
	FuncPattern EXTIntrruptHandlerSigs[9] = {
		{ 52, 18, 5, 2, 3, 4, NULL, 0, "EXTIntrruptHandlerD" },
		{ 60, 22, 4, 6, 3, 5, NULL, 0, "EXTIntrruptHandlerD" },
		{ 61, 23, 4, 6, 3, 5, NULL, 0, "EXTIntrruptHandlerD" },
		{ 55, 20, 4, 6, 3, 4, NULL, 0, "EXTIntrruptHandlerD" },
		{ 43, 17, 6, 1, 1, 7, NULL, 0, "EXTIntrruptHandler" },
		{ 50, 17, 4, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" },
		{ 50, 18, 4, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" },
		{ 52, 20, 8, 5, 1, 5, NULL, 0, "EXTIntrruptHandler" },
		{ 52, 18, 8, 5, 1, 6, NULL, 0, "EXTIntrruptHandler" }	// SN Systems ProDG
	};
	FuncPattern EXIInitSigs[11] = {
		{  58, 36,  6, 11, 1, 2, NULL, 0, "EXIInitD" },
		{  60, 37,  6, 12, 1, 2, NULL, 0, "EXIInitD" },
		{ 107, 61,  6, 16, 8, 2, NULL, 0, "EXIInitD" },
		{  65, 32, 12, 12, 1, 2, NULL, 0, "EXIInit" },
		{  69, 34, 14, 12, 1, 2, NULL, 0, "EXIInit" },
		{  73, 46, 10, 12, 1, 2, NULL, 0, "EXIInit" },
		{  71, 35, 14, 13, 1, 2, NULL, 0, "EXIInit" },
		{  89, 43, 14, 14, 4, 2, NULL, 0, "EXIInit" },
		{  87, 42, 14, 13, 4, 2, NULL, 0, "EXIInit" },
		{ 117, 58, 14, 17, 8, 2, NULL, 0, "EXIInit" },
		{ 103, 57,  6, 16, 8, 2, NULL, 0, "EXIInit" }	// SN Systems ProDG
	};
	FuncPattern EXILockSigs[8] = {
		{ 106, 35, 5,  9, 13,  6, NULL, 0, "EXILockD" },
		{ 106, 35, 5,  9, 13,  6, NULL, 0, "EXILockD" },
		{ 106, 35, 5,  9, 13,  6, NULL, 0, "EXILockD" },
		{  61, 18, 7,  5,  5,  6, NULL, 0, "EXILock" },
		{  61, 18, 7,  5,  5,  6, NULL, 0, "EXILock" },
		{  63, 19, 5,  5,  6,  6, NULL, 0, "EXILock" },
		{  61, 17, 7,  5,  5,  7, NULL, 0, "EXILock" },
		{ 110, 30, 7, 13, 22, 10, NULL, 0, "EXILock" }	// SN Systems ProDG
	};
	FuncPattern EXIUnlockSigs[8] = {
		{  60, 22, 4,  6,  5, 4, NULL, 0, "EXIUnlockD" },
		{  60, 22, 4,  6,  5, 4, NULL, 0, "EXIUnlockD" },
		{  61, 23, 4,  6,  5, 4, NULL, 0, "EXIUnlockD" },
		{  55, 21, 8,  5,  3, 2, NULL, 0, "EXIUnlock" },
		{  55, 21, 8,  5,  3, 2, NULL, 0, "EXIUnlock" },
		{  50, 18, 4,  5,  3, 3, NULL, 0, "EXIUnlock" },
		{  55, 21, 8,  5,  3, 3, NULL, 0, "EXIUnlock" },
		{ 101, 33, 8, 11, 20, 4, NULL, 0, "EXIUnlock" }	// SN Systems ProDG
	};
	FuncPattern EXIGetIDSigs[10] = {
		{   97,  27,  4, 11,   7,   9, NULL, 0, "EXIGetIDD" },
		{  152,  45,  6, 14,  14,  15, NULL, 0, "EXIGetIDD" },
		{  153,  46,  6, 14,  14,  15, NULL, 0, "EXIGetIDD" },
		{  168,  49,  7, 16,  16,  16, NULL, 0, "EXIGetIDD" },
		{  181,  54,  8, 24,  14,  13, NULL, 0, "EXIGetID" },
		{  223,  70, 11, 26,  20,  17, NULL, 0, "EXIGetID" },
		{  228,  66, 11, 26,  19,  19, NULL, 0, "EXIGetID" },
		{  223,  69, 11, 26,  20,  20, NULL, 0, "EXIGetID" },
		{  236,  71, 12, 28,  22,  21, NULL, 0, "EXIGetID" },
		{ 1087, 280, 44, 83, 140, 139, NULL, 0, "EXIGetID" }	// SN Systems ProDG
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
	FuncPattern DVDLowClearCallbackSigs[6] = {
		{ 12, 6, 4, 0, 0, 0, NULL, 0, "DVDLowClearCallbackD" },
		{ 14, 7, 5, 0, 0, 0, NULL, 0, "DVDLowClearCallbackD" },
		{  6, 3, 2, 0, 0, 0, NULL, 0, "DVDLowClearCallback" },
		{  6, 3, 2, 0, 0, 0, NULL, 0, "DVDLowClearCallback" },	// SN Systems ProDG
		{  7, 3, 3, 0, 0, 0, NULL, 0, "DVDLowClearCallback" },
		{  7, 3, 3, 0, 0, 0, NULL, 0, "DVDLowClearCallback" }	// SN Systems ProDG
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
	FuncPattern VISetRegsSigs[3] = {
		{ 54, 21, 6, 3, 3, 12, NULL, 0, "VISetRegsD" },
		{ 58, 21, 7, 3, 3, 12, NULL, 0, "VISetRegsD" },
		{ 60, 22, 8, 3, 3, 12, NULL, 0, "VISetRegsD" }
	};
	FuncPattern __VIRetraceHandlerSigs[10] = {
		{ 120, 41,  7, 10, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 121, 41,  7, 11, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 138, 48,  7, 15, 14,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 132, 39,  7, 10, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 137, 42,  8, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 138, 42,  9, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 140, 43, 10, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 147, 46, 13, 10, 15, 15, NULL, 0, "__VIRetraceHandler" },	// SN Systems ProDG
		{ 157, 50, 10, 15, 16, 13, NULL, 0, "__VIRetraceHandler" },
		{ 164, 53, 13, 14, 16, 15, NULL, 0, "__VIRetraceHandler" }	// SN Systems ProDG
	};
	FuncPattern AIInitDMASigs[3] = {
		{ 44, 12, 2, 3, 1, 6, NULL, 0, "AIInitDMAD" },
		{ 34,  8, 4, 2, 0, 5, NULL, 0, "AIInitDMA" },
		{ 31,  5, 4, 2, 0, 7, NULL, 0, "AIInitDMA" }	// SN Systems ProDG
	};
	FuncPattern AIGetDMAStartAddrSigs[2] = {
		{ 7, 2, 0, 0, 0, 0, NULL, 0, "AIGetDMAStartAddrD" },
		{ 7, 2, 0, 0, 0, 0, NULL, 0, "AIGetDMAStartAddr" }
	};
	FuncPattern GXPeekARGBSigs[3] = {
		{ 9, 1, 1, 0, 0, 1, GXPeekARGBPatch, GXPeekARGBPatchLength, "GXPeekARGB" },
		{ 9, 1, 1, 0, 0, 1, GXPeekARGBPatch, GXPeekARGBPatchLength, "GXPeekARGB" },	// SN Systems ProDG
		{ 9, 3, 1, 0, 0, 0, GXPeekARGBPatch, GXPeekARGBPatchLength, "GXPeekARGB" }
	};
	FuncPattern GXPokeARGBSigs[3] = {
		{ 8, 0, 1, 0, 0, 1, GXPokeARGBPatch, GXPokeARGBPatchLength, "GXPokeARGB" },
		{ 8, 0, 1, 0, 0, 1, GXPokeARGBPatch, GXPokeARGBPatchLength, "GXPokeARGB" },	// SN Systems ProDG
		{ 8, 2, 1, 0, 0, 0, GXPokeARGBPatch, GXPokeARGBPatchLength, "GXPokeARGB" }
	};
	FuncPattern GXPeekZSigs[3] = {
		{ 10, 1, 1, 0, 0, 1, GXPeekZPatch, GXPeekZPatchLength, "GXPeekZ" },
		{ 10, 1, 1, 0, 0, 1, GXPeekZPatch, GXPeekZPatchLength, "GXPeekZ" },	// SN Systems ProDG
		{  9, 3, 1, 0, 0, 0, GXPeekZPatch, GXPeekZPatchLength, "GXPeekZ" }
	};
	FuncPattern GXPokeZSigs[3] = {
		{ 9, 0, 1, 0, 0, 1, GXPokeZPatch, GXPokeZPatchLength, "GXPokeZ" },
		{ 9, 0, 1, 0, 0, 1, GXPokeZPatch, GXPokeZPatchLength, "GXPokeZ" },	// SN Systems ProDG
		{ 8, 2, 1, 0, 0, 0, GXPokeZPatch, GXPokeZPatchLength, "GXPokeZ" }
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
		if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
			(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
			(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
			(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
			get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
			get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
			break;
		}
	}
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if ((data[i - 1] != 0x4E800020 &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x4E800020) &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x00000000 || data[i - 3] != 0x4E800020) &&
			 data[i - 1] != 0x4C000064 &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
			continue;
		
		if (data[i + 0] != 0x7C0802A6 && (data[i + 0] & 0xFFFF0000) != 0x94210000) {
			if (!memcmp(data + i, _memcpy, sizeof(_memcpy)))
				memcpySig.offsetFoundAt = i;
			else if (!memcmp(data + i, _ppchalt, sizeof(_ppchalt)))
				PPCHaltSig.offsetFoundAt = i;
			else if (!memcmp(data + i, _dvdgettransferredsize, sizeof(_dvdgettransferredsize)))
				DVDGetTransferredSizeSig.offsetFoundAt = i;
			else if (!memcmp(data + i, _aigetdmastartaddrd, sizeof(_aigetdmastartaddrd)))
				AIGetDMAStartAddrSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _aigetdmastartaddr, sizeof(_aigetdmastartaddr)))
				AIGetDMAStartAddrSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekargb_a, sizeof(_gxpeekargb_a)))
				GXPeekARGBSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekargb_b, sizeof(_gxpeekargb_b)))
				GXPeekARGBSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekargb_c, sizeof(_gxpeekargb_c)))
				GXPeekARGBSigs[2].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokeargb_a, sizeof(_gxpokeargb_a)))
				GXPokeARGBSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokeargb_b, sizeof(_gxpokeargb_b)))
				GXPokeARGBSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokeargb_c, sizeof(_gxpokeargb_c)))
				GXPokeARGBSigs[2].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_a, sizeof(_gxpeekz_a)))
				GXPeekZSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_b, sizeof(_gxpeekz_b)))
				GXPeekZSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpeekz_c, sizeof(_gxpeekz_c)))
				GXPeekZSigs[2].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokez_a, sizeof(_gxpokez_a)))
				GXPokeZSigs[0].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokez_b, sizeof(_gxpokez_b)))
				GXPokeZSigs[1].offsetFoundAt = i;
			else if (!memcmp(data + i, _gxpokez_c, sizeof(_gxpokez_c)))
				GXPokeZSigs[2].offsetFoundAt = i;
			continue;
		}
		
		FuncPattern fp;
		make_pattern(data, dataType, i, length, &fp);
		
		for (j = 0; j < sizeof(ClearArenaSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &ClearArenaSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &OSGetResetCodeSigs[0]) &&
							findx_pattern(data, dataType, i +  8, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 10, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 12, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 23, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 25, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 27, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 45, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 48, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 51, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 53, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 55, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 60, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 62, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 66, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 69, length, &OSGetArenaHiSigs[0]))
							ClearArenaSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  4, length, &OSGetResetCodeSigs[0]) &&
							findx_pattern(data, dataType, i + 12, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 14, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 16, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 30, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 32, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 34, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 53, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 57, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 61, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 63, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 65, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 70, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 73, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 77, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 81, length, &OSGetArenaHiSigs[0]))
							ClearArenaSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  6, length, &OSGetResetCodeSigs[1]) &&
							findx_pattern(data, dataType, i + 15, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 17, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 19, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 27, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 29, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 31, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 50, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 54, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 58, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 60, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 62, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 67, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 70, length, &OSGetArenaLoSigs[0]) &&
							findx_pattern(data, dataType, i + 74, length, &OSGetArenaHiSigs[0]) &&
							findx_pattern(data, dataType, i + 78, length, &OSGetArenaHiSigs[0]))
							ClearArenaSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  5, length, &OSGetResetCodeSigs[2]) &&
							findx_pattern(data, dataType, i +  9, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 11, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 13, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 23, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 25, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 27, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 32, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 35, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 40, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 42, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 47, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 49, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 53, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 56, length, &OSGetArenaHiSigs[1]))
							ClearArenaSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_patterns(data, dataType, i +  4, length, &OSGetResetCodeSigs[2],
							                                               &OSGetResetCodeSigs[3], NULL) &&
							findx_pattern (data, dataType, i + 11, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 13, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 15, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 27, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 29, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 31, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 36, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 40, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 44, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 46, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 48, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 53, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 56, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern (data, dataType, i + 60, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 64, length, &OSGetArenaHiSigs[1]))
							ClearArenaSigs[j].offsetFoundAt = i;
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
							findx_pattern(data, dataType, i + 227, length, &ClearArenaSigs[0]) &&
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
							findx_pattern(data, dataType, i + 230, length, &ClearArenaSigs[0]) &&
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
							findx_pattern(data, dataType, i + 234, length, &ClearArenaSigs[1]) &&
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
							findx_pattern(data, dataType, i + 234, length, &ClearArenaSigs[1]) &&
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
							findx_pattern(data, dataType, i + 204, length, &ClearArenaSigs[1]) &&
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
							findx_pattern(data, dataType, i + 229, length, &ClearArenaSigs[1]) &&
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
							findx_pattern(data, dataType, i + 231, length, &ClearArenaSigs[2]) &&
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
							findx_pattern(data, dataType, i + 184, length, &ClearArenaSigs[3]) &&
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
							findx_pattern(data, dataType, i + 200, length, &ClearArenaSigs[3]) &&
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
							findx_pattern(data, dataType, i + 201, length, &ClearArenaSigs[3]) &&
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
							findx_pattern(data, dataType, i + 204, length, &ClearArenaSigs[3]) &&
							findx_pattern(data, dataType, i + 205, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 23:
						if (findx_pattern (data, dataType, i +  15, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern (data, dataType, i +  18, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  59, length, &OSSetArenaLoSig) &&
							findx_pattern (data, dataType, i +  74, length, &OSSetArenaLoSig) &&
							findx_pattern (data, dataType, i +  82, length, &OSSetArenaHiSig) &&
							findx_pattern (data, dataType, i +  83, length, &OSExceptionInitSigs[2]) &&
							findx_pattern (data, dataType, i +  84, length, &__OSInitSystemCallSigs[1]) &&
							findx_pattern (data, dataType, i +  87, length, &__OSInterruptInitSigs[1]) &&
							findx_pattern (data, dataType, i +  91, length, &__OSSetInterruptHandlerSigs[1]) &&
							findx_pattern (data, dataType, i + 194, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern (data, dataType, i + 196, length, &OSGetArenaLoSigs[1]) &&
							findx_patterns(data, dataType, i + 209, length, &ClearArenaSigs[3],
							                                                &ClearArenaSigs[4], NULL) &&
							findx_pattern (data, dataType, i + 210, length, &OSEnableInterruptsSig))
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
							findx_pattern(data, dataType, i + 209, length, &ClearArenaSigs[4]) &&
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
							findx_pattern(data, dataType, i + 193, length, &ClearArenaSigs[4]) &&
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
							findx_pattern(data, dataType, i + 182, length, &OSGetResetCodeSigs[4]) &&
							findx_pattern(data, dataType, i + 189, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 191, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 193, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 205, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 207, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 209, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 214, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 218, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 222, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 224, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 226, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 231, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 234, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 238, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 242, length, &OSGetArenaHiSigs[2]) &&
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
							findx_pattern(data, dataType, i + 217, length, &ClearArenaSigs[4]) &&
							findx_pattern(data, dataType, i + 218, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 28:
						if (findx_pattern(data, dataType, i +  12, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i +  15, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  64, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  79, length, &OSSetArenaLoSig) &&
							findx_pattern(data, dataType, i +  86, length, &OSSetArenaHiSig) &&
							findx_pattern(data, dataType, i +  87, length, &OSExceptionInitSigs[3]) &&
							findx_pattern(data, dataType, i +  88, length, &__OSInitSystemCallSigs[2]) &&
							findx_pattern(data, dataType, i +  91, length, &__OSInterruptInitSigs[2]) &&
							findx_pattern(data, dataType, i +  95, length, &__OSSetInterruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 190, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 192, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 207, length, &OSGetResetCodeSigs[4]) &&
							findx_pattern(data, dataType, i + 214, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 216, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 218, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 230, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 232, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 234, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 239, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 243, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 247, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 249, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 251, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 256, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 259, length, &OSGetArenaLoSigs[2]) &&
							findx_pattern(data, dataType, i + 263, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 267, length, &OSGetArenaHiSigs[2]) &&
							findx_pattern(data, dataType, i + 272, length, &OSEnableInterruptsSig))
							OSInitSigs[j].offsetFoundAt = i;
						break;
					case 29:
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
							findx_pattern(data, dataType, i + 219, length, &OSGetResetCodeSigs[5]) &&
							findx_pattern(data, dataType, i + 228, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 230, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 232, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 241, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 243, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 245, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 250, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 254, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 258, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 260, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 262, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 267, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 270, length, &OSGetArenaLoSigs[1]) &&
							findx_pattern(data, dataType, i + 274, length, &OSGetArenaHiSigs[1]) &&
							findx_pattern(data, dataType, i + 279, length, &OSGetArenaHiSigs[1]) &&
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
						if (findx_pattern (data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 14, length, &__OSGetSystemTimeSigs[2]) &&
							findx_patterns(data, dataType, i + 19, length, &InsertAlarmSigs[2],
							                                               &InsertAlarmSigs[3], NULL) &&
							findx_pattern (data, dataType, i + 21, length, &OSRestoreInterruptsSig))
							OSSetAlarmSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSCancelAlarmSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSCancelAlarmSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  5, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 11, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 32, length, &SetTimerSig) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig))
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
		
		for (j = 0; j < sizeof(DecrementerExceptionCallbackSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &DecrementerExceptionCallbackSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +   7, length, &OSGetTimeSig) &&
							findx_pattern(data, dataType, i +  14, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  25, length, &SetTimerSig) &&
							findx_pattern(data, dataType, i +  27, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  64, length, &InsertAlarmSigs[0]) &&
							findx_pattern(data, dataType, i +  77, length, &SetTimerSig) &&
							findx_pattern(data, dataType, i +  87, length, &OSLoadContextSigs[1]))
							DecrementerExceptionCallbackSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +   7, length, &__OSGetSystemTimeSigs[0]) &&
							findx_patterns(data, dataType, i +  14, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i +  25, length, &SetTimerSig) &&
							findx_patterns(data, dataType, i +  27, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i +  64, length, &InsertAlarmSigs[0]) &&
							findx_pattern (data, dataType, i +  77, length, &SetTimerSig) &&
							findx_pattern (data, dataType, i +  80, length, &OSClearContextSigs[0]) &&
							findx_pattern (data, dataType, i +  82, length, &OSSetCurrentContextSig) &&
							findx_pattern (data, dataType, i +  89, length, &OSClearContextSigs[0]) &&
							findx_pattern (data, dataType, i +  91, length, &OSSetCurrentContextSig) &&
							findx_patterns(data, dataType, i +  95, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL))
							DecrementerExceptionCallbackSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +   8, length, &OSGetTimeSig) &&
							findx_pattern(data, dataType, i +  16, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  26, length, &OSGetTimeSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSLoadContextSigs[1]) &&
							findx_pattern(data, dataType, i +  82, length, &InsertAlarmSigs[1]) &&
							findx_pattern(data, dataType, i +  86, length, &OSGetTimeSig) &&
							findx_pattern(data, dataType, i + 123, length, &OSLoadContextSigs[1]))
							DecrementerExceptionCallbackSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern (data, dataType, i +   8, length, &__OSGetSystemTimeSigs[1]) &&
							findx_patterns(data, dataType, i +  16, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i +  26, length, &__OSGetSystemTimeSigs[1]) &&
							findx_patterns(data, dataType, i +  55, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL) &&
							findx_pattern (data, dataType, i +  82, length, &InsertAlarmSigs[1]) &&
							findx_pattern (data, dataType, i +  86, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern (data, dataType, i + 116, length, &OSClearContextSigs[1]) &&
							findx_pattern (data, dataType, i + 118, length, &OSSetCurrentContextSig) &&
							findx_pattern (data, dataType, i + 125, length, &OSClearContextSigs[1]) &&
							findx_pattern (data, dataType, i + 127, length, &OSSetCurrentContextSig) &&
							findx_patterns(data, dataType, i + 131, length, &OSLoadContextSigs[1],
							                                                &OSLoadContextSigs[2], NULL))
							DecrementerExceptionCallbackSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern (data, dataType, i +   8, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern (data, dataType, i +  16, length, &OSLoadContextSigs[2]) &&
							findx_pattern (data, dataType, i +  26, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern (data, dataType, i +  55, length, &OSLoadContextSigs[2]) &&
							findx_patterns(data, dataType, i +  82, length, &InsertAlarmSigs[2],
							                                                &InsertAlarmSigs[3], NULL) &&
							findx_pattern (data, dataType, i +  86, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern (data, dataType, i + 116, length, &OSClearContextSigs[2]) &&
							findx_pattern (data, dataType, i + 118, length, &OSSetCurrentContextSig) &&
							findx_pattern (data, dataType, i + 125, length, &OSClearContextSigs[2]) &&
							findx_pattern (data, dataType, i + 127, length, &OSSetCurrentContextSig) &&
							findx_pattern (data, dataType, i + 131, length, &OSLoadContextSigs[2]))
							DecrementerExceptionCallbackSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__OSInterruptInitSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__OSInterruptInitSigs[j])) {
				switch (j) {
					case 0:
						if (get_immediate (data,   i + 20, i + 21, &address) && address == 0xCC003000 &&
							findx_pattern (data, dataType, i + 24, length, &__OSMaskInterruptsSigs[0]) &&
							findi_patterns(data, dataType, i + 26, i + 27, length, &ExternalInterruptHandlerSigs[0],
							                                                       &ExternalInterruptHandlerSigs[1],
							                                                       &ExternalInterruptHandlerSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 28, length, &__OSSetExceptionHandlerSigs[0]) &&
							get_immediate (data,   i + 30, i + 31, &address) && address == 0xCC003000 &&
							findx_pattern (data, dataType, i + 33, length, &__OSUnmaskInterruptsSigs[0]))
							__OSInterruptInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (get_immediate (data,   i + 13, i + 14, &address) && address == 0xCC003000 &&
							findx_pattern (data, dataType, i + 19, length, &__OSMaskInterruptsSigs[1]) &&
							findi_patterns(data, dataType, i + 20, i + 21, length, &ExternalInterruptHandlerSigs[0],
							                                                       &ExternalInterruptHandlerSigs[1],
							                                                       &ExternalInterruptHandlerSigs[2], NULL) &&
							findx_pattern (data, dataType, i + 23, length, &__OSSetExceptionHandlerSigs[1]))
							__OSInterruptInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (get_immediate(data,   i + 13, i + 16, &address) && address == 0xCC003004 &&
							findx_pattern(data, dataType, i + 17, length, &__OSMaskInterruptsSigs[2]) &&
							findi_pattern(data, dataType, i + 18, i + 20, length, &ExternalInterruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 21, length, &__OSSetExceptionHandlerSigs[2]))
							__OSInterruptInitSigs[j].offsetFoundAt = i;
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
							find_pattern_before(data, dataType, length, &fp, &__OSMaskInterruptsSigs[0]))
							__OSUnmaskInterruptsSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_patterns(data, dataType, i + 21, length, &SetInterruptMaskSigs[3],
							                                               &SetInterruptMaskSigs[4],
							                                               &SetInterruptMaskSigs[5], NULL) &&
							findx_pattern (data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &__OSMaskInterruptsSigs[1]))
							__OSUnmaskInterruptsSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 19, length, &SetInterruptMaskSigs[6]) &&
							findx_pattern(data, dataType, i + 23, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &__OSMaskInterruptsSigs[2]))
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
						if (findx_pattern(data, dataType, i +  50, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  70, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  74, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 16:
						if (findx_pattern(data, dataType, i +  64, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  86, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  91, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 17:
						if (findx_pattern(data, dataType, i +  65, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  87, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  92, length, &ICFlashInvalidateSig))
							OSResetSystemSigs[j].offsetFoundAt = i;
						break;
					case 18:
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
					case 7:
						if ((data[i + 10] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  23, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &EXIClearInterruptsSigs[4]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[2]) &&
							get_immediate(data,  i + 119, i + 120, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 136, i + 137, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i + 142, length, &OSRestoreInterruptsSig))
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
					case 7:
						if ((data[i + 10] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  14, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  23, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  34, length, &EXIClearInterruptsSigs[4]) &&
							findx_pattern(data, dataType, i +  38, length, &__OSUnmaskInterruptsSigs[2]) &&
							get_immediate(data,  i +  43, i +  44, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  53, length, &OSRestoreInterruptsSig))
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
					case 12:
						if ((data[i + 6] & 0xFC00FFFF) == 0x54003032 &&
							get_immediate(data,  i +  12, i +  13, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  19, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  34, i +  35, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 114, i + 115, &address) && address == 0xCC006800 &&
							get_immediate(data,  i + 134, i + 135, &address) && address == 0x800030E6 &&
							findx_pattern(data, dataType, i + 140, length, &OSRestoreInterruptsSig))
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
					case 7:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,  i +  20, i +  21, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  35, i +  36, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  40, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  41, i +  42, &address) && address == 0x800000F8 &&
							get_immediate(data,  i +  55, i +  56, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  70, i +  71, &address) && address == 0x800030C0 &&
							get_immediate(data,  i +  82, i +  83, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  87, length, &OSRestoreInterruptsSig))
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
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[0]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 35, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 44, length, &__OSMaskInterruptsSigs[0]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[1]))
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
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[2]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 7] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[3]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[4]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 15, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 34, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 36, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[5]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 27, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 36, length, &__OSMaskInterruptsSigs[1]) &&
							findx_pattern(data, dataType, i + 38, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[6]))
							EXIDetachSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if ((data[i + 7] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 11, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 16, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 34, length, &__OSMaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 36, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXIAttachSigs[7]))
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
				find_pattern_after(data, dataType, length, &fp, &EXISelectSigs[6]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[0]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[1]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[2]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[3]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[3]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[4]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[5]))
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
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[6]))
							EXIDeselectSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  17, length, &OSRestoreInterruptsSig) &&
							get_immediate(data,   i + 24, i +  25, &address) && address == 0xCC006800 &&
							findx_pattern(data, dataType, i +  39, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  42, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i +  44, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i +  54, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 57, i +  58, &address) && address == 0xCC006800 &&
							get_immediate(data,  i +  72, i +  73, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i +  77, length, &OSGetTimeSig) &&
							get_immediate(data,  i +  78, i +  79, &address) && address == 0x800000F8 &&
							get_immediate(data,  i +  92, i +  93, &address) && address == 0x800030C0 &&
							get_immediate(data,  i + 107, i + 108, &address) && address == 0x800030C0 &&
							get_immediate(data,  i + 118, i + 119, &address) && address == 0x800030C0 &&
							findx_pattern(data, dataType, i + 122, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXISelectSigs[7]))
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
					case 10:
						if (get_immediate(data,   i +  3, i +  4, &address) && address == 0xCC006834 &&
							get_immediate(data,   i +  3, i +  5, &address) && address == 0xCC00680C &&
							get_immediate(data,   i +  3, i +  6, &address) && address == 0xCC006820 &&
							findx_pattern(data, dataType, i + 21, length, &__OSMaskInterruptsSigs[2]) &&
							get_immediate(data,   i + 23, i + 24, &address) && address == 0xCC006800 &&
							get_immediate(data,   i + 23, i + 25, &address) && address == 0xCC006814 &&
							get_immediate(data,   i + 23, i + 26, &address) && address == 0xCC006828 &&
							get_immediate(data,   i + 23, i + 28, &address) && address == 0xCC006800 &&
							findi_pattern(data, dataType, i + 30, i + 31, length, &EXIIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 32, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 34, i + 35, length, &TCIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 36, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 38, i + 39, length, &EXTIntrruptHandlerSigs[8]) &&
							findx_pattern(data, dataType, i + 40, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 42, i + 43, length, &EXIIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 44, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 46, i + 47, length, &TCIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 48, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 50, i + 51, length, &EXTIntrruptHandlerSigs[8]) &&
							findx_pattern(data, dataType, i + 52, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 54, i + 55, length, &EXIIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 56, length, &__OSSetInterruptHandlerSigs[2]) &&
							findi_pattern(data, dataType, i + 58, i + 59, length, &TCIntrruptHandlerSigs[7]) &&
							findx_pattern(data, dataType, i + 60, length, &__OSSetInterruptHandlerSigs[2]) &&
							findx_pattern(data, dataType, i + 64, length, &EXIGetIDSigs[9]) &&
							findx_pattern(data, dataType, i + 68, length, &EXIProbeResetSigs[2]) &&
							findx_pattern(data, dataType, i + 73, length, &EXIGetIDSigs[9]) &&
							findx_pattern(data, dataType, i + 87, length, &EXIGetIDSigs[9]))
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
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[0]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 19, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 25, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 33, length, &SetExiInterruptMaskSigs[1]) &&
							findx_pattern(data, dataType, i + 53, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[1]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 20, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 34, length, &SetExiInterruptMaskSigs[2]) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[2]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if ((data[i + 8] & 0xFC00FFFF) == 0x1C000038 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[3]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[3]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if ((data[i + 9] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[4]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[4]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if ((data[i + 5] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i +  9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 15, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 23, length, &SetExiInterruptMaskSigs[5]) &&
							findx_pattern(data, dataType, i + 43, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[5]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 18, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 26, length, &SetExiInterruptMaskSigs[6]) &&
							findx_pattern(data, dataType, i + 45, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[6]))
							EXIUnlockSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if ((data[i + 8] & 0xFC00FFFF) == 0x54003032 &&
							findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 17, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 44, length, &__OSMaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 47, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 56, length, &__OSMaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 59, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 69, length, &__OSMaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 72, length, &__OSUnmaskInterruptsSigs[2]) &&
							findx_pattern(data, dataType, i + 91, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &EXILockSigs[7]))
							EXIUnlockSigs[j].offsetFoundAt = i;
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
							find_pattern_before(data, dataType, length, &SetTimeoutAlarmSigs[0], &AlarmHandlerForTimeoutSigs[0]))
							ReadSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i + 14, length, &__OSGetSystemTimeSigs[0],
							                                               &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 43, length, &SetTimeoutAlarmSigs[0]) &&
							findx_pattern (data, dataType, i + 50, length, &SetTimeoutAlarmSigs[0]) &&
							find_pattern_before(data, dataType, length, &SetTimeoutAlarmSigs[0], &AlarmHandlerForTimeoutSigs[0]))
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
							
							if (find_pattern_after(data, dataType, length, &DVDLowReadSigs[0], &DVDLowSeekSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowSeekSigs[0], &DVDLowWaitCoverCloseSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowWaitCoverCloseSigs[0], &DVDLowReadDiskIDSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowReadDiskIDSigs[0], &DVDLowStopMotorSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[0], &DVDLowRequestErrorSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestErrorSigs[0], &DVDLowInquirySigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowInquirySigs[0], &DVDLowAudioStreamSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioStreamSigs[0], &DVDLowRequestAudioStatusSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestAudioStatusSigs[0], &DVDLowAudioBufferConfigSigs[0]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[0], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  52, length, &DoJustReadSigs[0]) &&
							findx_pattern (data, dataType, i +  75, length, &DoJustReadSigs[0]) &&
							findx_patterns(data, dataType, i +  89, length, &__OSGetSystemTimeSigs[0],
							                                                &__OSGetSystemTimeSigs[1], NULL) &&
							findx_pattern (data, dataType, i + 112, length, &DoJustReadSigs[0])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, dataType, length, &DVDLowReadSigs[1], &DVDLowSeekSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowWaitCoverCloseSigs[1], &DVDLowReadDiskIDSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowReadDiskIDSigs[1], &DVDLowStopMotorSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestErrorSigs[1], &DVDLowInquirySigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowInquirySigs[1], &DVDLowAudioStreamSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioStreamSigs[1], &DVDLowRequestAudioStatusSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestAudioStatusSigs[1], &DVDLowAudioBufferConfigSigs[1]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
						}
						break;
					case 3:
						if (findx_patterns(data, dataType, i +  28, length, &ReadSigs[2], &ReadSigs[3], NULL) &&
							findx_patterns(data, dataType, i +  83, length, &ReadSigs[2], &ReadSigs[3], NULL) &&
							findx_pattern (data, dataType, i +  97, length, &__OSGetSystemTimeSigs[1]) &&
							findx_patterns(data, dataType, i + 125, length, &ReadSigs[2], &ReadSigs[3], NULL)) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, dataType, length, &DVDLowReadSigs[3], &DVDLowSeekSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowSeekSigs[3], &DVDLowWaitCoverCloseSigs[2]) &&
								find_pattern_after(data, dataType, length, &DVDLowWaitCoverCloseSigs[2], &DVDLowReadDiskIDSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowReadDiskIDSigs[3], &DVDLowStopMotorSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[3], &DVDLowRequestErrorSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestErrorSigs[3], &DVDLowInquirySigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowInquirySigs[3], &DVDLowAudioStreamSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioStreamSigs[3], &DVDLowRequestAudioStatusSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestAudioStatusSigs[3], &DVDLowAudioBufferConfigSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[3], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  28, length, &ReadSigs[4]) &&
							findx_pattern(data, dataType, i +  83, length, &ReadSigs[4]) &&
							findx_pattern(data, dataType, i +  97, length, &__OSGetSystemTimeSigs[1]) &&
							findx_pattern(data, dataType, i + 125, length, &ReadSigs[4])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, dataType, length, &DVDLowReadSigs[4], &DVDLowSeekSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]) &&
								find_pattern_after(data, dataType, length, &DVDLowWaitCoverCloseSigs[3], &DVDLowReadDiskIDSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowReadDiskIDSigs[4], &DVDLowStopMotorSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestErrorSigs[4], &DVDLowInquirySigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowInquirySigs[4], &DVDLowAudioStreamSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioStreamSigs[4], &DVDLowRequestAudioStatusSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestAudioStatusSigs[4], &DVDLowAudioBufferConfigSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
						}
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  28, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 136, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 191, length, &__OSGetSystemTimeSigs[2]) &&
							findx_pattern(data, dataType, i + 219, length, &__OSGetSystemTimeSigs[2])) {
							DVDLowReadSigs[j].offsetFoundAt = i;
							
							if (find_pattern_after(data, dataType, length, &DVDLowReadSigs[5], &DVDLowSeekSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowSeekSigs[5], &DVDLowWaitCoverCloseSigs[4]) &&
								find_pattern_after(data, dataType, length, &DVDLowWaitCoverCloseSigs[4], &DVDLowReadDiskIDSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowReadDiskIDSigs[5], &DVDLowStopMotorSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[5], &DVDLowRequestErrorSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestErrorSigs[5], &DVDLowInquirySigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowInquirySigs[5], &DVDLowAudioStreamSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowAudioStreamSigs[5], &DVDLowRequestAudioStatusSigs[5]) &&
								find_pattern_after(data, dataType, length, &DVDLowRequestAudioStatusSigs[5], &DVDLowAudioBufferConfigSigs[5]))
								find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[5], &DVDLowResetSigs[2]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[0], &DVDLowWaitCoverCloseSigs[0]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[0], &DVDLowRequestErrorSigs[0]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[0], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[1], &DVDLowWaitCoverCloseSigs[1]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[1], &DVDLowRequestErrorSigs[1]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[1], &DVDLowResetSigs[0]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[0], &DVDLowSetResetCoverCallbackSigs[0]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[2], &DVDLowWaitCoverCloseSigs[2]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[2], &DVDLowRequestErrorSigs[2]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[2], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[3], &DVDLowWaitCoverCloseSigs[2]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[3], &DVDLowRequestErrorSigs[3]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[3], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[5], &DVDLowWaitCoverCloseSigs[4]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[5], &DVDLowRequestErrorSigs[5]);
							find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[5], &DVDLowResetSigs[2]);
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
							
							find_pattern_after(data, dataType, length, &DVDLowSeekSigs[4], &DVDLowWaitCoverCloseSigs[3]);
							find_pattern_after(data, dataType, length, &DVDLowStopMotorSigs[4], &DVDLowRequestErrorSigs[4]);
							
							if (find_pattern_after(data, dataType, length, &DVDLowAudioBufferConfigSigs[4], &DVDLowResetSigs[1]))
								find_pattern_after(data, dataType, length, &DVDLowResetSigs[1], &DVDLowSetResetCoverCallbackSigs[1]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[0], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[1], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[1], &SetBreakAlarmSigs[0]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[0], &AlarmHandlerForBreakSigs[0]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[0], &DoBreakSigs[0]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[3], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[4], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
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
							
							if (find_pattern_before(data, dataType, length, &DVDLowBreakSigs[4], &SetBreakAlarmSigs[1]) &&
								find_pattern_before(data, dataType, length, &SetBreakAlarmSigs[1], &AlarmHandlerForBreakSigs[1]))
								find_pattern_before(data, dataType, length, &AlarmHandlerForBreakSigs[1], &DoBreakSigs[1]);
						}
						break;
					case 8:
						if (findx_pattern (data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i +  31, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i +  42, length, &DVDLowBreakSigs[5]) &&
							findx_pattern (data, dataType, i +  45, length, &__DVDDequeueWaitingQueueSigs[2]) &&
							findx_pattern (data, dataType, i +  87, length, &OSRestoreInterruptsSig) &&
							findx_patterns(data, dataType, i +  94, length, &DVDLowClearCallbackSigs[3],
							                                                &DVDLowClearCallbackSigs[5], NULL) &&
							findx_pattern (data, dataType, i + 100, length, &OSRestoreInterruptsSig) &&
							findx_pattern (data, dataType, i + 149, length, &OSRestoreInterruptsSig))
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
							find_pattern_before(data, dataType, length, &fp, &DVDGetCurrentDiskIDSigs[0]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 59, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &DVDGetCurrentDiskIDSigs[0]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 50, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 54, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
							DVDCheckDiskSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 55, length, &OSRestoreInterruptsSig) &&
							find_pattern_before(data, dataType, length, &fp, &DVDGetCurrentDiskIDSigs[1]))
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
		
		for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__VIRetraceHandlerSigs[j])) {
				switch (j) {
					case 0:
						if (get_immediate(data,  i +   6, i +   7, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  11, i +  12, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  14, i +  15, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  19, i +  20, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  22, i +  23, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  27, i +  28, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  35, i +  36, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  38, i +  39, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  45, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  61, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  74, length, &VISetRegsSigs[0]) &&
							findx_pattern(data, dataType, i + 104, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 112, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 114, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (get_immediate(data,  i +   6, i +   7, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  11, i +  12, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  14, i +  15, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  19, i +  20, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  22, i +  23, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  27, i +  28, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  35, i +  36, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  38, i +  39, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  45, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  61, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  74, length, &VISetRegsSigs[1]) &&
							findx_pattern(data, dataType, i + 105, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 113, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 115, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (get_immediate(data,  i +   6, i +   7, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  11, i +  12, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  14, i +  15, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  19, i +  20, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  22, i +  23, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  27, i +  28, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  35, i +  36, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  38, i +  39, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  45, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  60, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  62, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  76, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i +  78, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  91, length, &VISetRegsSigs[2]) &&
							findx_pattern(data, dataType, i + 122, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 130, length, &OSClearContextSigs[0]) &&
							findx_pattern(data, dataType, i + 132, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (get_immediate(data,  i +   1, i +   7, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  13, i +  14, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  20, i +  21, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  27, i +  28, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  45, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  64, i +  66, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 116, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 124, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 126, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (get_immediate(data,  i +   1, i +   7, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  13, i +  14, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  20, i +  21, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  27, i +  28, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  45, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  64, i +  66, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 121, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 129, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 131, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (get_immediate(data,  i +   1, i +   3, &address) && address == 0xCC002000 &&
							get_immediate(data,  i +  16, i +  17, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  23, i +  24, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  48, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  66, i +  67, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 122, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 130, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 132, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (get_immediate(data,  i +   1, i +   3, &address) && address == 0xCC002000 &&
							get_immediate(data,  i +  16, i +  17, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  23, i +  24, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  48, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  66, i +  67, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 124, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 132, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 134, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (get_immediate(data,  i +   2, i +   9, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +   2, i +  15, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  16, i +  17, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  16, i +  23, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  24, i +  25, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  24, i +  31, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  32, i +  33, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  32, i +  39, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  41, i +  42, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  53, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  72, i +  74, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 129, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 137, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 139, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (get_immediate(data,  i +   1, i +   3, &address) && address == 0xCC002000 &&
							get_immediate(data,  i +  16, i +  17, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  23, i +  24, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  30, i +  31, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  42, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  44, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  57, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  65, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i +  67, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  83, i +  84, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 141, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 149, length, &OSClearContextSigs[1]) &&
							findx_pattern(data, dataType, i + 151, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 9:
						if (get_immediate(data,  i +   2, i +   9, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +   2, i +  15, &address) && address == 0xCC002030 &&
							get_immediate(data,  i +  16, i +  17, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  16, i +  23, &address) && address == 0xCC002034 &&
							get_immediate(data,  i +  24, i +  25, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  24, i +  31, &address) && address == 0xCC002038 &&
							get_immediate(data,  i +  32, i +  33, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  32, i +  39, &address) && address == 0xCC00203C &&
							get_immediate(data,  i +  41, i +  42, &address) && address == 0xCC00203C &&
							findx_pattern(data, dataType, i +  47, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i +  49, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  62, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i +  64, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  70, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i +  72, length, &OSSetCurrentContextSig) &&
							get_immediate(data,  i +  89, i +  91, &address) && address == 0xCC002000 &&
							findx_pattern(data, dataType, i + 146, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 154, length, &OSClearContextSigs[2]) &&
							findx_pattern(data, dataType, i + 156, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
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
					case 2:
						if (findx_pattern(data, dataType, i +  7, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i +  8, i + 10, &address) && address == 0xCC005030 &&
							get_immediate(data,   i +  8, i + 15, &address) && address == 0xCC005030 &&
							get_immediate(data,   i +  8, i + 16, &address) && address == 0xCC005032 &&
							get_immediate(data,   i +  8, i + 19, &address) && address == 0xCC005032 &&
							get_immediate(data,   i +  8, i + 20, &address) && address == 0xCC005036 &&
							get_immediate(data,   i +  8, i + 23, &address) && address == 0xCC005036 &&
							findx_pattern(data, dataType, i + 24, length, &OSRestoreInterruptsSig))
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
	
	if ((i = memcpySig.offsetFoundAt)) {
		u32 *_memcpy = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *backwards_memcpy;
		
		if (_memcpy) {
			backwards_memcpy = getPatchAddr(BACKWARDS_MEMCPY);
			
			memset(data + i, 0, memcpySig.Length * sizeof(u32));
			memcpy(data + i, memcpySig.Patch, memcpySig.PatchLength * sizeof(u32));
			data[i + memcpySig.PatchLength - 1] = branch(backwards_memcpy, _memcpy + memcpySig.PatchLength - 1);
			
			print_debug("Found:[%s] @ %08X\n", memcpySig.Name, _memcpy);
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
			print_debug("Found:[%s$%i] @ %08X\n", OSExceptionInitSigs[j].Name, j, OSExceptionInit);
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
			
			print_debug("Found:[%s$%i] @ %08X\n", OSSetAlarmSigs[j].Name, j, OSSetAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(OSCancelAlarmSigs) / sizeof(FuncPattern); j++)
	if ((i = OSCancelAlarmSigs[j].offsetFoundAt)) {
		u32 *OSCancelAlarm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSCancelAlarm) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 1] = (u32)OSCancelAlarm;
			
			print_debug("Found:[%s$%i] @ %08X\n", OSCancelAlarmSigs[j].Name, j, OSCancelAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DecrementerExceptionCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = DecrementerExceptionCallbackSigs[j].offsetFoundAt)) {
		u32 *DecrementerExceptionCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *CallAlarmHandler;
		
		if (DecrementerExceptionCallback) {
			if (devices[DEVICE_CUR]->emulated() & EMU_READ_SPEED) {
				switch (j) {
					case 0:
						CallAlarmHandler = getPatchAddr(OS_CALLALARMHANDLER);
						
						data[i +  81] = 0x38BB0000;	// addi		r5, r27, 0
						data[i +  82] = 0x60000000;	// nop
						data[i +  83] = branchAndLink(CallAlarmHandler, DecrementerExceptionCallback +  83);
						break;
					case 2:
						CallAlarmHandler = getPatchAddr(OS_CALLALARMHANDLER);
						
						data[i + 115] = 0x38BE0000;	// addi		r5, r30, 0
						data[i + 116] = 0x60000000;	// nop
						data[i + 119] = branchAndLink(CallAlarmHandler, DecrementerExceptionCallback + 119);
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", DecrementerExceptionCallbackSigs[j].Name, j, DecrementerExceptionCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(OSLoadContextSigs) / sizeof(FuncPattern); j++)
	if ((i = OSLoadContextSigs[j].offsetFoundAt)) {
		u32 *OSLoadContext = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSLoadContext) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 2] = (u32)OSLoadContext;
			
			print_debug("Found:[%s$%i] @ %08X\n", OSLoadContextSigs[j].Name, j, OSLoadContext);
			patched++;
		}
	}
	
	if ((i = OSDisableInterruptsSig.offsetFoundAt)) {
		u32 *OSDisableInterrupts = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSDisableInterrupts) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 3] = (u32)OSDisableInterrupts;
			
			print_debug("Found:[%s$%i] @ %08X\n", OSDisableInterruptsSig.Name, j, OSDisableInterrupts);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSInterruptInitSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSInterruptInitSigs[j].offsetFoundAt)) {
		u32 *__OSInterruptInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSInterruptInit) {
			switch (j) {
				case 0:
					data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 30] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 13] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __OSInterruptInitSigs[j].Name, j, __OSInterruptInit);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SetInterruptMaskSigs) / sizeof(FuncPattern); j++)
	if ((i = SetInterruptMaskSigs[j].offsetFoundAt)) {
		u32 *SetInterruptMask = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SetInterruptMask) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
						data[i +  72] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  88] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  92] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 109] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 114] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 126] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 2:
						data[i +  82] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  98] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 102] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 119] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 124] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 136] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 3:
					case 4:
						data[i +  70] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  85] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  89] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 110] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 5:
						data[i +  80] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  95] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  99] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 120] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 6:
						data[i +  67] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  79] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i +  84] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i +  96] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 100] = 0x3CA00C00;	// lis		r5, 0x0C00
						data[i + 109] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
				}
			}
			switch (j) {
				case 0:
					data[i + 168] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 1:
					data[i + 172] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 2:
					data[i + 182] = 0x3CA00C00;	// lis		r5, 0x0C00
					break;
				case 3:
					data[i + 163] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 4:
					data[i + 167] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 5:
					data[i + 177] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 6:
					data[i + 144] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", SetInterruptMaskSigs[j].Name, j, SetInterruptMask);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__OSDispatchInterruptSigs) / sizeof(FuncPattern); j++)
	if ((i = __OSDispatchInterruptSigs[j].offsetFoundAt)) {
		u32 *__OSDispatchInterrupt = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__OSDispatchInterrupt) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  85] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  99] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 114] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
					case 5:
						data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  90] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 105] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i +  58] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  69] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  80] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			switch (j) {
				case 0:
					data[i +   7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  12] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 170] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 173] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 179] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 182] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 191] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 1:
				case 2:
					data[i +   7] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i +  12] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 174] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 177] = 0x3CA00C00;	// lis		r5, 0x0C00
					data[i + 183] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 186] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 195] = 0x3C800C00;	// lis		r4, 0x0C00
					break;
				case 3:
				case 4:
				case 5:
					data[i +   7] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i +   2] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 4] = (u32)__OSDispatchInterrupt;
			
			print_debug("Found:[%s$%i] @ %08X\n", __OSDispatchInterruptSigs[j].Name, j, __OSDispatchInterrupt);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(ExternalInterruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = ExternalInterruptHandlerSigs[j].offsetFoundAt)) {
		u32 *ExternalInterruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (ExternalInterruptHandler) {
			data[i + ExternalInterruptHandlerSigs[j].Length - 1] = branch(DISPATCH_INTERRUPT, ExternalInterruptHandler + ExternalInterruptHandlerSigs[j].Length - 1);
			
			print_debug("Found:[%s$%i] @ %08X\n", ExternalInterruptHandlerSigs[j].Name, j, ExternalInterruptHandler);
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
			if ((k = PPCHaltSig.offsetFoundAt)) {
				u32 *PPCHalt = Calc_ProperAddress(data, dataType, k * sizeof(u32));
				
				if (PPCHalt) {
					if (swissSettings.igrType != IGR_OFF)
						data[k + 4] = branch(__OSDoHotReset, PPCHalt + 4);
					
					print_debug("Found:[%s] @ %08X\n", PPCHaltSig.Name, PPCHalt);
					patched++;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", __OSDoHotResetSigs[j].Name, j, __OSDoHotReset);
			patched++;
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
				case 15: data[i + 70] = branchAndLink(FINI, OSResetSystem + 70); break;
				case 16: data[i + 86] = branchAndLink(FINI, OSResetSystem + 86); break;
				case 17: data[i + 87] = branchAndLink(FINI, OSResetSystem + 87); break;
				case 18: data[i + 74] = branchAndLink(FINI, OSResetSystem + 74); break;
			}
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 5] = (u32)OSResetSystem;
			
			print_debug("Found:[%s$%i] @ %08X\n", OSResetSystemSigs[j].Name, j, OSResetSystem);
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
					data[i +  4] = 0x38600000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) & 0xFFFF);
					data[i + 12] = 0x3CA00000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i + 13] = 0x38050000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i + 17] = 0x38800020;	// li		r4, 32
					
					if (get_immediate(data, OSGetArenaLoSigs[0].offsetFoundAt, 13, &__OSArenaLo) ||
						get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 20] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 20] = 0x38600000;
					
					if (get_immediate(data, OSGetArenaHiSigs[0].offsetFoundAt, 13, &__OSArenaHi) ||
						get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 21] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 21] = 0x38800000;
					
					data[i + 22] = branchAndLink(INIT, __OSInitSystemCall + 22);
					break;
				case 1:
					data[i +  4] = 0x3CA00000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) + 0x8000) >> 16;
					data[i +  6] = 0x3C600000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i +  7] = 0x3BE50000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) & 0xFFFF);
					data[i +  8] = 0x38030000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i + 14] = 0x38800020;	// li		r4, 32
					
					if (get_immediate(data, OSGetArenaLoSigs[1].offsetFoundAt, 13, &__OSArenaLo) ||
						get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 17] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 17] = 0x38600000;
					
					if (get_immediate(data, OSGetArenaHiSigs[1].offsetFoundAt, 13, &__OSArenaHi) ||
						get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 18] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 18] = 0x38800000;
					
					data[i + 19] = branchAndLink(INIT, __OSInitSystemCall + 19);
					break;
				case 2:
					data[i +  3] = 0x3C600000 | ((u32)__OSInitSystemCall + 0x8000) >> 16;
					data[i +  6] = 0x3CA00000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) + 0x8000) >> 16;
					data[i +  7] = 0x38030000 | ((u32)__OSInitSystemCall & 0xFFFF);
					data[i +  8] = 0x38650000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) & 0xFFFF);
					data[i + 11] = 0x3C600000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) + 0x8000) >> 16;
					data[i + 12] = 0x38800020;	// li		r4, 32
					data[i + 13] = 0x38630000 | ((u32)(VAR_JUMP_TABLE - VAR_AREA + 0x80000000) & 0xFFFF);
					data[i + 16] = 0x60000000;	// nop
					
					if (get_immediate(data, OSGetArenaLoSigs[2].offsetFoundAt, 13, &__OSArenaLo) ||
						get_immediate(data, OSSetArenaLoSig.offsetFoundAt, 13, &__OSArenaLo))
						data[i + 17] = 0x386D0000 | ((__OSArenaLo - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 17] = 0x38600000;
					
					if (get_immediate(data, OSGetArenaHiSigs[2].offsetFoundAt, 13, &__OSArenaHi) ||
						get_immediate(data, OSSetArenaHiSig.offsetFoundAt, 13, &__OSArenaHi))
						data[i + 18] = 0x388D0000 | ((__OSArenaHi - _SDA_BASE_) & 0xFFFF);
					else
						data[i + 18] = 0x38800000;
					
					data[i + 19] = branchAndLink(INIT, __OSInitSystemCall + 19);
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __OSInitSystemCallSigs[j].Name, j, __OSInitSystemCall);
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
			print_debug("Found:[%s$%i] @ %08X\n", SelectThreadSigs[j].Name, j, SelectThread);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(CompleteTransferSigs) / sizeof(FuncPattern); j++)
	if ((i = CompleteTransferSigs[j].offsetFoundAt)) {
		u32 *CompleteTransfer = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (CompleteTransfer) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
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
			print_debug("Found:[%s$%i] @ %08X\n", CompleteTransferSigs[j].Name, j, CompleteTransfer);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIImmSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIImmSigs[j].offsetFoundAt)) {
		u32 *EXIImm = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIImm) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  93] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 111] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i + 119] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 133] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +  58] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  76] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 118] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 135] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i + 119] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 136] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIImmSigs[j].Name, j, EXIImm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIDmaSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIDmaSigs[j].offsetFoundAt)) {
		u32 *EXIDma = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIDma) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  88] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  94] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 102] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i +  39] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +  47] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  54] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  63] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i +  42] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i +  43] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIDmaSigs[j].Name, j, EXIDma);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXISyncSigs) / sizeof(FuncPattern); j++)
	if ((i = EXISyncSigs[j].offsetFoundAt)) {
		u32 *EXISync = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISync) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
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
					case 12:
						data[i +  12] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  34] = 0x3C800C00;	// lis		r4, 0x0C00
						data[i + 114] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXISyncSigs[j].Name, j, EXISync);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIClearInterruptsSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIClearInterruptsSigs[j].offsetFoundAt)) {
		u32 *EXIClearInterrupts = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIClearInterrupts) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
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
					case 4:
						data[i +  1] = 0x3CE00C00;	// lis		r7, 0x0C00
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIClearInterruptsSigs[j].Name, j, EXIClearInterrupts);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__EXIProbeSigs) / sizeof(FuncPattern); j++)
	if ((i = __EXIProbeSigs[j].offsetFoundAt)) {
		u32 *__EXIProbe = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__EXIProbe) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
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
					case 7:
						data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", __EXIProbeSigs[j].Name, j, __EXIProbe);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIAttachSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIAttachSigs[j].offsetFoundAt)) {
		u32 *EXIAttach = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIAttach) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				if (j == 7) {
					data[i +  19] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 115] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 188] = 0x3C600C00;	// lis		r3, 0x0C00
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIAttachSigs[j].Name, j, EXIAttach);
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
			print_debug("Found:[%s$%i] @ %08X\n", EXIDetachSigs[j].Name, j, EXIDetach);
			patched++;
		}
	}
	
	if ((i = EXISelectSDSig.offsetFoundAt)) {
		u32 *EXISelectSD = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISelectSD) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER))
				data[i + 55] = 0x3C600C00;	// lis		r3, 0x0C00
			
			print_debug("Found:[%s] @ %08X\n", EXISelectSDSig.Name, EXISelectSD);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXISelectSigs) / sizeof(FuncPattern); j++)
	if ((i = EXISelectSigs[j].offsetFoundAt)) {
		u32 *EXISelect = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXISelect) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i +  79] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  91] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i +  38] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i +  42] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  55] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i +  41] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i +  32] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 114] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXISelectSigs[j].Name, j, EXISelect);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIDeselectSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIDeselectSigs[j].offsetFoundAt)) {
		u32 *EXIDeselect = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIDeselect) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
						data[i + 33] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 39] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i + 34] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 40] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
					case 5:
						data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 8:
						data[i + 24] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 57] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIDeselectSigs[j].Name, j, EXIDeselect);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *EXIIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
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
					case 7:
						data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIIntrruptHandlerSigs[j].Name, j, EXIIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(TCIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = TCIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *TCIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (TCIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 3:
					case 4:
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
						data[i + 20] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 53] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 23] = 0x3CC00C00;	// lis		r6, 0x0C00
						break;
					case 7:
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", TCIntrruptHandlerSigs[j].Name, j, TCIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXTIntrruptHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = EXTIntrruptHandlerSigs[j].offsetFoundAt)) {
		u32 *EXTIntrruptHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXTIntrruptHandler) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
					case 1:
						data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 4:
						data[i + 18] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 5:
						data[i + 17] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
					case 6:
						data[i + 17] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXTIntrruptHandlerSigs[j].Name, j, EXTIntrruptHandler);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIInitSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIInitSigs[j].offsetFoundAt)) {
		u32 *EXIInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIInit) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				switch (j) {
					case 0:
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 1:
						data[i +  9] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 12] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 15] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 18] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 2:
						data[i +  3] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i +  8] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 22] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 31] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 3:
					case 4:
						data[i + 10] = 0x3CA00C00;	// lis		r5, 0x0C00
						break;
					case 5:
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 10] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 13] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 16] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 6:
						data[i + 13] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
					case 7:
					case 8:
					case 9:
						data[i +  7] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 25] = 0x3C800C00;	// lis		r4, 0x0C00
						break;
					case 10:
						data[i +  3] = 0x3C600C00;	// lis		r3, 0x0C00
						data[i + 23] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIInitSigs[j].Name, j, EXIInit);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXILockSigs) / sizeof(FuncPattern); j++)
	if ((i = EXILockSigs[j].offsetFoundAt)) {
		u32 *EXILock = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXILock) {
			if (devices[DEVICE_CUR]->emulated() & EMU_BUS_ARBITER) {
				switch (j) {
					case 0:
						data[i + 26] = 0x281D0003;	// cmplwi	r29, 3
						data[i + 27] = 0x41800018;	// blt		+6
						data[i + 46] = 0x2C000002;	// cmpwi	r0, 2
						data[i + 47] = 0x41800018;	// blt		+6
						data[i + 56] = 0x38030024;	// addi		r0, r3, 36
						data[i + 58] = 0x7C00C840;	// cmplw	r0, r25
						break;
					case 1:
						data[i + 26] = 0x281D0003;	// cmplwi	r29, 3
						data[i + 27] = 0x41800018;	// blt		+6
						data[i + 46] = 0x2C000002;	// cmpwi	r0, 2
						data[i + 47] = 0x41800018;	// blt		+6
						data[i + 56] = 0x3803002C;	// addi		r0, r3, 44
						data[i + 58] = 0x7C00C840;	// cmplw	r0, r25
						break;
					case 2:
						data[i + 26] = 0x281C0003;	// cmplwi	r28, 3
						data[i + 27] = 0x41800018;	// blt		+6
						data[i + 46] = 0x2C000002;	// cmpwi	r0, 2
						data[i + 47] = 0x41800018;	// blt		+6
						data[i + 56] = 0x3803002C;	// addi		r0, r3, 44
						data[i + 58] = 0x7C00C840;	// cmplw	r0, r25
						break;
					case 3:
						data[i + 23] = 0x80030024;	// lwz		r0, 36 (r3)
						data[i + 24] = 0x7C00E840;	// cmplw	r0, r29
						break;
					case 4:
						data[i + 23] = 0x8003002C;	// lwz		r0, 44 (r3)
						data[i + 24] = 0x7C00E840;	// cmplw	r0, r29
						break;
					case 5:
						data[i + 21] = 0x3803002C;	// addi		r0, r3, 44
						data[i + 23] = 0x7C00D840;	// cmplw	r0, r27
						break;
					case 6:
						data[i + 23] = 0x8003002C;	// lwz		r0, 44 (r3)
						data[i + 24] = 0x7C00E040;	// cmplw	r0, r28
						break;
					case 7:
						data[i + 24] = 0x8003002C;	// lwz		r0, 44 (r3)
						data[i + 25] = 0x7C00E040;	// cmplw	r0, r28
						break;
				}
			}
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 6] = (u32)EXILock;
			
			print_debug("Found:[%s$%i] @ %08X\n", EXILockSigs[j].Name, j, EXILock);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIUnlockSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIUnlockSigs[j].offsetFoundAt)) {
		u32 *EXIUnlock = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIUnlock) {
			if ((k = SystemCallVectorSig.offsetFoundAt))
				data[k + 7] = (u32)EXIUnlock;
			
			print_debug("Found:[%s$%i] @ %08X\n", EXIUnlockSigs[j].Name, j, EXIUnlock);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(EXIGetIDSigs) / sizeof(FuncPattern); j++)
	if ((i = EXIGetIDSigs[j].offsetFoundAt)) {
		u32 *EXIGetID = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (EXIGetID) {
			if (devices[DEVICE_CUR]->emulated() & (EMU_MEMCARD | EMU_ETHERNET | EMU_BUS_ARBITER)) {
				if (j == 9) {
					data[i +  33] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 127] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 188] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 344] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 426] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 476] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 493] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 512] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 534] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 614] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 665] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 683] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 694] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 716] = 0x3C800C00;	// lis		r4, 0x0C00
					data[i + 797] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 842] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 875] = 0x3C600C00;	// lis		r3, 0x0C00
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", EXIGetIDSigs[j].Name, j, EXIGetID);
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
			print_debug("Found:[%s$%i] @ %08X\n", __DVDInterruptHandlerSigs[j].Name, j, __DVDInterruptHandler);
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
			print_debug("Found:[%s$%i] @ %08X\n", ReadSigs[j].Name, j, Read);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowReadSigs[j].Name, j, DVDLowRead);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowSeekSigs[j].Name, j, DVDLowSeek);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowWaitCoverCloseSigs[j].Name, j, DVDLowWaitCoverClose);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowReadDiskIDSigs[j].Name, j, DVDLowReadDiskID);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowStopMotorSigs[j].Name, j, DVDLowStopMotor);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowRequestErrorSigs[j].Name, j, DVDLowRequestError);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowInquirySigs[j].Name, j, DVDLowInquiry);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowAudioStreamSigs[j].Name, j, DVDLowAudioStream);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowRequestAudioStatusSigs[j].Name, j, DVDLowRequestAudioStatus);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowAudioBufferConfigSigs[j].Name, j, DVDLowAudioBufferConfig);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowResetSigs[j].Name, j, DVDLowReset);
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
			print_debug("Found:[%s$%i] @ %08X\n", DoBreakSigs[j].Name, j, DoBreak);
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
			print_debug("Found:[%s$%i] @ %08X\n", AlarmHandlerForBreakSigs[j].Name, j, AlarmHandlerForBreak);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowBreakSigs[j].Name, j, DVDLowBreak);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowClearCallbackSigs[j].Name, j, DVDLowClearCallback);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDLowGetCoverStatusSigs[j].Name, j, DVDLowGetCoverStatus);
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
			
			print_debug("Found:[%s] @ %08X\n", __DVDLowTestAlarmSig.Name, __DVDLowTestAlarm);
			patched++;
		}
	}
	
	if ((i = DVDGetTransferredSizeSig.offsetFoundAt)) {
		u32 *DVDGetTransferredSize = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDGetTransferredSize) {
			data[i + 20] = 0x3C800C00;	// lis		r4, 0x0C00
			
			print_debug("Found:[%s] @ %08X\n", DVDGetTransferredSizeSig.Name, DVDGetTransferredSize);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(DVDInitSigs) / sizeof(FuncPattern); j++)
	if ((i = DVDInitSigs[j].offsetFoundAt)) {
		u32 *DVDInit = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (DVDInit) {
			switch (j) {
				case 0:
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 1:
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 30] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 2:
					data[i + 29] = 0x3C600C00;	// lis		r3, 0x0C00
					data[i + 32] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 3:
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 4:
					data[i + 27] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 5:
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 6:
					data[i + 25] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
				case 7:
					data[i + 28] = 0x3C600C00;	// lis		r3, 0x0C00
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", DVDInitSigs[j].Name, j, DVDInit);
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
			print_debug("Found:[%s$%i] @ %08X\n", cbForStateGettingErrorSigs[j].Name, j, cbForStateGettingError);
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
			print_debug("Found:[%s$%i] @ %08X\n", cbForUnrecoveredErrorRetrySigs[j].Name, j, cbForUnrecoveredErrorRetry);
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
			print_debug("Found:[%s$%i] @ %08X\n", cbForStateMotorStoppedSigs[j].Name, j, cbForStateMotorStopped);
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
			print_debug("Found:[%s$%i] @ %08X\n", stateBusySigs[j].Name, j, stateBusy);
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
			print_debug("Found:[%s$%i] @ %08X\n", cbForStateBusySigs[j].Name, j, cbForStateBusy);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDResetSigs[j].Name, j, DVDReset);
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
			print_debug("Found:[%s$%i] @ %08X\n", DVDCheckDiskSigs[j].Name, j, DVDCheckDisk);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(VISetRegsSigs) / sizeof(FuncPattern); j++)
	if ((i = VISetRegsSigs[j].offsetFoundAt)) {
		u32 *VISetRegs = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VISetRegs) {
			if (swissSettings.pauseAVOutput && !(devices[DEVICE_CUR]->emulated() & EMU_NO_PAUSING)) {
				switch (j) {
					case 0:
					case 1:
					case 2:
						data[i + 21] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", VISetRegsSigs[j].Name, j, VISetRegs);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern); j++)
	if ((i = __VIRetraceHandlerSigs[j].offsetFoundAt)) {
		u32 *__VIRetraceHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__VIRetraceHandler) {
			if (swissSettings.pauseAVOutput && !(devices[DEVICE_CUR]->emulated() & EMU_NO_PAUSING)) {
				switch (j) {
					case 3:
					case 4:
						data[i + 64] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 5:
					case 6:
						data[i + 66] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 7:
						data[i + 72] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 8:
						data[i + 83] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
					case 9:
						data[i + 89] = 0x3C600C00;	// lis		r3, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", __VIRetraceHandlerSigs[j].Name, j, __VIRetraceHandler);
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
					case 2:
						data[i +  8] = 0x3CE00C00;	// lis		r7, 0x0C00
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", AIInitDMASigs[j].Name, j, AIInitDMA);
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
			print_debug("Found:[%s] @ %08X\n", AIGetDMAStartAddrSigs[j].Name, AIGetDMAStartAddr);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(GXPeekARGBSigs) / sizeof(FuncPattern); j++)
	if ((i = GXPeekARGBSigs[j].offsetFoundAt)) {
		u32 *GXPeekARGB = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXPeekARGB) {
			memset(data + i, 0, GXPeekARGBSigs[j].Length * sizeof(u32));
			memcpy(data + i, GXPeekARGBSigs[j].Patch, GXPeekARGBSigs[j].PatchLength * sizeof(u32));
			
			print_debug("Found:[%s$%i] @ %08X\n", GXPeekARGBSigs[j].Name, j, GXPeekARGB);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(GXPokeARGBSigs) / sizeof(FuncPattern); j++)
	if ((i = GXPokeARGBSigs[j].offsetFoundAt)) {
		u32 *GXPokeARGB = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXPokeARGB) {
			memset(data + i, 0, GXPokeARGBSigs[j].Length * sizeof(u32));
			memcpy(data + i, GXPokeARGBSigs[j].Patch, GXPokeARGBSigs[j].PatchLength * sizeof(u32));
			
			print_debug("Found:[%s$%i] @ %08X\n", GXPokeARGBSigs[j].Name, j, GXPokeARGB);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(GXPeekZSigs) / sizeof(FuncPattern); j++)
	if ((i = GXPeekZSigs[j].offsetFoundAt)) {
		u32 *GXPeekZ = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXPeekZ) {
			memset(data + i, 0, GXPeekZSigs[j].Length * sizeof(u32));
			memcpy(data + i, GXPeekZSigs[j].Patch, GXPeekZSigs[j].PatchLength * sizeof(u32));
			
			data[i + 1] = 0x3CC00800;	// lis		r6, 0x0800
			
			print_debug("Found:[%s$%i] @ %08X\n", GXPeekZSigs[j].Name, j, GXPeekZ);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(GXPokeZSigs) / sizeof(FuncPattern); j++)
	if ((i = GXPokeZSigs[j].offsetFoundAt)) {
		u32 *GXPokeZ = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXPokeZ) {
			memset(data + i, 0, GXPokeZSigs[j].Length * sizeof(u32));
			memcpy(data + i, GXPokeZSigs[j].Patch, GXPokeZSigs[j].PatchLength * sizeof(u32));
			
			print_debug("Found:[%s$%i] @ %08X\n", GXPokeZSigs[j].Name, j, GXPokeZ);
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
			print_debug("Found:[%s$%i] @ %08X\n", __VMBASESetupExceptionHandlersSigs[j].Name, j, __VMBASESetupExceptionHandlers);
			patched++;
		}
	}
	
	if ((i = __VMBASEISIExceptionHandlerSig.offsetFoundAt)) {
		u32 *__VMBASEISIExceptionHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__VMBASEISIExceptionHandler) {
			data[i +  5] = 0x7CBA02A6;	// mfsrr0	r5
			data[i + 10] = 0x7CBA02A6;	// mfsrr0	r5
			
			print_debug("Found:[%s] @ %08X\n", __VMBASEISIExceptionHandlerSig.Name, __VMBASEISIExceptionHandler);
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

void Patch_Video(u32 *data, u32 length, int dataType)
{
	int i, j, k;
	u32 address;
	FuncPattern LCStoreDataSigs[2] = {
		{ 64, 17, 3, 4, 7, 4, NULL, 0, "LCStoreDataD" },
		{ 43,  9, 6, 2, 5, 5, NULL, 0, "LCStoreData" }
	};
	FuncPattern LCQueueWaitSigs[2] = {
		{ 6, 1, 0, 0, 0, 1, NULL, 0, "LCQueueWait" },
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "LCQueueWait" }
	};
	FuncPattern OSSetCurrentContextSig = 
		{ 23, 4, 4, 0, 0, 5, NULL, 0, "OSSetCurrentContext" };
	FuncPattern OSDisableInterruptsSig = 
		{ 5, 0, 0, 0, 0, 2, NULL, 0, "OSDisableInterrupts" };
	FuncPattern OSRestoreInterruptsSig = 
		{ 9, 0, 0, 0, 2, 2, NULL, 0, "OSRestoreInterrupts" };
	FuncPattern __OSLockSramSigs[3] = {
		{  9, 3, 2, 1, 0, 2, NULL, 0, "__OSLockSramD" },
		{ 23, 7, 5, 2, 2, 2, NULL, 0, "__OSLockSram" },
		{ 20, 7, 4, 2, 2, 3, NULL, 0, "__OSLockSram" }	// SN Systems ProDG
	};
	FuncPattern UnlockSramSigs[4] = {
		{  74, 20, 7, 3,  6,  8, NULL, 0, "UnlockSramD" },
		{  88, 22, 7, 3,  7,  8, NULL, 0, "UnlockSramD" },
		{ 194, 39, 7, 9,  9, 38, NULL, 0, "UnlockSram" },
		{ 207, 42, 7, 9, 10, 38, NULL, 0, "UnlockSram" }
	};
	FuncPattern __OSUnlockSramSigs[3] = {
		{  11,  4,  3, 1, 0,  2, NULL, 0, "__OSUnlockSramD" },
		{   9,  3,  2, 1, 0,  2, NULL, 0, "__OSUnlockSram" },
		{ 173, 53, 11, 9, 9, 25, NULL, 0, "__OSUnlockSram" }	// SN Systems ProDG
	};
	FuncPattern OSGetProgressiveModeSigs[4] = {
		{ 16,  3, 2, 2, 0, 3, NULL, 0, "OSGetProgressiveModeD" },
		{ 32,  9, 5, 3, 4, 2, NULL, 0, "OSGetProgressiveMode" },
		{ 28,  9, 5, 3, 2, 2, NULL, 0, "OSGetProgressiveMode" },
		{ 31, 11, 6, 3, 2, 2, NULL, 0, "OSGetProgressiveMode" }	// SN Systems ProDG
	};
	FuncPattern OSSetProgressiveModeSigs[3] = {
		{  39,  8,  2,  4,  3,  6, NULL, 0, "OSSetProgressiveModeD" },
		{  41, 12,  6,  4,  3,  4, NULL, 0, "OSSetProgressiveMode" },
		{ 192, 54, 10, 14, 11, 27, NULL, 0, "OSSetProgressiveMode" }	// SN Systems ProDG
	};
	FuncPattern OSGetEuRgb60ModeSigs[3] = {
		{ 16,  3, 2, 2, 0, 3, NULL, 0, "OSGetEuRgb60ModeD" },
		{ 28,  9, 5, 3, 2, 2, NULL, 0, "OSGetEuRgb60Mode" },
		{ 31, 11, 6, 3, 2, 2, NULL, 0, "OSGetEuRgb60Mode" }	// SN Systems ProDG
	};
	FuncPattern OSSetEuRgb60ModeSigs[3] = {
		{  39,  8,  2,  4,  3,  6, NULL, 0, "OSSetEuRgb60ModeD" },
		{  41, 12,  6,  4,  3,  4, NULL, 0, "OSSetEuRgb60Mode" },
		{ 194, 54, 10, 14, 11, 27, NULL, 0, "OSSetEuRgb60Mode" }	// SN Systems ProDG
	};
	FuncPattern OSSleepThreadSigs[2] = {
		{ 93, 32, 14, 9, 8, 8, NULL, 0, "OSSleepThreadD" },
		{ 59, 17, 16, 3, 7, 5, NULL, 0, "OSSleepThread" }
	};
	FuncPattern VISetRegsSigs[3] = {
		{ 54, 21, 6, 3, 3, 12, NULL, 0, "VISetRegsD" },
		{ 58, 21, 7, 3, 3, 12, NULL, 0, "VISetRegsD" },
		{ 60, 22, 8, 3, 3, 12, NULL, 0, "VISetRegsD" }
	};
	FuncPattern __VIRetraceHandlerSigs[10] = {
		{ 120, 41,  7, 10, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 121, 41,  7, 11, 13,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 138, 48,  7, 15, 14,  6, NULL, 0, "__VIRetraceHandlerD" },
		{ 132, 39,  7, 10, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 137, 42,  8, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 138, 42,  9, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 140, 43, 10, 11, 15, 13, NULL, 0, "__VIRetraceHandler" },
		{ 147, 46, 13, 10, 15, 15, NULL, 0, "__VIRetraceHandler" },	// SN Systems ProDG
		{ 157, 50, 10, 15, 16, 13, NULL, 0, "__VIRetraceHandler" },
		{ 164, 53, 13, 14, 16, 15, NULL, 0, "__VIRetraceHandler" }	// SN Systems ProDG
	};
	FuncPattern getTimingSigs[11] = {
		{  30,  12,  2,  0,  7,  2,           NULL,                    0, "getTimingD" },
		{  40,  16,  2,  0, 12,  2, getTimingPatch, getTimingPatchLength, "getTimingD" },
		{  44,  20,  2,  0, 14,  2,           NULL,                    0, "getTimingD" },
		{  46,  21,  2,  0, 15,  2,           NULL,                    0, "getTimingD" },
		{  26,  11,  0,  0,  0,  3,           NULL,                    0, "getTiming" },
		{  30,  13,  0,  0,  0,  3,           NULL,                    0, "getTiming" },
		{  36,  15,  0,  0,  0,  4, getTimingPatch, getTimingPatchLength, "getTiming" },
		{  40,  19,  0,  0,  0,  2,           NULL,                    0, "getTiming" },
		{ 559, 112, 44, 14, 53, 48,           NULL,                    0, "getTiming" },	// SN Systems ProDG
		{ 560, 112, 44, 14, 53, 48,           NULL,                    0, "getTiming" },	// SN Systems ProDG
		{  42,  20,  0,  0,  0,  2,           NULL,                    0, "getTiming" }
	};
	FuncPattern __VIInitSigs[9] = {
		{ 159, 44, 5, 4,  4, 16, NULL, 0, "__VIInitD" },
		{ 161, 44, 5, 4,  5, 16, NULL, 0, "__VIInitD" },
		{ 163, 44, 5, 4,  6, 16, NULL, 0, "__VIInitD" },
		{ 168, 44, 5, 4,  8, 15, NULL, 0, "__VIInitD" },
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
	FuncPattern VIInitSigs[9] = {
		{ 208, 60, 18,  8,  1, 24, NULL, 0, "VIInitD" },
		{ 218, 64, 21,  8,  1, 22, NULL, 0, "VIInitD" },
		{ 235, 71, 23, 10,  1, 22, NULL, 0, "VIInitD" },
		{ 269, 37, 18,  7, 18, 57, NULL, 0, "VIInit" },
		{ 270, 37, 19,  7, 18, 57, NULL, 0, "VIInit" },
		{ 283, 40, 24,  7, 18, 49, NULL, 0, "VIInit" },
		{ 286, 41, 25,  7, 18, 49, NULL, 0, "VIInit" },
		{ 300, 48, 27,  8, 17, 48, NULL, 0, "VIInit" },
		{ 335, 85, 23,  9, 17, 42, NULL, 0, "VIInit" }	// SN Systems ProDG
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
	FuncPattern setVerticalRegsSigs[5] = {
		{ 118, 17, 11, 0, 4, 23, NULL, 0, "setVerticalRegsD" },
		{ 118, 17, 11, 0, 4, 23, NULL, 0, "setVerticalRegsD" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs" },
		{ 104, 22, 14, 0, 4, 25, NULL, 0, "setVerticalRegs" },
		{ 114, 19, 13, 0, 4, 25, NULL, 0, "setVerticalRegs" }	// SN Systems ProDG
	};
	FuncPattern VIConfigureSigs[14] = {
		{ 280,  74, 15, 20, 21, 20, NULL, 0, "VIConfigureD" },
		{ 314,  86, 15, 21, 27, 20, NULL, 0, "VIConfigureD" },
		{ 357,  95, 15, 22, 45, 20, NULL, 0, "VIConfigureD" },
		{ 338,  86, 15, 22, 40, 20, NULL, 0, "VIConfigureD" },
		{ 428,  90, 43,  6, 32, 60, NULL, 0, "VIConfigure" },
		{ 420,  87, 41,  6, 31, 60, NULL, 0, "VIConfigure" },
		{ 423,  88, 41,  6, 31, 61, NULL, 0, "VIConfigure" },
		{ 464, 100, 43, 13, 34, 61, NULL, 0, "VIConfigure" },
		{ 462,  99, 43, 12, 34, 61, NULL, 0, "VIConfigure" },
		{ 487, 105, 44, 12, 38, 63, NULL, 0, "VIConfigure" },
		{ 522, 111, 44, 13, 53, 64, NULL, 0, "VIConfigure" },
		{ 559, 112, 44, 14, 53, 48, NULL, 0, "VIConfigure" },	// SN Systems ProDG
		{ 560, 112, 44, 14, 53, 48, NULL, 0, "VIConfigure" },	// SN Systems ProDG
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
	FuncPattern GetCurrentDisplayPositionSigs[2] = {
		{ 19, 4, 3, 0, 0, 1, NULL, 0, "GetCurrentDisplayPositionD" },
		{ 15, 3, 2, 0, 0, 1, NULL, 0, "GetCurrentDisplayPosition" }
	};
	FuncPattern getCurrentHalfLineSigs[3] = {
		{ 26, 9, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD" },
		{ 24, 7, 1, 0, 0, 3, NULL, 0, "getCurrentHalfLineD" },
		{ 19, 9, 2, 1, 0, 4, NULL, 0, "getCurrentHalfLineD" }
	};
	FuncPattern getCurrentFieldEvenOddSigs[6] = {
		{ 33,  7, 2, 3, 4, 5, NULL, 0, "getCurrentFieldEvenOddD" },
		{ 15,  5, 2, 1, 2, 3, NULL, 0, "getCurrentFieldEvenOddD" },
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
	FuncPattern __GXInitGXSigs[11] = {
		{ 1130, 567, 66, 133, 46, 46, NULL, 0, "__GXInitGXD" },
		{  544, 319, 33, 109, 18,  5, NULL, 0, "__GXInitGXD" },
		{  581, 337, 31, 120, 20,  5, NULL, 0, "__GXInitGXD" },
		{  976, 454, 81, 119, 43, 36, NULL, 0, "__GXInitGX" },
		{  530, 307, 35, 107, 18, 10, NULL, 0, "__GXInitGX" },
		{  545, 310, 35, 108, 24, 11, NULL, 0, "__GXInitGX" },
		{  561, 313, 36, 110, 28, 11, NULL, 0, "__GXInitGX" },
		{  546, 293, 37, 110,  7,  9, NULL, 0, "__GXInitGX" },	// SN Systems ProDG
		{  549, 289, 38, 110,  7,  9, NULL, 0, "__GXInitGX" },	// SN Systems ProDG
		{  590, 333, 34, 119, 28, 11, NULL, 0, "__GXInitGX" },
		{  576, 331, 34, 119, 15, 12, NULL, 0, "__GXInitGX" }	// SN Systems ProDG
	};
	FuncPattern GXAdjustForOverscanSigs[5] = {
		{ 57,  6,  4, 0, 3, 11, NULL, 0, "GXAdjustForOverscanD" },
		{ 67,  6,  4, 0, 3, 14, NULL, 0, "GXAdjustForOverscanD" },
		{ 72, 17, 15, 0, 3,  5, NULL, 0, "GXAdjustForOverscan" },
		{ 63, 11,  8, 1, 2, 10, NULL, 0, "GXAdjustForOverscan" },	// SN Systems ProDG
		{ 81, 17, 15, 0, 3,  7, NULL, 0, "GXAdjustForOverscan" }
	};
	FuncPattern GXSetDispCopySrcSigs[7] = {
		{ 104, 44, 10, 5, 5, 6, NULL, 0, "GXSetDispCopySrcD" },
		{  97, 46, 10, 5, 5, 2, NULL, 0, "GXSetDispCopySrcD" },
		{  48, 19,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc" },
		{  36,  9,  8, 0, 0, 4, NULL, 0, "GXSetDispCopySrc" },
		{  14,  2,  2, 0, 0, 2, NULL, 0, "GXSetDispCopySrc" },	// SN Systems ProDG
		{  31, 11,  8, 0, 0, 0, NULL, 0, "GXSetDispCopySrc" },
		{  31, 11,  8, 0, 0, 0, NULL, 0, "GXSetDispCopySrc" }	// SN Systems ProDG
	};
	FuncPattern GXSetDispCopyYScaleSigs[9] = {
		{ 100, 33, 8, 8, 4, 7, NULL, 0, "GXSetDispCopyYScaleD" },
		{  85, 32, 4, 6, 4, 7, NULL, 0, "GXSetDispCopyYScaleD" },
		{  82, 33, 4, 6, 4, 5, NULL, 0, "GXSetDispCopyYScaleD" },
		{  47, 15, 8, 2, 0, 4, NULL, 0, "GXSetDispCopyYScale" },
		{  53, 17, 4, 1, 5, 8, NULL, 0, "GXSetDispCopyYScale" },
		{  50, 14, 4, 1, 5, 8, NULL, 0, "GXSetDispCopyYScale" },
		{  44,  8, 4, 1, 2, 3, NULL, 0, "GXSetDispCopyYScale" },	// SN Systems ProDG
		{  51, 16, 4, 1, 5, 7, NULL, 0, "GXSetDispCopyYScale" },
		{  52, 17, 4, 1, 5, 7, NULL, 0, "GXSetDispCopyYScale" }		// SN Systems ProDG
	};
	FuncPattern GXSetCopyFilterSigs[7] = {
		{ 567, 183, 44, 32, 36, 38, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilterD" },
		{ 514, 196, 44, 32, 36,  7, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilterD" },
		{ 138,  15,  7,  0,  4,  5, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilter" },
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilter" },	// SN Systems ProDG
		{ 163,  19, 23,  0,  3, 14, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilter" },	// SN Systems ProDG
		{ 130,  25,  7,  0,  4,  0, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilter" },
		{ 124,  19,  6,  0,  4,  0, GXSetCopyFilterPatch, GXSetCopyFilterPatchLength, "GXSetCopyFilter" }	// SN Systems ProDG
	};
	FuncPattern GXSetDispCopyGammaSigs[5] = {
		{ 34, 12, 3, 2, 2, 3, NULL, 0, "GXSetDispCopyGammaD" },
		{ 32, 12, 3, 2, 2, 2, NULL, 0, "GXSetDispCopyGammaD" },
		{  7,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma" },
		{  6,  1, 1, 0, 0, 1, NULL, 0, "GXSetDispCopyGamma" },	// SN Systems ProDG
		{  5,  2, 1, 0, 0, 0, NULL, 0, "GXSetDispCopyGamma" }
	};
	FuncPattern __GXVerifCopySig = 
		{ 149, 62, 3, 14, 14, 3, NULL, 0, "__GXVerifCopyD" };
	FuncPattern GXCopyDispSigs[7] = {
		{ 149,  62,  3, 14, 14, 3, NULL, 0, "GXCopyDispD" },
		{ 240, 140, 16,  4,  6, 9, NULL, 0, "GXCopyDispD" },
		{  92,  34, 14,  0,  3, 1, NULL, 0, "GXCopyDisp" },
		{  87,  29, 14,  0,  3, 1, NULL, 0, "GXCopyDisp" },
		{  69,  15, 12,  0,  1, 1, NULL, 0, "GXCopyDisp" },	// SN Systems ProDG
		{  90,  35, 14,  0,  3, 0, NULL, 0, "GXCopyDisp" },
		{  89,  34, 14,  0,  3, 0, NULL, 0, "GXCopyDisp" }	// SN Systems ProDG
	};
	FuncPattern GXSetBlendModeSigs[7] = {
		{ 154, 66, 10, 7, 9, 17, GXSetBlendModePatch1, GXSetBlendModePatch1Length, "GXSetBlendModeD" },
		{ 105, 38,  4, 7, 8, 10, GXSetBlendModePatch1, GXSetBlendModePatch1Length, "GXSetBlendModeD" },
		{  65, 20,  8, 0, 2,  6, GXSetBlendModePatch1, GXSetBlendModePatch1Length, "GXSetBlendMode" },
		{  21,  6,  2, 0, 0,  2, GXSetBlendModePatch2, GXSetBlendModePatch2Length, "GXSetBlendMode" },
		{  23,  5,  2, 0, 0,  2,                 NULL,                          0, "GXSetBlendMode" },	// SN Systems ProDG
		{  36,  2,  2, 0, 0,  6, GXSetBlendModePatch3, GXSetBlendModePatch3Length, "GXSetBlendMode" },	// SN Systems ProDG
		{  38,  2,  2, 0, 0,  8, GXSetBlendModePatch3, GXSetBlendModePatch3Length, "GXSetBlendMode" }	// SN Systems ProDG
	};
	FuncPattern __GXSetViewportSigs[2] = {
		{ 163, 75, 15, 2, 12, 14, NULL, 0, "__GXSetViewportD" },
		{  36, 15,  7, 0,  0,  0, NULL, 0, "__GXSetViewport" }
	};
	FuncPattern GXSetViewportJitterSigs[7] = {
		{ 193, 76, 22, 4, 15, 22, NULL, 0, "GXSetViewportJitterD" },
		{  52, 22, 14, 2,  1,  3, NULL, 0, "GXSetViewportJitterD" },
		{  71, 20, 15, 1,  1,  3, NULL, 0, "GXSetViewportJitter" },
		{  65, 14, 15, 1,  1,  3, NULL, 0, "GXSetViewportJitter" },
		{  31,  6, 10, 1,  0,  4, NULL, 0, "GXSetViewportJitter" },	// SN Systems ProDG
		{  22,  6,  8, 1,  0,  2, NULL, 0, "GXSetViewportJitter" },
		{  48, 17, 13, 0,  0,  0, NULL, 0, "GXSetViewportJitter" }	// SN Systems ProDG
	};
	FuncPattern GXSetViewportSigs[7] = {
		{ 21,  9,  8, 1, 0, 2, NULL, 0, "GXSetViewportD" },
		{ 21,  9,  8, 1, 0, 2, NULL, 0, "GXSetViewportD" },
		{  9,  3,  2, 1, 0, 2, NULL, 0, "GXSetViewport" },
		{  9,  3,  2, 1, 0, 2, NULL, 0, "GXSetViewport" },
		{  2,  1,  0, 0, 1, 0, NULL, 0, "GXSetViewport" },	// SN Systems ProDG
		{ 18,  5,  8, 1, 0, 2, NULL, 0, "GXSetViewport" },
		{ 44, 16, 13, 0, 0, 0, NULL, 0, "GXSetViewport" }	// SN Systems ProDG
	};
	FuncPattern __THPDecompressiMCURow512x448Sigs[2] = {
		{ 1666, 208, 19, 10, 22, 65, NULL, 0, "__THPDecompressiMCURow512x448" },
		{ 1698, 210, 17, 10, 22, 77, NULL, 0, "__THPDecompressiMCURow512x448" }
	};
	FuncPattern __THPDecompressiMCURow640x480Sigs[2] = {
		{ 1667, 208, 19, 10, 22, 65, NULL, 0, "__THPDecompressiMCURow640x480" },
		{ 1699, 210, 17, 10, 22, 77, NULL, 0, "__THPDecompressiMCURow640x480" }
	};
	FuncPattern __THPDecompressiMCURowNxNSigs[2] = {
		{ 1664, 196, 15, 10, 22, 68, NULL, 0, "__THPDecompressiMCURowNxN" },
		{ 1707, 202, 19, 10, 22, 80, NULL, 0, "__THPDecompressiMCURowNxN" }
	};
	FuncPattern __THPHuffDecodeDCTCompYSigs[2] = {
		{ 415, 74, 31, 0, 45, 52, NULL, 0, "__THPHuffDecodeDCTCompY" },
		{ 415, 74, 31, 0, 45, 52, NULL, 0, "__THPHuffDecodeDCTCompY" }
	};
	FuncPattern __THPHuffDecodeDCTCompUSigs[2] = {
		{ 426, 73, 43, 0, 45, 67, NULL, 0, "__THPHuffDecodeDCTCompU" },
		{ 426, 74, 43, 0, 45, 69, NULL, 0, "__THPHuffDecodeDCTCompU" }
	};
	FuncPattern __THPHuffDecodeDCTCompVSigs[2] = {
		{ 426, 73, 43, 0, 45, 67, NULL, 0, "__THPHuffDecodeDCTCompV" },
		{ 426, 74, 43, 0, 45, 69, NULL, 0, "__THPHuffDecodeDCTCompV" }
	};
	FuncPattern IDct64_GCSig = 
		{ 435, 6, 1, 5, 1, 5, NULL, 0, "IDct64_GC" };
	FuncPattern IDct10_GCSig = 
		{ 221, 6, 1, 3, 1, 5, NULL, 0, "IDct10_GC" };
	_SDA2_BASE_ = _SDA_BASE_ = 0;
	
	if (in_range(swissSettings.gameVMode, 1, 7)) {
		for (j = 0; j < sizeof(GXAdjustForOverscanSigs) / sizeof(FuncPattern); j++) {
			GXAdjustForOverscanSigs[j].Patch       = GXAdjustForOverscanPatch;
			GXAdjustForOverscanSigs[j].PatchLength = GXAdjustForOverscanPatchLength;
		}
	}
	
	if (in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) && swissSettings.rt4kOptim) {
		for (j = 0; j < sizeof(GXSetDispCopyYScaleSigs) / sizeof(FuncPattern); j++) {
			if (j == 6) {
				GXSetDispCopyYScaleSigs[j].Patch       = GXSetDispCopyYScaleStub2;
				GXSetDispCopyYScaleSigs[j].PatchLength = GXSetDispCopyYScaleStub2Length;
			} else {
				GXSetDispCopyYScaleSigs[j].Patch       = GXSetDispCopyYScaleStub1;
				GXSetDispCopyYScaleSigs[j].PatchLength = GXSetDispCopyYScaleStub1Length;
			}
		}
	} else if (in_range(swissSettings.gameVMode, 1, 7)) {
		for (j = 0; j < sizeof(GXSetDispCopyYScaleSigs) / sizeof(FuncPattern); j++) {
			if (j == 6) {
				GXSetDispCopyYScaleSigs[j].Patch       = GXSetDispCopyYScalePatch2;
				GXSetDispCopyYScaleSigs[j].PatchLength = GXSetDispCopyYScalePatch2Length;
			} else {
				GXSetDispCopyYScaleSigs[j].Patch       = GXSetDispCopyYScalePatch1;
				GXSetDispCopyYScaleSigs[j].PatchLength = GXSetDispCopyYScalePatch1Length;
			}
		}
	}
	
	if (swissSettings.forceVJitter == 3) {
		for (j = 0; j < sizeof(__GXSetViewportSigs) / sizeof(FuncPattern); j++) {
			__GXSetViewportSigs[j].Patch       = __GXSetViewportTAAPatch;
			__GXSetViewportSigs[j].PatchLength = __GXSetViewportTAAPatchLength;
		}
		
		for (j = 0; j < sizeof(GXSetViewportJitterSigs) / sizeof(FuncPattern); j++) {
			switch (j) {
				case 0:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterTAAPatch1;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterTAAPatch1Length;
					break;
				case 1:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch2;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch2Length;
					break;
				case 2:
				case 3:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterTAAPatch1;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterTAAPatch1Length;
					break;
				case 5:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch2;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch2Length;
					break;
				case 6:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterTAAPatch3;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterTAAPatch3Length;
					break;
			}
		}
		
		for (j = 0; j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern); j++) {
			switch (j) {
				case 0:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch1;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch1Length;
					break;
				case 1:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch2;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch2Length;
					break;
				case 2:
				case 3:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch1;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch1Length;
					break;
				case 5:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch2;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch2Length;
					break;
				case 6:
					GXSetViewportSigs[j].Patch       = GXSetViewportTAAPatch3;
					GXSetViewportSigs[j].PatchLength = GXSetViewportTAAPatch3Length;
					break;
			}
		}
	} else if (swissSettings.forceVJitter == 1 || (swissSettings.forceVJitter != 2 && swissSettings.fixPixelCenter)) {
		for (j = 0; j < sizeof(__GXSetViewportSigs) / sizeof(FuncPattern); j++) {
			__GXSetViewportSigs[j].Patch       = __GXSetViewportPatch;
			__GXSetViewportSigs[j].PatchLength = __GXSetViewportPatchLength;
		}
		
		for (j = 0; j < sizeof(GXSetViewportJitterSigs) / sizeof(FuncPattern); j++) {
			switch (j) {
				case 0:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch1;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch1Length;
					break;
				case 1:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch2;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch2Length;
					break;
				case 2:
				case 3:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch1;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch1Length;
					break;
				case 5:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch2;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch2Length;
					break;
				case 6:
					GXSetViewportJitterSigs[j].Patch       = GXSetViewportJitterPatch3;
					GXSetViewportJitterSigs[j].PatchLength = GXSetViewportJitterPatch3Length;
					break;
			}
		}
		
		for (j = 0; j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern); j++) {
			switch (j) {
				case 0:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch1;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch1Length;
					break;
				case 1:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch2;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch2Length;
					break;
				case 2:
				case 3:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch1;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch1Length;
					break;
				case 5:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch2;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch2Length;
					break;
				case 6:
					GXSetViewportSigs[j].Patch       = GXSetViewportPatch3;
					GXSetViewportSigs[j].PatchLength = GXSetViewportPatch3Length;
					break;
			}
		}
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
		if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
			(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
			(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
			(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
			get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
			get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
			break;
		}
	}
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if ((data[i + 0] != 0x7C0802A6 && (data[i + 0] & 0xFFFF0000) != 0x94210000) ||
			(data[i - 1] != 0x4E800020 &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x4E800020) &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x00000000 || data[i - 3] != 0x4E800020) &&
			 data[i - 1] != 0x4C000064 &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
			continue;
		
		FuncPattern fp;
		make_pattern(data, dataType, i, length, &fp);
		
		for (j = 0; j < sizeof(OSGetProgressiveModeSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSGetProgressiveModeSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &__OSLockSramSigs[0]) &&
							(data[i +  6] & 0xFC00FFFF) == 0x88000013 &&
							(data[i +  7] & 0xFC00FFFF) == 0x5400CFFE &&
							findx_pattern(data, dataType, i +  9, length, &__OSUnlockSramSigs[0]))
							OSGetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 11, length, &OSRestoreInterruptsSig) &&
							(data[i + 17] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 18] & 0xFC00FFFF) == 0x54000631 &&
							findx_pattern(data, dataType, i + 25, length, &UnlockSramSigs[2]))
							OSGetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern (data, dataType, i +  6, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 11, length, &OSRestoreInterruptsSig) &&
							(data[i + 17] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 20] & 0xFC00FFFF) == 0x5400CFFE &&
							findx_patterns(data, dataType, i + 21, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL))
							OSGetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 10, length, &OSRestoreInterruptsSig) &&
							(data[i + 17] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 22] & 0xFC00FFFF) == 0x5400CFFE &&
							findx_pattern(data, dataType, i + 24, length, &OSRestoreInterruptsSig))
							OSGetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSSetProgressiveModeSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSSetProgressiveModeSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 15] & 0xFC00FFFF) == 0x54003830 &&
							(data[i + 16] & 0xFC00FFFF) == 0x54000630 &&
							findx_pattern(data, dataType, i + 17, length, &__OSLockSramSigs[0]) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000630 &&
							findx_pattern(data, dataType, i + 24, length, &__OSUnlockSramSigs[0]) &&
							(data[i + 26] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 27] & 0xFC00FFFF) == 0x5400066E &&
							(data[i + 28] & 0xFC00FFFF) == 0x98000013 &&
							(data[i + 29] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 31] & 0xFC00FFFF) == 0x98000013 &&
							findx_pattern(data, dataType, i + 33, length, &__OSUnlockSramSigs[0]))
							OSSetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i +  7] & 0xFC00FFFF) == 0x54003E30 &&
							findx_pattern (data, dataType, i +  8, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 13, length, &OSRestoreInterruptsSig) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000630 &&
							findx_patterns(data, dataType, i + 25, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL) &&
							(data[i + 27] & 0xFC00FFFF) == 0x5400066E &&
							(data[i + 28] & 0xFC00FFFF) == 0x98000013 &&
							(data[i + 31] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 33] & 0xFC00FFFF) == 0x98000013 &&
							findx_patterns(data, dataType, i + 34, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL))
							OSSetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i +  5] & 0xFC00FFFF) == 0x54003E30 &&
							findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  12, length, &OSRestoreInterruptsSig) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000013 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000630 &&
							findx_pattern(data, dataType, i +  28, length, &OSRestoreInterruptsSig) &&
							(data[i + 30] & 0xFC00FFFF) == 0x5400067E &&
							(data[i + 33] & 0xFC00FFFF) == 0x98000013 &&
							(data[i + 37] & 0xFC00FFFF) == 0x98000013 &&
							findx_pattern(data, dataType, i + 185, length, &OSRestoreInterruptsSig))
							OSSetProgressiveModeSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSGetEuRgb60ModeSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSGetEuRgb60ModeSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  4, length, &__OSLockSramSigs[0]) &&
							(data[i +  6] & 0xFC00FFFF) == 0x88000011 &&
							(data[i +  7] & 0xFC00FFFF) == 0x5400D7FE &&
							findx_pattern(data, dataType, i +  9, length, &__OSUnlockSramSigs[0]))
							OSGetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern (data, dataType, i +  6, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 11, length, &OSRestoreInterruptsSig) &&
							(data[i + 17] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 20] & 0xFC00FFFF) == 0x5400D7FE &&
							findx_patterns(data, dataType, i + 21, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL))
							OSGetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  4, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 10, length, &OSRestoreInterruptsSig) &&
							(data[i + 17] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 22] & 0xFC00FFFF) == 0x5400D7FE &&
							findx_pattern(data, dataType, i + 24, length, &OSRestoreInterruptsSig))
							OSGetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(OSSetEuRgb60ModeSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &OSSetEuRgb60ModeSigs[j])) {
				switch (j) {
					case 0:
						if ((data[i + 15] & 0xFC00FFFF) == 0x54003032 &&
							(data[i + 16] & 0xFC00FFFF) == 0x54000672 &&
							findx_pattern(data, dataType, i + 17, length, &__OSLockSramSigs[0]) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000672 &&
							findx_pattern(data, dataType, i + 24, length, &__OSUnlockSramSigs[0]) &&
							(data[i + 26] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 27] & 0xFC00FFFF) == 0x540006B0 &&
							(data[i + 28] & 0xFC00FFFF) == 0x98000011 &&
							(data[i + 29] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 31] & 0xFC00FFFF) == 0x98000011 &&
							findx_pattern(data, dataType, i + 33, length, &__OSUnlockSramSigs[0]))
							OSSetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if ((data[i +  7] & 0xFC00FFFF) == 0x54003672 &&
							findx_pattern (data, dataType, i +  8, length, &OSDisableInterruptsSig) &&
							findx_pattern (data, dataType, i + 13, length, &OSRestoreInterruptsSig) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000672 &&
							findx_patterns(data, dataType, i + 25, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL) &&
							(data[i + 27] & 0xFC00FFFF) == 0x540006B0 &&
							(data[i + 28] & 0xFC00FFFF) == 0x98000011 &&
							(data[i + 31] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 33] & 0xFC00FFFF) == 0x98000011 &&
							findx_patterns(data, dataType, i + 34, length, &UnlockSramSigs[2],
							                                               &UnlockSramSigs[3], NULL))
							OSSetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if ((data[i +  5] & 0xFC00FFFF) == 0x54003672 &&
							findx_pattern(data, dataType, i +   6, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  12, length, &OSRestoreInterruptsSig) &&
							(data[i + 19] & 0xFC00FFFF) == 0x88000011 &&
							(data[i + 20] & 0xFC00FFFF) == 0x54000672 &&
							findx_pattern(data, dataType, i +  28, length, &OSRestoreInterruptsSig) &&
							(data[i + 30] & 0xFC00FFFF) == 0x540006B0 &&
							(data[i + 33] & 0xFC00FFFF) == 0x98000011 &&
							(data[i + 36] & 0xFC00FFFF) == 0x98000011 &&
							findx_pattern(data, dataType, i + 187, length, &OSRestoreInterruptsSig))
							OSSetEuRgb60ModeSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
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
						if (findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  62, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  78, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  91, length, &VISetRegsSigs[2]) &&
							findx_pattern(data, dataType, i + 132, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 126, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 60, length, &getCurrentFieldEvenOddSigs[3]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[3], &VIGetRetraceCountSig);
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  39, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 131, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 60, length, &getCurrentFieldEvenOddSigs[4]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[4], &VIGetRetraceCountSig);
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 132, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 63, length, &getCurrentFieldEvenOddSigs[4]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[4], &VIGetRetraceCountSig);
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  42, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  50, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 134, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 63, length, &getCurrentFieldEvenOddSigs[4]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[4], &VIGetRetraceCountSig);
						break;
					case 7:
						if (findx_pattern(data, dataType, i +  47, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  55, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 139, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 68, length, &getCurrentFieldEvenOddSigs[5]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[5], &VIGetRetraceCountSig);
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  44, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  59, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  67, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 151, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 80, length, &getCurrentFieldEvenOddSigs[4]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[4], &VIGetRetraceCountSig);
						break;
					case 9:
						if (findx_pattern(data, dataType, i +  49, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  64, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i +  72, length, &OSSetCurrentContextSig) &&
							findx_pattern(data, dataType, i + 156, length, &OSSetCurrentContextSig))
							__VIRetraceHandlerSigs[j].offsetFoundAt = i;
						
						if (findx_pattern(data, dataType, i + 85, length, &getCurrentFieldEvenOddSigs[5]))
							find_pattern_before(data, dataType, length, &getCurrentFieldEvenOddSigs[5], &VIGetRetraceCountSig);
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
						if (findx_patterns(data, dataType, i +  22, length, &__VIInitSigs[2],
							                                                &__VIInitSigs[3], NULL) &&
							findx_pattern (data, dataType, i + 129, length, &ImportAdjustingValuesSig) &&
							findx_pattern (data, dataType, i + 177, length, &AdjustPositionSig))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
					case 4:
						if (findx_pattern(data, dataType, i +  16, length, &__VIInitSigs[4]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  19, length, &__VIInitSigs[4]))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_patterns(data, dataType, i +  19, length, &__VIInitSigs[4],
							                                                &__VIInitSigs[5], NULL))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 7:
						if (findx_patterns(data, dataType, i +  25, length, &__VIInitSigs[6],
							                                                &__VIInitSigs[8], NULL))
							VIInitSigs[j].offsetFoundAt = i;
						break;
					case 8:
						if (findx_pattern(data, dataType, i +  18, length, &__VIInitSigs[7]))
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
						if (findx_pattern(data, dataType, i +  65, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 351, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 180, length, &AdjustPositionSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 176, length, &getTimingSigs[2]);
						findx_pattern(data, dataType, i + 338, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 349, length, &setVerticalRegsSigs[1]);
						break;
					case 3:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 332, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 171, length, &AdjustPositionSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 167, length, &getTimingSigs[3]);
						findx_pattern(data, dataType, i + 319, length, &setFbbRegsSigs[0]);
						findx_pattern(data, dataType, i + 330, length, &setVerticalRegsSigs[1]);
						break;
					case 4:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 422, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  77, length, &getTimingSigs[4]);
						findx_pattern(data, dataType, i + 409, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 420, length, &setVerticalRegsSigs[2]);
						break;
					case 5:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 414, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  69, length, &getTimingSigs[4]);
						findx_pattern(data, dataType, i + 401, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 412, length, &setVerticalRegsSigs[2]);
						break;
					case 6:
						if (findx_pattern(data, dataType, i +   7, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 417, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i +  72, length, &getTimingSigs[4]);
						findx_pattern(data, dataType, i + 404, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 415, length, &setVerticalRegsSigs[2]);
						break;
					case 7:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 458, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 104, length, &getTimingSigs[5]);
						findx_pattern(data, dataType, i + 445, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 456, length, &setVerticalRegsSigs[2]);
						break;
					case 8:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 456, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 107, length, &getTimingSigs[5]);
						findx_pattern(data, dataType, i + 443, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 454, length, &setVerticalRegsSigs[2]);
						break;
					case 9:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 481, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 123, length, &getTimingSigs[6]);
						findx_pattern(data, dataType, i + 468, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 479, length, &setVerticalRegsSigs[2]);
						break;
					case 10:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 516, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[7]);
						findx_pattern(data, dataType, i + 503, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 514, length, &setVerticalRegsSigs[3]);
						break;
					case 11:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 552, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = getTimingSigs[8].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 537, length, &setFbbRegsSigs[2]);
						findx_pattern(data, dataType, i + 550, length, &setVerticalRegsSigs[4]);
						break;
					case 12:
						if (findx_pattern(data, dataType, i +   8, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 553, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = getTimingSigs[9].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 538, length, &setFbbRegsSigs[2]);
						findx_pattern(data, dataType, i + 551, length, &setVerticalRegsSigs[4]);
						break;
					case 13:
						if (findx_pattern(data, dataType, i +   9, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 508, length, &OSRestoreInterruptsSig))
							VIConfigureSigs[j].offsetFoundAt = i;
						
						findx_pattern(data, dataType, i + 155, length, &getTimingSigs[10]);
						findx_pattern(data, dataType, i + 495, length, &setFbbRegsSigs[1]);
						findx_pattern(data, dataType, i + 506, length, &setVerticalRegsSigs[3]);
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
							find_pattern_before(data, dataType, length, &getCurrentHalfLineSigs[0], &VIGetRetraceCountSig);
							getCurrentFieldEvenOddSigs[j].offsetFoundAt = i;
						}
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  3, length, &getCurrentHalfLineSigs[1])) {
							find_pattern_before(data, dataType, length, &getCurrentHalfLineSigs[1], &VIGetRetraceCountSig);
							getCurrentFieldEvenOddSigs[j].offsetFoundAt = i;
						}
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  3, length, &getCurrentHalfLineSigs[2]) &&
							find_pattern_before(data, dataType, length, &getCurrentHalfLineSigs[2], &GetCurrentDisplayPositionSigs[0])) {
							find_pattern_before(data, dataType, length, &GetCurrentDisplayPositionSigs[0], &VIGetRetraceCountSig);
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
									findx_pattern(data, dataType, i +  9, length, &GetCurrentDisplayPositionSigs[1])) {
									find_pattern_before(data, dataType, length, &GetCurrentDisplayPositionSigs[1], &VIGetRetraceCountSig);
									VIGetNextFieldSigs[k].offsetFoundAt = i;
								}
								break;
						}
					}
				}
			}
		}
		
		for (j = 0; j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &VIGetDTVStatusSigs[j])) {
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
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[0], &GXAdjustForOverscanSigs[0]);
						
						findx_pattern(data, dataType, i + 1088, length, &GXSetDispCopyYScaleSigs[0]);
						findx_pattern(data, dataType, i + 1095, length, &GXSetCopyFilterSigs[0]);
						
						if (findx_pattern(data, dataType, i + 1097, length, &GXSetDispCopyGammaSigs[0]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[0], &GXCopyDispSigs[0]);
						
						findx_pattern(data, dataType, i + 1033, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i +  727, length, &GXSetViewportSigs[0]);
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 480, length, &GXSetDispCopySrcSigs[0]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[0], &GXAdjustForOverscanSigs[0]);
						
						findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[1]);
						findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[0]);
						
						if (findx_pattern(data, dataType, i + 508, length, &GXSetDispCopyGammaSigs[0]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[0], &GXCopyDispSigs[0]);
						
						findx_pattern(data, dataType, i + 444, length, &GXSetBlendModeSigs[0]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[0]);
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 517, length, &GXSetDispCopySrcSigs[1]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[1], &GXAdjustForOverscanSigs[1]);
						
						findx_pattern(data, dataType, i + 536, length, &GXSetDispCopyYScaleSigs[2]);
						findx_pattern(data, dataType, i + 543, length, &GXSetCopyFilterSigs[1]);
						
						if (findx_pattern(data, dataType, i + 545, length, &GXSetDispCopyGammaSigs[1]) &&
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[1], &__GXVerifCopySig))
							find_pattern_after(data, dataType, length, &__GXVerifCopySig, &GXCopyDispSigs[1]);
						
						findx_pattern(data, dataType, i + 481, length, &GXSetBlendModeSigs[1]);
						findx_pattern(data, dataType, i + 210, length, &GXSetViewportSigs[1]);
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 917, length, &GXSetDispCopySrcSigs[2]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[2], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 934, length, &GXSetDispCopyYScaleSigs[3]);
						findx_pattern(data, dataType, i + 941, length, &GXSetCopyFilterSigs[2]);
						
						if (findx_pattern(data, dataType, i + 943, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[2]);
						
						findx_pattern(data, dataType, i + 881, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 553, length, &GXSetViewportSigs[2]);
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 467, length, &GXSetDispCopySrcSigs[2]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[2], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 484, length, &GXSetDispCopyYScaleSigs[3]);
						findx_pattern(data, dataType, i + 491, length, &GXSetCopyFilterSigs[2]);
						
						if (findx_pattern(data, dataType, i + 493, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[2]);
						
						findx_pattern(data, dataType, i + 431, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 186, length, &GXSetViewportSigs[2]);
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 482, length, &GXSetDispCopySrcSigs[2]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[2], &GXAdjustForOverscanSigs[2]);
						
						if (data[i + __GXInitGXSigs[j].Length - 2] == 0x7C0803A6)
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[4]);
						else
							findx_pattern(data, dataType, i + 499, length, &GXSetDispCopyYScaleSigs[3]);
						
						findx_pattern(data, dataType, i + 506, length, &GXSetCopyFilterSigs[2]);
						
						if (findx_pattern(data, dataType, i + 508, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[2]);
						
						findx_pattern(data, dataType, i + 446, length, &GXSetBlendModeSigs[2]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[2]);
						break;
					case 6:
						if (findx_pattern(data, dataType, i + 497, length, &GXSetDispCopySrcSigs[3]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[3], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 514, length, &GXSetDispCopyYScaleSigs[5]);
						findx_pattern(data, dataType, i + 521, length, &GXSetCopyFilterSigs[2]);
						
						if (findx_pattern(data, dataType, i + 523, length, &GXSetDispCopyGammaSigs[2]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[2], &GXCopyDispSigs[3]);
						
						findx_pattern(data, dataType, i + 461, length, &GXSetBlendModeSigs[3]);
						findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[3]);
						break;
					case 7:
						if (findx_pattern(data, dataType, i + 473, length, &GXSetDispCopySrcSigs[4]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[4], &GXAdjustForOverscanSigs[3]);
						
						findx_pattern(data, dataType, i + 492, length, &GXSetDispCopyYScaleSigs[6]);
						findx_pattern(data, dataType, i + 499, length, &GXSetCopyFilterSigs[3]);
						
						if (findx_pattern(data, dataType, i + 501, length, &GXSetDispCopyGammaSigs[3]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[3], &GXCopyDispSigs[4]);
						
						findx_pattern(data, dataType, i + 433, length, &GXSetBlendModeSigs[5]);
						findx_pattern(data, dataType, i + 202, length, &GXSetViewportSigs[4]);
						break;
					case 8:
						if (findx_pattern(data, dataType, i + 475, length, &GXSetDispCopySrcSigs[4]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[4], &GXAdjustForOverscanSigs[3]);
						
						findx_pattern(data, dataType, i + 494, length, &GXSetDispCopyYScaleSigs[6]);
						findx_pattern(data, dataType, i + 501, length, &GXSetCopyFilterSigs[4]);
						
						if (findx_pattern(data, dataType, i + 503, length, &GXSetDispCopyGammaSigs[3]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[3], &GXCopyDispSigs[4]);
						
						findx_pattern(data, dataType, i + 435, length, &GXSetBlendModeSigs[6]);
						findx_pattern(data, dataType, i + 204, length, &GXSetViewportSigs[4]);
						break;
					case 9:
						if (findx_pattern(data, dataType, i + 526, length, &GXSetDispCopySrcSigs[5]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[5], &GXAdjustForOverscanSigs[4]);
						
						findx_pattern(data, dataType, i + 543, length, &GXSetDispCopyYScaleSigs[7]);
						findx_pattern(data, dataType, i + 550, length, &GXSetCopyFilterSigs[5]);
						
						if (findx_pattern(data, dataType, i + 552, length, &GXSetDispCopyGammaSigs[4]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[4], &GXCopyDispSigs[5]);
						
						findx_pattern(data, dataType, i + 490, length, &GXSetBlendModeSigs[3]);
						
						if (findx_pattern(data, dataType, i + 215, length, &GXSetViewportSigs[5]))
							find_pattern_before(data, dataType, length, &GXSetViewportSigs[5], &GXSetViewportJitterSigs[5]);
						break;
					case 10:
						if (findx_pattern(data, dataType, i + 512, length, &GXSetDispCopySrcSigs[6]))
							find_pattern_before(data, dataType, length, &GXSetDispCopySrcSigs[6], &GXAdjustForOverscanSigs[2]);
						
						findx_pattern(data, dataType, i + 529, length, &GXSetDispCopyYScaleSigs[8]);
						findx_pattern(data, dataType, i + 536, length, &GXSetCopyFilterSigs[6]);
						
						if (findx_pattern(data, dataType, i + 538, length, &GXSetDispCopyGammaSigs[4]))
							find_pattern_after(data, dataType, length, &GXSetDispCopyGammaSigs[4], &GXCopyDispSigs[6]);
						
						findx_pattern(data, dataType, i + 478, length, &GXSetBlendModeSigs[4]);
						
						if (findx_pattern(data, dataType, i + 209, length, &GXSetViewportSigs[6]))
							find_pattern_before(data, dataType, length, &GXSetViewportSigs[6], &GXSetViewportJitterSigs[6]);
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__THPDecompressiMCURow512x448Sigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__THPDecompressiMCURow512x448Sigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +   16, length, &LCQueueWaitSigs[0]) &&
							findx_pattern(data, dataType, i +   28, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   31, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   34, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   37, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   40, length, &__THPHuffDecodeDCTCompUSigs[0]) &&
							findx_pattern(data, dataType, i +   43, length, &__THPHuffDecodeDCTCompVSigs[0]) &&
							findx_pattern(data, dataType, i + 1635, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1639, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1643, length, &LCStoreDataSigs[1]))
							__THPDecompressiMCURow512x448Sigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i +   13, length, &LCQueueWaitSigs[0],
							                                                 &LCQueueWaitSigs[1], NULL) &&
							findx_pattern (data, dataType, i +   23, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   26, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   29, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   32, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   35, length, &__THPHuffDecodeDCTCompUSigs[1]) &&
							findx_pattern (data, dataType, i +   38, length, &__THPHuffDecodeDCTCompVSigs[1]) &&
							findx_patterns(data, dataType, i + 1664, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1669, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1674, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL))
							__THPDecompressiMCURow512x448Sigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__THPDecompressiMCURow640x480Sigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__THPDecompressiMCURow640x480Sigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +   16, length, &LCQueueWaitSigs[0]) &&
							findx_pattern(data, dataType, i +   28, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   31, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   34, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   37, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   40, length, &__THPHuffDecodeDCTCompUSigs[0]) &&
							findx_pattern(data, dataType, i +   43, length, &__THPHuffDecodeDCTCompVSigs[0]) &&
							findx_pattern(data, dataType, i + 1636, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1640, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1644, length, &LCStoreDataSigs[1]))
							__THPDecompressiMCURow640x480Sigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i +   13, length, &LCQueueWaitSigs[0],
							                                                 &LCQueueWaitSigs[1], NULL) &&
							findx_pattern (data, dataType, i +   23, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   26, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   29, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   32, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   35, length, &__THPHuffDecodeDCTCompUSigs[1]) &&
							findx_pattern (data, dataType, i +   38, length, &__THPHuffDecodeDCTCompVSigs[1]) &&
							findx_patterns(data, dataType, i + 1665, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1670, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1675, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL))
							__THPDecompressiMCURow640x480Sigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(__THPDecompressiMCURowNxNSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &__THPDecompressiMCURowNxNSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +   14, length, &LCQueueWaitSigs[0]) &&
							findx_pattern(data, dataType, i +   27, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   30, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   33, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   36, length, &__THPHuffDecodeDCTCompYSigs[0]) &&
							findx_pattern(data, dataType, i +   39, length, &__THPHuffDecodeDCTCompUSigs[0]) &&
							findx_pattern(data, dataType, i +   42, length, &__THPHuffDecodeDCTCompVSigs[0]) &&
							findx_pattern(data, dataType, i + 1634, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1638, length, &LCStoreDataSigs[1]) &&
							findx_pattern(data, dataType, i + 1642, length, &LCStoreDataSigs[1]))
							__THPDecompressiMCURowNxNSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i +   17, length, &LCQueueWaitSigs[0],
							                                                 &LCQueueWaitSigs[1], NULL) &&
							findx_pattern (data, dataType, i +   28, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   31, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   34, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   37, length, &__THPHuffDecodeDCTCompYSigs[1]) &&
							findx_pattern (data, dataType, i +   40, length, &__THPHuffDecodeDCTCompUSigs[1]) &&
							findx_pattern (data, dataType, i +   43, length, &__THPHuffDecodeDCTCompVSigs[1]) &&
							findx_patterns(data, dataType, i + 1669, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1674, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL) &&
							findx_patterns(data, dataType, i + 1679, length, &LCStoreDataSigs[0],
							                                                 &LCStoreDataSigs[1], NULL))
							__THPDecompressiMCURowNxNSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		if (compare_pattern(&fp, &IDct64_GCSig)) {
			if (branchResolve(data, dataType, i +  94) == i +  96 &&
				branchResolve(data, dataType, i +  95) == i + 133 &&
				branchResolve(data, dataType, i + 205) == i +  96 &&
				branchResolve(data, dataType, i + 278) == i +  96 &&
				branchResolve(data, dataType, i + 351) == i +  96 &&
				branchResolve(data, dataType, i + 378) == i +  96)
				IDct64_GCSig.offsetFoundAt = i;
		}
		
		if (compare_pattern(&fp, &IDct10_GCSig)) {
			if (branchResolve(data, dataType, i +  66) == i + 68 &&
				branchResolve(data, dataType, i +  67) == i + 97 &&
				branchResolve(data, dataType, i + 141) == i + 68 &&
				branchResolve(data, dataType, i + 164) == i + 68)
				IDct10_GCSig.offsetFoundAt = i;
		}
		i += fp.Length - 1;
	}
	
	for (j = 0; j < sizeof(OSGetProgressiveModeSigs) / sizeof(FuncPattern); j++)
	if ((i = OSGetProgressiveModeSigs[j].offsetFoundAt)) {
		u32 *OSGetProgressiveMode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSGetProgressiveMode) {
			if (swissSettings.gameVMode < 0) {
				memset(data + i, 0, OSGetProgressiveModeSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000;	// li		r3, 0
				data[i + 1] = 0x4E800020;	// blr
			} else if (swissSettings.gameVMode > 0) {
				memset(data + i, 0, OSGetProgressiveModeSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000 | (swissSettings.sramProgressive & 0xFFFF);
				data[i + 1] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", OSGetProgressiveModeSigs[j].Name, j, OSGetProgressiveMode);
		}
	}
	
	for (j = 0; j < sizeof(OSSetProgressiveModeSigs) / sizeof(FuncPattern); j++)
	if ((i = OSSetProgressiveModeSigs[j].offsetFoundAt)) {
		u32 *OSSetProgressiveMode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSSetProgressiveMode) {
			if (swissSettings.gameVMode != 0) {
				memset(data + i, 0, OSSetProgressiveModeSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", OSSetProgressiveModeSigs[j].Name, j, OSSetProgressiveMode);
		}
	}
	
	for (j = 0; j < sizeof(OSGetEuRgb60ModeSigs) / sizeof(FuncPattern); j++)
	if ((i = OSGetEuRgb60ModeSigs[j].offsetFoundAt)) {
		u32 *OSGetEuRgb60Mode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSGetEuRgb60Mode) {
			if (in_range(swissSettings.gameVMode, 1, 7)) {
				memset(data + i, 0, OSGetEuRgb60ModeSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000 | (swissSettings.sram60Hz & 0xFFFF);
				data[i + 1] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", OSGetEuRgb60ModeSigs[j].Name, j, OSGetEuRgb60Mode);
		}
	}
	
	for (j = 0; j < sizeof(OSSetEuRgb60ModeSigs) / sizeof(FuncPattern); j++)
	if ((i = OSSetEuRgb60ModeSigs[j].offsetFoundAt)) {
		u32 *OSSetEuRgb60Mode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (OSSetEuRgb60Mode) {
			if (in_range(swissSettings.gameVMode, 1, 7)) {
				memset(data + i, 0, OSSetEuRgb60ModeSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", OSSetEuRgb60ModeSigs[j].Name, j, OSSetEuRgb60Mode);
		}
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
				case 2:
					data[i +  7] = 0x4180000C;	// blt		+3
					data[i +  8] = 0x5400083C;	// slwi		r0, r0, 1
					data[i +  9] = 0x7C601850;	// sub		r3, r3, r0
					data[i + 10] = 0x7C630E70;	// srawi	r3, r3, 1
					break;
				case 3:
					data[i + 39] = 0x4180000C;	// blt		+3
					data[i + 40] = 0x5529083C;	// slwi		r9, r9, 1
					data[i + 41] = 0x7C090050;	// sub		r0, r0, r9
					data[i + 42] = 0x7C030E70;	// srawi	r3, r0, 1
					break;
				case 4:
				case 5:
					data[i + 21] = 0x4180000C;	// blt		+3
					data[i + 22] = 0x5400083C;	// slwi		r0, r0, 1
					data[i + 23] = 0x7C601850;	// sub		r3, r3, r0
					data[i + 24] = 0x7C630E70;	// srawi	r3, r3, 1
					break;
			}
			if (swissSettings.gameVMode == -2 || in_range(swissSettings.gameVMode, 8, 14)) {
				switch (k) {
					case 0:
					case 3:
						data[i + 8] = 0x38600006;	// li		r3, 6
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", getCurrentFieldEvenOddSigs[k].Name, k, getCurrentFieldEvenOdd);
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
					case 2:
						data[i + 10] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 11] = 0x408200AC;	// bne		+43
						break;
				}
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
					switch (j) {
						case 0:
							data[i +  4] = branchAndLink(getCurrentFieldEvenOdd, VISetRegs + 4);
							data[i +  5] = 0x2C030000;	// cmpwi	r3, 0
							data[i +  6] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i +  7] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i +  8] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
						case 1:
						case 2:
							data[i +  6] = branchAndLink(getCurrentFieldEvenOdd, VISetRegs + 6);
							data[i +  7] = 0x2C030000;	// cmpwi	r3, 0
							data[i +  8] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i +  9] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 10] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
					}
				}
				print_debug("Found:[%s$%i] @ %08X\n", VISetRegsSigs[j].Name, j, VISetRegs);
			}
		}
		
		if (j < sizeof(__VIRetraceHandlerSigs) / sizeof(FuncPattern) && (i = __VIRetraceHandlerSigs[j].offsetFoundAt)) {
			u32 *__VIRetraceHandler = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *__VIRetraceHandlerHook = NULL;
			
			if (__VIRetraceHandler && getCurrentFieldEvenOdd) {
				switch (j) {
					case 3:
						data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 62] = 0x408200B4;	// bne		+45
						break;
					case 4:
						data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 62] = 0x408200C4;	// bne		+49
						break;
					case 5:
						data[i + 64] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 65] = 0x408200BC;	// bne		+47
						break;
					case 6:
						data[i + 64] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 65] = 0x408200C4;	// bne		+49
						break;
					case 7:
						data[i + 69] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 70] = 0x408200C4;	// bne		+49
						break;
					case 8:
						data[i + 81] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 82] = 0x408200C4;	// bne		+49
						break;
					case 9:
						data[i + 86] = 0x2C030000;	// cmpwi	r3, 0
						data[i + 87] = 0x408200C4;	// bne		+49
						break;
				}
				if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
					switch (j) {
						case 3:
						case 4:
							data[i + 57] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 57);
							data[i + 58] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 59] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 60] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 61] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
						case 5:
						case 6:
							data[i + 60] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 60);
							data[i + 61] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 62] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 63] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 64] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
						case 7:
							data[i + 65] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 65);
							data[i + 66] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 67] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 68] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 69] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
						case 8:
							data[i + 77] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 77);
							data[i + 78] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 79] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 80] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 81] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
						case 9:
							data[i + 82] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 82);
							data[i + 83] = 0x2C030000;	// cmpwi	r3, 0
							data[i + 84] = 0x54630FFE;	// srwi		r3, r3, 31
							data[i + 85] = 0x3C800000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) + 0x8000) >> 16;
							data[i + 86] = 0x98640000 | ((u32)(VAR_CURRENT_FIELD - VAR_AREA + 0x80000000) & 0xFFFF);
							break;
					}
				}
				if (swissSettings.forceVJitter == 1 || swissSettings.forceVJitter == 3) {
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
							data[i + 136] = data[i + 135];
							data[i + 135] = data[i + 134];
							data[i + 134] = data[i + 133];
							data[i + 133] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 133);
							break;
						case 3:
							data[i + 130] = data[i + 129];
							data[i + 129] = data[i + 128];
							data[i + 128] = data[i + 127];
							data[i + 127] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 127);
							break;
						case 4:
							data[i + 135] = data[i + 134];
							data[i + 134] = data[i + 133];
							data[i + 133] = data[i + 132];
							data[i + 132] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 132);
							break;
						case 5:
							data[i + 136] = data[i + 135];
							data[i + 135] = data[i + 134];
							data[i + 134] = data[i + 133];
							data[i + 133] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 133);
							break;
						case 6:
							data[i + 138] = data[i + 137];
							data[i + 137] = data[i + 136];
							data[i + 136] = data[i + 135];
							data[i + 135] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 135);
							break;
						case 7:
							data[i + 144] = data[i + 143];
							data[i + 143] = data[i + 142];
							data[i + 142] = data[i + 141];
							data[i + 141] = data[i + 140];
							data[i + 140] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 140);
							break;
						case 8:
							data[i + 155] = data[i + 154];
							data[i + 154] = data[i + 153];
							data[i + 153] = data[i + 152];
							data[i + 152] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 152);
							break;
						case 9:
							data[i + 161] = data[i + 160];
							data[i + 160] = data[i + 159];
							data[i + 159] = data[i + 158];
							data[i + 158] = data[i + 157];
							data[i + 157] = branchAndLink(getCurrentFieldEvenOdd, __VIRetraceHandler + 157);
							break;
					}
					data[i + __VIRetraceHandlerSigs[j].Length - 1] = branch(__VIRetraceHandlerHook, __VIRetraceHandler + __VIRetraceHandlerSigs[j].Length - 1);
				}
				if (swissSettings.wiirdEngine && !in_range(dataType, PATCH_APPLOADER, PATCH_EXEC)) {
					if (__VIRetraceHandlerHook)
						__VIRetraceHandlerHook[5] = branch(CHEATS_ENGINE_START, __VIRetraceHandlerHook + 5);
					else
						data[i + __VIRetraceHandlerSigs[j].Length - 1] = branch(CHEATS_ENGINE_START, __VIRetraceHandler + __VIRetraceHandlerSigs[j].Length - 1);
				}
				print_debug("Found:[%s$%i] @ %08X\n", __VIRetraceHandlerSigs[j].Name, j, __VIRetraceHandler);
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
			if (j == 8 || j == 9) {
				timingTableAddr = (data[i +   5] << 16) + (s16)data[i +   7];
				jumpTableAddr   = (data[i + 147] << 16) + (s16)data[i + 149];
				k = (u16)data[i + 144] + 1;
			} else if (j >= 4) {
				timingTableAddr = (data[i + 1] << 16) + (s16)data[i + 2];
				jumpTableAddr   = (data[i + 4] << 16) + (s16)data[i + 5];
				k = (u16)data[i] + 1;
			} else {
				timingTableAddr = (data[i + 2] << 16) + (s16)data[i + 3];
				jumpTableAddr   = (data[i + 6] << 16) + (s16)data[i + 7];
				k = (u16)data[i + 4] + 1;
			}
			if ((j > 1 && j < 4) || j > 6) timingTableAddr += 68;
			
			timingTable = Calc_Address(data, dataType, timingTableAddr);
			jumpTable   = Calc_Address(data, dataType, jumpTableAddr);
			
			memcpy(timingTable, video_timing, sizeof(video_timing));
			
			if (swissSettings.gameVMode == 7 || swissSettings.gameVMode == 14)
				memcpy(timingTable + 6, video_timing_540p60, sizeof(video_timing_540p60));
			else if (swissSettings.gameVMode == 6 || swissSettings.gameVMode == 13)
				memcpy(timingTable + 6, video_timing_1080i60, sizeof(video_timing_1080i60));
			else if (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11)
				memcpy(timingTable + 6, video_timing_960i, sizeof(video_timing_960i));
			
			if (swissSettings.aveCompat == AVE_N_DOL_COMPAT)
				memcpy(timingTable + 2, video_timing_ntsc50, sizeof(video_timing_ntsc50));
			
			if ((j >= 1 && j < 4) || j >= 6) {
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
			else if (swissSettings.gameVMode == -2 || in_range(swissSettings.gameVMode, 8, 14)) {
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
				memcpy(data + i, getTimingSigs[j].Patch, getTimingSigs[j].PatchLength * sizeof(u32));
				
				data[i + 1] |= ((u32)(getTiming + getTimingSigs[j].PatchLength) + 0x8000) >> 16;
				data[i + 2] |= ((u32)(getTiming + getTimingSigs[j].PatchLength) & 0xFFFF);
				
				for (k = 6; k < getTimingSigs[j].PatchLength; k++)
					data[i + k] += timingTableAddr;
			}
			print_debug("Found:[%s$%i] @ %08X\n", getTimingSigs[j].Name, j, getTiming);
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
					data[i +  10] = 0x57DB07BE;	// clrlwi	r27, r30, 30
					data[i + 135] = 0x281B0002;	// cmplwi	r27, 2
					data[i + 137] = 0x281B0003;	// cmplwi	r27, 3
					data[i + 139] = 0x281B0001;	// cmplwi	r27, 1
					data[i + 140] = 0x4181002C;	// bgt		+11
					data[i + 141] = 0x5760177A;	// rlwinm	r0, r27, 2, 29, 29
					break;
				case 4:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 5:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x281E0003;	// cmplwi	r30, 3
					data[i + 106] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 6:
					data[i +  11] = 0x57BE07BE;	// clrlwi	r30, r29, 30
					data[i +  35] = 0x281E0002;	// cmplwi	r30, 2
					data[i + 104] = 0x281E0003;	// cmplwi	r30, 3
					data[i + 106] = 0x281E0001;	// cmplwi	r30, 1
					data[i + 107] = 0x41810020;	// bgt		+8
					data[i + 108] = 0x57C3177A;	// rlwinm	r3, r30, 2, 29, 29
					break;
				case 7:
					data[i +   5] = 0x546607BE;	// clrlwi	r6, r3, 30
					data[i +  90] = 0x28060002;	// cmplwi	r6, 2
					data[i + 157] = 0x28060003;	// cmplwi	r6, 3
					data[i + 159] = 0x28060001;	// cmplwi	r6, 1
					data[i + 160] = 0x41810020;	// bgt		+8
					data[i + 161] = 0x54C5177A;	// rlwinm	r5, r6, 2, 29, 29
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __VIInitSigs[j].Name, j, __VIInit);
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
			
			print_debug("Found:[%s] @ %08X\n", AdjustPositionSig.Name, AdjustPosition);
		}
	}
	
	if ((i = ImportAdjustingValuesSig.offsetFoundAt)) {
		u32 *ImportAdjustingValues = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (ImportAdjustingValues) {
			data[i + 17] = 0x38000000 | (swissSettings.forceVOffset & 0xFFFF);
			
			print_debug("Found:[%s] @ %08X\n", ImportAdjustingValuesSig.Name, ImportAdjustingValues);
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
				case 2: flushFlag = (s16)data[i + 38]; break;
				case 3: flushFlag = (s16)data[i + 28]; break;
				case 4: flushFlag = (s16)data[i + 29]; break;
				case 5:
				case 6: flushFlag = (s16)data[i + 31]; break;
				case 7: flushFlag = (s16)data[i + 37]; break;
				case 8: flushFlag = (s16)data[i + 35]; break;
			}
			switch (k) {
				case 0:
				case 1: data[i + 14] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
				case 2: data[i + 21] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
				case 3:
				case 4: data[i + 15] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
				case 5:
				case 6: data[i + 18] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
				case 7: data[i + 24] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
				case 8: data[i + 17] = 0x38600000 | (newmode->viTVMode & 0xFFFF); break;
			}
			if (swissSettings.gameVMode > 0) {
				switch (k) {
					case 1: data[i + 143] = 0x38000000 | (swissSettings.sramVideo & 0xFFFF); break;
					case 2: data[i + 160] = 0x38000000 | (swissSettings.sramVideo & 0xFFFF); break;
					case 6: data[i + 138] = 0x38800000 | (swissSettings.sramVideo & 0xFFFF); break;
					case 7: data[i + 152] = 0x38800000 | (swissSettings.sramVideo & 0xFFFF); break;
					case 8: data[i + 201] = 0x39200000 | (swissSettings.sramVideo & 0xFFFF); break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", VIInitSigs[k].Name, k, VIInit);
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
				print_debug("Found:[%s$%i] @ %08X\n", VIWaitForRetraceSigs[j].Name, j, VIWaitForRetrace);
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
			print_debug("Found:[%s$%i] @ %08X\n", setFbbRegsSigs[j].Name, j, setFbbRegs);
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
					data[i + 12] = 0x7C6B0734;	// extsh	r11, r3
					break;
				case 2:
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
				case 3:
					data[i + 15] = 0x7C7E0734;	// extsh	r30, r3
					break;
				case 4:
					data[i + 14] = 0x7C7F0734;	// extsh	r31, r3
					break;
			}
			if (swissSettings.forceVideoActive) {
				switch (j) {
					case 0:
					case 1: data[i + 51] = 0x38000000; break;	// li		r0, 0
					case 2:
					case 3: data[i +  4] = 0x3BE00000; break;	// li		r31, 0
					case 4: data[i +  4] = 0x39800000; break;	// li		r12, 0
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", setVerticalRegsSigs[j].Name, j, setVerticalRegs);
		}
	}
	
	for (j = 0; j < sizeof(VIConfigureSigs) / sizeof(FuncPattern); j++)
		if (VIConfigureSigs[j].offsetFoundAt) break;
	
	if (j < sizeof(VIConfigureSigs) / sizeof(FuncPattern) && (i = VIConfigureSigs[j].offsetFoundAt)) {
		u32 *VIConfigure = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *VIConfigureHook1, *VIConfigureHook2;
		
		if (VIConfigure) {
			VIConfigureHook2 = getPatchAddr(VI_CONFIGUREHOOK2);
			
			if (in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT)) {
				if (swissSettings.rt4kOptim)
					VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1_RT4K);
				else
					VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1_GCVIDEO);
			} else
				VIConfigureHook1 = getPatchAddr(VI_CONFIGUREHOOK1);
			
			if (swissSettings.forceVJitter == 1 || (swissSettings.forceVJitter != 2 && swissSettings.fixPixelCenter))
				VIConfigureHook1 = getPatchAddr(VI_CONFIGUREFIELDMODE);
			
			switch (swissSettings.gameVMode) {
				case -2:
				case -1: VIConfigureHook1 = getPatchAddr(VI_CONFIGUREAUTOP);   break;
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
			
			if (in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) && swissSettings.rt4kOptim)
				VIConfigureHook1 = getPatchAddr(VI_CONFIGURENOYSCALE);
			
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
					data[i +  66] = data[i + 8];
					data[i +  65] = data[i + 7];
					data[i +   8] = data[i + 6];
					data[i +   7] = data[i + 5];
					data[i +   5] = branchAndLink(VIConfigureHook1, VIConfigure + 5);
					data[i +   6] = 0x7C771B78;	// mr		r23, r3
					data[i +  18] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  41] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 121] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i + 131] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 137] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 289] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 290] = 0x28000002;	// cmplwi	r0, 2
					data[i + 292] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 293] = 0x28000003;	// cmplwi	r0, 3
					data[i + 295] = 0x801F0114;	// lwz		r0, 276 (r31)
					data[i + 296] = 0x28000001;	// cmplwi	r0, 1
					data[i + 297] = 0x40810010;	// ble		+4
					data[i + 341] = 0xA87F00FA;	// lha		r3, 250 (r31)
					data[i + 351] = branchAndLink(VIConfigureHook2, VIConfigure + 351);
					break;
				case 3:
					data[i +  10] = data[i + 8];
					data[i +   9] = data[i + 7];
					data[i +   8] = data[i + 6];
					data[i +   7] = data[i + 5];
					data[i +   5] = branchAndLink(VIConfigureHook1, VIConfigure + 5);
					data[i +   6] = 0x7C761B78;	// mr		r22, r3
					data[i +  28] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i +  46] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 112] = 0xA01E0012;	// lhz		r0, 18 (r30)
					data[i + 122] = 0xA01E0014;	// lhz		r0, 20 (r30)
					data[i + 128] = 0xA01E0016;	// lhz		r0, 22 (r30)
					data[i + 322] = 0xA87F00FA;	// lha		r3, 250 (r31)
					data[i + 332] = branchAndLink(VIConfigureHook2, VIConfigure + 332);
					break;
				case 4:
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
				case 5:
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
				case 6:
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
				case 7:
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
				case 8:
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
				case 9:
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
				case 10:
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
				case 11:
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
				case 12:
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
					data[i + 333] = 0x815D0024;	// lwz		r10, 36 (r29)
					data[i + 337] = 0x280A0002;	// cmplwi	r10, 2
					data[i + 344] = 0x280A0003;	// cmplwi	r10, 3
					data[i + 346] = 0x280A0001;	// cmplwi	r10, 1
					data[i + 347] = 0x40810010;	// ble		+4
					data[i + 543] = 0xA87E000A;	// lha		r3, 10 (r30)
					data[i + 553] = branchAndLink(VIConfigureHook2, VIConfigure + 553);
					break;
				case 13:
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
			if (swissSettings.gameVMode != 0) {
				switch (j) {
					case 0:
						data[i +  18] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  35] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  38] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  55] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  63] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  73] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i + 107] = 0x881E0016;	// lbz		r0, 22 (r30)
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
						data[i + 100] = 0x881E0016;	// lbz		r0, 22 (r30)
						break;
					case 2:
						data[i +  18] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  21] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  24] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  27] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  41] = 0x881E0017;	// lbz		r0, 23 (r30)
						data[i +  44] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  47] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  50] = 0xA01E0002;	// lhz		r0, 2 (r30)
						data[i +  67] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  75] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i + 137] = 0x881E0016;	// lbz		r0, 22 (r30)
						break;
					case 3:
						data[i +  11] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i +  28] = 0x881E0016;	// lbz		r0, 22 (r30)
						data[i +  46] = 0x881E0016;	// lbz		r0, 22 (r30)
						data[i +  65] = 0xA01E0000;	// lhz		r0, 0 (r30)
						data[i + 128] = 0x881E0016;	// lbz		r0, 22 (r30)
						break;
					case 4:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  54] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  76] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 5:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  46] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i +  68] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 6:
						data[i +   8] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  46] = 0x887F0016;	// lbz		r3, 22 (r31)
						break;
					case 7:
						data[i +  10] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  45] = 0xA07F0000;	// lhz		r3, 0 (r31)
						data[i +  81] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i + 103] = 0xA07F0000;	// lhz		r3, 0 (r31)
						data[i + 237] = 0xA0DF0000;	// lhz		r6, 0 (r31)
						break;
					case 8:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  84] = 0x887F0016;	// lbz		r3, 22 (r31)
						data[i + 106] = 0xA07F0000;	// lhz		r3, 0 (r31)
						break;
					case 9:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i +  84] = 0x887F0016;	// lbz		r3, 22 (r31)
						break;
					case 10:
						data[i +  10] = 0xA09F0000;	// lhz		r4, 0 (r31)
						data[i +  20] = 0xA01F0000;	// lhz		r0, 0 (r31)
						data[i + 116] = 0x887F0016;	// lbz		r3, 22 (r31)
						break;
					case 11:
					case 12:
						data[i +  10] = 0xA0BB0000;	// lhz		r5, 0 (r27)
						data[i +  20] = 0xA01B0000;	// lhz		r0, 0 (r27)
						data[i + 112] = 0x887B0016;	// lbz		r3, 22 (r27)
						break;
					case 13:
						data[i +  10] = 0xA0930000;	// lhz		r4, 0 (r19)
						data[i +  20] = 0xA0130000;	// lhz		r0, 0 (r19)
						data[i + 116] = 0x88730016;	// lbz		r3, 22 (r19)
						break;
				}
			}
			if (swissSettings.gameVMode > 0) {
				switch (j) {
					case 0:
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 120] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 122] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 123] = 0x28000001;	// cmplwi	r0, 1
						data[i + 125] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 126] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 128] = 0xA01E0010;	// lhz		r0, 16 (r30)
						break;
					case 1:
						data[i +  90] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 113] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 118] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 120] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 121] = 0x28000001;	// cmplwi	r0, 1
						data[i + 123] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 124] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 126] = 0xA01E0010;	// lhz		r0, 16 (r30)
						break;
					case 2:
						data[i + 127] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 150] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 155] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 157] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 158] = 0x28000001;	// cmplwi	r0, 1
						data[i + 160] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 161] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 163] = 0xA01E0010;	// lhz		r0, 16 (r30)
						break;
					case 3:
						data[i + 118] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 141] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 146] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 148] = 0x801F0114;	// lwz		r0, 276 (r31)
						data[i + 149] = 0x28000001;	// cmplwi	r0, 1
						data[i + 151] = 0xA01E0010;	// lhz		r0, 16 (r30)
						data[i + 152] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 154] = 0xA01E0010;	// lhz		r0, 16 (r30)
						break;
					case 4:
						data[i +  35] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  67] = 0x801B0000;	// lwz		r0, 0 (r27)
						data[i +  68] = 0x28000001;	// cmplwi	r0, 1
						data[i +  70] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  71] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  73] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 5:
						data[i +  27] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  57] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  59] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  60] = 0x28000001;	// cmplwi	r0, 1
						data[i +  62] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  63] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 6:
						data[i +  27] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  57] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  59] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  60] = 0x28000001;	// cmplwi	r0, 1
						data[i +  62] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  63] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  65] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 7:
						data[i +  62] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  92] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  94] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  95] = 0x28000001;	// cmplwi	r0, 1
						data[i +  97] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  98] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 8:
						data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  97] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i +  98] = 0x28000001;	// cmplwi	r0, 1
						data[i + 100] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 101] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 103] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 9:
						data[i +  65] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i +  95] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i +  99] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 101] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i + 102] = 0x28000001;	// cmplwi	r0, 1
						data[i + 104] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 105] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 107] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 10:
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 127] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 131] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 133] = 0x801C0000;	// lwz		r0, 0 (r28)
						data[i + 134] = 0x28000001;	// cmplwi	r0, 1
						data[i + 136] = 0xA01F0010;	// lhz		r0, 16 (r31)
						data[i + 137] = 0x5400003C;	// clrrwi	r0, r0, 1
						data[i + 139] = 0xA01F0010;	// lhz		r0, 16 (r31)
						break;
					case 11:
					case 12:
						data[i + 101] = 0x5408003C;	// clrrwi	r8, r0, 1
						data[i + 123] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 127] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 129] = 0x28090001;	// cmplwi	r9, 1
						data[i + 131] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						data[i + 133] = 0xA0FB0010;	// lhz		r7, 16 (r27)
						break;
					case 13:
						data[i +  97] = 0x5400003C;	// clrrwi	r0, r0, 1
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
			if (swissSettings.aveCompat != AVE_RVL_COMPAT && (swissSettings.gameVMode == 4 || swissSettings.gameVMode == 11 ||
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
						data[i + 229] = 0x579C07B8;	// rlwinm	r28, r28, 0, 30, 28
						data[i + 230] = 0x501C177A;	// rlwimi	r28, r0, 2, 29, 29
						break;
					case 3:
						data[i + 221] = 0x579C07B8;	// rlwinm	r28, r28, 0, 30, 28
						data[i + 222] = 0x501C177A;	// rlwimi	r28, r0, 2, 29, 29
						break;
					case 4:
						data[i + 205] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 206] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 5:
						data[i + 197] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 198] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 6:
						data[i + 200] = 0x54A607B8;	// rlwinm	r6, r5, 0, 30, 28
						data[i + 201] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						break;
					case 7:
						data[i + 232] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 233] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 8:
						data[i + 235] = 0x546307B8;	// rlwinm	r3, r3, 0, 30, 28
						data[i + 236] = 0x5003177A;	// rlwimi	r3, r0, 2, 29, 29
						break;
					case 9:
						data[i + 253] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 254] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 10:
						data[i + 285] = 0x54A507B8;	// rlwinm	r5, r5, 0, 30, 28
						data[i + 286] = 0x5005177A;	// rlwimi	r5, r0, 2, 29, 29
						break;
					case 11:
						data[i + 309] = 0x7D004378;	// mr		r0, r8
						data[i + 310] = 0x5160177A;	// rlwimi	r0, r11, 2, 29, 29
						break;
					case 12:
						data[i + 310] = 0x7D004378;	// mr		r0, r8
						data[i + 311] = 0x5160177A;	// rlwimi	r0, r11, 2, 29, 29
						break;
					case 13:
						data[i + 288] = 0x5006177A;	// rlwimi	r6, r0, 2, 29, 29
						data[i + 289] = 0x54E9003C;	// rlwinm	r9, r7, 0, 0, 30
						data[i + 290] = 0x61290001;	// ori		r9, r9, 1
						break;
				}
			}
			if (swissSettings.aveCompat == AVE_N_DOL_COMPAT) {
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
						data[i + 215] = 0x801F0118;	// lwz		r0, 280 (r31)
						data[i + 216] = 0x28000001;	// cmplwi	r0, 1
						data[i + 218] = 0x38000004;	// li		r0, 4
						break;
					case 3:
						data[i + 206] = 0x801F0118;	// lwz		r0, 280 (r31)
						data[i + 207] = 0x28000001;	// cmplwi	r0, 1
						data[i + 209] = 0x38000004;	// li		r0, 4
						break;
					case 4:
						data[i + 167] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 168] = 0x28000001;	// cmplwi	r0, 1
						data[i + 170] = 0x38000000;	// li		r0, 0
						break;
					case 5:
						data[i + 159] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 160] = 0x28000001;	// cmplwi	r0, 1
						data[i + 162] = 0x38000000;	// li		r0, 0
						break;
					case 6:
						data[i + 162] = 0x80180000;	// lwz		r0, 0 (r24)
						data[i + 163] = 0x28000001;	// cmplwi	r0, 1
						data[i + 165] = 0x38000000;	// li		r0, 0
						break;
					case 7:
						data[i + 194] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 195] = 0x28000001;	// cmplwi	r0, 1
						data[i + 197] = 0x38000000;	// li		r0, 0
						break;
					case 8:
						data[i + 197] = 0x801D0118;	// lwz		r0, 280 (r29)
						data[i + 198] = 0x28000001;	// cmplwi	r0, 1
						data[i + 200] = 0x38000004;	// li		r0, 4
						break;
					case 9:
						data[i + 213] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 214] = 0x28000001;	// cmplwi	r0, 1
						data[i + 216] = 0x38000004;	// li		r0, 4
						break;
					case 10:
						data[i + 245] = 0x80150000;	// lwz		r0, 0 (r21)
						data[i + 246] = 0x28000001;	// cmplwi	r0, 1
						data[i + 248] = 0x38000004;	// li		r0, 4
						break;
					case 11:
					case 12:
						data[i + 232] = 0x81250028;	// lwz		r9, 40 (r5)
						data[i + 250] = 0x28090001;	// cmplwi	r9, 1
						data[i + 273] = 0x38000004;	// li		r0, 4
						break;
					case 13:
						data[i + 245] = 0x80160000;	// lwz		r0, 0 (r22)
						data[i + 246] = 0x28000001;	// cmplwi	r0, 1
						data[i + 248] = 0x38000004;	// li		r0, 4
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", VIConfigureSigs[j].Name, j, VIConfigure);
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
			print_debug("Found:[%s$%i] @ %08X\n", VIConfigurePanSigs[j].Name, j, VIConfigurePan);
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
			print_debug("Found:[%s$%i] @ %08X\n", VISetBlackSigs[j].Name, j, VISetBlack);
		}
	}
	
	if (swissSettings.gameVMode == 2 || swissSettings.gameVMode == 9) {
		if ((i = VIGetRetraceCountSig.offsetFoundAt)) {
			u32 *VIGetRetraceCount = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *VIGetRetraceCountHook;
			
			if (VIGetRetraceCount) {
				VIGetRetraceCountHook = getPatchAddr(VI_GETRETRACECOUNTHOOK);
				
				data[i + 1] = branch(VIGetRetraceCountHook, VIGetRetraceCount + 1);
				
				print_debug("Found:[%s] @ %08X\n", VIGetRetraceCountSig.Name, VIGetRetraceCount);
			}
		}
	}
	
	for (j = 0; j < sizeof(VIGetNextFieldSigs) / sizeof(FuncPattern); j++)
	if ((i = VIGetNextFieldSigs[j].offsetFoundAt)) {
		u32 *VIGetNextField = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VIGetNextField) {
			if (j == 0)
				data[i + 7] = 0x547F0FFE;	// srwi		r31, r3, 31
			
			if (swissSettings.gameVMode == -2 || in_range(swissSettings.gameVMode, 8, 14))
				if (j == 1)
					data[i + 12] = 0x38600006;	// li		r3, 6
			
			print_debug("Found:[%s$%i] @ %08X\n", VIGetNextFieldSigs[j].Name, j, VIGetNextField);
		}
	}
	
	for (j = 0; j < sizeof(VIGetDTVStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = VIGetDTVStatusSigs[j].offsetFoundAt)) {
		u32 *VIGetDTVStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (VIGetDTVStatus) {
			if (in_range(swissSettings.aveCompat, AVE_N_DOL_COMPAT, AVE_P_DOL_COMPAT)) {
				memset(data + i, 0, VIGetDTVStatusSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000;	// li		r3, 0
				data[i + 1] = 0x4E800020;	// blr
			} else if (swissSettings.forceDTVStatus) {
				memset(data + i, 0, VIGetDTVStatusSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600001;	// li		r3, 1
				data[i + 1] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", VIGetDTVStatusSigs[j].Name, j, VIGetDTVStatus);
		}
	}
	
	for (j = 0; j < sizeof(__GXInitGXSigs) / sizeof(FuncPattern); j++)
	if ((i = __GXInitGXSigs[j].offsetFoundAt)) {
		u32 *__GXInitGX = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__GXInitGX) {
			if (swissSettings.forceVJitter == 1) {
				switch (j) {
					case  0: data[i + 1055] = 0x38600001; break;
					case  1: data[i +  466] = 0x38600001; break;
					case  2: data[i +  503] = 0x38600001; break;
					case  3: data[i +  911] = 0x38600001; break;
					case  4: data[i +  461] = 0x38600001; break;
					case  5: data[i +  476] = 0x38600001; break;
					case  6: data[i +  491] = 0x38600001; break;
					case  7: data[i +  461] = 0x38600001; break;
					case  8: data[i +  463] = 0x38600001; break;
					case  9: data[i +  520] = 0x38600001; break;
					case 10: data[i +  503] = 0x38600001; break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", __GXInitGXSigs[j].Name, j, __GXInitGX);
		}
	}
	
	for (j = 0; j < sizeof(GXAdjustForOverscanSigs) / sizeof(FuncPattern); j++)
	if ((i = GXAdjustForOverscanSigs[j].offsetFoundAt)) {
		u32 *GXAdjustForOverscan = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (GXAdjustForOverscan) {
			if (GXAdjustForOverscanSigs[j].Patch) {
				memset(data + i, 0, GXAdjustForOverscanSigs[j].Length * sizeof(u32));
				memcpy(data + i, GXAdjustForOverscanSigs[j].Patch, GXAdjustForOverscanSigs[j].PatchLength * sizeof(u32));
			}
			print_debug("Found:[%s$%i] @ %08X\n", GXAdjustForOverscanSigs[j].Name, j, GXAdjustForOverscan);
		}
	}
	
	for (j = 0; j < sizeof(GXSetDispCopyYScaleSigs) / sizeof(FuncPattern); j++)
	if ((i = GXSetDispCopyYScaleSigs[j].offsetFoundAt)) {
		u32 *GXSetDispCopyYScale = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 __GXData = 0;
		
		if (GXSetDispCopyYScale) {
			switch (j) {
				case 0: __GXData = data[i + 65]      & 0x1FFFFF; break;
				case 1: __GXData = data[i + 56]      & 0x1FFFFF; break;
				case 2: __GXData = data[i + 55]      & 0x1FFFFF; break;
				case 3:
				case 4:
				case 5: __GXData = data[i +  7]      & 0x1FFFFF; break;
				case 6: __GXData = data[i +  7] >> 5 & 0x1F0000; break;
				case 7: __GXData = data[i +  7]      & 0x1FFFFF; break;
				case 8: __GXData = data[i + 13]      & 0x1FFFFF; break;
			}
			if (GXSetDispCopyYScaleSigs[j].Patch) {
				memset(data + i, 0, GXSetDispCopyYScaleSigs[j].Length * sizeof(u32));
				memcpy(data + i, GXSetDispCopyYScaleSigs[j].Patch, GXSetDispCopyYScaleSigs[j].PatchLength * sizeof(u32));
				
				if (GXSetDispCopyYScaleSigs[j].Patch == GXSetDispCopyYScalePatch1)
					data[i + 6] |= __GXData;
				if (GXSetDispCopyYScaleSigs[j].Patch == GXSetDispCopyYScaleStub1)
					data[i + 1] |= __GXData;
			}
			print_debug("Found:[%s$%i] @ %08X\n", GXSetDispCopyYScaleSigs[j].Name, j, GXSetDispCopyYScale);
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
					memcpy(data + i, GXSetCopyFilterSigs[j].Patch, GXSetCopyFilterSigs[j].PatchLength * sizeof(u32));
				}
				print_debug("Found:[%s$%i] @ %08X\n", GXSetCopyFilterSigs[j].Name, j, GXSetCopyFilter);
			}
		}
	}
	
	for (j = 0; j < sizeof(GXCopyDispSigs) / sizeof(FuncPattern); j++)
	if ((i = GXCopyDispSigs[j].offsetFoundAt)) {
		u32 *GXCopyDisp = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *GXCopyDispHook;
		
		if (GXCopyDisp) {
			if (swissSettings.forceVJitter == 1 || swissSettings.forceVJitter == 3) {
				GXCopyDispHook = getPatchAddr(GX_COPYDISPHOOK);
				
				data[i + GXCopyDispSigs[j].Length - 1] = branch(GXCopyDispHook, GXCopyDisp + GXCopyDispSigs[j].Length - 1);
			}
			print_debug("Found:[%s$%i] @ %08X\n", GXCopyDispSigs[j].Name, j, GXCopyDisp);
		}
	}
	
	if (swissSettings.disableDithering) {
		for (j = 0; j < sizeof(GXSetBlendModeSigs) / sizeof(FuncPattern); j++)
			if (GXSetBlendModeSigs[j].offsetFoundAt) break;
		
		if (j < sizeof(GXSetBlendModeSigs) / sizeof(FuncPattern) && (i = GXSetBlendModeSigs[j].offsetFoundAt)) {
			u32 *GXSetBlendMode = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 __GXData = 0;
			
			if (GXSetBlendMode) {
				switch (j) {
					case 0: __GXData = data[i + 147] & 0x1FFFFF; break;
					case 1: __GXData = data[i +  98] & 0x1FFFFF; break;
					case 2: __GXData = data[i +  60] & 0x1FFFFF; break;
					case 3: __GXData = data[i +   0] & 0x1FFFFF; break;
					case 4: __GXData = data[i +   0] & 0x1FFFFF; break;
					case 5: __GXData = data[i +  27] & 0x1F0000; break;
					case 6: __GXData = data[i +  35] & 0x1F0000; break;
				}
				if (GXSetBlendModeSigs[j].Patch) {
					memset(data + i, 0, GXSetBlendModeSigs[j].Length * sizeof(u32));
					memcpy(data + i, GXSetBlendModeSigs[j].Patch, GXSetBlendModeSigs[j].PatchLength * sizeof(u32));
					
					if (GXSetBlendModeSigs[j].Patch != GXSetBlendModePatch3)
						data[i +  0] |= __GXData;
					if (GXSetBlendModeSigs[j].Patch == GXSetBlendModePatch2)
						data[i + 21] |= __GXData;
				}
				print_debug("Found:[%s$%i] @ %08X\n", GXSetBlendModeSigs[j].Name, j, GXSetBlendMode);
			}
		}
	}
	
	for (j = 0; j < sizeof(GXSetViewportSigs) / sizeof(FuncPattern); j++)
	if ((i = GXSetViewportSigs[j].offsetFoundAt)) {
		u32 *GXSetViewport = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *GXSetViewportJitter = NULL;
		u32 *__GXSetViewport = NULL;
		u32 __GXData = 0;
		u32 vpScale = 0;
		u32 vpOffset = 0;
		f32 *constant = NULL;
		
		if (GXSetViewport) {
			switch (j) {
				case 0:
					if (findx_pattern(data, dataType, i + 16, length, &GXSetViewportJitterSigs[0]))
						GXSetViewportJitter = GXSetViewportJitterSigs[0].properAddress;
					break;
				case 1:
					if (findx_pattern(data, dataType, i + 16, length, &GXSetViewportJitterSigs[1]) &&
						find_pattern_before(data, dataType, length, &GXSetViewportJitterSigs[1], &__GXSetViewportSigs[0])) {
						__GXSetViewport = __GXSetViewportSigs[0].properAddress;
						
						__GXData = data[GXSetViewportJitterSigs[1].offsetFoundAt + 27] & 0x1FFFFF;
					}
					break;
				case 2:
					if (findx_pattern(data, dataType, i +  4, length, &GXSetViewportJitterSigs[2]))
						GXSetViewportJitter = GXSetViewportJitterSigs[2].properAddress;
					break;
				case 3:
					if (findx_pattern(data, dataType, i +  4, length, &GXSetViewportJitterSigs[3]))
						GXSetViewportJitter = GXSetViewportJitterSigs[3].properAddress;
					break;
				case 4:
					if (findx_pattern(data, dataType, i +  1, length, &GXSetViewportJitterSigs[4]))
						GXSetViewportJitter = GXSetViewportJitterSigs[4].properAddress;
					break;
				case 5:
					if (findx_pattern(data, dataType, i + 10, length, &__GXSetViewportSigs[1]))
						__GXSetViewport = __GXSetViewportSigs[1].properAddress;
					
					__GXData = data[i + 3] & 0x1FFFFF;
					break;
				case 6:
					__GXData = data[i + 0] & 0x1FFFFF;
					vpScale  = data[i + 2] & 0x1FFFFF;
					vpOffset = data[i + 7] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 7, 2);
					break;
			}
			if (swissSettings.fixPixelCenter)
				if (constant) *constant = truncf(*constant) + 0.5f / 12.0f * swissSettings.fixPixelCenter;
			
			if (GXSetViewportSigs[j].Patch) {
				memset(data + i, 0, GXSetViewportSigs[j].Length * sizeof(u32));
				memcpy(data + i, GXSetViewportSigs[j].Patch, GXSetViewportSigs[j].PatchLength * sizeof(u32));
				
				if (GXSetViewportSigs[j].Patch == GXSetViewportPatch1) {
					data[i + GXSetViewportSigs[j].PatchLength - 1] = branch(GXSetViewportJitter, GXSetViewport + GXSetViewportSigs[j].PatchLength - 1);
				} else if (GXSetViewportSigs[j].Patch == GXSetViewportPatch2) {
					data[i +  0] |= __GXData;
					data[i + GXSetViewportSigs[j].PatchLength - 1] = branch(__GXSetViewport, GXSetViewport + GXSetViewportSigs[j].PatchLength - 1);
				} else if (GXSetViewportSigs[j].Patch == GXSetViewportPatch3 || GXSetViewportSigs[j].Patch == GXSetViewportTAAPatch3) {
					data[i +  0] |= __GXData;
					data[i +  2] |= vpScale;
					data[i + 15] |= vpOffset;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", GXSetViewportSigs[j].Name, j, GXSetViewport);
		}
	}
	
	for (j = 0; j < sizeof(GXSetViewportJitterSigs) / sizeof(FuncPattern); j++)
	if ((i = GXSetViewportJitterSigs[j].offsetFoundAt)) {
		u32 *GXSetViewportJitter = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *__GXSetViewport = NULL;
		u32 __GXData = 0;
		u32 vpScale = 0;
		u32 vpZScale = 0;
		u32 vpOffset = 0;
		f32 *constant = NULL;
		
		if (GXSetViewportJitter) {
			switch (j) {
				case 0:
					vpScale  = data[i + 31] & 0x1FFFFF;
					vpOffset = data[i + 38] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 38, 2);
					vpZScale = data[i + 48] & 0x1FFFFF;
					__GXData = data[i + 54] & 0x1FFFFF;
					break;
				case 1:
					if (findx_pattern(data, dataType, i + 43, length, &__GXSetViewportSigs[0]))
						__GXSetViewport = __GXSetViewportSigs[0].properAddress;
					
					__GXData = data[i + 27] & 0x1FFFFF;
					break;
				case 2:
				case 3:
					vpScale  = data[i + 11] & 0x1FFFFF;
					vpZScale = data[i + 15] & 0x1FFFFF;
					vpOffset = data[i + 16] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 16, 2);
					__GXData = data[i + 18] & 0x1FFFFF;
					break;
				case 5:
					if (findx_pattern(data, dataType, i + 14, length, &__GXSetViewportSigs[1]))
						__GXSetViewport = __GXSetViewportSigs[1].properAddress;
					
					__GXData = data[i +  7] & 0x1FFFFF;
					break;
				case 6:
					vpScale  = data[i +  2] & 0x1FFFFF;
					__GXData = data[i +  4] & 0x1FFFFF;
					vpOffset = data[i + 11] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 11, 2);
					break;
			}
			if (swissSettings.forceVJitter == 2) {
				switch (j) {
					case 0:
						data[i + 32] = 0x60000000;	// nop
						break;
					case 1:
						data[i + 25] = 0x60000000;	// nop
						break;
					case 2:
					case 3:
						data[i + 12] = 0x60000000;	// nop
						break;
					case 4:
						data[i + 11] = 0x60000000;	// nop
						break;
					case 5:
						data[i +  6] = 0x60000000;	// nop
						break;
					case 6:
						data[i +  3] = 0x60000000;	// nop
						break;
				}
			}
			if (swissSettings.fixPixelCenter)
				if (constant) *constant = truncf(*constant) + 0.5f / 12.0f * swissSettings.fixPixelCenter;
			
			if (GXSetViewportJitterSigs[j].Patch) {
				memset(data + i, 0, GXSetViewportJitterSigs[j].Length * sizeof(u32));
				memcpy(data + i, GXSetViewportJitterSigs[j].Patch, GXSetViewportJitterSigs[j].PatchLength * sizeof(u32));
				
				if (GXSetViewportJitterSigs[j].Patch == GXSetViewportJitterPatch1 || GXSetViewportJitterSigs[j].Patch == GXSetViewportJitterTAAPatch1) {
					data[i +  1] |= __GXData;
					data[i +  2] |= vpScale;
					data[i + 11] |= vpZScale;
					data[i + 12] |= vpOffset;
				} else if (GXSetViewportJitterSigs[j].Patch == GXSetViewportJitterPatch2) {
					data[i +  0] |= __GXData;
					data[i + GXSetViewportJitterSigs[j].PatchLength - 1] = branch(__GXSetViewport, GXSetViewportJitter + GXSetViewportJitterSigs[j].PatchLength - 1);
				} else if (GXSetViewportJitterSigs[j].Patch == GXSetViewportJitterPatch3 || GXSetViewportJitterSigs[j].Patch == GXSetViewportJitterTAAPatch3) {
					data[i +  1] |= __GXData;
					data[i +  2] |= vpScale;
					data[i + 13] |= vpOffset;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", GXSetViewportJitterSigs[j].Name, j, GXSetViewportJitter);
		}
	}
	
	for (j = 0; j < sizeof(__GXSetViewportSigs) / sizeof(FuncPattern); j++)
	if ((i = __GXSetViewportSigs[j].offsetFoundAt)) {
		u32 *__GXSetViewport = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 vpScale = 0;
		u32 vpOffset = 0;
		f32 *constant = NULL;
		
		if (__GXSetViewport) {
			switch (j) {
				case 0:
					vpScale  = data[i +  8] & 0x1FFFFF;
					vpOffset = data[i + 15] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 15, 2);
					break;
				case 1:
					vpScale  = data[i +  2] & 0x1FFFFF;
					vpOffset = data[i + 20] & 0x1FFFFF;
					constant = loadResolve(data, dataType, i + 20, 2);
					break;
			}
			if (swissSettings.fixPixelCenter)
				if (constant) *constant = truncf(*constant) + 0.5f / 12.0f * swissSettings.fixPixelCenter;
			
			if (__GXSetViewportSigs[j].Patch) {
				memset(data + i, 0, __GXSetViewportSigs[j].Length * sizeof(u32));
				memcpy(data + i, __GXSetViewportSigs[j].Patch, __GXSetViewportSigs[j].PatchLength * sizeof(u32));
				
				data[i + 1] |= vpScale;
				data[i + 4] |= vpOffset;
			}
			print_debug("Found:[%s] @ %08X\n", __GXSetViewportSigs[j].Name, __GXSetViewport);
		}
	}
	
	for (j = 0; j < sizeof(__THPDecompressiMCURow512x448Sigs) / sizeof(FuncPattern); j++)
	if ((i = __THPDecompressiMCURow512x448Sigs[j].offsetFoundAt)) {
		u32 *__THPDecompressiMCURow512x448 = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__THPDecompressiMCURow512x448) {
			switch (j) {
				case 0:
					data[i +  125] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  130] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  378] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  383] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  633] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  639] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  894] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  900] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1161] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1166] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1420] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1425] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
				case 1:
					data[i +  123] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  129] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  383] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  389] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  642] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  648] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  903] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  909] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1173] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1179] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1439] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1445] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __THPDecompressiMCURow512x448Sigs[j].Name, j, __THPDecompressiMCURow512x448);
		}
	}
	
	for (j = 0; j < sizeof(__THPDecompressiMCURow640x480Sigs) / sizeof(FuncPattern); j++)
	if ((i = __THPDecompressiMCURow640x480Sigs[j].offsetFoundAt)) {
		u32 *__THPDecompressiMCURow640x480 = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__THPDecompressiMCURow640x480) {
			switch (j) {
				case 0:
					data[i +  125] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  130] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  378] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  383] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  633] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  639] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  894] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  900] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1161] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1166] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1420] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1425] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
				case 1:
					data[i +  123] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  129] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  383] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  389] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  642] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  648] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  903] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  909] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1173] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1179] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1439] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1445] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __THPDecompressiMCURow640x480Sigs[j].Name, j, __THPDecompressiMCURow640x480);
		}
	}
	
	for (j = 0; j < sizeof(__THPDecompressiMCURowNxNSigs) / sizeof(FuncPattern); j++)
	if ((i = __THPDecompressiMCURowNxNSigs[j].offsetFoundAt)) {
		u32 *__THPDecompressiMCURowNxN = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__THPDecompressiMCURowNxN) {
			switch (j) {
				case 0:
					data[i +  123] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  128] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  376] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  381] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  631] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  637] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  892] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  898] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1158] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1163] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1417] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1422] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
				case 1:
					data[i +  127] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  133] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  387] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  393] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  646] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  652] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i +  907] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i +  913] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1176] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1182] = 0x11081828;	// ps_sub	f8, f8, f3
					data[i + 1442] = 0x11071F7C;	// ps_nmsub f8, f7, f29, f3
					data[i + 1448] = 0x11081828;	// ps_sub	f8, f8, f3
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", __THPDecompressiMCURowNxNSigs[j].Name, j, __THPDecompressiMCURowNxN);
		}
	}
	
	if ((i = IDct64_GCSig.offsetFoundAt)) {
		u32 *IDct64_GC = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (IDct64_GC) {
			data[i + 107] = 0x1272982A;	// ps_add	f19, f18, f19
			
			print_debug("Found:[%s] @ %08X\n", IDct64_GCSig.Name, IDct64_GC);
		}
	}
	
	if ((i = IDct10_GCSig.offsetFoundAt)) {
		u32 *IDct10_GC = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (IDct10_GC) {
			data[i + 76] = 0x1272982A;	// ps_add	f19, f18, f19
			
			print_debug("Found:[%s] @ %08X\n", IDct10_GCSig.Name, IDct10_GC);
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
		if (find_pattern(data, dataType, i, length, &MTXFrustumSig)) {
			u32 *MTXFrustum = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXFrustumHook = NULL;
			if (MTXFrustum) {
				print_debug("Found:[%s] @ %08X\n", MTXFrustumSig.Name, MTXFrustum);
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
		if (find_pattern(data, dataType, i, length, &MTXLightFrustumSig)) {
			u32 *MTXLightFrustum = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXLightFrustumHook = NULL;
			if (MTXLightFrustum) {
				print_debug("Found:[%s] @ %08X\n", MTXLightFrustumSig.Name, MTXLightFrustum);
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
		if (find_pattern(data, dataType, i, length, &MTXPerspectiveSig)) {
			u32 *MTXPerspective = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXPerspectiveHook = NULL;
			if (MTXPerspective) {
				print_debug("Found:[%s] @ %08X\n", MTXPerspectiveSig.Name, MTXPerspective);
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
		if (find_pattern(data, dataType, i, length, &MTXLightPerspectiveSig)) {
			u32 *MTXLightPerspective = Calc_ProperAddress(data, dataType, i * sizeof(u32));
			u32 *MTXLightPerspectiveHook = NULL;
			if (MTXLightPerspective) {
				print_debug("Found:[%s] @ %08X\n", MTXLightPerspectiveSig.Name, MTXLightPerspective);
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
			if (find_pattern(data, dataType, i, length, &MTXOrthoSig)) {
				u32 *MTXOrtho = Calc_ProperAddress(data, dataType, i * sizeof(u32));
				u32 *MTXOrthoHook = NULL;
				if (MTXOrtho) {
					print_debug("Found:[%s] @ %08X\n", MTXOrthoSig.Name, MTXOrtho);
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
			make_pattern(data, dataType, i, length, &fp);
			
			for (j = 0; j < sizeof(GXSetScissorSigs) / sizeof(FuncPattern); j++) {
				if (compare_pattern(&fp, &GXSetScissorSigs[j])) {
					u32 *GXSetScissor = Calc_ProperAddress(data, dataType, i * sizeof(u32));
					u32 *GXSetScissorHook = NULL;
					if (GXSetScissor) {
						print_debug("Found:[%s$%i] @ %08X\n", GXSetScissorSigs[j].Name, j, GXSetScissor);
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
			make_pattern(data, dataType, i, length, &fp);
			
			for (j = 0; j < sizeof(GXSetProjectionSigs) / sizeof(FuncPattern); j++) {
				if (compare_pattern(&fp, &GXSetProjectionSigs[j])) {
					u32 *GXSetProjection = Calc_ProperAddress(data, dataType, i * sizeof(u32));
					u32 *GXSetProjectionHook = NULL;
					if (GXSetProjection) {
						print_debug("Found:[%s$%i] @ %08X\n", GXSetProjectionSigs[j].Name, j, GXSetProjection);
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
		make_pattern(data, dataType, i, length, &fp);
		
		for (j = 0; j < sizeof(GXInitTexObjLODSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &GXInitTexObjLODSigs[j])) {
				u32 *GXInitTexObjLOD = Calc_ProperAddress(data, dataType, i * sizeof(u32));
				u32 *GXInitTexObjLODHook = NULL;
				if (GXInitTexObjLOD) {
					print_debug("Found:[%s$%i] @ %08X\n", GXInitTexObjLODSigs[j].Name, j, GXInitTexObjLOD);
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
	print_debug("Patch:[fontEncode] applied %d times\n", patched);
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
				print_debug("Patched:[__GXSetVAT] PAL\n");
				patched=1;
			}
			if(mode == 2 && memcmp(addr_start,GXSETVAT_NTSC_orig,sizeof(GXSETVAT_NTSC_orig))==0) 
			{
				memcpy(addr_start, GXSETVAT_NTSC_patched, sizeof(GXSETVAT_NTSC_patched));
				print_debug("Patched:[__GXSetVAT] NTSC\n");
				patched=1;
			}
			addr_start += 4;
		}
	} else if (dataType == PATCH_BS2) {
		switch (length) {
			case 1435200:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(s16 *)(data + 0x81300712 - 0x81300000 + 0x20) = newmode->viTVMode & ~0x3;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300E8A - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300EA2 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300EAA - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813016A0 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302F00 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B3E4 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B40A - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130B412 - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130B416 - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130B422 - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130B42E - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130B43A - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130B446 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130B44E - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130B452 - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130B45A - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130B45E - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130B466 - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130B476 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130B47E - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130B482 - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130B48E - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130E8B2 - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8130E8B6 - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8130E8C2 - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130E8C6 - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130E8CA - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8130E8CE - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8130E8D2 - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130E8D6 - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8130E8DE - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8130E8E2 - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x8130E91C - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x8130E924 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x8130E928 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x8130E92C - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x8130E930 - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8135DDE0 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1448280:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(u32 *)(data + 0x81300710 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x81300714 - 0x81300000 + 0x20) = 0x38600000 | (newmode->viTVMode & 0xFFFC);
				
				// Accept any region code.
				*(s16 *)(data + 0x8130092E - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300946 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130094E - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81302E60 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813046C0 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130CBA4 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130CBCA - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130CBD2 - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130CBD6 - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130CBE2 - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130CBEE - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130CBFA - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130CC06 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130CC0E - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130CC12 - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130CC1A - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130CC1E - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130CC26 - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130CC36 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130CC3E - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130CC42 - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130CC4E - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x81310072 - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x81310076 - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x81310082 - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x81310086 - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8131008A - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8131008E - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x81310092 - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x81310096 - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8131009E - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x813100A2 - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x813100DC - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x813100E4 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x813100E8 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x813100EC - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x813100F0 - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8135FD00 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1449848:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// Force 24MB physical memory size.
				*(s16 *)(data + 0x813004AE - 0x81300000 + 0x20) = 0x1800000 >> 16;
				
				// __VIInit(newmode->viTVMode & ~0x3);
				*(u32 *)(data + 0x813009BC - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x813009C0 - 0x81300000 + 0x20) = 0x38600000 | (newmode->viTVMode & 0xFFFC);
				
				// Force production mode.
				*(s16 *)(data + 0x81300A4E - 0x81300000 + 0x20) = 1;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130121E - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813031A0 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x8130497C - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130CE60 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130CE86 - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130CE8E - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130CE92 - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130CE9E - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130CEAA - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130CEB6 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130CEC2 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130CECA - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130CECE - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130CED6 - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130CEDA - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130CEE2 - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130CEF2 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130CEFA - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130CEFE - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130CF0A - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8131032E - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x81310332 - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8131033E - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x81310342 - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x81310346 - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8131034A - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8131034E - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x81310352 - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8131035A - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8131035E - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x81310398 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x813103A0 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x813103A4 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x813103A8 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x813103AC - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x81360160 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "DEV  Revision 1.0");
				patched++;
				break;
			case 1583088:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B55C - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B592 - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130B5B2 - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130B5BA - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130B5BE - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130B5C6 - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130B5CA - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130B5D2 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130B5D6 - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130B5DE - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130B5E2 - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130B5EA - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130B5EE - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130B602 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130B606 - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130B60E - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130B61A - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EAAA - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8130EAAE - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8130EABA - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130EABE - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130EAC2 - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8130EAC6 - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8130EACA - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130EACE - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8130EAD6 - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8130EADA - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x8130EB14 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x8130EB1C - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x8130EB20 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EB24 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x8130EB28 - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8137D9F0 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "NTSC Revision 1.1");
				patched++;
				break;
			case 1763048:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (getTVFormat() != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130F1C6 - 0x81300000 + 0x20) = 10;
					*(s16 *)(data + 0x8130F1CA - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130F1D6 - 0x81300000 + 0x20) = 7;
					*(s16 *)(data + 0x8130F1DA - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130F1DE - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130F1E2 - 0x81300000 + 0x20) = 16;
					*(s16 *)(data + 0x8130F1E6 - 0x81300000 + 0x20) = 18;
					*(s16 *)(data + 0x8130F1EA - 0x81300000 + 0x20) = 20;
					*(s16 *)(data + 0x8130F1F2 - 0x81300000 + 0x20) = 40;
					*(s16 *)(data + 0x8130F1F6 - 0x81300000 + 0x20) = 60;
					*(u32 *)(data + 0x8130F230 - 0x81300000 + 0x20) = 0xB3E30016;
					*(u32 *)(data + 0x8130F238 - 0x81300000 + 0x20) = 0x9963000B;
					*(u32 *)(data + 0x8130F23C - 0x81300000 + 0x20) = 0x9BC3001C;
					*(u32 *)(data + 0x8130F240 - 0x81300000 + 0x20) = 0x9BA30037;
					*(u32 *)(data + 0x8130F244 - 0x81300000 + 0x20) = 0x99430035;
					
					memcpy(data + 0x81380FD0 - 0x81300000 + 0x20, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_debug("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1760152:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300458 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130046C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(u32 *)(data + 0x81300764 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x81300768 - 0x81300000 + 0x20) = 0x38600000 | (newmode->viTVMode & 0xFFFF);
				
				// Accept any region code.
				*(s16 *)(data + 0x813009D6 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x813009FA - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81302AEC - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813043EC - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (getTVFormat() != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130FDDA - 0x81300000 + 0x20) = 10;
					*(s16 *)(data + 0x8130FDDE - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130FDEA - 0x81300000 + 0x20) = 7;
					*(s16 *)(data + 0x8130FDEE - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130FDF2 - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130FDF6 - 0x81300000 + 0x20) = 16;
					*(s16 *)(data + 0x8130FDFA - 0x81300000 + 0x20) = 18;
					*(s16 *)(data + 0x8130FDFE - 0x81300000 + 0x20) = 20;
					*(s16 *)(data + 0x8130FE06 - 0x81300000 + 0x20) = 40;
					*(s16 *)(data + 0x8130FE0A - 0x81300000 + 0x20) = 60;
					*(u32 *)(data + 0x8130FE44 - 0x81300000 + 0x20) = 0xB3E30016;
					*(u32 *)(data + 0x8130FE4C - 0x81300000 + 0x20) = 0x9963000B;
					*(u32 *)(data + 0x8130FE50 - 0x81300000 + 0x20) = 0x9BC3001C;
					*(u32 *)(data + 0x8130FE54 - 0x81300000 + 0x20) = 0x9BA30037;
					*(u32 *)(data + 0x8130FE58 - 0x81300000 + 0x20) = 0x99430035;
					
					memcpy(data + 0x81380384 - 0x81300000 + 0x20, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_debug("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1561776:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300522 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x8130077E - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x813007A2 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x813014A8 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302DE8 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (getTVFormat() != VI_PAL) {
					memcpy(data + 0x8137D910 - 0x81300000 + 0x20, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				} else {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130E9D6 - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8130E9DA - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8130E9E6 - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130E9EA - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130E9EE - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8130E9F2 - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8130E9F6 - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130E9FA - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8130EA02 - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8130EA06 - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x8130EA40 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x8130EA48 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x8130EA4C - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EA50 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x8130EA54 - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8137D910 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "MPAL Revision 1.1");
				patched++;
				break;
			case 1607576:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300478 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x8130048C - 0x81300000 + 0x20) = 0x60000000;
				
				// Force 24MB physical memory size.
				*(s16 *)(data + 0x81300496 - 0x81300000 + 0x20) = 0x1800000 >> 16;
				
				// __VIInit(newmode->viTVMode);
				*(u32 *)(data + 0x813007F0 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x813007F4 - 0x81300000 + 0x20) = 0x38600000 | (newmode->viTVMode & 0xFFFF);
				
				// Force production mode.
				*(s16 *)(data + 0x813008A6 - 0x81300000 + 0x20) = 1;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300C02 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300C26 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81303BF4 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x813054B0 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130DBFC - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130DC32 - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130DC52 - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130DC5A - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130DC5E - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130DC66 - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130DC6A - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130DC72 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130DC76 - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130DC7E - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130DC82 - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130DC8A - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130DC8E - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130DCA2 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130DCA6 - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130DCAE - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130DCBA - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8131114A - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8131114E - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8131115A - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8131115E - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x81311162 - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x81311166 - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8131116A - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8131116E - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x81311176 - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8131117A - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x813111B4 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x813111BC - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x813111C0 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x813111C4 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x813111C8 - 0x81300000 + 0x20) = 0x99630035;
				}
				print_debug("Patched:[%s]\n", "TDEV Revision 1.1");
				patched++;
				break;
			case 1586352:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300876 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300ACE - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300AF2 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x8130185C - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81303184 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B8D0 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B906 - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130B926 - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130B92E - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130B932 - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130B93A - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130B93E - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130B946 - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130B94A - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130B952 - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130B956 - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130B95E - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130B962 - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130B976 - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130B97A - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130B982 - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130B98E - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EE1E - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8130EE22 - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8130EE2E - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130EE32 - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130EE36 - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8130EE3A - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8130EE3E - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130EE42 - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8130EE4A - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8130EE4E - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x8130EE88 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x8130EE90 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x8130EE94 - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EE98 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x8130EE9C - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8137ECB8 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1587504:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300876 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300ACE - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x81300AF2 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81301874 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x8130319C - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				// Force text encoding.
				*(u32 *)(data + 0x8130B8E8 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.fontEncode << 1) & 2);
				
				// Force English language, the hard way.
				*(s16 *)(data + 0x8130B91E - 0x81300000 + 0x20) = 38;
				*(s16 *)(data + 0x8130B93E - 0x81300000 + 0x20) = 10;
				*(s16 *)(data + 0x8130B946 - 0x81300000 + 0x20) = 39;
				*(s16 *)(data + 0x8130B94A - 0x81300000 + 0x20) = 15;
				*(s16 *)(data + 0x8130B952 - 0x81300000 + 0x20) = 7;
				*(s16 *)(data + 0x8130B956 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x8130B95E - 0x81300000 + 0x20) = 4;
				*(s16 *)(data + 0x8130B962 - 0x81300000 + 0x20) = 45;
				*(s16 *)(data + 0x8130B96A - 0x81300000 + 0x20) = 46;
				*(s16 *)(data + 0x8130B96E - 0x81300000 + 0x20) = 42;
				*(s16 *)(data + 0x8130B976 - 0x81300000 + 0x20) = 40;
				*(s16 *)(data + 0x8130B97A - 0x81300000 + 0x20) = 43;
				*(s16 *)(data + 0x8130B98E - 0x81300000 + 0x20) = 31;
				*(s16 *)(data + 0x8130B992 - 0x81300000 + 0x20) = 29;
				*(s16 *)(data + 0x8130B99A - 0x81300000 + 0x20) = 30;
				*(s16 *)(data + 0x8130B9A6 - 0x81300000 + 0x20) = 80;
				
				if (getTVFormat() == VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130EE36 - 0x81300000 + 0x20) = 8;
					*(s16 *)(data + 0x8130EE3A - 0x81300000 + 0x20) = 15;
					*(s16 *)(data + 0x8130EE46 - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130EE4A - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130EE4E - 0x81300000 + 0x20) = 4;
					*(s16 *)(data + 0x8130EE52 - 0x81300000 + 0x20) = 13;
					*(s16 *)(data + 0x8130EE56 - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130EE5A - 0x81300000 + 0x20) = 17;
					*(s16 *)(data + 0x8130EE62 - 0x81300000 + 0x20) = 33;
					*(s16 *)(data + 0x8130EE66 - 0x81300000 + 0x20) = 50;
					*(u32 *)(data + 0x8130EEA0 - 0x81300000 + 0x20) = 0xB1630016;
					*(u32 *)(data + 0x8130EEA8 - 0x81300000 + 0x20) = 0x9943000B;
					*(u32 *)(data + 0x8130EEAC - 0x81300000 + 0x20) = 0x9BA3001C;
					*(u32 *)(data + 0x8130EEB0 - 0x81300000 + 0x20) = 0x9B830037;
					*(u32 *)(data + 0x8130EEB4 - 0x81300000 + 0x20) = 0x99630035;
					
					memcpy(data + 0x8137F138 - 0x81300000 + 0x20, BS2Pal520IntAa, sizeof(BS2Pal520IntAa));
				}
				print_debug("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1766768:
				// Don't clear OSLoMem.
				*(u32 *)(data + 0x81300474 - 0x81300000 + 0x20) = 0x60000000;
				*(u32 *)(data + 0x81300488 - 0x81300000 + 0x20) = 0x60000000;
				
				// __VIInit(newmode->viTVMode);
				*(s16 *)(data + 0x81300612 - 0x81300000 + 0x20) = newmode->viTVMode;
				
				// Accept any region code.
				*(s16 *)(data + 0x81300882 - 0x81300000 + 0x20) = 1;
				*(s16 *)(data + 0x813008A6 - 0x81300000 + 0x20) = 1;
				
				// Don't panic on field rendering.
				*(u32 *)(data + 0x81301628 - 0x81300000 + 0x20) = 0x60000000;
				
				// Force boot sound.
				*(u32 *)(data + 0x81302F50 - 0x81300000 + 0x20) = 0x38600000 | ((swissSettings.bs2Boot - 1) & 0xFFFF);
				
				if (getTVFormat() != VI_PAL) {
					// Fix logo animation speed.
					*(s16 *)(data + 0x8130F306 - 0x81300000 + 0x20) = 10;
					*(s16 *)(data + 0x8130F30A - 0x81300000 + 0x20) = 255;
					*(s16 *)(data + 0x8130F316 - 0x81300000 + 0x20) = 7;
					*(s16 *)(data + 0x8130F31A - 0x81300000 + 0x20) = 6;
					*(s16 *)(data + 0x8130F31E - 0x81300000 + 0x20) = 5;
					*(s16 *)(data + 0x8130F322 - 0x81300000 + 0x20) = 16;
					*(s16 *)(data + 0x8130F326 - 0x81300000 + 0x20) = 18;
					*(s16 *)(data + 0x8130F32A - 0x81300000 + 0x20) = 20;
					*(s16 *)(data + 0x8130F332 - 0x81300000 + 0x20) = 40;
					*(s16 *)(data + 0x8130F336 - 0x81300000 + 0x20) = 60;
					*(u32 *)(data + 0x8130F370 - 0x81300000 + 0x20) = 0xB3E30016;
					*(u32 *)(data + 0x8130F378 - 0x81300000 + 0x20) = 0x9963000B;
					*(u32 *)(data + 0x8130F37C - 0x81300000 + 0x20) = 0x9BC3001C;
					*(u32 *)(data + 0x8130F380 - 0x81300000 + 0x20) = 0x9BA30037;
					*(u32 *)(data + 0x8130F384 - 0x81300000 + 0x20) = 0x99430035;
					
					memcpy(data + 0x81382470 - 0x81300000 + 0x20, BS2Ntsc448IntAa, sizeof(BS2Ntsc448IntAa));
				}
				print_debug("Patched:[%s]\n", "PAL  Revision 1.2");
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "D58E01", 6) || !strncmp(gameID, "D59E01", 6) || !strncmp(gameID, "D62E01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4336128:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F614 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CC4C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CD00 - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005CD24 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086CCC - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086DD0 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8008789C - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "D78P01", 6) || !strncmp(gameID, "D86U01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4548544:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x800089D8 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800569C8 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80056A7C - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x80056AA0 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800815F0 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x800816F4 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80082090 - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "D86P01", 6) || !strncmp(gameID, "D89U01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4101504:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005614 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005E08 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80005F10 - 0x800055E0 + 0x25E0) = 0x60000000;
				*(u32 *)(data + 0x80005F34 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80038D24 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80038E14 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				*(u32 *)(data + 0x80038ECC - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "E22J01", 6) || !strncmp(gameID, "E23J01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4205664:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F510 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005C9A4 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CA0C - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005CA30 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086448 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8008654C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086FDC - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GC6J01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3699680:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005CA4 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDKEA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3009280:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x801F1FF2 - 0x800056C0 + 0x2600) = 128;
				} else {
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x801F1FF2 - 0x800056C0 + 0x2600) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDKJA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3075488:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x801FF53E - 0x800056C0 + 0x2600) = 128;
				} else {
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x801FF53E - 0x800056C0 + 0x2600) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDREAF", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1881216:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x8000AF34 - 0x800032C0 + 0x2C0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GDRP69", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1950240:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x8000B7EC - 0x800032E0 + 0x2E0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x800EFF3A - 0x800129E0 + 0x2520) = memSize < 32 ? 64 : memSize;
					
					*(s16 *)(data + 0x800F0082 - 0x800129E0 + 0x2520) = memSize < 32 ? 64 : memSize;
					
					*(s16 *)(data + 0x800F0502 - 0x800129E0 + 0x2520) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x800E65EE - 0x80012260 + 0x2520) = memSize < 32 ? 64 : memSize;
					
					*(s16 *)(data + 0x800E6736 - 0x80012260 + 0x2520) = memSize < 32 ? 64 : memSize;
					
					*(s16 *)(data + 0x800E6BB6 - 0x80012260 + 0x2520) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GFZE01", 6) || !strncmp(gameID, "UFZE01", 6)) && dataType == PATCH_DOL) {
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GFZJ01", 6) || !strncmp(gameID, "UFZJ01", 6)) && dataType == PATCH_DOL) {
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GFZP01", 6) || !strncmp(gameID, "UFZP01", 6)) && dataType == PATCH_DOL) {
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GGIJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 205696:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x8001501C - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 266336:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x807023F0 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GHAE08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 800672:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x800339E4 - 0x80006820 + 0x2620) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GHAJ08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1072288:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x80065FFC - 0x80006820 + 0x2620) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GHAP08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 805984:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x80033D60 - 0x80006820 + 0x2620) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GLEE08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2303296:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x80150E94 - 0x80006760 + 0x2620) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GLEJ08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2298208:
				// Fix race condition with ARAM DMA transfer.
				*(u32 *)(data + 0x8015110C - 0x80006760 + 0x2620) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GOND69", 6) || !strncmp(gameID, "GONE69", 6) || !strncmp(gameID, "GONF69", 6) || !strncmp(gameID, "GONP69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 277472:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80014C54 - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 339520:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80701C38 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GONJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 276416:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80014C54 - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 338400:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80701C38 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GOYD69", 6) || !strncmp(gameID, "GOYE69", 6) || !strncmp(gameID, "GOYF69", 6) || !strncmp(gameID, "GOYP69", 6) || !strncmp(gameID, "GOYS69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 207232:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x8001501C - 0x800134A0 + 0x4A0) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 268384:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x807023F0 - 0x807003A0 + 0x500) = 0x807E0000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 213952:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F2BE - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F2C6 - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F566 - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F56E - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 5233088:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8036F5C6 - 0x8000F740 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8036F5CE - 0x8000F740 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 214880:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F2BE - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F2C6 - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F566 - 0x8000E780 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F56E - 0x8000E780 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 5234880:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8036F1FA - 0x8000F740 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8036F202 - 0x8000F740 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 5235488:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8036F446 - 0x8000F740 + 0x2620) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8036F44E - 0x8000F740 + 0x2620) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 220800:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F742 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F74A - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4785536:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x800590F6 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x800590FE - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x80315B92 - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80315B9A - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 222048:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F606 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F60E - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4781856:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x800591A2 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x800591AA - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x80314B3A - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x80314B42 - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 221152:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8000F35E - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F366 - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				*(s16 *)(data + 0x8000F742 - 0x8000E820 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x8000F74A - 0x8000E820 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4794720:
				// Move buffer from 0x80001800 to debug monitor.
				addr = getPatchAddr(OS_RESERVED);
				
				*(s16 *)(data + 0x8005925E - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) + 0x8000) >> 16;
				*(s16 *)(data + 0x80059266 - 0x8000E680 + 0x26C0) = (((u32)addr >> 3) & 0xFFFF);
				
				*(s16 *)(data + 0x803167F2 - 0x8000E680 + 0x26C0) = ((u32)addr + 0x8000) >> 16;
				*(s16 *)(data + 0x803167FA - 0x8000E680 + 0x26C0) = ((u32)addr & 0xFFFF);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "GR8D69", 6) || !strncmp(gameID, "GR8E69", 6) || !strncmp(gameID, "GR8F69", 6) || !strncmp(gameID, "GR8P69", 6)) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 206016:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x4A0) = 0x387D2720;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 260192:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x500) = 0x387D2720;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GRZJ13", 6) && (dataType == PATCH_DOL || dataType == PATCH_ELF)) {
		switch (length) {
			case 206240:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x4A0) = 0x387D2800;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 260864:
				// Fix framebuffer initialization.
				*(u32 *)(data + 0x80500EB4 - 0x805003A0 + 0x500) = 0x387D2800;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTEA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 839584:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x8003758E - 0x8000AEA0 + 0x2520) = 128;
				} else {
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x8003758E - 0x8000AEA0 + 0x2520) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTJA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 804544:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x8003740A - 0x8000AE60 + 0x2520) = 128;
				} else {
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x8003740A - 0x8000AE60 + 0x2520) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GWTPA4", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 834912:
				// Accept Memory Card 2043 or 1019 instead of Memory Card 507.
				if (devices[DEVICE_CUR]->emulated() & EMU_MEMCARD) {
					*(s16 *)(data + 0x800374D6 - 0x8000AE60 + 0x2520) = 128;
				} else {
					s32 memSize = 0;
					
					while (CARD_ProbeEx(CARD_SLOTA, &memSize, NULL) == CARD_ERROR_BUSY);
					
					*(s16 *)(data + 0x800374D6 - 0x8000AE60 + 0x2520) = memSize < 32 ? 64 : memSize;
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4336128:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F614 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CC4C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CD00 - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005CD24 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086CCC - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086DD0 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8008789C - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "NXXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4205664:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x8000F510 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005C9A4 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8005CA0C - 0x800056A0 + 0x2600) = 0x60000000;
				*(u32 *)(data + 0x8005CA30 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086448 - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x8008654C - 0x800056A0 + 0x2600) = 0x60000000;
				
				*(u32 *)(data + 0x80086FDC - 0x800056A0 + 0x2600) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PCKJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3771872:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005DD0 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "PCSJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3771872:
				// Strip anti-debugging code.
				*(u32 *)(data + 0x80005DD0 - 0x800055E0 + 0x25E0) = 0x60000000;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
	
	if (!strcmp(fileName, "boot.bin")) {
		DiskHeader *diskHeader = (DiskHeader *) data;
		diskHeader->DVDMagicWord = DVD_MAGIC;
		memset(data + 0x200, 0, 0x200);
		diskHeader->MaxFSTSize = GCMDisk.MaxFSTSize;
		
		print_debug("Patched:[%s]\n", fileName);
		patched++;
		
		data    += 0x440;
		length  -= 0x440;
		fileName = "bi2.bin";
	}
	if (!strncmp(gameID, "DPOJ8P", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x4) = 0x1800000;
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GHAE08", 6)) {
		if (!strcasecmp(fileName, "claire.rel")) {
			*(u32 *)(data + 0x190294) = 0x60000000;
			
			*(u8 *)(data + 0x1EFDF6) = R_PPC_NONE;
			*(u32 *)(data + 0x1EFDF8) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon.rel")) {
			*(u32 *)(data + 0x1903FC) = 0x60000000;
			
			*(u8 *)(data + 0x1EFF86) = R_PPC_NONE;
			*(u32 *)(data + 0x1EFF88) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GHAJ08", 6)) {
		if (!strcasecmp(fileName, "claire.rel")) {
			*(u32 *)(data + 0x1903FC) = 0x60000000;
			
			*(u8 *)(data + 0x1FD19A) = R_PPC_NONE;
			*(u32 *)(data + 0x1FD19C) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon.rel")) {
			*(u32 *)(data + 0x1904C4) = 0x60000000;
			
			*(u8 *)(data + 0x1FD30A) = R_PPC_NONE;
			*(u32 *)(data + 0x1FD30C) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GHAP08", 6)) {
		if (!strcasecmp(fileName, "claire.rel")) {
			*(u32 *)(data + 0x190710) = 0x60000000;
			
			*(u8 *)(data + 0x1F0FBE) = R_PPC_NONE;
			*(u32 *)(data + 0x1F0FC0) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "claire_f.rel")) {
			*(u32 *)(data + 0x1912E8) = 0x60000000;
			
			*(u8 *)(data + 0x1F251E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F2520) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "claire_g.rel")) {
			*(u32 *)(data + 0x1905A8) = 0x60000000;
			
			*(u8 *)(data + 0x1F1AE6) = R_PPC_NONE;
			*(u32 *)(data + 0x1F1AE8) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "claire_i.rel")) {
			*(u32 *)(data + 0x19113C) = 0x60000000;
			
			*(u8 *)(data + 0x1F269E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F26A0) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "claire_s.rel")) {
			*(u32 *)(data + 0x1912E4) = 0x60000000;
			
			*(u8 *)(data + 0x1F281E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F2820) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon.rel")) {
			*(u32 *)(data + 0x190878) = 0x60000000;
			
			*(u8 *)(data + 0x1F116E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F1170) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon_f.rel")) {
			*(u32 *)(data + 0x191408) = 0x60000000;
			
			*(u8 *)(data + 0x1F25EE) = R_PPC_NONE;
			*(u32 *)(data + 0x1F25F0) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon_g.rel")) {
			*(u32 *)(data + 0x190638) = 0x60000000;
			
			*(u8 *)(data + 0x1F1AF6) = R_PPC_NONE;
			*(u32 *)(data + 0x1F1AF8) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon_i.rel")) {
			*(u32 *)(data + 0x19125C) = 0x60000000;
			
			*(u8 *)(data + 0x1F278E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F2790) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "leon_s.rel")) {
			*(u32 *)(data + 0x191380) = 0x60000000;
			
			*(u8 *)(data + 0x1F286E) = R_PPC_NONE;
			*(u32 *)(data + 0x1F2870) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GHQE7D", 6) || !strncmp(gameID, "GHQP7D", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x24) = 5;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GLEP08", 6)) {
		if (!strcasecmp(fileName, "eng.rel")) {
			*(u32 *)(data + 0x14C354) = 0x60000000;
			
			*(u8 *)(data + 0x1BFCE6) = R_PPC_NONE;
			*(u32 *)(data + 0x1BFCE8) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "fra.rel")) {
			*(u32 *)(data + 0x14D21C) = 0x60000000;
			
			*(u8 *)(data + 0x1C0FE6) = R_PPC_NONE;
			*(u32 *)(data + 0x1C0FE8) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "ger.rel")) {
			*(u32 *)(data + 0x14D020) = 0x60000000;
			
			*(u8 *)(data + 0x1C1886) = R_PPC_NONE;
			*(u32 *)(data + 0x1C1888) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "ita.rel")) {
			*(u32 *)(data + 0x14D084) = 0x60000000;
			
			*(u8 *)(data + 0x1C1246) = R_PPC_NONE;
			*(u32 *)(data + 0x1C1248) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		} else if (!strcasecmp(fileName, "spa.rel")) {
			*(u32 *)(data + 0x14D204) = 0x60000000;
			
			*(u8 *)(data + 0x1C1386) = R_PPC_NONE;
			*(u32 *)(data + 0x1C1388) = 0;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GMRE70", 6) || !strncmp(gameID, "GMRP70", 6)) {
		if (!strcmp(fileName, "bi2.bin")) {
			*(u32 *)(data + 0x24) = 5;
			
			print_debug("Patched:[%s]\n", fileName);
			patched++;
		}
	} else if (!strncmp(gameID, "GS8P7D", 6)) {
		if (!strcasecmp(fileName, "SPYROCFG_NGC.CFG")) {
			if (in_range(swissSettings.gameVMode, 1, 7)) {
				addr = strnstr(data, "\tHeight:\t\t\t496\r\n", length);
				if (addr) memcpy(addr, "\tHeight:\t\t\t448\r\n", 16);
			}
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			
			print_debug("Patched:[%s]\n", fileName);
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
			case 1435200:
				*(s16 *)(data + 0x813007C2 - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1448280:
				*(s16 *)(data + 0x813007DA - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x81301422 - 0x81300000 + 0x20) = 0x0C00;
				*(s16 *)(data + 0x81301476 - 0x81300000 + 0x20) = 0x0C00;
				*(u32 *)(data + 0x81301484 - 0x81300000 + 0x20) = 0x80050004;
				*(u32 *)(data + 0x813014A8 - 0x81300000 + 0x20) = 0x90050004;
				
				*(s16 *)(data + 0x8130170E - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x813019AE - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "NTSC Revision 1.0");
				patched++;
				break;
			case 1449848:
				*(s16 *)(data + 0x81300B4E - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x81301762 - 0x81300000 + 0x20) = 0x0C00;
				*(s16 *)(data + 0x813017B6 - 0x81300000 + 0x20) = 0x0C00;
				*(u32 *)(data + 0x813017C4 - 0x81300000 + 0x20) = 0x80050004;
				*(u32 *)(data + 0x813017E8 - 0x81300000 + 0x20) = 0x90050004;
				
				*(s16 *)(data + 0x81301A4E - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x81301CEE - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "DEV  Revision 1.0");
				patched++;
				break;
			case 1583088:
				*(s16 *)(data + 0x813005EA - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "NTSC Revision 1.1");
				patched++;
				break;
			case 1763048:
				*(s16 *)(data + 0x813005EA - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1760152:
				*(s16 *)(data + 0x8130082E - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x81301496 - 0x81300000 + 0x20) = 0x0C00;
				*(s16 *)(data + 0x813014E6 - 0x81300000 + 0x20) = 0x0C00;
				*(u32 *)(data + 0x813014F4 - 0x81300000 + 0x20) = 0x80050004;
				*(u32 *)(data + 0x81301518 - 0x81300000 + 0x20) = 0x90050004;
				
				*(s16 *)(data + 0x81301772 - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x813018A6 - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "PAL  Revision 1.0");
				patched++;
				break;
			case 1561776:
				*(s16 *)(data + 0x813005EA - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "MPAL Revision 1.1");
				patched++;
				break;
			case 1607576:
				*(s16 *)(data + 0x81300A5A - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x8130221A - 0x81300000 + 0x20) = 0x0C00;
				*(s16 *)(data + 0x8130226E - 0x81300000 + 0x20) = 0x0C00;
				*(u32 *)(data + 0x8130227C - 0x81300000 + 0x20) = 0x80050004;
				*(u32 *)(data + 0x813022A0 - 0x81300000 + 0x20) = 0x90050004;
				
				*(s16 *)(data + 0x81302506 - 0x81300000 + 0x20) = 0x0C00;
				
				*(s16 *)(data + 0x813027A6 - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "TDEV Revision 1.1");
				patched++;
				break;
			case 1586352:
				*(s16 *)(data + 0x81300926 - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1587504:
				*(s16 *)(data + 0x81300926 - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "NTSC Revision 1.2");
				patched++;
				break;
			case 1766768:
				*(s16 *)(data + 0x813006DA - 0x81300000 + 0x20) = 0x0C00;
				
				print_debug("Patched:[%s]\n", "PAL  Revision 1.2");
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "D58E01", 6) || !strncmp(gameID, "D59E01", 6) || !strncmp(gameID, "D62E01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4336128:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AE366 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "D78P01", 6) || !strncmp(gameID, "D86U01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4548544:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802A95B2 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if ((!strncmp(gameID, "E22J01", 6) || !strncmp(gameID, "E23J01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4205664:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AB256 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4333056:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AE0F2 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
			case 4336128:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AE366 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4191264:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802A8B1A - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "GXXP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4573216:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802B0046 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
				patched++;
				break;
		}
	} else if (!strncmp(gameID, "NXXJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4205664:
				// Move up DSI exception handler.
				*(s16 *)(data + 0x802AB256 - 0x800056A0 + 0x2600) = 0x0304;
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
	if ((!strncmp(gameID, "GAEE01", 6) || !strncmp(gameID, "GAEJ01", 6)) && dataType == PATCH_DOL) {
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GAFE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 910368:
				*(s16 *)(data + 0x8000672E - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006732 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006736 - 0x800056C0 + 0x2620) = 640;
				*(s16 *)(data + 0x8000673A - 0x800056C0 + 0x2620) = 480;
				
				*(s16 *)(data + 0x80006EF2 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006EF6 - 0x800056C0 + 0x2620) = 0;
				*(s16 *)(data + 0x80006EFA - 0x800056C0 + 0x2620) = 640;
				*(s16 *)(data + 0x80006EFE - 0x800056C0 + 0x2620) = 480;
				
				SET_VI_WIDTH(data + 0x800AFE58 - 0x800AD3C0 + 0xAA3C0, 640);
				SET_VI_WIDTH(data + 0x800AFE94 - 0x800AD3C0 + 0xAA3C0, 640);
				SET_VI_WIDTH(data + 0x800AFED0 - 0x800AD3C0 + 0xAA3C0, 640);
				SET_VI_WIDTH(data + 0x800AFF0C - 0x800AD3C0 + 0xAA3C0, 640);
				SET_VI_WIDTH(data + 0x800AFF48 - 0x800AD3C0 + 0xAA3C0, 704);
				SET_VI_WIDTH(data + 0x800AFF84 - 0x800AD3C0 + 0xAA3C0, 704);
				
				SET_VI_WIDTH(data + 0x800E15B0 - 0x800AD3C0 + 0xAA3C0, 704);
				SET_VI_WIDTH(data + 0x800E15EC - 0x800AD3C0 + 0xAA3C0, 704);
				SET_VI_WIDTH(data + 0x800E1628 - 0x800AD3C0 + 0xAA3C0, 704);
				SET_VI_WIDTH(data + 0x800E16A0 - 0x800AD3C0 + 0xAA3C0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GB4E51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2013952:
				SET_VI_WIDTH(data + 0x801E42B0 - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E42EC - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E4328 - 0x80199FA0 + 0x196F40, 704);
				SET_VI_WIDTH(data + 0x801E43A0 - 0x80199FA0 + 0x196F40, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GB4P51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2013600:
				SET_VI_WIDTH(data + 0x801E4170 - 0x80199F20 + 0x196EE0, 704);
				SET_VI_WIDTH(data + 0x801E41AC - 0x80199F20 + 0x196EE0, 704);
				SET_VI_WIDTH(data + 0x801E4224 - 0x80199F20 + 0x196EE0, 704);
				
				if (in_range(swissSettings.gameVMode, 1, 7))
					*(s16 *)(data + 0x80008BD2 - 0x80005800 + 0x2620) = 0;
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GBOE51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1934336:
				SET_VI_WIDTH(data + 0x801D3588 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D35C4 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D3600 - 0x80189060 + 0x186040, 704);
				SET_VI_WIDTH(data + 0x801D3678 - 0x80189060 + 0x186040, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GBOP51", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1934688:
				SET_VI_WIDTH(data + 0x801D3708 - 0x80189180 + 0x186180, 704);
				SET_VI_WIDTH(data + 0x801D3744 - 0x80189180 + 0x186180, 704);
				SET_VI_WIDTH(data + 0x801D37BC - 0x80189180 + 0x186180, 704);
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x8000AA0E - 0x800057E0 + 0x2600) = 0;
					
					*(s16 *)(data + 0x800ADF46 - 0x800057E0 + 0x2600) = (0x801D37BC + 0x8000) >> 16;
					*(s16 *)(data + 0x800ADF4E - 0x800057E0 + 0x2600) = (0x801D37BC & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEDP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3183200:
				SET_VI_WIDTH(data + 0x80301140 - 0x8023B920 + 0x238920, 704);
				
				SET_VI_WIDTH(data + 0x80303BD8 - 0x8023B920 + 0x238920, 704);
				SET_VI_WIDTH(data + 0x80303C14 - 0x8023B920 + 0x238920, 704);
				SET_VI_WIDTH(data + 0x80303C8C - 0x8023B920 + 0x238920, 704);
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x80018436 - 0x800068E0 + 0x2600) = (0x80301140 + 0x8000) >> 16;
					*(s16 *)(data + 0x80018442 - 0x800068E0 + 0x2600) = (0x80301140 & 0xFFFF);
					
					*(u32 *)(data + 0x8061EB08 - 0x8061D420 + 0x303FA0) = 0;
					
					*(u32 *)(data + 0x8061EB18 - 0x8061D420 + 0x303FA0) = 0x80301140;
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEME7F", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1483520:
				SET_VI_WIDTH(data + 0x80167190 - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x801671CC - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x80167208 - 0x800945C0 + 0x915C0, 704);
				SET_VI_WIDTH(data + 0x80167280 - 0x800945C0 + 0x915C0, 704);
				
				if (swissSettings.gameVMode != 0) {
					*(s16 *)(data + 0x800331BE - 0x80005740 + 0x2520) = 448;
					*(u32 *)(data + 0x800331FC - 0x80005740 + 0x2520) = 0x60000000;
					
					*(u32 *)(data + 0x806D0898 - 0x806D06C0 + 0x169120) = 0x801671CC;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GEMJ28", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 1483264:
				SET_VI_WIDTH(data + 0x80167068 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x801670A4 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x801670E0 - 0x80094500 + 0x91500, 704);
				SET_VI_WIDTH(data + 0x80167158 - 0x80094500 + 0x91500, 704);
				
				if (swissSettings.gameVMode != 0) {
					*(s16 *)(data + 0x80033062 - 0x80005740 + 0x2520) = 448;
					*(u32 *)(data + 0x800330A0 - 0x80005740 + 0x2520) = 0x60000000;
					
					*(u32 *)(data + 0x806D0660 - 0x806D0480 + 0x169000) = 0x801670A4;
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
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
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7E08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2866304:
				*(s16 *)(data + 0x800802BE - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x800802C6 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x800802F6 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x800802FE - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80080326 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8008032E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8008060A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8008063E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80080642 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8008067A - 0x800056C0 + 0x2600) = 6;
				
				*(s16 *)(data + 0x8016420A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016420E - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8016424A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164252 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80164272 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8016427A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164386 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x801643CA - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164412 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164456 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80164922 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016495E - 0x800056C0 + 0x2600) = 6;
				
				*(s16 *)(data + 0x80164AB2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164AB6 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80164AF2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164AFA - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80164B1A - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80164B22 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164FC2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80164FFE - 0x800056C0 + 0x2600) = 6;
				
				SET_VI_WIDTH(data + 0x802B0FF8 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B1034 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B1070 - 0x80269E80 + 0x266E80, 704);
				SET_VI_WIDTH(data + 0x802B10E8 - 0x80269E80 + 0x266E80, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7J08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2891072:
				*(s16 *)(data + 0x80081FDE - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80081FE6 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80082016 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8008201E - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80082046 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8008204E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8008232A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8008235E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80082362 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x8008239A - 0x800056C0 + 0x2600) = 6;
				
				*(s16 *)(data + 0x80166CAA - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80166CAE - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80166CEA - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80166CF2 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80166D12 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80166D1A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80166E26 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80166E6A - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80166EB2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80166EF6 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x801673C2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x801673FE - 0x800056C0 + 0x2600) = 6;
				
				*(s16 *)(data + 0x80167552 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167556 - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x80167592 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016759A - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x801675BA - 0x800056C0 + 0x2600) = 6;
				*(s16 *)(data + 0x801675C2 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167A62 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167A9E - 0x800056C0 + 0x2600) = 6;
				
				SET_VI_WIDTH(data + 0x802B7078 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B70B4 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B70F0 - 0x8026C680 + 0x269680, 704);
				SET_VI_WIDTH(data + 0x802B7168 - 0x8026C680 + 0x269680, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GK7P08", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 2961056:
				*(s16 *)(data + 0x80080BB6 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80080BEE - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80080C26 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80080F02 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80080F36 - 0x800056C0 + 0x2600) = 4;
				
				*(s16 *)(data + 0x80167C66 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167CA6 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167CD6 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167E26 - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80167E6E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016837E - 0x800056C0 + 0x2600) = 4;
				
				*(s16 *)(data + 0x8016850E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016854E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x8016857E - 0x800056C0 + 0x2600) = 4;
				*(s16 *)(data + 0x80168A1E - 0x800056C0 + 0x2600) = 4;
				
				SET_VI_WIDTH(data + 0x8027D278 - 0x8027D200 + 0x27A200, 704);
				
				SET_VI_WIDTH(data + 0x802C81B8 - 0x8027D200 + 0x27A200, 704);
				SET_VI_WIDTH(data + 0x802C81F4 - 0x8027D200 + 0x27A200, 704);
				SET_VI_WIDTH(data + 0x802C826C - 0x8027D200 + 0x27A200, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GLME01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3799584:
				SET_VI_WIDTH(data + 0x802181D0 - 0x80218100 + 0x215100, 704);
				
				SET_VI_WIDTH(data + 0x80395C40 - 0x80218100 + 0x215100, 704);
				SET_VI_WIDTH(data + 0x80395C7C - 0x80218100 + 0x215100, 704);
				SET_VI_WIDTH(data + 0x80395CB8 - 0x80218100 + 0x215100, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GLMJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3754304:
				SET_VI_WIDTH(data + 0x8020D4B0 - 0x8020D3E0 + 0x20A3E0, 704);
				
				SET_VI_WIDTH(data + 0x8038ABA0 - 0x8020D3E0 + 0x20A3E0, 704);
				SET_VI_WIDTH(data + 0x8038ABDC - 0x8020D3E0 + 0x20A3E0, 704);
				SET_VI_WIDTH(data + 0x8038AC18 - 0x8020D3E0 + 0x20A3E0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x80007596 - 0x800057C0 + 0x2520) = 116;
					
					*(u32 *)(data + 0x804BD630 - 0x804BD620 + 0x384E80) = 0x803908D4;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPIE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3102432:
				SET_VI_WIDTH(data + 0x802A5664 - 0x80222D20 + 0x21FD20, 704);
				SET_VI_WIDTH(data + 0x802A56A0 - 0x80222D20 + 0x21FD20, 704);
				
				SET_VI_WIDTH(data + 0x802E8D28 - 0x80222D20 + 0x21FD20, 704);
				SET_VI_WIDTH(data + 0x802E8D64 - 0x80222D20 + 0x21FD20, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
			case 3102592:
				SET_VI_WIDTH(data + 0x802A5704 - 0x80222DC0 + 0x21FDC0, 704);
				SET_VI_WIDTH(data + 0x802A5740 - 0x80222DC0 + 0x21FDC0, 704);
				
				SET_VI_WIDTH(data + 0x802E8DC8 - 0x80222DC0 + 0x21FDC0, 704);
				SET_VI_WIDTH(data + 0x802E8E04 - 0x80222DC0 + 0x21FDC0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPIJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3127584:
				SET_VI_WIDTH(data + 0x802AABCC - 0x80227EA0 + 0x224EA0, 704);
				SET_VI_WIDTH(data + 0x802AAC08 - 0x80227EA0 + 0x224EA0, 704);
				
				SET_VI_WIDTH(data + 0x802EEF08 - 0x80227EA0 + 0x224EA0, 704);
				SET_VI_WIDTH(data + 0x802EEF44 - 0x80227EA0 + 0x224EA0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
			case 3128832:
				SET_VI_WIDTH(data + 0x802AB0AC - 0x80228380 + 0x225380, 704);
				SET_VI_WIDTH(data + 0x802AB0E8 - 0x80228380 + 0x225380, 704);
				
				SET_VI_WIDTH(data + 0x802EF3E8 - 0x80228380 + 0x225380, 704);
				SET_VI_WIDTH(data + 0x802EF424 - 0x80228380 + 0x225380, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(u32 *)(data + 0x803E24F0 - 0x803E1C60 + 0x2E9380) = 0x802A7CB4;
					
					*(f32 *)(data + 0x803ED5B0 - 0x803ED120 + 0x2F3DE0) = 1.0f;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GPTP41", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4017536:
				if (in_range(swissSettings.gameVMode, 1, 7)) {
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
				print_debug("Patched:[%.6s]\n", gameID);
				break;
			case 639584:
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x8002E65E - 0x8000B400 + 0x2600) = (0x80098534 + 0x8000) >> 16;
					*(s16 *)(data + 0x8002E666 - 0x8000B400 + 0x2600) = (0x80098534 & 0xFFFF);
					*(s16 *)(data + 0x8002E676 - 0x8000B400 + 0x2600) = 40;
					*(s16 *)(data + 0x8002E67E - 0x8000B400 + 0x2600) = 40;
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if ((!strncmp(gameID, "GPZE01", 6) || !strncmp(gameID, "GPZJ01", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 803424:
				*(s16 *)(data + 0x80006C72 - 0x80006780 + 0x2540) = 0;
				*(s16 *)(data + 0x80006C7A - 0x80006780 + 0x2540) = 640;
				
				*(s16 *)(data + 0x8000C1FA - 0x80006780 + 0x2540) = 0;
				*(s16 *)(data + 0x8000C202 - 0x80006780 + 0x2540) = 640;
				
				*(s16 *)(data + 0x8000C3DA - 0x80006780 + 0x2540) = 0;
				*(s16 *)(data + 0x8000C3E2 - 0x80006780 + 0x2540) = 640;
				
				SET_VI_WIDTH(data + 0x800C66D0 - 0x800BDCA0 + 0xBACA0, 704);
				SET_VI_WIDTH(data + 0x800C670C - 0x800BDCA0 + 0xBACA0, 704);
				SET_VI_WIDTH(data + 0x800C6748 - 0x800BDCA0 + 0xBACA0, 704);
				SET_VI_WIDTH(data + 0x800C67C0 - 0x800BDCA0 + 0xBACA0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x80020FFA - 0x800066E0 + 0x2620) = (0x8032FEAC + 0x8000) >> 16;
					*(s16 *)(data + 0x80020FFE - 0x800066E0 + 0x2620) = (0x8032FEAC & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
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
				
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x80020FFA - 0x800066E0 + 0x2620) = (0x8033006C + 0x8000) >> 16;
					*(s16 *)(data + 0x80020FFE - 0x800066E0 + 0x2620) = (0x8033006C & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if ((!strncmp(gameID, "GVLD69", 6) || !strncmp(gameID, "GVLF69", 6) || !strncmp(gameID, "GVLP69", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 5309888:
				*(s16 *)(data + 0x80219932 - 0x800034A0 + 0x4A0) = 384;
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GVLE69", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 5304256:
				*(s16 *)(data + 0x80219732 - 0x800034A0 + 0x4A0) = 384;
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3431456:
				SET_VI_WIDTH(data + 0x80341420 - 0x80173DA0 + 0x170DA0, 704);
				SET_VI_WIDTH(data + 0x8034145C - 0x80173DA0 + 0x170DA0, 704);
				SET_VI_WIDTH(data + 0x80341498 - 0x80173DA0 + 0x170DA0, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3418464:
				SET_VI_WIDTH(data + 0x8033E1E0 - 0x80170880 + 0x16D880, 704);
				SET_VI_WIDTH(data + 0x8033E21C - 0x80170880 + 0x16D880, 704);
				SET_VI_WIDTH(data + 0x8033E258 - 0x80170880 + 0x16D880, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GWRP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3442880:
				SET_VI_WIDTH(data + 0x80252AC0 - 0x80176760 + 0x173760, 704);
				
				SET_VI_WIDTH(data + 0x80343FC0 - 0x80176760 + 0x173760, 704);
				SET_VI_WIDTH(data + 0x80343FFC - 0x80176760 + 0x173760, 704);
				SET_VI_WIDTH(data + 0x80344038 - 0x80176760 + 0x173760, 704);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if ((!strncmp(gameID, "GX2D52", 6) || !strncmp(gameID, "GX2P52", 6) || !strncmp(gameID, "GX2S52", 6)) && dataType == PATCH_DOL) {
		switch (length) {
			case 4055712:
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x80010A1A - 0x80005760 + 0x2540) = (0x80368D5C + 0x8000) >> 16;
					*(s16 *)(data + 0x80010A1E - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x80010A22 - 0x80005760 + 0x2540) = (0x80368D5C & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLP52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4182304:
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x80384784 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x80384784 & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GXLX52", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 4182624:
				if (in_range(swissSettings.gameVMode, 1, 7)) {
					*(s16 *)(data + 0x8000E9F6 - 0x80005760 + 0x2540) = (0x803848C4 + 0x8000) >> 16;
					*(s16 *)(data + 0x8000E9FA - 0x80005760 + 0x2540) = VI_EURGB60;
					*(s16 *)(data + 0x8000E9FE - 0x80005760 + 0x2540) = (0x803848C4 & 0xFFFF);
				}
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GZLE01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3822272:
				SET_VI_WIDTH(data + 0x803716F0 - 0x80371580 + 0x36E580, 676);
				SET_VI_WIDTH(data + 0x8037172C - 0x80371580 + 0x36E580, 676);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GZLJ01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3770400:
				SET_VI_WIDTH(data + 0x80364B90 - 0x80364A20 + 0x361A20, 676);
				SET_VI_WIDTH(data + 0x80364BCC - 0x80364A20 + 0x361A20, 676);
				
				print_debug("Patched:[%.6s]\n", gameID);
				break;
		}
	} else if (!strncmp(gameID, "GZLP01", 6) && dataType == PATCH_DOL) {
		switch (length) {
			case 3850240:
				*(s16 *)(data + 0x800085C2 - 0x800056E0 + 0x2620) = 545;
				
				*(u16 *)(data + 0x803783D8 - 0x80378260 + 0x375260) = 545;
				*(u16 *)(data + 0x803783DC - 0x80378260 + 0x375260) = 15;
				*(u16 *)(data + 0x803783E0 - 0x80378260 + 0x375260) = 545;
				
				SET_VI_WIDTH(data + 0x803783D0 - 0x80378260 + 0x375260, 640);
				SET_VI_WIDTH(data + 0x8037840C - 0x80378260 + 0x375260, 676);
				
				print_debug("Patched:[%.6s]\n", gameID);
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
	FuncPattern OSGetConsoleTypeSigs[3] = {
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
	FuncPattern OSGetFontEncodeSigs[3] = {
		{ 31, 8, 2, 1, 7, 2, NULL, 0, "OSGetFontEncodeD" },
		{ 22, 6, 0, 0, 6, 0, NULL, 0, "OSGetFontEncode" },
		{ 18, 4, 0, 0, 4, 0, NULL, 0, "OSGetFontEncode" }	// SN Systems ProDG
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
	FuncPattern InitializeUARTSigs[6] = {
		{ 18,  7, 4, 1, 1, 2, NULL, 0, "InitializeUARTD" },
		{ 28, 12, 6, 1, 2, 2, NULL, 0, "InitializeUARTD" },
		{ 16,  6, 3, 1, 1, 2, NULL, 0, "InitializeUART" },
		{ 18,  7, 4, 1, 1, 2, NULL, 0, "InitializeUART" },
		{ 28, 12, 6, 1, 2, 2, NULL, 0, "InitializeUART" },
		{ 28, 12, 6, 1, 2, 2, NULL, 0, "InitializeUART" }	// SN Systems ProDG
	};
	FuncPattern QueueLengthSig = 
		{ 38, 21, 3, 6, 1, 2, NULL, 0, "QueueLengthD" };
	FuncPattern WriteUARTNSigs[6] = {
		{  99, 28, 3,  9, 17,  8, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTND" },
		{ 105, 28, 3, 12, 17,  9, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTND" },
		{ 128, 48, 4, 14, 18,  7, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTN" },
		{ 128, 47, 4, 14, 18,  7, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTN" },
		{ 135, 47, 4, 17, 18,  9, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTN" },
		{ 136, 46, 4, 19, 17, 11, WriteUARTNPatch, WriteUARTNPatchLength, "WriteUARTN" }	// SN Systems ProDG
	};
	FuncPattern SISetXYSigs[5] = {
		{ 49, 15, 4, 5, 3, 4, NULL, 0, "SISetXYD" },
		{ 51, 16, 5, 5, 3, 4, NULL, 0, "SISetXYD" },
		{ 24,  7, 5, 2, 0, 3, NULL, 0, "SISetXY" },
		{ 27, 10, 6, 2, 0, 3, NULL, 0, "SISetXY" },
		{ 24,  7, 6, 2, 0, 3, NULL, 0, "SISetXY" }	// SN Systems ProDG
	};
	FuncPattern SIGetResponseSigs[6] = {
		{ 36, 13, 4, 1, 2,  4, NULL, 0, "SIGetResponseD" },
		{ 48, 12, 5, 4, 3,  7, NULL, 0, "SIGetResponseD" },
		{  9,  4, 2, 0, 0,  1, NULL, 0, "SIGetResponse" },
		{ 52, 13, 8, 3, 2, 10, NULL, 0, "SIGetResponse" },
		{ 49, 13, 8, 3, 2,  7, NULL, 0, "SIGetResponse" },
		{ 74, 26, 9, 4, 3, 14, NULL, 0, "SIGetResponse" }	// SN Systems ProDG
	};
	FuncPattern SISetSamplingRateSigs[7] = {
		{ 49, 11, 2, 5,  9, 5, NULL, 0, "PADSetSamplingRateD" },
		{ 62, 14, 3, 7, 11, 6, NULL, 0, "PADSetSamplingRateD" },
		{ 62, 11, 3, 6, 13, 6, NULL, 0, "SISetSamplingRateD" },
		{ 44, 10, 5, 4,  8, 4, NULL, 0, "PADSetSamplingRate" },
		{ 59, 14, 7, 6, 10, 6, NULL, 0, "PADSetSamplingRate" },
		{ 57, 12, 6, 5, 12, 6, NULL, 0, "SISetSamplingRate" },
		{ 54, 12, 6, 5, 10, 7, NULL, 0, "SISetSamplingRate" }	// SN Systems ProDG
	};
	FuncPattern SIRefreshSamplingRateSigs[3] = {
		{  9,  3, 2, 1,  0, 2, NULL, 0, "PADRefreshSamplingRate" },
		{  9,  3, 2, 1,  0, 2, NULL, 0, "SIRefreshSamplingRate" },
		{ 54, 13, 6, 5, 10, 6, NULL, 0, "SIRefreshSamplingRate" }	// SN Systems ProDG
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
	FuncPattern TimeoutHandlerSigs[4] = {
		{ 50, 18, 4, 2, 7, 3, NULL, 0, "TimeoutHandlerD" },
		{ 38, 15, 5, 1, 3, 5, NULL, 0, "TimeoutHandler" },
		{ 41, 16, 5, 1, 4, 5, NULL, 0, "TimeoutHandler" },
		{ 40, 14, 5, 1, 4, 6, NULL, 0, "TimeoutHandler" }	// SN Systems ProDG
	};
	FuncPattern SetupTimeoutAlarmSigs[4] = {
		{ 61, 20, 3, 3,  8, 17, NULL, 0, "SetupTimeoutAlarmD" },
		{ 91, 28, 3, 4, 10, 30, NULL, 0, "SetupTimeoutAlarmD" },
		{ 62, 20, 3, 3,  8, 14, NULL, 0, "SetupTimeoutAlarm" },
		{ 91, 27, 3, 4, 10, 25, NULL, 0, "SetupTimeoutAlarm" }
	};
	FuncPattern RetrySigs[6] = {
		{  98, 34, 2, 15,  8,  3, NULL, 0, "RetryD" },
		{  98, 33, 2, 15,  8,  3, NULL, 0, "RetryD" },
		{ 123, 40, 4, 15, 11, 13, NULL, 0, "Retry" },
		{ 139, 48, 4, 16, 14, 14, NULL, 0, "Retry" },
		{ 133, 45, 4, 16, 13, 13, NULL, 0, "Retry" },	// SN Systems ProDG
		{ 168, 54, 4, 17, 16, 25, NULL, 0, "Retry" }
	};
	FuncPattern __CARDStartSigs[7] = {
		{  65, 24, 6,  5,  7,  2, NULL, 0, "__CARDStartD" },
		{  70, 20, 6,  7,  7,  3, NULL, 0, "__CARDStartD" },
		{  88, 30, 8,  5, 10, 13, NULL, 0, "__CARDStart" },
		{ 104, 38, 8,  6, 13, 14, NULL, 0, "__CARDStart" },
		{ 181, 56, 8, 17, 27, 13, NULL, 0, "__CARDStart" },	// SN Systems ProDG
		{ 109, 32, 6,  8, 13, 14, NULL, 0, "__CARDStart" },
		{ 137, 37, 6,  9, 15, 25, NULL, 0, "__CARDStart" }
	};
	FuncPattern __CARDGetFontEncodeSig = 
		{ 2, 0, 0, 0, 0, 0, NULL, 0, "__CARDGetFontEncode" };
	FuncPattern __CARDGetControlBlockSigs[5] = {
		{ 46, 11, 6, 2, 5, 4, NULL, 0, "__CARDGetControlBlockD" },
		{ 46, 11, 6, 2, 5, 4, NULL, 0, "__CARDGetControlBlockD" },
		{ 44, 12, 7, 2, 5, 4, NULL, 0, "__CARDGetControlBlock" },
		{ 46, 13, 8, 2, 5, 2, NULL, 0, "__CARDGetControlBlock" },
		{ 46, 13, 8, 2, 5, 3, NULL, 0, "__CARDGetControlBlock" }	// SN Systems ProDG
	};
	FuncPattern __CARDPutControlBlockSigs[5] = {
		{ 29, 8, 3, 3, 1, 3, NULL, 0, "__CARDPutControlBlockD" },
		{ 34, 9, 4, 3, 2, 3, NULL, 0, "__CARDPutControlBlockD" },
		{ 20, 5, 5, 2, 1, 2, NULL, 0, "__CARDPutControlBlock" },
		{ 25, 6, 6, 2, 2, 2, NULL, 0, "__CARDPutControlBlock" },
		{ 25, 6, 6, 2, 2, 4, NULL, 0, "__CARDPutControlBlock" }	// SN Systems ProDG
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
	FuncPattern DoMountSigs[12] = {
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
		{ 288, 50, 11, 19, 36, 24, NULL, 0, "DoMount" },	// SN Systems ProDG
		{ 277, 69, 11, 22, 27, 35, NULL, 0, "DoMount" }
	};
	_SDA2_BASE_ = _SDA_BASE_ = 0;
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if ((data[i + 0] & 0xFFFF0000) == 0x3C400000 &&
			(data[i + 1] & 0xFFFF0000) == 0x60420000 &&
			(data[i + 2] & 0xFFFF0000) == 0x3DA00000 &&
			(data[i + 3] & 0xFFFF0000) == 0x61AD0000) {
			get_immediate(data, i + 0, i + 1, &_SDA2_BASE_);
			get_immediate(data, i + 2, i + 3, &_SDA_BASE_);
			break;
		}
	}
	
	for (i = 0; i < length / sizeof(u32); i++) {
		if ((data[i + 0] != 0x7C0802A6 && (data[i + 0] & 0xFFFF0000) != 0x94210000) ||
			(data[i - 1] != 0x4E800020 &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x4E800020) &&
			(data[i - 1] != 0x00000000 || data[i - 2] != 0x00000000 || data[i - 3] != 0x4E800020) &&
			 data[i - 1] != 0x4C000064 &&
			(data[i - 1] != 0x60000000 || data[i - 2] != 0x4C000064) &&
			branchResolve(data, dataType, i - 1) == 0))
			continue;
		
		FuncPattern fp;
		make_pattern(data, dataType, i, length, &fp);
		
		for (j = 0; j < sizeof(InitializeUARTSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &InitializeUARTSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i +  3, length, &OSGetConsoleTypeSigs[0]) &&
							get_immediate(data,   i + 10, i + 11, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (get_immediate (data,   i +  4, i +  5, &constant) && constant == 0xA5FF005A &&
							findx_patterns(data, dataType, i +  9, length, &OSGetConsoleTypeSigs[0],
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
						if (get_immediate (data,   i +  4, i +  5, &constant) && constant == 0xA5FF005A &&
							findx_patterns(data, dataType, i +  9, length, &OSGetConsoleTypeSigs[1],
							                                               &OSGetConsoleTypeSigs[2], NULL) &&
							get_immediate (data,   i + 16, i + 17, &constant) && constant == 0xA5FF005A)
							InitializeUARTSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (get_immediate(data,   i +  4, i +  5, &constant) && constant == 0xA5FF005A &&
							findx_pattern(data, dataType, i +  9, length, &OSGetConsoleTypeSigs[2]) &&
							get_immediate(data,   i + 16, i + 18, &constant) && constant == 0xA5FF005A)
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
					case 5:
						if (get_immediate(data,  i +   8, i +   9, &constant) && constant == 0xA5FF005A &&
							findx_pattern(data, dataType, i +  13, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  23, length, &OSRestoreInterruptsSig) &&
							findx_pattern(data, dataType, i + 128, length, &OSRestoreInterruptsSig))
							WriteUARTNSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(SISetSamplingRateSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &SISetSamplingRateSigs[j])) {
				switch (j) {
					case 0:
						if (findx_pattern(data, dataType, i + 41, length, &SISetXYSigs[0]))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i + 17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 38, i + 39, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 52, length, &SISetXYSigs[0]) &&
							findx_pattern(data, dataType, i + 56, length, &OSRestoreInterruptsSig))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 17, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 40, i + 41, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 54, length, &SISetXYSigs[1]) &&
							findx_pattern(data, dataType, i + 56, length, &OSRestoreInterruptsSig))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i + 34, length, &SISetXYSigs[2]))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i + 13, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 34, i + 35, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 46, length, &SISetXYSigs[2]) &&
							findx_pattern(data, dataType, i + 50, length, &OSRestoreInterruptsSig))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 35, i + 36, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 47, length, &SISetXYSigs[3]) &&
							findx_pattern(data, dataType, i + 49, length, &OSRestoreInterruptsSig))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 35, i + 37, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 44, length, &SISetXYSigs[4]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig))
							SISetSamplingRateSigs[j].offsetFoundAt = i;
						break;
				}
			}
		}
		
		for (j = 0; j < sizeof(SIRefreshSamplingRateSigs) / sizeof(FuncPattern); j++) {
			if (compare_pattern(&fp, &SIRefreshSamplingRateSigs[j])) {
				switch (j) {
					case 0:
						if (findx_patterns(data, dataType, i + 4, length, &SISetSamplingRateSigs[1],
							                                              &SISetSamplingRateSigs[4], NULL))
							SIRefreshSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_patterns(data, dataType, i + 4, length, &SISetSamplingRateSigs[2],
							                                              &SISetSamplingRateSigs[5], NULL))
							SIRefreshSamplingRateSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i + 12, length, &OSDisableInterruptsSig) &&
							get_immediate(data,   i + 35, i + 37, &constant) && constant == 0xCC00206C &&
							findx_pattern(data, dataType, i + 44, length, &SISetXYSigs[4]) &&
							findx_pattern(data, dataType, i + 46, length, &OSRestoreInterruptsSig))
							SIRefreshSamplingRateSigs[j].offsetFoundAt = i;
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
							get_immediate(data,  i +  62, i +  63, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  77, length, &SISetSamplingRateSigs[0]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 1:
						if (findx_pattern(data, dataType, i +  13, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  57, i +  58, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  65, i +  66, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  80, length, &SISetSamplingRateSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 2:
						if (findx_pattern(data, dataType, i +  13, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  59, i +  60, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  67, i +  68, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  79, length, &SIRefreshSamplingRateSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 3:
						if (findx_pattern(data, dataType, i +  15, length, &PADSetSpecSigs[0]) &&
							get_immediate(data,  i +  61, i +  62, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  69, i +  70, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  81, length, &SIRefreshSamplingRateSigs[1]))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 4:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  50, length, &SISetSamplingRateSigs[3]) &&
							findx_pattern(data, dataType, i +  52, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  77, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 5:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  50, length, &SISetSamplingRateSigs[3]) &&
							findx_pattern(data, dataType, i +  53, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i +  84, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 6:
						if (findx_pattern(data, dataType, i +  10, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  45, i +  46, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  50, length, &SISetSamplingRateSigs[3]) &&
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
							findx_pattern(data, dataType, i +  69, length, &SISetSamplingRateSigs[3]) &&
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
							findx_pattern(data, dataType, i +  69, length, &SISetSamplingRateSigs[3]) &&
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
							findx_pattern(data, dataType, i +  72, length, &SISetSamplingRateSigs[4]) &&
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
							findx_pattern(data, dataType, i +  72, length, &SISetSamplingRateSigs[4]) &&
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
							findx_pattern(data, dataType, i +  71, length, &SIRefreshSamplingRateSigs[1]) &&
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
							findx_pattern(data, dataType, i +  73, length, &SIRefreshSamplingRateSigs[1]) &&
							findx_pattern(data, dataType, i +  78, length, &OSDisableInterruptsSig) &&
							findx_pattern(data, dataType, i + 127, length, &OSRestoreInterruptsSig))
							PADInitSigs[j].offsetFoundAt = i;
						break;
					case 13:
						if (findx_pattern(data, dataType, i +  15, length, &PADSetSpecSigs[1]) &&
							get_immediate(data,  i +  43, i +  47, &address) && address == 0x800030E0 &&
							get_immediate(data,  i +  49, i +  51, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  62, length, &SIRefreshSamplingRateSigs[2]) &&
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
							get_immediate(data,  i +  56, i +  69, &address) && address == 0x800030E0 &&
							findx_pattern(data, dataType, i +  73, length, &SIRefreshSamplingRateSigs[1]))
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
						if (findx_pattern(data, dataType, i +  20, length, &OSCancelAlarmSigs[2]) &&
							get_immediate(data,  i +  32, i +  34, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  35, i +  36, length, &TimeoutHandlerSigs[3]) &&
							findx_pattern(data, dataType, i +  44, length, &OSSetAlarmSigs[2]) &&
							get_immediate(data,  i +  47, i +  48, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  49, i +  59, length, &TimeoutHandlerSigs[3]) &&
							findx_pattern(data, dataType, i +  66, length, &OSSetAlarmSigs[2]))
							RetrySigs[j].offsetFoundAt = i;
						break;
					case 5:
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
						if (findx_pattern(data, dataType, i +  36, length, &OSCancelAlarmSigs[2]) &&
							get_immediate(data,  i +  48, i +  50, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  51, i +  52, length, &TimeoutHandlerSigs[3]) &&
							findx_pattern(data, dataType, i +  60, length, &OSSetAlarmSigs[2]) &&
							get_immediate(data,  i +  63, i +  64, &address) && address == 0x800000F8 &&
							findi_pattern(data, dataType, i +  65, i +  75, length, &TimeoutHandlerSigs[3]) &&
							findx_pattern(data, dataType, i +  82, length, &OSSetAlarmSigs[2]))
							__CARDStartSigs[j].offsetFoundAt = i;
						break;
					case 5:
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
					case 6:
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
						if (findx_pattern(data, dataType, i + 123, length, &__OSLockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 177, length, &__OSUnlockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 182, length, &__OSLockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 210, length, &__OSUnlockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 226, length, &__OSLockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 230, length, &__OSUnlockSramExSigs[2]) &&
							findx_pattern(data, dataType, i + 271, length, &__CARDPutControlBlockSigs[4]))
							DoMountSigs[j].offsetFoundAt = i;
						break;
					case 11:
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
			if (devices[DEVICE_CUR]->emulated() && swissSettings.enableUSBGecko && !swissSettings.wiirdDebug) {
				memset(data + i, 0, InitializeUARTSigs[j].Length * sizeof(u32));
				
				data[i + 0] = 0x38600000;	// li		r3, 0
				data[i + 1] = 0x4E800020;	// blr
			}
			print_debug("Found:[%s$%i] @ %08X\n", InitializeUARTSigs[j].Name, j, InitializeUART);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(WriteUARTNSigs) / sizeof(FuncPattern); j++)
	if ((i = WriteUARTNSigs[j].offsetFoundAt)) {
		u32 *WriteUARTN = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (WriteUARTN) {
			if (devices[DEVICE_CUR]->emulated() && swissSettings.enableUSBGecko && !swissSettings.wiirdDebug) {
				memset(data + i, 0, WriteUARTNSigs[j].Length * sizeof(u32));
				memcpy(data + i, WriteUARTNSigs[j].Patch, WriteUARTNSigs[j].PatchLength * sizeof(u32));
			}
			print_debug("Found:[%s$%i] @ %08X\n", WriteUARTNSigs[j].Name, j, WriteUARTN);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SISetSamplingRateSigs) / sizeof(FuncPattern); j++)
	if ((i = SISetSamplingRateSigs[j].offsetFoundAt)) {
		u32 *SISetSamplingRate = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SISetSamplingRate) {
			if (swissSettings.forcePollRate) {
				switch (j) {
					case 0: data[i + 4] = 0x3BC00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
					case 1: data[i + 4] = 0x3BE00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
					case 2: data[i + 4] = 0x3BC00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
					case 3: data[i + 6] = 0x3BA00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
					case 4: data[i + 7] = 0x3B800000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
					case 5:
					case 6: data[i + 6] = 0x3BA00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF); break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", SISetSamplingRateSigs[j].Name, j, SISetSamplingRate);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SIRefreshSamplingRateSigs) / sizeof(FuncPattern); j++)
	if ((i = SIRefreshSamplingRateSigs[j].offsetFoundAt)) {
		u32 *SIRefreshSamplingRate = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SIRefreshSamplingRate) {
			if (swissSettings.forcePollRate)
				if (j == 2)
					data[i + 8] = 0x3BA00000 | ((swissSettings.forcePollRate - 1) & 0xFFFF);
			
			print_debug("Found:[%s$%i] @ %08X\n", SIRefreshSamplingRateSigs[j].Name, j, SIRefreshSamplingRate);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(UpdateOriginSigs) / sizeof(FuncPattern); j++)
	if ((i = UpdateOriginSigs[j].offsetFoundAt)) {
		u32 *UpdateOrigin = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (UpdateOrigin) {
			if (swissSettings.invertCStick & 1) {
				switch (j) {
					case 0:
						if (swissSettings.swapCStick & 1)
							data[i + 73] = 0x2004007F;	// subfic	r0, r4, 127
						else
							data[i + 79] = 0x2004007F;	// subfic	r0, r4, 127
						break;
					case 1:
						if (swissSettings.swapCStick & 1)
							data[i + 76] = 0x2003007F;	// subfic	r0, r3, 127
						else
							data[i + 82] = 0x2003007F;	// subfic	r0, r3, 127
						break;
					case 2:
						if (swissSettings.swapCStick & 1)
							data[i + 69] = 0x2004007F;	// subfic	r0, r4, 127
						else
							data[i + 75] = 0x2004007F;	// subfic	r0, r4, 127
						break;
					case 3:
						if (swissSettings.swapCStick & 1)
							data[i + 71] = 0x20A5007F;	// subfic	r5, r5, 127
						else
							data[i + 77] = 0x20A5007F;	// subfic	r5, r5, 127
						break;
					case 4:
						if (swissSettings.swapCStick & 1)
							data[i + 75] = 0x2084007F;	// subfic	r4, r4, 127
						else
							data[i + 81] = 0x2084007F;	// subfic	r4, r4, 127
						break;
				}
			}
			if (swissSettings.invertCStick & 2) {
				switch (j) {
					case 0:
						if (swissSettings.swapCStick & 2)
							data[i + 76] = 0x2004007F;	// subfic	r0, r4, 127
						else
							data[i + 82] = 0x2004007F;	// subfic	r0, r4, 127
						break;
					case 1:
						if (swissSettings.swapCStick & 2)
							data[i + 79] = 0x2003007F;	// subfic	r0, r3, 127
						else
							data[i + 85] = 0x2003007F;	// subfic	r0, r3, 127
						break;
					case 2:
						if (swissSettings.swapCStick & 2)
							data[i + 72] = 0x2004007F;	// subfic	r0, r4, 127
						else
							data[i + 78] = 0x2004007F;	// subfic	r0, r4, 127
						break;
					case 3:
						if (swissSettings.swapCStick & 2)
							data[i + 74] = 0x20A5007F;	// subfic	r5, r5, 127
						else
							data[i + 80] = 0x20A5007F;	// subfic	r5, r5, 127
						break;
					case 4:
						if (swissSettings.swapCStick & 2)
							data[i + 78] = 0x2084007F;	// subfic	r4, r4, 127
						else
							data[i + 84] = 0x2084007F;	// subfic	r4, r4, 127
						break;
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", UpdateOriginSigs[j].Name, j, UpdateOrigin);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADOriginCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = PADOriginCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginCallback) {
			if (j == 5) {
				if (swissSettings.invertCStick & 1) {
					if (swissSettings.swapCStick & 1)
						data[i + 79] = 0x20A5007F;	// subfic	r5, r5, 127
					else
						data[i + 86] = 0x2004007F;	// subfic	r0, r4, 127
				}
				if (swissSettings.invertCStick & 2) {
					if (swissSettings.swapCStick & 2)
						data[i + 83] = 0x2004007F;	// subfic	r0, r4, 127
					else
						data[i + 89] = 0x2004007F;	// subfic	r0, r4, 127
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", PADOriginCallbackSigs[j].Name, j, PADOriginCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADOriginUpdateCallbackSigs) / sizeof(FuncPattern); j++)
	if ((i = PADOriginUpdateCallbackSigs[j].offsetFoundAt)) {
		u32 *PADOriginUpdateCallback = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (PADOriginUpdateCallback) {
			if (j == 5) {
				if (swissSettings.invertCStick & 1) {
					if (swissSettings.swapCStick & 1)
						data[i + 86] = 0x2063007F;	// subfic	r3, r3, 127
					else
						data[i + 93] = 0x2003007F;	// subfic	r0, r3, 127
				}
				if (swissSettings.invertCStick & 2) {
					if (swissSettings.swapCStick & 2)
						data[i + 90] = 0x2003007F;	// subfic	r0, r3, 127
					else
						data[i + 96] = 0x2003007F;	// subfic	r0, r3, 127
				}
			}
			print_debug("Found:[%s$%i] @ %08X\n", PADOriginUpdateCallbackSigs[j].Name, j, PADOriginUpdateCallback);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(PADReadSigs) / sizeof(FuncPattern); j++)
	if ((i = PADReadSigs[j].offsetFoundAt)) {
		u32 *PADRead = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		u32 *CheckStatus;
		
		if (PADRead) {
			if (swissSettings.aveCompat == GCDIGITAL_COMPAT)
				CheckStatus = getPatchAddr(PAD_CHECKSTATUS_GCDIGITAL);
			else
				CheckStatus = getPatchAddr(PAD_CHECKSTATUS);
			
			switch (j) {
				case 0:
					data[i + 159] = 0x387E0000;	// addi		r3, r30, 0
					data[i + 160] = 0x389F0000;	// addi		r4, r31, 0
					data[i + 161] = branchAndLink(CheckStatus, PADRead + 161);
					break;
				case 1:
					data[i + 156] = 0x387E0000;	// addi		r3, r30, 0
					data[i + 157] = 0x389F0000;	// addi		r4, r31, 0
					data[i + 158] = branchAndLink(CheckStatus, PADRead + 158);
					break;
				case 2:
					data[i + 100] = 0x387D0000;	// addi		r3, r29, 0
					data[i + 101] = 0x389B0000;	// addi		r4, r27, 0
					data[i + 102] = branchAndLink(CheckStatus, PADRead + 102);
					break;
				case 3:
					data[i + 185] = 0x38760000;	// addi		r3, r22, 0
					data[i + 186] = 0x38950000;	// addi		r4, r21, 0
					data[i + 187] = branchAndLink(CheckStatus, PADRead + 187);
					break;
				case 4:
					data[i + 190] = 0x38770000;	// addi		r3, r23, 0
					data[i + 191] = 0x38950000;	// addi		r4, r21, 0
					data[i + 192] = branchAndLink(CheckStatus, PADRead + 192);
					break;
				case 5:
					data[i + 221] = 0x38750000;	// addi		r3, r21, 0
					data[i + 222] = 0x389F0000;	// addi		r4, r31, 0
					data[i + 223] = branchAndLink(CheckStatus, PADRead + 223);
					break;
				case 6:
					data[i + 219] = 0x38750000;	// addi		r3, r21, 0
					data[i + 220] = 0x389F0000;	// addi		r4, r31, 0
					data[i + 221] = branchAndLink(CheckStatus, PADRead + 221);
					break;
				case 7:
					data[i + 216] = 0x7F63DB78;	// mr		r3, r27
					data[i + 217] = 0x7F24CB78;	// mr		r4, r25
					data[i + 218] = branchAndLink(CheckStatus, PADRead + 218);
					break;
				case 8:
					data[i + 176] = 0x38790000;	// addi		r3, r25, 0
					data[i + 177] = 0x38970000;	// addi		r4, r23, 0
					data[i + 178] = branchAndLink(CheckStatus, PADRead + 178);
					break;
			}
			print_debug("Found:[%s$%i] @ %08X\n", PADReadSigs[j].Name, j, PADRead);
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
			print_debug("Found:[%s$%i] @ %08X\n", PADSetSpecSigs[j].Name, j, PADSetSpec);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SPEC0_MakeStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = SPEC0_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC0_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC0_MakeStatus) {
			if (swissSettings.swapCStick & 1) {
				switch (j) {
					case 0:
						data[i + 50] = 0x98040004;	// stb		r0, 4 (r4)
						data[i + 57] = 0x98040002;	// stb		r0, 2 (r4)
						break;
					case 1:
						data[i + 49] = 0x98640004;	// stb		r3, 4 (r4)
						data[i + 56] = 0x98640002;	// stb		r3, 2 (r4)
						break;
					case 2:
						data[i + 44] = 0x98640004;	// stb		r3, 4 (r4)
						data[i + 49] = 0x98640002;	// stb		r3, 2 (r4)
						break;
				}
			}
			if (swissSettings.swapCStick & 2) {
				switch (j) {
					case 0:
						data[i + 54] = 0x98040005;	// stb		r0, 5 (r4)
						data[i + 61] = 0x98040003;	// stb		r0, 3 (r4)
						break;
					case 1:
						data[i + 53] = 0x98640005;	// stb		r3, 5 (r4)
						data[i + 60] = 0x98640003;	// stb		r3, 3 (r4)
						break;
					case 2:
						data[i + 47] = 0x98640005;	// stb		r3, 5 (r4)
						data[i + 52] = 0x98640003;	// stb		r3, 3 (r4)
						break;
				}
			}
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
			print_debug("Found:[%s$%i] @ %08X\n", SPEC0_MakeStatusSigs[j].Name, j, SPEC0_MakeStatus);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(SPEC1_MakeStatusSigs) / sizeof(FuncPattern); j++)
	if ((i = SPEC1_MakeStatusSigs[j].offsetFoundAt)) {
		u32 *SPEC1_MakeStatus = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (SPEC1_MakeStatus) {
			if (swissSettings.swapCStick & 1) {
				switch (j) {
					case 0:
						data[i + 50] = 0x98040004;	// stb		r0, 4 (r4)
						data[i + 57] = 0x98040002;	// stb		r0, 2 (r4)
						break;
					case 1:
						data[i + 49] = 0x98640004;	// stb		r3, 4 (r4)
						data[i + 56] = 0x98640002;	// stb		r3, 2 (r4)
						break;
					case 2:
						data[i + 44] = 0x98640004;	// stb		r3, 4 (r4)
						data[i + 49] = 0x98640002;	// stb		r3, 2 (r4)
						break;
				}
			}
			if (swissSettings.swapCStick & 2) {
				switch (j) {
					case 0:
						data[i + 54] = 0x98040005;	// stb		r0, 5 (r4)
						data[i + 61] = 0x98040003;	// stb		r0, 3 (r4)
						break;
					case 1:
						data[i + 53] = 0x98640005;	// stb		r3, 5 (r4)
						data[i + 60] = 0x98640003;	// stb		r3, 3 (r4)
						break;
					case 2:
						data[i + 47] = 0x98640005;	// stb		r3, 5 (r4)
						data[i + 52] = 0x98640003;	// stb		r3, 3 (r4)
						break;
				}
			}
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
			print_debug("Found:[%s$%i] @ %08X\n", SPEC1_MakeStatusSigs[j].Name, j, SPEC1_MakeStatus);
			patched++;
		}
	}
	
	if ((i = ClampS8Sig.offsetFoundAt)) {
		u32 *ClampS8 = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (ClampS8) {
			data[i + 3] = 0x41800020;	// blt		+8
			data[i + 4] = 0x3BE4FF81;	// subi		r31, r4, 127
			
			print_debug("Found:[%s] @ %08X\n", ClampS8Sig.Name, ClampS8);
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
			if (swissSettings.swapCStick & 1) {
				switch (j) {
					case 0:
						data[i +  13] = 0x981F0004;	// stb		r0, 4 (r31)
						data[i +  46] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i +  67] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i +  88] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 108] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 126] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 158] = 0x889D0004;	// lbz		r4, 4 (r29)
						data[i + 166] = 0x889D0002;	// lbz		r4, 2 (r29)
						break;
					case 1:
						data[i +  13] = 0x981F0004;	// stb		r0, 4 (r31)
						data[i +  46] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i +  67] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i +  88] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 108] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 126] = 0x981F0002;	// stb		r0, 2 (r31)
						data[i + 190] = 0x889D0004;	// lbz		r4, 4 (r29)
						data[i + 198] = 0x889D0002;	// lbz		r4, 2 (r29)
						break;
					case 2:
						data[i +   6] = 0x98040004;	// stb		r0, 4 (r4)
						data[i +  39] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  60] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  81] = 0x98040002;	// stb		r0, 2 (r4)
						data[i + 102] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 119] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 147] = 0x88E30004;	// lbz		r7, 4 (r3)
						data[i + 193] = 0x88E30002;	// lbz		r7, 2 (r3)
						break;
					case 3:
						data[i +   5] = 0x98040004;	// stb		r0, 4 (r4)
						data[i +  36] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  55] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  74] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  93] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 108] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 135] = 0x88E30004;	// lbz		r7, 4 (r3)
						data[i + 177] = 0x88E30002;	// lbz		r7, 2 (r3)
						break;
					case 4:
						data[i +   6] = 0x98040004;	// stb		r0, 4 (r4)
						data[i +  39] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  60] = 0x98040002;	// stb		r0, 2 (r4)
						data[i +  81] = 0x98040002;	// stb		r0, 2 (r4)
						data[i + 102] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 119] = 0x98C40002;	// stb		r6, 2 (r4)
						data[i + 178] = 0x88E30004;	// lbz		r7, 4 (r3)
						data[i + 223] = 0x88E30002;	// lbz		r7, 2 (r3)
						break;
				}
			}
			if (swissSettings.swapCStick & 2) {
				switch (j) {
					case 0:
						data[i +  16] = 0x981F0005;	// stb		r0, 5 (r31)
						data[i +  50] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i +  71] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i +  92] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 112] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 130] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 162] = 0x889D0005;	// lbz		r4, 5 (r29)
						data[i + 170] = 0x889D0003;	// lbz		r4, 3 (r29)
						break;
					case 1:
						data[i +  16] = 0x981F0005;	// stb		r0, 5 (r31)
						data[i +  50] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i +  71] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i +  92] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 112] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 130] = 0x981F0003;	// stb		r0, 3 (r31)
						data[i + 194] = 0x889D0005;	// lbz		r4, 5 (r29)
						data[i + 202] = 0x889D0003;	// lbz		r4, 3 (r29)
						break;
					case 2:
						data[i +   9] = 0x98040005;	// stb		r0, 5 (r4)
						data[i +  43] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  64] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  85] = 0x98040003;	// stb		r0, 3 (r4)
						data[i + 106] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 123] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 170] = 0x88E30005;	// lbz		r7, 5 (r3)
						data[i + 216] = 0x88E30003;	// lbz		r7, 3 (r3)
						break;
					case 3:
						data[i +   7] = 0x98040005;	// stb		r0, 5 (r4)
						data[i +  39] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  58] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  77] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  96] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 111] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 156] = 0x88E30005;	// lbz		r7, 5 (r3)
						data[i + 198] = 0x88E30003;	// lbz		r7, 3 (r3)
						break;
					case 4:
						data[i +   9] = 0x98040005;	// stb		r0, 5 (r4)
						data[i +  43] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  64] = 0x98040003;	// stb		r0, 3 (r4)
						data[i +  85] = 0x98040003;	// stb		r0, 3 (r4)
						data[i + 106] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 123] = 0x98C40003;	// stb		r6, 3 (r4)
						data[i + 200] = 0x88E30005;	// lbz		r7, 5 (r3)
						data[i + 246] = 0x88E30003;	// lbz		r7, 3 (r3)
						break;
				}
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
			print_debug("Found:[%s$%i] @ %08X\n", SPEC2_MakeStatusSigs[j].Name, j, SPEC2_MakeStatus);
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
			print_debug("Found:[%s$%i] @ %08X\n", SetupTimeoutAlarmSigs[j].Name, j, SetupTimeoutAlarm);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(RetrySigs) / sizeof(FuncPattern); j++)
	if ((i = RetrySigs[j].offsetFoundAt)) {
		u32 *Retry = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (Retry) {
			switch (j) {
				case 3: data[i + 41] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 4: data[i + 43] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 5: data[i + 41] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
			}
			print_debug("Found:[%s$%i] @ %08X\n", RetrySigs[j].Name, j, Retry);
			patched++;
		}
	}
	
	for (j = 0; j < sizeof(__CARDStartSigs) / sizeof(FuncPattern); j++)
	if ((i = __CARDStartSigs[j].offsetFoundAt)) {
		u32 *__CARDStart = Calc_ProperAddress(data, dataType, i * sizeof(u32));
		
		if (__CARDStart) {
			switch (j) {
				case 3: data[i + 66] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 4: data[i + 59] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
				case 5:
				case 6: data[i + 69] = 0x1CC007D0; break;	// mulli	r6, r0, 2000
			}
			print_debug("Found:[%s$%i] @ %08X\n", __CARDStartSigs[j].Name, j, __CARDStart);
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
			print_debug("Found:[%s$%i] @ %08X\n", CARDGetEncodingSigs[j].Name, j, CARDGetEncoding);
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
			
			print_debug("Found:[%s$%i] @ %08X\n", VerifyIDSigs[j].Name, j, VerifyID);
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
				case 10: data[i + 218] = 0x48000004; break;	// b		+1
				case 11: data[i + 202] = 0x48000004; break;	// b		+1
			}
			print_debug("Found:[%s$%i] @ %08X\n", DoMountSigs[j].Name, j, DoMount);
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
			print_debug("DOL Header doesn't look valid %08X\n",hdr->textOffset[0]);
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

			if(strncmp(apploaderHeader->date, "2004/02/01", 10) <= 0 || strncmp(apploaderHeader->date, "2005/10/08", 10) >= 0) {
				if(offsetFoundAt >= offset && offsetFoundAt < offset + apploaderHeader->rebootSize)
					return (void*)(offsetFoundAt+0x81300000-offset);
			}
			else {
				return Calc_ProperAddress(data + offset, PATCH_DOL, offsetFoundAt - offset);
			}
		}
	}
	else if(dataType == PATCH_BS2) {
		u32 offset = sizeof(BS2Header);
		BS2Header *bs2Header = (BS2Header *) data;

		if(offsetFoundAt >= offset && offsetFoundAt < offset + bs2Header->bss - 0x81300000)
			return (void*)(offsetFoundAt+0x81300000-offset);
	}
	else if(dataType == PATCH_EXEC) {
		if(offsetFoundAt < 0x400000)
			return (void*)(offsetFoundAt+0x81300000);
	}
	else if(dataType == PATCH_BIN) {
		if(offsetFoundAt < 0x400000)
			return (void*)(offsetFoundAt+0x80003100);
	}
	return NULL;
}

void *Calc_Address(void *data, int dataType, u32 properAddress) {
	if(dataType == PATCH_DOL || dataType == PATCH_DOL_PRS) {
		int i;
		DOLHEADER *hdr = (DOLHEADER *) data;

		// Doesn't look valid
		if (hdr->textOffset[0] != DOLHDRLENGTH) {
			print_debug("DOL Header doesn't look valid %08X\n",hdr->textOffset[0]);
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

			if(strncmp(apploaderHeader->date, "2004/02/01", 10) <= 0 || strncmp(apploaderHeader->date, "2005/10/08", 10) >= 0) {
				if(properAddress >= 0x81300000 && properAddress < 0x81300000 + apploaderHeader->rebootSize)
					return data+properAddress-0x81300000+offset;
			}
			else {
				return Calc_Address(data + offset, PATCH_DOL, properAddress);
			}
		}
	}
	else if(dataType == PATCH_BS2) {
		u32 offset = sizeof(BS2Header);
		BS2Header *bs2Header = (BS2Header *) data;

		if(properAddress >= 0x81300000 && properAddress < bs2Header->bss)
			return data+properAddress-0x81300000+offset;
	}
	else if(dataType == PATCH_EXEC) {
		if(properAddress >= 0x81300000 && properAddress < 0x81700000)
			return data+properAddress-0x81300000;
	}
	else if(dataType == PATCH_BIN) {
		if(properAddress >= 0x80003100 && properAddress < 0x80403100)
			return data+properAddress-0x80003100;
	}
	if(properAddress >= 0x80000000 && properAddress < 0x80003100) {
		return VAR_AREA+properAddress-0x80000000;
	}
	else if(properAddress >= (u32)SYS_GetArenaHi() && properAddress < 0x81800000) {
		return (void*)properAddress;
	}
	return NULL;
}

// Ocarina cheat engine hook - Patch OSSleepThread
int Patch_CheatsHook(u8 *data, u32 length, u32 type) {
	if(in_range(type, PATCH_APPLOADER, PATCH_EXEC))
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
				print_debug("Found:[Hook:OSSleepThread] @ %08X\n", properAddress );
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
				print_debug("Found:[Hook:GXDrawDone] @ %08X\n", properAddress );
				*(vu32*)(data+i+j) = branch(CHEATS_ENGINE_START, properAddress);
				break;
			}
		}
	}
	return 0;
}

int Patch_ExecutableFile(void **buffer, u32 *sizeToRead, const char *gameID, int type)
{
	int i, j;
	int patched = 0;
	void *data = *buffer;
	u32 length = *sizeToRead;
	
	int patch(void *buffer, u32 sizeToRead, const char *gameID, int type)
	{
		int patched = 0;
		
		// Patch hypervisor
		if (devices[DEVICE_CUR]->emulated()) {
			patched += Patch_Hypervisor(buffer, sizeToRead, type);
			patched += Patch_GameSpecificHypervisor(buffer, sizeToRead, gameID, type);
		}
		
		// Patch specific game hacks
		patched += Patch_GameSpecific(buffer, sizeToRead, gameID, type);
		
		// Patch CARD, PAD
		patched += Patch_Miscellaneous(buffer, sizeToRead, type);
		
		// Force Video Mode
		if (swissSettings.disableVideoPatches < 2) {
			if (swissSettings.disableVideoPatches < 1)
				Patch_GameSpecificVideo(buffer, sizeToRead, gameID, type);
			Patch_Video(buffer, sizeToRead, type);
		}
		// Cheats
		else if (swissSettings.wiirdEngine)
			Patch_CheatsHook(buffer, sizeToRead, type);
		
		// Force Widescreen
		if (swissSettings.forceWidescreen)
			Patch_Widescreen(buffer, sizeToRead, type);
		
		// Force Anisotropy
		if (swissSettings.forceAnisotropy)
			Patch_TexFilt(buffer, sizeToRead, type);
		
		// Force Text Encoding
		patched += Patch_FontEncode(buffer, sizeToRead);
		
		return patched;
	}
	
	if ((!strncmp(gameID, "GLRD64", 6) || !strncmp(gameID, "GLRE64", 6) || !strncmp(gameID, "GLRF64", 6) || !strncmp(gameID, "GLRP64", 6) || !strncmp(gameID, "GWXJ13", 6)) && type == PATCH_DOL) {
		struct {
			uLongf deflateLength;
			uLongf inflateLength;
			Bytef data[];
		} *zlib = Calc_Address(data, type, 0x80200000);
		
		if (zlib == NULL || __builtin_add_overflow_p(length, zlib->inflateLength, (size_t)0))
			return patch(data, length, gameID, type);
		
		DOLHEADER *dol = realloc(data, length + zlib->inflateLength);
		
		if (dol == NULL)
			return patch(data, length, gameID, type);
		if (dol != data) {
			data = dol;
			zlib = Calc_Address(data, type, 0x80200000);
		}
		
		DOLHEADER *dol2 = data + length;
		DOLHEADER dolhdr;
		
		if (uncompress((Bytef *)dol2, &zlib->inflateLength, zlib->data, zlib->deflateLength) == Z_OK) {
			memcpy(&dolhdr, dol, DOLHDRLENGTH);
			
			for (j = 0; j < MAXTEXTSECTION; j++) {
				if (dol2->textOffset[j]) {
					for (i = 0; i < MAXTEXTSECTION; i++) {
						if (!dol->textOffset[i]) {
							dol->textOffset[i] = dol2->textOffset[j] + length;
							dol->textAddress[i] = dol2->textAddress[j];
							dol->textLength[i] = dol2->textLength[j];
							break;
						}
					}
				}
			}
			
			for (j = 0; j < MAXDATASECTION; j++) {
				if (dol2->dataOffset[j]) {
					for (i = 0; i < MAXDATASECTION; i++) {
						if (!dol->dataOffset[i]) {
							dol->dataOffset[i] = dol2->dataOffset[j] + length;
							dol->dataAddress[i] = dol2->dataAddress[j];
							dol->dataLength[i] = dol2->dataLength[j];
							break;
						}
					}
				}
			}
			
			length = DOLSize(data);
			patched = patch(data, length, gameID, type);
			
			memcpy(dol, &dolhdr, DOLHDRLENGTH);
			
			struct libdeflate_compressor *compressor = libdeflate_alloc_compressor(7);
			if (compressor) zlib->deflateLength = libdeflate_zlib_compress(compressor, (Bytef *)dol2, zlib->inflateLength, zlib->data, zlib->deflateLength);
			libdeflate_free_compressor(compressor);
			
			length = DOLSize(data);
			data = realloc(data, length);
		} else {
			data = realloc(data, length);
			patched = patch(data, length, gameID, type);
		}
		
		*buffer = data;
		*sizeToRead = length;
		return patched;
	}
	
	patched = patch(data, length, gameID, type);
	
	if (type == PATCH_DOL)
		length = DOLSizeFix(data);
	
	return patched;
}
