/* deviceHandler-wiikeyfusion.c
	- device implementation for Wiikey Fusion (FAT filesystem)
	by emu_kidid
 */

#include <fat.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <sys/dir.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "wkf.h"
#include "frag.h"

const DISC_INTERFACE* wkf = &__io_wkf;
int singleFileMode = 0;

file_handle initial_WKF =
	{ "wkf:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};


int deviceHandler_WKF_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	
	if(singleFileMode) {
		return -1;
	}
	
	DIR* dp = opendir( ffile->name );
	if(!dp) return -1;
	struct dirent *entry;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[1024];
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(*dir,0,sizeof(file_handle) * num_entries);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");
	
	// Read each entry of the directory
	while( (entry = readdir(dp)) != NULL ){
		if(strlen(entry->d_name) <= 2  && (entry->d_name[0] == '.' || entry->d_name[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,1024);
		sprintf(&file_name[0], "%s/%s", ffile->name, entry->d_name);
		stat(&file_name[0],&fstat);
		// Do we want this one?
		if(type == -1 || ((fstat.st_mode & S_IFDIR) ? (type==IS_DIR) : (type==IS_FILE))) {
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			sprintf((*dir)[i].name, "%s/%s", ffile->name, entry->d_name);
			(*dir)[i].offset = 0;
			(*dir)[i].size     = fstat.st_size;
			(*dir)[i].fileAttrib   = (fstat.st_mode & S_IFDIR) ? IS_DIR : IS_FILE;
			(*dir)[i].fp = 0;
			if((*dir)[i].fileAttrib == IS_FILE) {
				get_frag_list((*dir)[i].name);
				u32 file_base = frag_list->num > 1 ? -1 : frag_list->frag[0].sector;
				(*dir)[i].fileBase = file_base;
			}
			else {
				(*dir)[i].fileBase = 0;
			}		
			++i;
		}
	}
	
	closedir(dp);
	return num_entries;
}


int deviceHandler_WKF_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}


int deviceHandler_WKF_readFile(file_handle* file, void* buffer, unsigned int length){
	if(singleFileMode) {
		wkfRead(buffer, length, file->offset);
		file->offset += length;
		return length;
	}
	else {
		if(!file->fp) {
			file->fp = fopen( file->name, "rb" );
		}
		if(!file->fp) return -1;
		
		fseek(file->fp, file->offset, SEEK_SET);
		int bytes_read = fread(buffer, 1, length, file->fp);
		if(bytes_read > 0) file->offset += bytes_read;
		return bytes_read;
	}
}


int deviceHandler_WKF_writeFile(file_handle* file, void* buffer, unsigned int length){
	return -1;
}


int deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2) {
	if(file) {
		// Lookup FAT fragments
		get_frag_list(file->name);

		// Setup fragments if need be
		if(frag_list->num > 1) {
			/*// Setup the Fragments on the Wiikey Fusion
			int i = 0;
			for(i = 0; i < frag_list->num; i++) {		
				wkfWriteRam(i*4, (frag_list->frag[i].sector>>16)&0xFFFF);
				wkfWriteRam(i*4+2, frag_list->frag[i].sector&0xFFFF);
			}
			// last chunk sig
			wkfWriteRam(frag_list->num *4, 0xFFFF);*/
		}
		wkfWriteOffset(frag_list->frag[0].sector);
		
		if(((u32)(file->fileBase&0xFFFFFFFF) == -1)) {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Fragmented file support is not implemented yet.");
			DrawFrameFinish();
			sleep(5);
		}
		singleFileMode = 1;
	}
	else {
		wkfReinit();
		singleFileMode = 0;
	}
	return 1;
}

int deviceHandler_WKF_init(file_handle* file){
	if(!singleFileMode) {
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Init Wiikey Fusion");
		DrawFrameFinish();
		return fatMountSimple ("wkf", wkf) ? 1 : 0;
	}
	return 0;
}

int deviceHandler_WKF_deinit(file_handle* file) {
	if(!singleFileMode && file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	return 0;
}

int deviceHandler_WKF_deleteFile(file_handle* file) {
	return -1;
}

