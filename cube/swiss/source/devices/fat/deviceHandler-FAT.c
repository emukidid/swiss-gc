/* deviceHandler-FAT.c
	- device implementation for FAT (via SD Card Adapters/IDE-EXI)
	by emu_kidid
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/card_cmn.h>
#include <sdcard/card_io.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"
#include "patcher.h"
#include "dvd.h"

static FATFS *fs[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
#define SD_COUNT 3
#define IS_SDCARD(file) (file->name[0] == 's' && file->name[1] == 'd')
int GET_SLOT(file_handle* file) {
	if(IS_SDCARD(file)) {
		return file->name[2] - 'a';
	}
	// IDE-EXI
	return file->name[3] - 'a';
}

file_handle initial_SD_A =
	{ "sda:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_SD_B =
	{ "sdb:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_SD_C =
	{ "sdc:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};	
	
file_handle initial_ATA_A =
	{ "ataa:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_ATA_B =
	{ "atab:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_ATA_C =
	{ "atac:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

static device_info initial_FAT_info[FF_VOLUMES];

device_info* deviceHandler_FAT_info(file_handle* file) {
	device_info* info = NULL;
	DWORD freeClusters;
	FATFS* fatfs;
	if(f_getfree(file->name, &freeClusters, &fatfs) == FR_OK) {
		info = &initial_FAT_info[fatfs->pdrv];
		info->freeSpace = (u64)(freeClusters) * fatfs->csize * fatfs->ssize;
		info->totalSpace = (u64)(fatfs->n_fatent - 2) * fatfs->csize * fatfs->ssize;
		info->metric = true;
	}
	return info;
}

s32 deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir, u32 type) {	

	DIRF* dp = malloc(sizeof(DIRF));
	memset(dp, 0, sizeof(DIRF));
	if(f_opendir(dp, ffile->name) != FR_OK) return -1;
	FILINFO entry;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileAttrib = IS_SPECIAL;

	// Read each entry of the directory
	while( f_readdir(dp, &entry) == FR_OK && entry.fname[0] != '\0') {
		if(!swissSettings.showHiddenFiles && ((entry.fattrib & AM_HID) || entry.fname[0] == '.')) {
			continue;
		}
		// Do we want this one?
		if((type == -1 || ((entry.fattrib & AM_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			if(!(entry.fattrib & AM_DIR)) {
				if(!checkExtension(entry.fname)) continue;
			}
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			if(concat_path((*dir)[i].name, ffile->name, entry.fname) < PATHNAME_MAX && entry.fsize <= UINT32_MAX) {
				(*dir)[i].size       = entry.fsize;
				(*dir)[i].fileAttrib = (entry.fattrib & AM_DIR) ? IS_DIR : IS_FILE;
				++i;
			}
		}
	}
	
	f_closedir(dp);
	free(dp);
	return i;
}

s64 deviceHandler_FAT_seekFile(file_handle* file, s64 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_FAT_readFile(file_handle* file, void* buffer, u32 length) {
	if(!file->ffsFp) {
		file->ffsFp = malloc(sizeof(FIL));
		if(f_open(file->ffsFp, file->name, FA_READ ) != FR_OK) {
			free(file->ffsFp);
			file->ffsFp = NULL;
			return -1;
		}
	}
	f_lseek(file->ffsFp, file->offset);
	
	UINT bytes_read;
	if(f_read(file->ffsFp, buffer, length, &bytes_read) != FR_OK || bytes_read != length) {
		return -1;
	}
	file->offset = f_tell(file->ffsFp);
	file->size = f_size(file->ffsFp);
	return bytes_read;
}

s32 deviceHandler_FAT_writeFile(file_handle* file, const void* buffer, u32 length) {
	if(!file->ffsFp) {
		file->ffsFp = malloc(sizeof(FIL));
		if(f_open(file->ffsFp, file->name, FA_CREATE_ALWAYS | FA_WRITE ) != FR_OK) {
			free(file->ffsFp);
			file->ffsFp = NULL;
			return -1;
		}
		f_expand(file->ffsFp, file->offset + length, 1);
	}
	f_lseek(file->ffsFp, file->offset);
	
	UINT bytes_written;
	if(f_write(file->ffsFp, buffer, length, &bytes_written) != FR_OK || bytes_written != length) {
		return -1;
	}
	file->offset = f_tell(file->ffsFp);
	file->size = f_size(file->ffsFp);
	return bytes_written;
}

s32 deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	// Look for patch files, if we find some, open them and add them as fragments
	file_handle patchFile;
	for(i = 0; i < numToPatch; i++) {
		if(!filesToPatch[i].patchFile) continue;
		if(!getFragments(DEVICE_CUR, filesToPatch[i].patchFile, &fragList, &numFrags, filesToPatch[i].file == file2, filesToPatch[i].offset, filesToPatch[i].size)) {
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
		concat_path(patchFile.name, devices[DEVICE_CUR]->initial->name, "swiss/patches/apploader.img");
		
		ApploaderHeader apploaderHeader;
		if(devices[DEVICE_CUR]->readFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader)) != sizeof(ApploaderHeader) || apploaderHeader.rebootSize != reboot_bin_size) {
			devices[DEVICE_CUR]->deleteFile(&patchFile);
			
			memset(&apploaderHeader, 0, sizeof(ApploaderHeader));
			apploaderHeader.rebootSize = reboot_bin_size;
			
			devices[DEVICE_CUR]->seekFile(&patchFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->writeFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader));
			devices[DEVICE_CUR]->writeFile(&patchFile, reboot_bin, reboot_bin_size);
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
		
		getFragments(DEVICE_CUR, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
		devices[DEVICE_CUR]->closeFile(&patchFile);
	}
	
	if(swissSettings.emulateMemoryCard) {
		if(devices[DEVICE_CUR] != &__device_sd_a && devices[DEVICE_CUR] != &__device_ata_a && devices[DEVICE_CUR] != &__device_ata_c) {
			memset(&patchFile, 0, sizeof(file_handle));
			concatf_path(patchFile.name, devices[DEVICE_CUR]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
			concatf_path(txtbuffer, devices[DEVICE_CUR]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
			ensure_path(DEVICE_CUR, "swiss/saves", NULL);
			devices[DEVICE_CUR]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
			
			if(devices[DEVICE_CUR]->readFile(&patchFile, NULL, 0) != 0) {
				devices[DEVICE_CUR]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->writeFile(&patchFile, NULL, 0);
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
			
			if(getFragments(DEVICE_CUR, &patchFile, &fragList, &numFrags, FRAGS_CARD_A, 0, 31.5*1024*1024))
				*(vu8*)VAR_CARD_A_ID = (patchFile.size * 8/1024/1024) & 0xFC;
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
		
		if(devices[DEVICE_CUR] != &__device_sd_b && devices[DEVICE_CUR] != &__device_ata_b) {
			memset(&patchFile, 0, sizeof(file_handle));
			concatf_path(patchFile.name, devices[DEVICE_CUR]->initial->name, "swiss/patches/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
			concatf_path(txtbuffer, devices[DEVICE_CUR]->initial->name, "swiss/saves/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
			ensure_path(DEVICE_CUR, "swiss/saves", NULL);
			devices[DEVICE_CUR]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
			
			if(devices[DEVICE_CUR]->readFile(&patchFile, NULL, 0) != 0) {
				devices[DEVICE_CUR]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->writeFile(&patchFile, NULL, 0);
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
			
			if(getFragments(DEVICE_CUR, &patchFile, &fragList, &numFrags, FRAGS_CARD_B, 0, 31.5*1024*1024))
				*(vu8*)VAR_CARD_B_ID = (patchFile.size * 8/1024/1024) & 0xFC;
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
	}
	
	if(fragList) {
		print_frag_list(fragList, numFrags);
		*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
		free(fragList);
		fragList = NULL;
	}
	
	s32 exi_channel, exi_device;
	if(getExiDeviceByLocation(devices[DEVICE_CUR]->location, &exi_channel, &exi_device)) {
		if(IS_SDCARD(file)) {
			exi_device = sdgecko_getDevice(exi_channel);
			// Card Type
			*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(exi_channel) ? 0:9;
			// Copy the actual freq
			*(vu8*)VAR_EXI_CPR = (exi_channel << 6) | ((1 << exi_device) << 3) | sdgecko_getSpeed(exi_channel);
		}
		else {
			// Is the HDD in use a 48 bit LBA supported HDD?
			*(vu8*)VAR_ATA_LBA48 = ataDriveInfo.lba48Support;
			// Copy the actual freq
			*(vu8*)VAR_EXI_CPR = (exi_channel << 6) | ((1 << exi_device) << 3) | (swissSettings.exiSpeed ? EXI_SPEED32MHZ:EXI_SPEED16MHZ);
		}
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (exi_device << 2) | exi_channel;
		*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[exi_channel];
	}
	return 1;
}

s32 fatFs_Mount(u8 devNum, char *path) {
	if(fs[devNum] != NULL) {
		print_gecko("Unmount %i devnum, %s path\r\n", devNum, path);
		f_unmount(path);
		free(fs[devNum]);
		fs[devNum] = NULL;
		disk_shutdown(devNum);
	}
	fs[devNum] = (FATFS*)malloc(sizeof(FATFS));
	return f_mount(fs[devNum], path, 1);
}

void setSDGeckoSpeed(int slot, bool fast) {
	sdgecko_setSpeed(slot, fast ? EXI_SPEED32MHZ:EXI_SPEED16MHZ);
	print_gecko("SD speed set to %s\r\n", (fast ? "32MHz":"16MHz"));
}

s32 deviceHandler_FAT_init(file_handle* file) {
	int isSDCard = IS_SDCARD(file);
	int slot = GET_SLOT(file);
	file->status = 0;
	print_gecko("Init %s %i\r\n", (isSDCard ? "SD":"ATA"), slot);
	// SD Card - Slot A
	if(isSDCard && slot == 0) {
		setSDGeckoSpeed(slot, swissSettings.exiSpeed);
		file->status = fatFs_Mount(DEV_SDA, "sda:/");
		if(sdgecko_getSpeed(slot) < EXI_SPEED32MHZ)
			__device_sd_a.quirks |=  QUIRK_EXI_SPEED;
		else
			__device_sd_a.quirks &= ~QUIRK_EXI_SPEED;
	}
	// SD Card - Slot B
	if(isSDCard && slot == 1) {
		setSDGeckoSpeed(slot, swissSettings.exiSpeed);
		file->status = fatFs_Mount(DEV_SDB, "sdb:/");
		if(sdgecko_getSpeed(slot) < EXI_SPEED32MHZ)
			__device_sd_b.quirks |=  QUIRK_EXI_SPEED;
		else
			__device_sd_b.quirks &= ~QUIRK_EXI_SPEED;
	}
	// SD Card - SD2SP2
	if(isSDCard && slot == 2) {
		setSDGeckoSpeed(slot, swissSettings.exiSpeed);
		file->status = fatFs_Mount(DEV_SDC, "sdc:/");
		if(sdgecko_getSpeed(slot) < EXI_SPEED32MHZ)
			__device_sd_c.quirks |=  QUIRK_EXI_SPEED;
		else
			__device_sd_c.quirks &= ~QUIRK_EXI_SPEED;
	}
	// IDE-EXI - Slot A
	if(!isSDCard && slot == 0) {
		file->status = fatFs_Mount(DEV_ATAA, "ataa:/");
	}
	// IDE-EXI - Slot B
	if(!isSDCard && slot == 1) {
		file->status = fatFs_Mount(DEV_ATAB, "atab:/");
	}
	// M.2 Loader
	if(!isSDCard && slot == 2) {
		file->status = fatFs_Mount(DEV_ATAC, "atac:/");
	}
	return file->status == FR_OK ? 0 : EIO;
}

s32 deviceHandler_FAT_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->ffsFp) {
		ret = f_close(file->ffsFp);
		free(file->ffsFp);
		file->ffsFp = NULL;
	}
	return ret;
}

s32 deviceHandler_FAT_deinit(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	if(file) {
		int isSDCard = IS_SDCARD(file);
		int slot = GET_SLOT(file);
		f_unmount(file->name);
		free(fs[isSDCard ? slot : SD_COUNT+slot]);
		fs[isSDCard ? slot : SD_COUNT+slot] = NULL;
		disk_shutdown(isSDCard ? slot : SD_COUNT+slot);
	}
	return 0;
}

s32 deviceHandler_FAT_deleteFile(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	return f_unlink(file->name);
}

s32 deviceHandler_FAT_renameFile(file_handle* file, char* name) {
	deviceHandler_FAT_closeFile(file);
	int ret = f_rename(file->name, name);
	strcpy(file->name, name);
	return ret;
}

s32 deviceHandler_FAT_makeDir(file_handle* dir) {
	return f_mkdir(dir->name);
}

bool deviceHandler_FAT_test_sd_a() {
	bool ret = __io_gcsda.startup() && __io_gcsda.shutdown();

	if(__io_gcsda.features & FEATURE_GAMECUBE_SLOTA) {
		__device_sd_a.deviceName = "SD Card - Slot A";
		__device_sd_a.location = LOC_MEMCARD_SLOT_A;
	}
	else if(__io_gcsda.features & FEATURE_GAMECUBE_PORT1) {
		__device_sd_a.deviceName = "SD Card - SD2SP1";
		__device_sd_a.location = LOC_SERIAL_PORT_1;
	}
	return ret;
}
bool deviceHandler_FAT_test_sd_b() {
	bool ret = __io_gcsdb.startup() && __io_gcsdb.shutdown();

	if (ret) {
		if (sdgecko_getTransferMode(1) == CARDIO_TRANSFER_DMA)
			__device_sd_b.hwName = "Semi-Passive SD Card Adapter";
		else
			__device_sd_b.hwName = "Passive SD Card Adapter";
	}
	return ret;
}
bool deviceHandler_FAT_test_sd_c() {
	bool ret = __io_gcsd2.startup() && __io_gcsd2.shutdown();

	if (ret) {
		if (sdgecko_getTransferMode(2) == CARDIO_TRANSFER_DMA)
			__device_sd_c.hwName = "Semi-Passive SD Card Adapter";
		else if (sdgecko_getDevice(2) == EXI_DEVICE_0)
			__device_sd_c.hwName = "Passive SD Card Adapter";
		else
			__device_sd_c.hwName = "ETH2GC Sidecar+";
	}
	return ret;
}
bool deviceHandler_FAT_test_ata_a() {
	return ide_exi_inserted(0);
}
bool deviceHandler_FAT_test_ata_b() {
	return ide_exi_inserted(1);
}
bool deviceHandler_FAT_test_ata_c() {
	return ide_exi_inserted(2);
}

u32 deviceHandler_FAT_emulated_sd() {
	if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
		swissSettings.emulateAudioStream > 1)
		return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER;
	else if (swissSettings.emulateReadSpeed)
		return EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER;
	else if (swissSettings.emulateEthernet && (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
		return EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER;
	else if (swissSettings.emulateMemoryCard)
		return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
	else
		return EMU_READ | EMU_BUS_ARBITER;
}

u32 deviceHandler_FAT_emulated_ata() {
	if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
		swissSettings.emulateAudioStream > 1)
		return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER;
	else if (swissSettings.emulateEthernet && (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
		return EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER;
	else if (swissSettings.emulateMemoryCard)
		return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
	else
		return EMU_READ | EMU_BUS_ARBITER;
}

char* deviceHandler_FAT_status(file_handle* file) {
	switch(file->status) {
		case FR_OK:			/* (0) Succeeded */
			return NULL;
		case FR_DISK_ERR:
			return "Error occurred in the low level disk I/O layer";
		case FR_INT_ERR:
			return "Assertion failed";
		case FR_NOT_READY:
			return "The physical drive cannot work";
		case FR_NO_FILE:
			return "Could not find the file";
		case FR_NO_PATH:
			return "Could not find the path";
		case FR_INVALID_NAME:
			return "The path name format is invalid";
		case FR_DENIED:
			return "Access denied due to prohibited access or directory full";
		case FR_EXIST:
			return "Access denied due to prohibited access";
		case FR_INVALID_OBJECT:
			return "The file/directory object is invalid";
		case FR_WRITE_PROTECTED:
			return "The physical drive is write protected";
		case FR_INVALID_DRIVE:
			return "The logical drive number is invalid";
		case FR_NOT_ENABLED:
			return "The volume has no work area";
		case FR_NO_FILESYSTEM:
			return "There is no valid FAT volume";
		case FR_MKFS_ABORTED:
			return "The f_mkfs() aborted due to any problem";
		case FR_TIMEOUT:
			return "Could not get a grant to access the volume within defined period";
		case FR_LOCKED:
			return "The operation is rejected according to the file sharing policy";
		case FR_NOT_ENOUGH_CORE:
			return "LFN working buffer could not be allocated";
		case FR_TOO_MANY_OPEN_FILES:
			return "Number of open files > FF_FS_LOCK";
		case FR_INVALID_PARAMETER:
			return "Given parameter is invalid";
		default:
			return "Unknown error occurred";
	}
}

