/* deviceHandler-wiikeyfusion.c
	- device implementation for Wiikey Fusion (FAT filesystem)
	by emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "wkf.h"
#include "patcher.h"
#include "dvd.h"

FATFS *wkffs = NULL;

file_handle initial_WKF =
	{ "wkf:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

s32 deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0) {
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, 0, 0, 0) || numFrags != 1) {
			free(fragList);
			return 0;
		}
		wkfWriteOffset((*fragList).fileBase);
		free(fragList);
		return 1;
	}
	
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			if(!filesToPatch[i].patchFile) continue;
			if(!getFragments(DEVICE_PATCHES, filesToPatch[i].patchFile, &fragList, &numFrags, filesToPatch[i].file == file2, filesToPatch[i].offset, filesToPatch[i].size)) {
				free(fragList);
				return 0;
			}
		}
		
		// If disc 1 is fragmented, make a note of the fragments and their sizes
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, UINT32_MAX)) {
			free(fragList);
			return 0;
		}
		
		// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
		if(file2) {
			if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, UINT32_MAX)) {
				free(fragList);
				return 0;
			}
		}
		
		if(swissSettings.igrType == IGR_BOOTBIN || endsWith(file->name,".tgc")) {
			memset(&patchFile, 0, sizeof(file_handle));
			concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
			
			ApploaderHeader apploaderHeader;
			if(devices[DEVICE_PATCHES]->readFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader)) != sizeof(ApploaderHeader) || apploaderHeader.rebootSize != reboot_bin_size) {
				devices[DEVICE_PATCHES]->deleteFile(&patchFile);
				
				memset(&apploaderHeader, 0, sizeof(ApploaderHeader));
				apploaderHeader.rebootSize = reboot_bin_size;
				
				devices[DEVICE_PATCHES]->seekFile(&patchFile, 0, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_PATCHES]->writeFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader));
				devices[DEVICE_PATCHES]->writeFile(&patchFile, reboot_bin, reboot_bin_size);
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_A, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_A_ID = (patchFile.size * 8/1024/1024) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_B, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_B_ID = (patchFile.size * 8/1024/1024) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
		*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[*(vu8*)VAR_EXI_SLOT];
	}
	else {
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, UINT32_MAX) || numFrags != 1) {
			free(fragList);
			return 0;
		}
		
		if(file2) {
			if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, UINT32_MAX) || numFrags != 2) {
				free(fragList);
				return 0;
			}
		}
	}
	
	if(fragList) {
		print_frag_list(fragList, numFrags);
		*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
		free(fragList);
		fragList = NULL;
	}
	return 1;
}

bool deviceHandler_WKF_test() {
	return swissSettings.hasDVDDrive && (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF);
}

s32 deviceHandler_WKF_init(file_handle* file){
	if(!deviceHandler_WKF_test()) return ENODEV;
	wkfReinit();	// TODO extended error status
	if(wkffs != NULL) {
		f_unmount("wkf:/");
		free(wkffs);
		wkffs = NULL;
		disk_shutdown(DEV_WKF);
	}
	wkffs = (FATFS*)malloc(sizeof(FATFS));
	return f_mount(wkffs, "wkf:/", 1) == FR_OK ? 0 : EIO;
		}

s32 deviceHandler_WKF_deinit(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	if(file) {
		f_unmount(file->name);
		free(wkffs);
		wkffs = NULL;
		disk_shutdown(DEV_WKF);
	}
	return 0;
}

u32 deviceHandler_WKF_emulated() {
	if (devices[DEVICE_PATCHES]) {
		if (swissSettings.audioStreaming)
			return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER;
		else if (swissSettings.emulateMemoryCard)
			return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
		else
			return EMU_READ | EMU_BUS_ARBITER;
	} else {
		if (swissSettings.audioStreaming)
			return EMU_READ | EMU_AUDIO_STREAMING;
		else
			return EMU_READ;
	}
}

DEVICEHANDLER_INTERFACE __device_wkf = {
	DEVICE_ID_B,
	"WKF / WASP ODE",
	"Wiikey / Wasp Fusion",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_WIIKEY, 115, 71, 120, 80},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_DVD_CONNECTOR,
	&initial_WKF,
	(_fn_test)&deviceHandler_WKF_test,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_WKF_init,
	(_fn_makeDir)&deviceHandler_FAT_makeDir,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_renameFile)&deviceHandler_FAT_renameFile,
	(_fn_setupFile)&deviceHandler_WKF_setupFile,
	(_fn_deinit)&deviceHandler_WKF_deinit,
	(_fn_emulated)&deviceHandler_WKF_emulated,
	(_fn_status)&deviceHandler_FAT_status,
};
