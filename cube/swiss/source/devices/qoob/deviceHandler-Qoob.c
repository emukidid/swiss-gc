/* deviceHandler-Qoob.c
	- device implementation for Qoob Flash
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
	
device_info initial_Qoob_info = {
	0,
	2048
};
	
device_info* deviceHandler_Qoob_info() {
	return &initial_Qoob_info;
}
	
s32 deviceHandler_Qoob_readDir(file_handle* ffile, file_handle** dir, u32 type) {	
  
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading Qoob"));
	// Set everything up to read
	int num_entries = 1, i = 1, block = 0;
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(&(*dir)[0], 0, sizeof(file_handle));
	strcpy((*dir)[0].name,"..");
	(*dir)[0].fileAttrib = IS_SPECIAL;
	
	u32 usedSpace = 0;
	
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
				memset(&(*dir)[i], 0, sizeof(file_handle));
				sprintf((*dir)[i].name, "%s/%s", ffile->name, (char*)dolName);
				(*dir)[i].size		= dolSize;
				(*dir)[i].fileAttrib = IS_FILE;
				(*dir)[i].fileBase = block+0x100;
				++i;
				usedSpace += dolSize;
			}
		}	
	}
	usedSpace >>= 10;
	initial_Qoob_info.freeSpaceInKB = initial_Qoob_info.totalSpaceInKB - usedSpace;
	DrawDispose(msgBox);
	return num_entries;
}

s32 deviceHandler_Qoob_seekFile(file_handle* file, u32 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_Qoob_readFile(file_handle* file, void* buffer, u32 length) {
	__SYS_ReadROM(buffer,length,file->fileBase+file->offset);
	return length;
}

s32 deviceHandler_Qoob_setupFile(file_handle* file, file_handle* file2) {
	return 1;
}

s32 deviceHandler_Qoob_init(file_handle* file) {
	ipl_set_config(0);
	return 1;
}

s32 deviceHandler_Qoob_deinit(file_handle* file) {
	ipl_set_config(6);
	return 0;
}

s32 deviceHandler_Qoob_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_Qoob_test() {
	// Read 1024 bytes from certain sections of the IPL Mask ROM and compare with Qoob enabled/disabled
	char *qoobData = (char*)memalign(32, 0x400);
	char *iplData = (char*)memalign(32, 0x400);
	
	memset(qoobData, 0, 0x400);
	memset(iplData, 0, 0x400);
	
	ipl_set_config(0);
	__SYS_ReadROM(qoobData,0x100,0);
	__SYS_ReadROM(qoobData+0x100,0x100,0x80000);
	__SYS_ReadROM(qoobData+0x200,0x100,0x1FCF00);
	__SYS_ReadROM(qoobData+0x300,0x100,0x1FFF00);
	
	
	ipl_set_config(6);
	__SYS_ReadROM(iplData,0x100,0);
	__SYS_ReadROM(iplData+0x100,0x100,0x80000);
	__SYS_ReadROM(iplData+0x200,0x100,0x1FCF00);
	__SYS_ReadROM(iplData+0x300,0x100,0x1FFF00);
	
	bool qoobFound = memcmp(qoobData, iplData, 0x400) != 0;
	
	free(qoobData);
	free(iplData);
	return qoobFound;
}

DEVICEHANDLER_INTERFACE __device_qoob = {
	DEVICE_ID_7,
	"Qoob Pro",
	"Qoob Pro Flash File System",
	{TEX_QOOB, 70, 80},
	FEAT_READ,
	LOC_SYSTEM,
	&initial_Qoob,
	(_fn_test)&deviceHandler_Qoob_test,
	(_fn_info)&deviceHandler_Qoob_info,
	(_fn_init)&deviceHandler_Qoob_init,
	(_fn_readDir)&deviceHandler_Qoob_readDir,
	(_fn_readFile)&deviceHandler_Qoob_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_Qoob_seekFile,
	(_fn_setupFile)NULL,
	(_fn_closeFile)&deviceHandler_Qoob_closeFile,
	(_fn_deinit)&deviceHandler_Qoob_deinit
};