DEVICEHANDLER_INTERFACE __device_sd_a = {
	.deviceUniqueId = DEVICE_ID_1,
	.hwName = "SD Card Adapter",
	.deviceName = "SD Card - Slot A",
	.deviceDescription = "SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_SDSMALL, 59, 78, 64, 80},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_MEMCARD_SLOT_A,
	.initial = &initial_SD_A,
	.test = deviceHandler_FAT_test_sd_a,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_sd,
	.status = deviceHandler_FAT_status,
};

DEVICEHANDLER_INTERFACE __device_sd_b = {
	.deviceUniqueId = DEVICE_ID_2,
	.hwName = "SD Card Adapter",
	.deviceName = "SD Card - Slot B",
	.deviceDescription = "SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_SDSMALL, 59, 78, 64, 80},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_MEMCARD_SLOT_B,
	.initial = &initial_SD_B,
	.test = deviceHandler_FAT_test_sd_b,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_sd,
	.status = deviceHandler_FAT_status,
};

DEVICEHANDLER_INTERFACE __device_ata_a = {
	.deviceUniqueId = DEVICE_ID_3,
	.hwName = "IDE-EXI",
	.deviceName = "IDE-EXI - Slot A",
	.deviceDescription = "IDE/PATA HDD - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_HDD, 104, 73, 104, 76},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_MEMCARD_SLOT_A,
	.initial = &initial_ATA_A,
	.test = deviceHandler_FAT_test_ata_a,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_ata,
	.status = deviceHandler_FAT_status,
};

