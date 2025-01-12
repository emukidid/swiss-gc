/* deviceHandler-flippydrive.c
	- device implementation for FlippyDrive
	by Extrems
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "flippy.h"
#include "patcher.h"

file_handle initial_Flippy =
	{ "fldrv:/",      // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

file_handle initial_FlippyFlash =
	{ "fdffs:/",      // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

device_info* deviceHandler_Flippy_info(file_handle* file) {
	return NULL;
}

s32 deviceHandler_Flippy_readDir(file_handle* ffile, file_handle** dir, u32 type) {

	flippydirinfo* dp = memalign(32, sizeof(flippydirinfo));
	if((getDeviceFromPath(ffile->name) == &__device_flippyflash ?
		flippy_flash_opendir(dp, getDevicePath(ffile->name)) :
		flippy_opendir(dp, getDevicePath(ffile->name))) != FLIPPY_RESULT_OK) return -1;
	flippyfilestat entry;
	flippyfilestat *result;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileType = IS_SPECIAL;
	
	// Read each entry of the directory
	while( flippy_readdir(dp, &entry, &result) == FLIPPY_RESULT_OK && result == &entry ) {
		if(!strcmp(entry.name, ".") || !strcmp(entry.name, "..") || !strcmp(entry.name, "?")) {
			continue;
		}
		// Do we want this one?
		if((type == -1 || ((entry.type == FLIPPY_TYPE_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			if(concat_path((*dir)[i].name, ffile->name, entry.name) < PATHNAME_MAX && entry.size <= UINT32_MAX) {
				(*dir)[i].size       = entry.size;
				(*dir)[i].fileType   = (entry.type == FLIPPY_TYPE_DIR) ? IS_DIR : IS_FILE;
				(*dir)[i].fileAttrib = entry.attributes;
				++i;
			}
		}
	}
	
	flippy_closedir(dp);
	free(dp);
	return i;
}

s64 deviceHandler_Flippy_seekFile(file_handle* file, s64 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

static u32 defaultFlags(file_handle* file) {
	if(endsWith(file->name,".fdi") || endsWith(file->name,".gcm") || endsWith(file->name,".iso") || endsWith(file->name,".tgc"))
		return FLIPPY_FLAG_DISABLE_DVDSPEED;
	return FLIPPY_FLAG_DEFAULT;
}

s32 deviceHandler_Flippy_readFile(file_handle* file, void* buffer, u32 length) {
	if(file->status == STATUS_MAPPED) {
		s32 bytes_read = DVD_ReadAbs((dvdcmdblk*)file->other, buffer, length, file->offset);
		if(bytes_read > 0) file->offset += bytes_read;
		return bytes_read;
	}
	if(!file->fp) {
		if(endsWith(file->name,".fdi") && (getDeviceFromPath(file->name)->quirks & QUIRK_FDI_EXCLUSIVE_OPEN)) {
			return -1;
		}
		file->fp = memalign(32, sizeof(flippyfileinfo));
		if((getDeviceFromPath(file->name) == &__device_flippyflash ?
			flippy_flash_open(file->fp, getDevicePath(file->name), defaultFlags(file)) :
			flippy_open(file->fp, getDevicePath(file->name), defaultFlags(file))) != FLIPPY_RESULT_OK) {
			free(file->fp);
			file->fp = NULL;
			return -1;
		}
		flippyfileinfo* info = file->fp;
		if(endsWith(file->name,".fdi") && (getDeviceFromPath(file->name)->quirks & QUIRK_FDI_BYTESWAP_SIZE)) {
			if(info->file.size != file->size) {
				info->file.size = __builtin_bswap32(info->file.size >> 9 << 8) << 9;
			}
		}
		file->size = info->file.size;
	}
	if(file->offset > file->size) {
		file->offset = file->size;
	}
	if(length + file->offset > file->size) {
		length = file->size - file->offset;
	}
	if(flippy_pread(file->fp, buffer, length, file->offset) != FLIPPY_RESULT_OK) {
		return -1;
	}
	file->offset += length;
	return length;
}

s32 deviceHandler_Flippy_writeFile(file_handle* file, const void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = memalign(32, sizeof(flippyfileinfo));
		if((getDeviceFromPath(file->name) == &__device_flippyflash ?
			flippy_flash_open(file->fp, getDevicePath(file->name), FLIPPY_FLAG_DEFAULT | FLIPPY_FLAG_WRITE) :
			flippy_open(file->fp, getDevicePath(file->name), FLIPPY_FLAG_DEFAULT | FLIPPY_FLAG_WRITE)) != FLIPPY_RESULT_OK) {
			free(file->fp);
			file->fp = NULL;
			return -1;
		}
		flippyfileinfo* info = file->fp;
		file->size = info->file.size;
	}
	if(flippy_pwrite(file->fp, buffer, length, file->offset) != FLIPPY_RESULT_OK) {
		return -1;
	}
	file->offset += length;
	return length;
}

s32 deviceHandler_Flippy_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0) {
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, 0, 0, 0) || numFrags != 1 ||
			flippy_mount(file->fp) != FLIPPY_RESULT_OK) {
			free(fragList);
			return 0;
		}
		
		if(file2) {
			if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, 0, 0, 0) || numFrags != 2 ||
				flippy_mount2(file->fp, file2->fp) != FLIPPY_RESULT_OK) {
				free(fragList);
				return 0;
			}
		}
		free(fragList);
		file->status = STATUS_MAPPED;
		return 1;
	}
	
	// Look for patch files, if we find some, open them and add them as fragments
	file_handle patchFile;
	for(i = 0; i < numToPatch; i++) {
		if(!filesToPatch[i].patchFile) continue;
		if(!getFragments(DEVICE_PATCHES, filesToPatch[i].patchFile, &fragList, &numFrags, filesToPatch[i].file == file2, filesToPatch[i].offset, filesToPatch[i].size)) {
			free(fragList);
			return 0;
		}
	}
	
	if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, 0) ||
		flippy_mount(file->fp) != FLIPPY_RESULT_OK) {
		free(fragList);
		return 0;
	}
	
	if(file2) {
		if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, 0) ||
			flippy_mount2(file->fp, file2->fp) != FLIPPY_RESULT_OK) {
			free(fragList);
			return 0;
		}
	}
	
	if(swissSettings.igrType == IGR_APPLOADER || endsWith(file->name,".tgc")) {
		memset(&patchFile, 0, sizeof(file_handle));
		concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
		
		getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
	}
	
	if(swissSettings.emulateMemoryCard) {
		memset(&patchFile, 0, sizeof(file_handle));
		concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
		concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
		ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
		devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
		
		if(devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0) == 0 && !patchFile.size) {
			devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
		}
		
		if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_A, 0, 0))
			*(vu8*)VAR_CARD_A_ID = (patchFile.size * 8/1024/1024) & 0xFC;
		
		memset(&patchFile, 0, sizeof(file_handle));
		concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
		concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
		ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
		devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
		
		if(devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0) == 0 && !patchFile.size) {
			devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
		}
		
		if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_B, 0, 0))
			*(vu8*)VAR_CARD_B_ID = (patchFile.size * 8/1024/1024) & 0xFC;
	}
	
	if(fragList) {
		print_frag_list(fragList, numFrags);
		*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
		free(fragList);
		fragList = NULL;
	}
	return 1;
}

s32 deviceHandler_Flippy_init(file_handle* file) {
	if(devices[DEVICE_CUR] && (devices[DEVICE_CUR]->location & LOC_DVD_CONNECTOR) &&
		devices[DEVICE_CUR] != &__device_flippy &&
		devices[DEVICE_CUR] != &__device_flippyflash) {
		return EBUSY;
	}
	return flippy_init() == FLIPPY_RESULT_OK ? 0 : ENODEV;
}

s32 deviceHandler_Flippy_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->fp) {
		ret = flippy_close(file->fp);
		free(file->fp);
		file->fp = NULL;
		file->status = STATUS_NOT_MAPPED;
	}
	return ret;
}

s32 deviceHandler_Flippy_deinit(file_handle* file) {
	deviceHandler_Flippy_closeFile(file);
	if(getDeviceFromPath(file->name) == &__device_flippyflash)
		flippy_closefrom(FLIPPY_FLASH_HANDLE);
	else
		flippy_close_range(1, FLIPPY_MAX_HANDLES + 1);
	return 0;
}

s32 deviceHandler_Flippy_deleteFile(file_handle* file) {
	deviceHandler_Flippy_closeFile(file);
	if(getDeviceFromPath(file->name) == &__device_flippyflash)
		return flippy_flash_unlink(getDevicePath(file->name));
	else
		return flippy_unlink(getDevicePath(file->name));
}

s32 deviceHandler_Flippy_renameFile(file_handle* file, char* name) {
	deviceHandler_Flippy_closeFile(file);
	int ret = flippy_rename(getDevicePath(file->name), getDevicePath(name));
	if(ret == FLIPPY_RESULT_OK || ret == FLIPPY_RESULT_NO_FILE || ret == FLIPPY_RESULT_NO_PATH)
		strcpy(file->name, name);
	return ret;
}

s32 deviceHandler_Flippy_makeDir(file_handle* dir) {
	return flippy_mkdir(getDevicePath(dir->name));
}

bool deviceHandler_Flippy_test() {
	while (DVD_LowGetCoverStatus() == 0);
	
	if (swissSettings.hasDVDDrive) {
		switch (driveInfo.rel_date) {
			case 0x20010608:
			case 0x20010831:
			case 0x20020402:
			case 0x20020823:
				if (flippy_bypass(false) != FLIPPY_RESULT_OK)
					return false;
				
				if (DVD_Inquiry(&commandBlock, &driveInfo) < 0) {
					swissSettings.hasDVDDrive = 0;
					return false;
				}
				break;
		}
		switch (driveInfo.rel_date) {
			case 0x20220420:
				if (flippy_boot(FLIPPY_MODE_BOOT) != FLIPPY_RESULT_OK ||
					flippy_boot(FLIPPY_MODE_NOUPDATE) != FLIPPY_RESULT_OK)
					return false;
				
				while (driveInfo.rel_date != 0x20220426) {
					if (DVD_Inquiry(&commandBlock, &driveInfo) < 0) {
						swissSettings.hasDVDDrive = 0;
						return false;
					}
				}
			case 0x20220426:
				flippyversion *version = (flippyversion *)driveInfo.pad;
				u32 flippy_version = FLIPPY_VERSION(version->major, version->minor, version->build);
				
				__device_flippy.quirks = QUIRK_NO_DEINIT;
				
				if (flippy_version >= FLIPPY_VERSION(1,3,0) &&
					flippy_version <= FLIPPY_VERSION(1,3,2))
					__device_flippy.quirks |= QUIRK_FDI_BYTESWAP_SIZE;
				
				if (flippy_version < FLIPPY_VERSION(1,3,1)) {
					__device_flippy.extraExtensions = NULL;
					__device_flippy.quirks |= QUIRK_FDI_EXCLUSIVE_OPEN;
				}
				swissSettings.hasFlippyDrive = 1;
				return true;
		}
	}
	
	return false;
}

bool deviceHandler_FlippyFlash_test() {
	return deviceHandler_getDeviceAvailable(&__device_flippy);
}

u32 deviceHandler_Flippy_emulated() {
	if (swissSettings.emulateReadSpeed)
		return EMU_READ | EMU_READ_SPEED;
	else if (swissSettings.emulateEthernet && (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
		return EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING;
	else if (swissSettings.emulateMemoryCard)
		return EMU_READ | EMU_MEMCARD;
	else
		return EMU_READ;
}

char* deviceHandler_Flippy_status(file_handle* file) {
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_flippy = {
	.deviceUniqueId = DEVICE_ID_J,
	.hwName = "FlippyDrive",
	.deviceName = "FlippyDrive",
	.deviceDescription = "Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_FLIPPY, 102, 56, 104, 58},
	.extraExtensions = (char*[]){".fdi", NULL},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	.quirks = QUIRK_NO_DEINIT,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_MEMCARD,
	.location = LOC_DVD_CONNECTOR,
	.initial = &initial_Flippy,
	.test = deviceHandler_Flippy_test,
	.info = deviceHandler_Flippy_info,
	.init = deviceHandler_Flippy_init,
	.makeDir = deviceHandler_Flippy_makeDir,
	.readDir = deviceHandler_Flippy_readDir,
	.seekFile = deviceHandler_Flippy_seekFile,
	.readFile = deviceHandler_Flippy_readFile,
	.writeFile = deviceHandler_Flippy_writeFile,
	.closeFile = deviceHandler_Flippy_closeFile,
	.deleteFile = deviceHandler_Flippy_deleteFile,
	.renameFile = deviceHandler_Flippy_renameFile,
	.setupFile = deviceHandler_Flippy_setupFile,
	.deinit = deviceHandler_Flippy_deinit,
	.emulated = deviceHandler_Flippy_emulated,
	.status = deviceHandler_Flippy_status,
};

DEVICEHANDLER_INTERFACE __device_flippyflash = {
	.deviceUniqueId = DEVICE_ID_K,
	.hwName = "FlippyDrive",
	.deviceName = "FlippyDrive Flash",
	.deviceDescription = "Supported File System(s): FAT12",
	.deviceTexture = {TEX_FLIPPY, 102, 56, 104, 58},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_HYPERVISOR,
	.quirks = QUIRK_NO_DEINIT,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_MEMCARD,
	.location = LOC_DVD_CONNECTOR|LOC_SYSTEM,
	.initial = &initial_FlippyFlash,
	.test = deviceHandler_FlippyFlash_test,
	.info = deviceHandler_Flippy_info,
	.init = deviceHandler_Flippy_init,
	.readDir = deviceHandler_Flippy_readDir,
	.seekFile = deviceHandler_Flippy_seekFile,
	.readFile = deviceHandler_Flippy_readFile,
	.writeFile = deviceHandler_Flippy_writeFile,
	.closeFile = deviceHandler_Flippy_closeFile,
	.deleteFile = deviceHandler_Flippy_deleteFile,
	.setupFile = deviceHandler_Flippy_setupFile,
	.deinit = deviceHandler_Flippy_deinit,
	.emulated = deviceHandler_Flippy_emulated,
	.status = deviceHandler_Flippy_status,
};
