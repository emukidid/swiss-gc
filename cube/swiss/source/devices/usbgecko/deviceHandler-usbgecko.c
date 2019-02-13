/* deviceHandler-USBGecko.c
	- device implementation for USBGecko
	by emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/machine/processor.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "usbgecko.h"
#include "patcher.h"

file_handle initial_USBGecko =
	{ "./",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

device_info initial_USBGecko_info = {
	0,
	0
};

extern char *getRelativeName(char *str);
	
device_info* deviceHandler_USBGecko_info() {
	return &initial_USBGecko_info;
}
	
s32 deviceHandler_USBGecko_readDir(file_handle* ffile, file_handle** dir, u32 type){	
  
	// Set everything up to read
	s32 num_entries = 0, i = 0;
	file_handle *entry = NULL;
	if(strlen(ffile->name)!=1) {
		i = num_entries = 1;
		*dir = malloc( num_entries * sizeof(file_handle) );
		memset(*dir,0,sizeof(file_handle) * num_entries);
		(*dir)[0].fileAttrib = IS_SPECIAL;
		strcpy((*dir)[0].name, "..");
	}
	
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading directory"));
	// Read each entry of the directory
	s32 res = usbgecko_open_dir(&ffile->name[0]);
	if(!res) return -1;
	while( (entry = usbgecko_get_entry()) != NULL ){
		if(entry->fileAttrib == IS_FILE) {
			if(!checkExtension(entry->name)) continue;
		}		
		// Make sure we have room for this one
		if(i == num_entries) {
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		}
		memset(&(*dir)[i], 0, sizeof(file_handle));
		sprintf((*dir)[i].name, "%s", entry->name);
		(*dir)[i].size			= entry->size;
		(*dir)[i].fileAttrib	= entry->fileAttrib;
		++i;
	}
	DrawDispose(msgBox);
	return num_entries;
}

s32 deviceHandler_USBGecko_seekFile(file_handle* file, s32 where, s32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_USBGecko_readFile(file_handle* file, void* buffer, u32 length){
	s32 reallength = length;
	if(file->offset + length > file->size) {
		reallength = file->size - file->offset;
	}
	if(reallength < 0) {
		return 0;
	}
  	s32 bytes_read = usbgecko_read_file(buffer, reallength, file->offset, file->name);
	if(bytes_read > 0) file->offset += bytes_read;
	
	return bytes_read;
}

s32 deviceHandler_USBGecko_writeFile(file_handle* file, void* buffer, u32 length) {	
	s32 bytes_written = usbgecko_write_file(buffer, length, file->offset, file->name);
	if(bytes_written > 0) file->offset += bytes_written;
	
	return bytes_written;
}

s32 deviceHandler_USBGecko_setupFile(file_handle* file, file_handle* file2) {
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
	fragList[1] = file->size;
	*(vu32*)VAR_DISC_1_LBA = 0;
	*(vu32*)VAR_DISC_2_LBA = 0;
	*(vu32*)VAR_CUR_DISC_LBA = 0;
	*(vu32*)VAR_TMP1 = 0;
	*(vu32*)VAR_TMP2 = 0;
	return 1;
}

s32 deviceHandler_USBGecko_init(file_handle* file) {
	s32 success = 0;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Looking for USBGecko in Slot B"));
	if(usb_isgeckoalive(1)) {
		s32 retries = 1000;
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawProgressBar(true, 0, "Waiting for PC ..."));
		
		usb_flush(1);
		usbgecko_lock_file(0);
		// Wait for the PC and retry 1000 times
		while(!usbgecko_pc_ready() && retries) {
			VIDEO_WaitVSync();
			retries--;
		}
		success = retries > 1 ? 1 : 0;
		if(!success) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_INFO,"Couldn't find PC!"));
			sleep(5);
		}
	}
	DrawDispose(msgBox);
	return success;
}

s32 deviceHandler_USBGecko_deinit(file_handle* file) {
	return 0;
}

s32 deviceHandler_USBGecko_deleteFile(file_handle* file) {
	return -1;
}

s32 deviceHandler_USBGecko_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_USBGecko_test() {
	return usb_isgeckoalive(1);
}

DEVICEHANDLER_INTERFACE __device_usbgecko = {
	DEVICE_ID_A,
	"USB Gecko - Slot B only",
	"Requires PC application to be up",
	{TEX_USBGECKO, 129, 80},
	FEAT_READ|FEAT_BOOT_GCM,
	LOC_MEMCARD_SLOT_B,
	&initial_USBGecko,
	(_fn_test)&deviceHandler_USBGecko_test,
	(_fn_info)&deviceHandler_USBGecko_info,
	(_fn_init)&deviceHandler_USBGecko_init,
	(_fn_readDir)&deviceHandler_USBGecko_readDir,
	(_fn_readFile)&deviceHandler_USBGecko_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_USBGecko_seekFile,
	(_fn_setupFile)&deviceHandler_USBGecko_setupFile,
	(_fn_closeFile)&deviceHandler_USBGecko_closeFile,
	(_fn_deinit)&deviceHandler_USBGecko_deinit
};
