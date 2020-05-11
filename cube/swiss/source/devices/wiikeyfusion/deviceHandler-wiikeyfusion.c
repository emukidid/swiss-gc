/* deviceHandler-wiikeyfusion.c
	- device implementation for Wiikey Fusion (FAT filesystem)
	by emu_kidid
 */

#include <fat.h>
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

const DISC_INTERFACE* wkf = &__io_wkf;
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


device_info initial_WKF_info = {
	0,
	0
};
	
device_info* deviceHandler_WKF_info(file_handle* file) {
	return &initial_WKF_info;
}

s32 deviceHandler_WKF_readDir(file_handle* ffile, file_handle** dir, u32 type){	
	return deviceHandler_FAT_readDir(ffile, dir, type);
}


s32 deviceHandler_WKF_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}


s32 deviceHandler_WKF_readFile(file_handle* file, void* buffer, u32 length){
	return deviceHandler_FAT_readFile(file, buffer, length);	// Same as FAT
}


s32 deviceHandler_WKF_writeFile(file_handle* file, void* buffer, u32 length){
	return -1;
}


s32 deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	
	// If there are 2 discs, we only allow 21 fragments per disc.
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	s32 frags = 0, totFrags = 0;
	
	memset(VAR_FRAG_LIST, 0, sizeof(VAR_FRAG_LIST));

	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		
		for(i = 0; i < numToPatch; i++) {
			u32 patchInfo[4];
			memset(patchInfo, 0, 16);
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%sswiss_patches/%s/%i",devices[DEVICE_PATCHES]->initial->name,gameID, i);
			print_gecko("Looking for file %s\r\n", &patchFile.name);
			FILINFO fno;
			if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
				break;	// Patch file doesn't exist, don't bother with fragments
			}
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
			if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
				if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
					return 0;
				}
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			else {
				break;
			}
		}
		// Check for igr.dol
		memset(&patchFile, 0, sizeof(file_handle));
		sprintf(&patchFile.name[0], "%sigr.dol", devices[DEVICE_PATCHES]->initial->name);

		FILINFO fno;
		if(f_stat(&patchFile.name[0], &fno) == FR_OK) {
			print_gecko("IGR Boot DOL exists\r\n");
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, 0xE0000000, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
				*(vu32*)VAR_IGR_DOL_SIZE = fno.fsize;
			}
		}
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
	}
	else {
		*(vu8*)VAR_SD_SHIFT = 32;
		*(vu8*)VAR_EXI_FREQ = -1;
		*(vu8*)VAR_EXI_SLOT = -1;
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(frags = getFragments(file, &fragList[totFrags*3], maxFrags-totFrags, 0, DISC_SIZE, DEVICE_CUR))) {
		return 0;
	}
	totFrags += frags;
	
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		// TODO fix 2 disc patched games
		if(!(frags = getFragments(file2, &fragList[totFrags*3], maxFrags-totFrags, 0x80000000, DISC_SIZE, DEVICE_CUR))) {
			return 0;
		}
		totFrags += frags;
	}
	
	print_frag_list(0);
	return 1;
}

s32 deviceHandler_WKF_init(file_handle* file){
	
	wkfReinit();
	if(wkffs != NULL) {
		f_unmount("wkf:/");
		free(wkffs);
		wkffs = NULL;
	}
	wkffs = (FATFS*)malloc(sizeof(FATFS));
	int ret = 0;
	
	if(((ret=f_mount(wkffs, "wkf:/", 1)) == FR_OK) && deviceHandler_getStatEnabled()) {	
		sprintf(txtbuffer, "Reading filesystem info for wkf:/");
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		
		DWORD free_clusters, free_sectors, total_sectors = 0;
		if(f_getfree("wkf:/", &free_clusters, &wkffs) == FR_OK) {
			total_sectors = (wkffs->n_fatent - 2) * wkffs->csize;
			free_sectors = free_clusters * wkffs->csize;
			initial_WKF_info.freeSpaceInKB = (u32)((free_sectors)>>1);
			initial_WKF_info.totalSpaceInKB = (u32)((total_sectors)>>1);
		}
		DrawDispose(msgBox);
	}
	else {
		initial_WKF_info.freeSpaceInKB = initial_WKF_info.totalSpaceInKB = 0;	
	}

	return ret == FR_OK;
}

s32 deviceHandler_WKF_deinit(file_handle* file) {
	int ret = 0;
	if(file && file->ffsFp) {
		ret = f_close(file->ffsFp);
		free(file->ffsFp);
		file->ffsFp = 0;
	}
	return ret;
}

s32 deviceHandler_WKF_deleteFile(file_handle* file) {
	return -1;
}

s32 deviceHandler_WKF_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_WKF_test() {
	return swissSettings.hasDVDDrive && (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF);
}

DEVICEHANDLER_INTERFACE __device_wkf = {
	DEVICE_ID_B,
	"WKF / WASP ODE",
	"Wiikey / Wasp Fusion",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_WIIKEY, 116, 72},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_REPLACES_DVD_FUNCS|FEAT_ALT_READ_PATCHES|FEAT_CAN_READ_PATCHES,
	LOC_DVD_CONNECTOR,
	&initial_WKF,
	(_fn_test)&deviceHandler_WKF_test,
	(_fn_info)&deviceHandler_WKF_info,
	(_fn_init)&deviceHandler_WKF_init,
	(_fn_readDir)&deviceHandler_WKF_readDir,
	(_fn_readFile)&deviceHandler_WKF_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_WKF_seekFile,
	(_fn_setupFile)&deviceHandler_WKF_setupFile,
	(_fn_closeFile)&deviceHandler_WKF_closeFile,
	(_fn_deinit)&deviceHandler_WKF_deinit
};