DEVICEHANDLER_INTERFACE __device_ata_b = {
	.deviceUniqueId = DEVICE_ID_4,
	.hwName = "IDE-EXI",
	.deviceName = "IDE-EXI - Slot B",
	.deviceDescription = "IDE/PATA HDD - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_HDD, 104, 73, 104, 76},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_MEMCARD_SLOT_B,
	.initial = &initial_ATA_B,
	.test = deviceHandler_FAT_test_ata_b,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_ata,
	.status = deviceHandler_FAT_status,
};

DEVICEHANDLER_INTERFACE __device_sd_c = {
	.deviceUniqueId = DEVICE_ID_F,
	.hwName = "SD Card Adapter",
	.deviceName = "SD Card - SD2SP2",
	.deviceDescription = "SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_SDSMALL, 59, 78, 64, 80},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_SERIAL_PORT_2,
	.initial = &initial_SD_C,
	.test = deviceHandler_FAT_test_sd_c,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_sd,
	.status = deviceHandler_FAT_status,
};

DEVICEHANDLER_INTERFACE __device_ata_c = {
	.deviceUniqueId = DEVICE_ID_H,
	.hwName = "M.2 Loader",
	.deviceName = "M.2 Loader",
	.deviceDescription = "M.2 SATA SSD - Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_M2LOADER, 112, 54, 112, 56},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING|FEAT_EXI_SPEED,
	.emulable = EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	.location = LOC_SERIAL_PORT_1,
	.initial = &initial_ATA_C,
	.test = deviceHandler_FAT_test_ata_c,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_FAT_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.setupFile = deviceHandler_FAT_setupFile,
	.deinit = deviceHandler_FAT_deinit,
	.emulated = deviceHandler_FAT_emulated_sd,
	.status = deviceHandler_FAT_status,
};
