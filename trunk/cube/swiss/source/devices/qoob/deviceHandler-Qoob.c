/* deviceHandler-Qoob.c
	- device implementation for Qoob Flash
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
#include "exi.h"

char iplBlock[256] __attribute__((aligned(32)));

file_handle initial_Qoob =
	{ "qoob:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};
	
int deviceHandler_Qoob_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	
  
	// Set everything up to read
	int num_entries = 1, i = 1, block = 0;
	*dir = malloc( num_entries * sizeof(file_handle) );
	strcpy((*dir)[0].name,"..");
	(*dir)[0].fileAttrib = IS_SPECIAL;
	
	for(block = 0; block <0x200000; block+=0x10000) {
		char dolName[64];
		u32 dolSize = 0;
		__SYS_ReadROM(iplBlock,256,block);
		// DOL blocks are all marked by "ELF "
		if (*(u32*)&iplBlock[0] == 0x454C4600) { //"ELF "
			memset(dolName, 0, 64);
			sprintf(&dolName[0],"%s.dol",&iplBlock[4]);
			dolSize = (*(u16*)&iplBlock[0xFC])*0x10000;
			__SYS_ReadROM(iplBlock,256,block+256);
			if (*(u32*)&iplBlock[0] == 0x00000100) { //DOL Header
				// Make sure we have room for this one
				if(i == num_entries){
					++num_entries;
					*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
				}
				sprintf((*dir)[i].name, "%s/%s", ffile->name, (char*)dolName);
				(*dir)[i].offset	= 0;
				(*dir)[i].size		= dolSize;
				(*dir)[i].fileAttrib = IS_FILE;
				(*dir)[i].fileBase = block+0x100;
				++i;
			}
		}	
	}
	
  return num_entries;
}

int deviceHandler_Qoob_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_Qoob_readFile(file_handle* file, void* buffer, unsigned int length){
	
	__SYS_ReadROM(buffer,length,file->fileBase+file->offset);
	return length;
}

int deviceHandler_Qoob_setupFile(file_handle* file, file_handle* file2) {
	return 1;
}

int deviceHandler_Qoob_init(file_handle* file){
		
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Reading Qoob");
	DrawFrameFinish();
	ipl_set_config(0);
	return 0;
}

int deviceHandler_Qoob_deinit(file_handle* file) {
	return 0;
}

