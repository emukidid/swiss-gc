/* deviceHandler-gcloader.c
	- device implementation for GCLoader (FAT filesystem)
	by meneerbeer, emu_kidid
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
#include "gcloader.h"
#include "patcher.h"

const DISC_INTERFACE* gcloader = &__io_gcloader;
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
	0,
	0
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
	return -1;
}


s32 deviceHandler_GCLOADER_setupFile(file_handle* file, file_handle* file2) {
	
	// GCLoader disc/file fragment setup
	s32 fragListSize = (3 * 4) * (MAX_GCLOADER_FRAGS_PER_DISC*2);
	vu32 *discFragList = (vu32*)memalign(32, fragListSize);
	s32 totFrags = 0, disc1Frags, disc2Frags, maxDiscFrags = (!file2 ? MAX_GCLOADER_FRAGS_PER_DISC : MAX_GCLOADER_FRAGS_PER_DISC>>1);

	memset((void*)discFragList, 0, fragListSize);

	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(disc1Frags = getFragments(file, &discFragList[0], maxDiscFrags, 0, 0, DEVICE_CUR))) {
		return 0;
	}
	devices[DEVICE_CUR]->deinit(file);
	totFrags += disc1Frags;
	
	// Disk 1 base sector
	*(vu32*)VAR_DISC_1_LBA = discFragList[2];
	// Disk 2 base sector (we don't care)
	*(vu32*)VAR_DISC_2_LBA = discFragList[2];
	// Currently selected disk base sector
	*(vu32*)VAR_CUR_DISC_LBA = discFragList[2];
	
	// write disc 1 frags
    gcloaderWriteFrags(0, &discFragList[0], disc1Frags);
	
	
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		memset((void*)discFragList, 0, fragListSize);
		// No fragment room left for the second disc, fail.
		if(totFrags+1 == maxDiscFrags) {
			return 0;
		}
		if(!(disc2Frags = getFragments(file2, &discFragList[0], maxDiscFrags, 0, 0, DEVICE_CUR))) {
			return 0;
		}
		totFrags += disc2Frags;
        gcloaderWriteFrags(1, &discFragList[0], disc2Frags);
		devices[DEVICE_CUR]->deinit(file2);
	}
	
    // set disc 1 as active disc
    gcloaderWriteDiscNum(0);

	// Set up Swiss game patches using a patch supporting device
	memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
#if 0	
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int maxFrags = (VAR_FRAG_SIZE/12), i = 0, frags = 0;
		vu32 *fragList = (vu32*)VAR_FRAG_LIST;
		totFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		
		for(i = 0; i < maxFrags; i++) {
			u32 patchInfo[4];
			patchInfo[0] = 0; patchInfo[1] = 0; 
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%sswiss_patches/%s/%i",devices[DEVICE_PATCHES]->initial->name,gameID, i);
			print_gecko("Looking for file %s\r\n", &patchFile.name);
			FILINFO fno;
			if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
				break;	// Patch file doesn't exist, don't bother with fragments
			}
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
			if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
				if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, patchInfo[0], patchInfo[1] | 0x80000000, DEVICE_PATCHES))) {
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
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, 0x60000000, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
				*(vu32*)VAR_IGR_DOL_SIZE = fno.fsize;
			}
		}
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(9 * sdgecko_getAddressingType(((devices[DEVICE_PATCHES]->location == LOC_MEMCARD_SLOT_A) ? 0:1)));
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(EXI_SPEED16MHZ);	// play it safe
		// Device slot (0 or 1)
		*(vu8*)VAR_EXI_SLOT = (u8)((devices[DEVICE_PATCHES]->location == LOC_MEMCARD_SLOT_A) ? EXI_CHANNEL_0:EXI_CHANNEL_1);
	}
#endif
    return 1;
}

s32 deviceHandler_GCLOADER_init(file_handle* file){

	if(gcloaderfs != NULL) {
		f_mount(0, "gcldr:/", 0);	// Unmount
		free(gcloaderfs);
		gcloaderfs = NULL;
	}
	gcloaderfs = (FATFS*)malloc(sizeof(FATFS));
	int ret = 0;
	
	if(((ret=f_mount(gcloaderfs, "gcldr:/", 0)) == FR_OK) && deviceHandler_getStatEnabled()) {	
		sprintf(txtbuffer, "Reading filesystem info for gcldr:/");
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		
		DWORD free_clusters, free_sectors, total_sectors = 0;
		if(f_getfree("gcldr:/", &free_clusters, &gcloaderfs) == FR_OK) {
			total_sectors = (gcloaderfs->n_fatent - 2) * gcloaderfs->csize;
			free_sectors = free_clusters * gcloaderfs->csize;
			initial_GCLOADER_info.freeSpaceInKB = (u32)((free_sectors)>>1);
			initial_GCLOADER_info.totalSpaceInKB = (u32)((total_sectors)>>1);
		}
		DrawDispose(msgBox);
	}
	else {
		initial_GCLOADER_info.freeSpaceInKB = initial_GCLOADER_info.totalSpaceInKB = 0;	
	}

	return ret == FR_OK;
}

s32 deviceHandler_GCLOADER_deinit(file_handle* file) {
	int ret = 0;
	if(file && file->ffsFp) {
		ret = f_close(file->ffsFp);
		free(file->ffsFp);
		file->ffsFp = 0;
	}
	return ret;
}

s32 deviceHandler_GCLOADER_deleteFile(file_handle* file) {
	return -1;
}

s32 deviceHandler_GCLOADER_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_GCLOADER_test() {
	return swissSettings.hasDVDDrive && (gcloaderReadId() != 0 && gcloaderReadId() != 0xFFFFFFFF);
}

DEVICEHANDLER_INTERFACE __device_gcloader = {
	DEVICE_ID_G,
	"GCLoader",
	"GameCube Optical Drive Emulator",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_GCLOADER, 134, 80},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_BOOT_DEVICE/*|FEAT_CAN_READ_PATCHES*/,
	LOC_DVD_CONNECTOR,
	&initial_GCLOADER,
	(_fn_test)&deviceHandler_GCLOADER_test,
	(_fn_info)&deviceHandler_GCLOADER_info,
	(_fn_init)&deviceHandler_GCLOADER_init,
	(_fn_readDir)&deviceHandler_GCLOADER_readDir,
	(_fn_readFile)&deviceHandler_GCLOADER_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_GCLOADER_seekFile,
	(_fn_setupFile)&deviceHandler_GCLOADER_setupFile,
	(_fn_closeFile)&deviceHandler_GCLOADER_closeFile,
	(_fn_deinit)&deviceHandler_GCLOADER_deinit
};
