/* deviceHandler-gcloader.c
	- device implementation for GCLoader (FAT filesystem)
	by meneerbeer, emu_kidid
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
#include "gcloader.h"
#include "patcher.h"
#include "dvd.h"

FATFS *gcloaderfs = NULL;

file_handle initial_GCLOADER =
	{ "gcldr:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};


device_info initial_GCLOADER_info = {
	0LL,
	0LL
};
	
device_info* deviceHandler_GCLOADER_info() {
	return &initial_GCLOADER_info;
}

s32 deviceHandler_GCLOADER_readDir(file_handle* ffile, file_handle** dir, u32 type){	
	return deviceHandler_FAT_readDir(ffile, dir, type);
}


s32 deviceHandler_GCLOADER_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}


s32 deviceHandler_GCLOADER_readFile(file_handle* file, void* buffer, u32 length){
	return deviceHandler_FAT_readFile(file, buffer, length);	// Same as FAT
}


s32 deviceHandler_GCLOADER_writeFile(file_handle* file, void* buffer, u32 length){
	return deviceHandler_FAT_writeFile(file, buffer, length);
}

static char *bootFile_names[] = {"boot.iso", "boot.iso.iso", "boot.gcm", "boot.gcm.gcm"};

s32 deviceHandler_GCLOADER_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	// GCLoader disc/file fragment setup
	s32 maxDiscFrags = MAX_GCLOADER_FRAGS_PER_DISC;
	u32 discFragList[maxDiscFrags][4];
	s32 discFrags = 0, disc1Frags = 0, disc2Frags = 0;
	
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		if(!(discFrags = getFragments(file2, &discFragList[disc2Frags], maxDiscFrags-disc2Frags, 0, 0, UINT32_MAX, DEVICE_CUR))) {
			return 0;
		}
		disc2Frags+=discFrags;
	}
	
	// write disc 2 frags
	gcloaderWriteFrags(1, discFragList, disc2Frags);
	
	if(numToPatch < -1) {
		file_handle bootFile;
		memset(&bootFile, 0, sizeof(file_handle));
		snprintf(&bootFile.name[0], PATHNAME_MAX, "%sboot.bin", devices[DEVICE_CUR]->initial->name);
		
		if(strstr(IPLInfo,"MPAL")!=NULL)
			GCMDisk.RegionCode = 1;
		else if(strstr(IPLInfo,"PAL")!=NULL)
			GCMDisk.RegionCode = 2;
		else
			GCMDisk.RegionCode = getFontEncode() ? 0:1;
		
		if(devices[DEVICE_CUR]->writeFile(&bootFile, &GCMDisk, sizeof(DiskHeader)) == sizeof(DiskHeader)) {
			if(!(discFrags = getFragments(&bootFile, &discFragList[disc1Frags], maxDiscFrags-disc1Frags, 0, 0, sizeof(DiskHeader), DEVICE_CUR))) {
				return 0;
			}
			disc1Frags+=discFrags;
			devices[DEVICE_CUR]->closeFile(&bootFile);
		}
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(discFrags = getFragments(file, &discFragList[disc1Frags], maxDiscFrags-disc1Frags, 0, 0, UINT32_MAX, DEVICE_CUR))) {
		return 0;
	}
	disc1Frags+=discFrags;
	
	// write disc 1 frags
	gcloaderWriteFrags(0, discFragList, disc1Frags);
	
	// set disc 1 as active disc
	gcloaderWriteDiscNum(0);
	
	if(numToPatch < 0) {
		return 1;
	}
	
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int i;
		u32 (*fragList)[4] = NULL;
		s32 frags = 0, totFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/patches/%.4s/%i", devices[DEVICE_PATCHES]->initial->name, (char*)&GCMDisk, i);
			
			if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) == 0) {
				u32 patchInfo[4];
				memset(patchInfo, 0, 16);
				devices[DEVICE_PATCHES]->seekFile(&patchFile, patchFile.size-16, DEVICE_HANDLER_SEEK_SET);
				if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
					if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
						return 0;
					}
					if(!(frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_DISC_1, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
						free(fragList);
						return 0;
					}
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				else {
					devices[DEVICE_PATCHES]->deleteFile(&patchFile);
					free(fragList);
					return 0;
				}
			}
			else {
				free(fragList);
				return 0;
			}
		}
		
		if(!(fragList = realloc(fragList, (totFrags + 1 + !!file2 + 1) * sizeof(*fragList)))) {
			return 0;
		}
		fragList[totFrags][0] = 0;
		fragList[totFrags][1] = file->size;
		fragList[totFrags][2] = (u16)(file->fileBase >> 32) | ((u8)FRAGS_DISC_1 << 24);
		fragList[totFrags][3] = (u32)(file->fileBase);
		totFrags++;
		
		if(devices[DEVICE_PATCHES] == &__device_gcloader) {
			if(!(fragList = realloc(fragList, (totFrags + maxDiscFrags + 1) * sizeof(*fragList)))) {
				return 0;
			}
			if(!(frags = getFragments(file, &fragList[totFrags], maxDiscFrags, FRAGS_DISC_1, 0, UINT32_MAX, DEVICE_CUR))) {
				free(fragList);
				return 0;
			}
			totFrags += frags;
		}
		
		if(file2) {
			fragList[totFrags][0] = 0;
			fragList[totFrags][1] = file2->size;
			fragList[totFrags][2] = (u16)(file2->fileBase >> 32) | ((u8)FRAGS_DISC_2 << 24);
			fragList[totFrags][3] = (u32)(file2->fileBase);
			totFrags++;
			
			if(devices[DEVICE_PATCHES] == &__device_gcloader) {
				if(!(fragList = realloc(fragList, (totFrags + maxDiscFrags + 1) * sizeof(*fragList)))) {
					return 0;
				}
				if(!(frags = getFragments(file2, &fragList[totFrags], maxDiscFrags, FRAGS_DISC_2, 0, UINT32_MAX, DEVICE_CUR))) {
					free(fragList);
					return 0;
				}
				totFrags += frags;
			}
		}
		
		for(i = 0; i < sizeof(bootFile_names)/sizeof(char*); i++) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%s%s", devices[DEVICE_CUR]->initial->name, bootFile_names[i]);
			
			if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
				return 0;
			}
			if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_BOOT_GCM, 0, UINT32_MAX, DEVICE_CUR))) {
				totFrags+=frags;
				devices[DEVICE_CUR]->closeFile(&patchFile);
				break;
			}
		}
		
		if(swissSettings.igrType == IGR_BOOTBIN) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/patches/apploader.img", devices[DEVICE_PATCHES]->initial->name);
			
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
			
			if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
				return 0;
			}
			if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_APPLOADER, 0x2440, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/saves/MemoryCardA.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				snprintf(txtbuffer, PATHNAME_MAX, "%sswiss/patches/MemoryCardA.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				f_rename(txtbuffer, &patchFile.name[0]);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
					return 0;
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_CARD_A, 0, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_A_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/saves/MemoryCardB.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				snprintf(txtbuffer, PATHNAME_MAX, "%sswiss/patches/MemoryCardB.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				f_rename(txtbuffer, &patchFile.name[0]);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
					return 0;
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_CARD_B, 0, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_B_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
		}
		
		if(fragList) {
			memset(&fragList[totFrags], 0, sizeof(*fragList));
			print_frag_list(fragList);
			
			*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (totFrags + 1) * sizeof(*fragList));
			free(fragList);
			fragList = NULL;
		}
		
		if(devices[DEVICE_PATCHES] != &__device_gcloader) {
			// Card Type
			*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
			// Copy the actual freq
			*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
			// Device slot (0, 1 or 2)
			*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
			*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[*(vu8*)VAR_EXI_SLOT];
		}
	}
	memcpy(VAR_DISC_1_ID, (char*)&GCMDisk, sizeof(VAR_DISC_1_ID));
	return 1;
}

s32 deviceHandler_GCLOADER_init(file_handle* file){

	if(gcloaderfs != NULL) {
		f_unmount("gcldr:/");
		free(gcloaderfs);
		gcloaderfs = NULL;
		disk_shutdown(DEV_GCLDR);
	}
	gcloaderfs = (FATFS*)malloc(sizeof(FATFS));
	int ret = 0;
	
	if(((ret=f_mount(gcloaderfs, "gcldr:/", 1)) == FR_OK) && deviceHandler_getStatEnabled()) {	
		sprintf(txtbuffer, "Reading filesystem info for gcldr:/");
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		
		DWORD freeClusters;
		FATFS *fatfs;
		if(f_getfree("gcldr:/", &freeClusters, &fatfs) == FR_OK) {
			initial_GCLOADER_info.freeSpace = (u64)(freeClusters) * fatfs->csize * fatfs->ssize;
			initial_GCLOADER_info.totalSpace = (u64)(fatfs->n_fatent - 2) * fatfs->csize * fatfs->ssize;
		}
		DrawDispose(msgBox);
	}
	else {
		initial_GCLOADER_info.freeSpace = initial_GCLOADER_info.totalSpace = 0LL;
	}

	return ret == FR_OK;
}

s32 deviceHandler_GCLOADER_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->ffsFp) {
		ret = f_close(file->ffsFp);
		free(file->ffsFp);
		file->ffsFp = 0;
		disk_flush(DEV_GCLDR);
	}
	return ret;
}

s32 deviceHandler_GCLOADER_deinit(file_handle* file) {
	deviceHandler_GCLOADER_closeFile(file);
	initial_GCLOADER_info.freeSpace = 0LL;
	initial_GCLOADER_info.totalSpace = 0LL;
	if(file) {
		f_unmount(file->name);
		free(gcloaderfs);
		gcloaderfs = NULL;
		disk_shutdown(DEV_GCLDR);
	}
	return 0;
}

s32 deviceHandler_GCLOADER_deleteFile(file_handle* file) {
	deviceHandler_GCLOADER_closeFile(file);
	return f_unlink(file->name);
}

bool deviceHandler_GCLOADER_test() {
	if (swissSettings.hasDVDDrive && *(u32*)&driveVersion[4] == 0x20196c64) {
		if (driveVersion[9] == 'w')
			__device_gcloader.features |=  (FEAT_WRITE|FEAT_CONFIG_DEVICE|FEAT_PATCHES);
		else
			__device_gcloader.features &= ~(FEAT_WRITE|FEAT_CONFIG_DEVICE|FEAT_PATCHES);
		return true;
	}
	return false;
}

u32 deviceHandler_GCLOADER_emulated() {
	if (swissSettings.emulateReadSpeed)
		return EMU_READ_SPEED;
	else if (swissSettings.emulateMemoryCard)
		return EMU_MEMCARD;
	else
		return EMU_NONE;
}

DEVICEHANDLER_INTERFACE __device_gcloader = {
	DEVICE_ID_G,
	"GC Loader",
	"GC Loader",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_GCLOADER, 116, 72},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	EMU_READ_SPEED|EMU_MEMCARD,
	LOC_DVD_CONNECTOR,
	&initial_GCLOADER,
	(_fn_test)&deviceHandler_GCLOADER_test,
	(_fn_info)&deviceHandler_GCLOADER_info,
	(_fn_init)&deviceHandler_GCLOADER_init,
	(_fn_readDir)&deviceHandler_GCLOADER_readDir,
	(_fn_readFile)&deviceHandler_GCLOADER_readFile,
	(_fn_writeFile)deviceHandler_GCLOADER_writeFile,
	(_fn_deleteFile)deviceHandler_GCLOADER_deleteFile,
	(_fn_seekFile)&deviceHandler_GCLOADER_seekFile,
	(_fn_setupFile)&deviceHandler_GCLOADER_setupFile,
	(_fn_closeFile)&deviceHandler_GCLOADER_closeFile,
	(_fn_deinit)&deviceHandler_GCLOADER_deinit,
	(_fn_emulated)&deviceHandler_GCLOADER_emulated,
};
