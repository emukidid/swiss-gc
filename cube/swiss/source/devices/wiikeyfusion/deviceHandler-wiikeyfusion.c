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
#include "flippy.h"
#include "wkf.h"
#include "patcher.h"
#include "dvd.h"

file_handle initial_WKF = {
	.name     = "wkf:/",
	.fileType = IS_DIR,
	.device   = &__device_wkf,
};

s32 deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0 || !devices[DEVICE_CUR]->emulated()) {
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
		print_debug("Save Patch device found\n");
		
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
		
		if(swissSettings.igrType == IGR_APPLOADER || endsWith(file->name,".tgc")) {
			memset(&patchFile, 0, sizeof(file_handle));
			concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
			
			getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->statFile(&patchFile)) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_A, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_A_ID = (patchFile.size << 3 >> 20) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->statFile(&patchFile)) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_B, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_B_ID = (patchFile.size << 3 >> 20) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		s32 exi_channel, exi_device;
		if(getExiDeviceByLocation(devices[DEVICE_PATCHES]->location, &exi_channel, &exi_device)) {
			exi_device = sdgecko_getDevice(exi_channel);
			// Card Type
			*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(exi_channel) ? 0:9;
			// Copy the actual freq
			*(vu8*)VAR_EXI_CPR = (exi_channel << 6) | ((1 << exi_device) << 3) | sdgecko_getSpeed(exi_channel);
			// Device slot (0, 1 or 2)
			*(vu8*)VAR_EXI_SLOT = (*(vu8*)VAR_EXI_SLOT & 0xF0) | (((exi_device << 2) | exi_channel) & 0x0F);
			*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[exi_channel];
		}
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
		printFragments(fragList, numFrags);
		*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
		free(fragList);
		fragList = NULL;
	}
	return 1;
}

bool deviceHandler_WKF_test() {
	return swissSettings.hasDVDDrive == 1 && (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF);
}

s32 deviceHandler_WKF_init(file_handle* file) {
	if(devices[DEVICE_CUR] == &__device_flippy || devices[DEVICE_CUR] == &__device_flippyflash) {
		return EBUSY;
	}
	if(swissSettings.hasFlippyDrive) flippy_bypass(true);
	if(!deviceHandler_WKF_test()) return ENODEV;
	
	wkfReinit();	// TODO extended error status
	file->status = fatFs_Mount(file);
	return file->status == FR_OK ? 0 : EIO;
}

u32 deviceHandler_WKF_emulated() {
	if (devices[DEVICE_PATCHES]) {
		if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
			swissSettings.emulateAudioStream > 1)
			return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER;
		else if (swissSettings.emulateMemoryCard)
			return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
		else
			return EMU_READ | EMU_BUS_ARBITER;
	} else {
		if (swissSettings.disableHypervisor)
			return EMU_NONE;
		else if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
				swissSettings.emulateAudioStream > 1)
			return EMU_READ | EMU_AUDIO_STREAMING;
		else
			return EMU_READ;
	}
}

DEVICEHANDLER_INTERFACE __device_wkf = {
	.deviceUniqueId = DEVICE_ID_B,
	.hwName = "WKF / WASP ODE",
	.deviceName = "Wiikey / Wasp Fusion",
	.deviceDescription = "Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_WIIKEY, 115, 71, 120, 80},
	.features = FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	.emulable = EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_DVD_CONNECTOR,
	.initial = &initial_WKF,
	.test = deviceHandler_WKF_test,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_WKF_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.statFile = deviceHandler_FAT_statFile,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.hideFile = deviceHandler_FAT_hideFile,
	.setupFile = deviceHandler_WKF_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_WKF_emulated,
	.status = deviceHandler_FAT_status,
};
