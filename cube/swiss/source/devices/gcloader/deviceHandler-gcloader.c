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


s32 deviceHandler_GCLOADER_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	
	// GCLoader disc/file fragment setup
	s32 maxDiscFrags = MAX_GCLOADER_FRAGS_PER_DISC;
	u32 discFragList[maxDiscFrags*3];
	s32 disc1Frags = 0, disc2Frags = 0;

	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		devices[DEVICE_CUR]->seekFile(file2, 0, DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_CUR]->readFile(file2, VAR_DISC_2_ID, sizeof(VAR_DISC_2_ID));
		
		if(!(disc2Frags = getFragments(file2, &discFragList[0], maxDiscFrags, 0, DISC_SIZE, DEVICE_CUR))) {
			return 0;
		}
	}

	// write disc 2 frags
	gcloaderWriteFrags(1, &discFragList[0], disc2Frags);

	devices[DEVICE_CUR]->seekFile(file, 0, DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, VAR_DISC_1_ID, sizeof(VAR_DISC_1_ID));
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(disc1Frags = getFragments(file, &discFragList[0], maxDiscFrags, 0, DISC_SIZE, DEVICE_CUR))) {
		return 0;
	}
	
	// write disc 1 frags
    gcloaderWriteFrags(0, &discFragList[0], disc1Frags);
	
    // set disc 1 as active disc
    gcloaderWriteDiscNum(0);

	// Set up Swiss game patches using a patch supporting device
	memset(VAR_FRAG_LIST, 0, sizeof(VAR_FRAG_LIST));

	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
		vu32 *fragList = (vu32*)VAR_FRAG_LIST;
		s32 frags = 0, totFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/%.4s/%i", devices[DEVICE_PATCHES]->initial->name, (char*)&GCMDisk, i);
			
			if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) == 0) {
				u32 patchInfo[4];
				memset(patchInfo, 0, 16);
				devices[DEVICE_PATCHES]->seekFile(&patchFile, patchFile.size-16, DEVICE_HANDLER_SEEK_SET);
				if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
					if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
						return 0;
					}
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				else {
					devices[DEVICE_PATCHES]->deleteFile(&patchFile);
					return 0;
				}
			}
			else {
				return 0;
			}
		}
		// Check for igr.dol
		if(swissSettings.igrType == IGR_BOOTBIN) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sigr.dol", devices[DEVICE_PATCHES]->initial->name);
			
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_IGR_DOL, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/MemoryCardA.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_A, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_A_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/MemoryCardB.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_B, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_B_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
		}
		
		memset(&patchFile, 0, sizeof(file_handle));
		snprintf(&patchFile.name[0], PATHNAME_MAX, "%sboot.iso", devices[DEVICE_CUR]->initial->name);
		
		if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_DISC_1, DISC_SIZE, DEVICE_CUR))) {
			totFrags+=frags;
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
		
		print_frag_list(0);
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
		*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[*(vu8*)VAR_EXI_SLOT];
	}
	else {
		*(vu8*)VAR_SD_SHIFT = 32;
		*(vu8*)VAR_EXI_FREQ = EXI_SPEED1MHZ;
		*(vu8*)VAR_EXI_SLOT = EXI_CHANNEL_MAX;
		*(vu32**)VAR_EXI_REGS = NULL;
	}

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
			__device_gcloader.features |=  (FEAT_WRITE|FEAT_CONFIG_DEVICE);
		else
			__device_gcloader.features &= ~(FEAT_WRITE|FEAT_CONFIG_DEVICE);
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
