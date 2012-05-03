/* deviceHandler-USBGecko.c
	- device implementation for USBGecko
	by emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/dir.h>
#include <ogc/machine/processor.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "usbgecko.h"

file_handle initial_USBGecko =
	{ ".",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

int deviceHandler_USBGecko_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	
  
	char *dirName = &ffile->name[0];
	int len = strlen(ffile->name);
	while(ffile->name[len-1] != '.') {
		len--;
	}
	dirName = &ffile->name[len-1];
	
	// Set everything up to read
	int num_entries = 0, i = 0;
	file_handle *entry = NULL;
	if(strlen(ffile->name)!=1) {
		i = num_entries = 1;
		*dir = malloc( num_entries * sizeof(file_handle) );
		memset(*dir,0,sizeof(file_handle) * num_entries);
		(*dir)[0].fileAttrib = IS_SPECIAL;
		strcpy((*dir)[0].name, "..");
	}
	
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Read directory!");
	DrawFrameFinish();
	// Read each entry of the directory
	int res = usbgecko_open_dir(dirName);
	if(!res) return -1;
	while( (entry = usbgecko_get_entry()) != NULL ){
		// Make sure we have room for this one
		if(i == num_entries) {
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		}
		sprintf((*dir)[i].name, "%s/%s", dirName, entry->name);
		(*dir)[i].offset		= 0;
		(*dir)[i].size			= entry->size;
		(*dir)[i].fileAttrib	= entry->fileAttrib;
		(*dir)[i].fp 			= 0;
		(*dir)[i].fileBase		= 0;
		++i;
	}
	
  return num_entries;
}

int deviceHandler_USBGecko_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_USBGecko_readFile(file_handle* file, void* buffer, unsigned int length){
  	int bytes_read = usbgecko_read_file(buffer, length, file->offset, file->name);
	if(bytes_read > 0) file->offset += bytes_read;
	
	return bytes_read;
}

int deviceHandler_USBGecko_writeFile(file_handle* file, void* buffer, unsigned int length) {	
	int bytes_written = usbgecko_write_file(buffer, length, file->offset, file->name);
	if(bytes_written > 0) file->offset += bytes_written;
	
	return bytes_written;
}

void deviceHandler_USBGecko_setupFile(file_handle* file, file_handle* file2) {
}

int deviceHandler_USBGecko_init(file_handle* file) {
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Looking for USBGecko in Slot B");
	DrawFrameFinish();
	if(usb_isgeckoalive(1)) {
		int retries = 1000;
		
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Waiting for PC ...");
		DrawFrameFinish();
		
		usb_flush(1);
		usbgecko_lock_file(0);
		// Wait for the PC and retry 1000 times
		while(!usbgecko_pc_ready() && retries) {
			VIDEO_WaitVSync();
			retries--;
		}
		if(!retries) {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Couldn't find PC!");
			DrawFrameFinish();
			sleep(5);
			return 0;	// Didn't find the PC
		}
		else {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Found PC !!");
			DrawFrameFinish();
			return 1;
		}
	}
	else {
		return 0;
	}
}

int deviceHandler_USBGecko_deinit(file_handle* file) {
	return 0;
}

int deviceHandler_USBGecko_deleteFile(file_handle* file) {
	return -1;
}
