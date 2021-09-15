/**
 * Swiss - deviceHandler-SMB.c (originally from WiiSX)
 * Copyright (C) 2010-2014 emu_kidid
 * 
 * fileBrowser module for Samba based shares
 *
 * WiiSX homepage: https://code.google.com/p/swiss-gc/
 * email address:  emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include <smb.h>
#include <sys/dir.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include "swiss.h"
#include "main.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "deviceHandler-SMB.h"
#include "exi.h"
#include "bba.h"

/* SMB Globals */
extern int net_initialized;
int smb_initialized = 0;

file_handle initial_SMB =
	{ "smb:/", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  IS_DIR,
	  0,
	  0
	};

device_info initial_SMB_info = {
	0LL,
	0LL
};
	
device_info* deviceHandler_SMB_info(file_handle* file) {
	return &initial_SMB_info;
}

void readDeviceInfoSMB() {
	struct statvfs buf;
	memset(&buf, 0, sizeof(statvfs));
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading filesystem info for smb:/"));
	int res = statvfs("smb:/", &buf);
	initial_SMB_info.freeSpace = !res ? (u64)((u64)buf.f_bfree*(u64)buf.f_bsize):0LL;
	initial_SMB_info.totalSpace = !res ? (u64)((u64)buf.f_blocks*(u64)buf.f_bsize):0LL;
	DrawDispose(msgBox);
}
	
// Connect to the share specified in settings.cfg
void init_samba() {
  
	int res = 0;

	if(smb_initialized) {
		return;
	}
	res = smbInit(&swissSettings.smbUser[0], &swissSettings.smbPassword[0], &swissSettings.smbShare[0], &swissSettings.smbServerIp[0]);
	print_gecko("SmbInit %i \r\n",res);
	if(res) {
		smb_initialized = 1;
	}
	else {
		smb_initialized = 0;
	}
}

s32 deviceHandler_SMB_readDir(file_handle* ffile, file_handle** dir, u32 type){	
   
	// We need at least a share name and ip addr in the settings filled out
	if(!strlen(&swissSettings.smbShare[0]) || !strlen(&swissSettings.smbServerIp[0])) {
		sprintf(txtbuffer, "Check SMB Configuration");
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return SMB_SMBCFGERR;
	}

	if(!net_initialized) {       //Init if we have to
		sprintf(txtbuffer, "Network has not been initialised");
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return SMB_NETINITERR;
	} 

	if(!smb_initialized) {       //Connect to the share
		init_samba();
		if(!smb_initialized) {
			sprintf(txtbuffer, "Error initialising SMB");
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return SMB_SMBERR; //fail
		}
		readDeviceInfoSMB();
	}
		
	DIR* dp = opendir( ffile->name );
	if(!dp) return -1;
	struct dirent *entry;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[PATHNAME_MAX];
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(*dir,0,sizeof(file_handle) * num_entries);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");
	
	// Read each entry of the directory
	while( (entry = readdir(dp)) != NULL ){
		if(strlen(entry->d_name) <= 2  && (entry->d_name[0] == '.' || entry->d_name[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,PATHNAME_MAX);
		snprintf(&file_name[0], PATHNAME_MAX, "%s/%s", ffile->name, entry->d_name);
		stat(&file_name[0],&fstat);
		// Do we want this one?
		if((type == -1 || ((fstat.st_mode & S_IFDIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			if(!(fstat.st_mode & S_IFDIR)) {
				if(!checkExtension(entry->d_name)) continue;
			}
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			snprintf((*dir)[i].name, PATHNAME_MAX, "%s/%s", ffile->name, entry->d_name);
			(*dir)[i].offset = 0;
			(*dir)[i].size     = fstat.st_size;
			(*dir)[i].fileAttrib   = (fstat.st_mode & S_IFDIR) ? IS_DIR : IS_FILE;
			++i;
		}
	}
	
	closedir(dp);
	return num_entries;
}

s32 deviceHandler_SMB_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_SMB_readFile(file_handle* file, void* buffer, u32 length){
	if(!file->fp) {
		file->fp = fopen(file->name, "rb");
	}
	if(!file->fp) return -1;
	if(file->size <= 0) {
		fseek(file->fp, 0, SEEK_END);
		file->size = ftell(file->fp);
	}
	
	fseek(file->fp, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}

s32 deviceHandler_SMB_writeFile(file_handle* file, void* buffer, u32 length){
	if(!file->fp) {
		file->fp = fopen(file->name, "wb");
	}
	if(!file->fp) return -1;
	
	fseek(file->fp, file->offset, SEEK_SET);
	int bytes_written = fwrite(buffer, 1, length, file->fp);
	if(bytes_written > 0) file->offset += bytes_written;
	return bytes_written;
}

s32 deviceHandler_SMB_init(file_handle* file) {
	init_network();
	return 1;
}

s32 deviceHandler_SMB_closeFile(file_handle* file) {
	if(file && file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	return 0;
}

s32 deviceHandler_SMB_deinit(file_handle* file) {
	deviceHandler_SMB_closeFile(file);
	initial_SMB_info.freeSpace = 0LL;
	initial_SMB_info.totalSpace = 0LL;
	if(smb_initialized) {
		smbClose("smb");
		smb_initialized = 0;
	}
	return 0;
}

s32 deviceHandler_SMB_deleteFile(file_handle* file) {
	deviceHandler_SMB_closeFile(file);
	return unlink(file->name);
}

bool deviceHandler_SMB_test() {
	return exi_bba_exists();
}

DEVICEHANDLER_INTERFACE __device_smb = {
	DEVICE_ID_8,
	"Broadband Adapter",
	"SMB 1.0/CIFS",
	"Configurable via the settings screen",
	{TEX_SAMBA, 140, 64},
	FEAT_READ|FEAT_WRITE,
	EMU_NONE,
	LOC_SERIAL_PORT_1,
	&initial_SMB,
	(_fn_test)&deviceHandler_SMB_test,
	(_fn_info)&deviceHandler_SMB_info,
	(_fn_init)&deviceHandler_SMB_init,
	(_fn_readDir)&deviceHandler_SMB_readDir,
	(_fn_readFile)&deviceHandler_SMB_readFile,
	(_fn_writeFile)deviceHandler_SMB_writeFile,
	(_fn_deleteFile)deviceHandler_SMB_deleteFile,
	(_fn_seekFile)&deviceHandler_SMB_seekFile,
	(_fn_setupFile)NULL,
	(_fn_closeFile)&deviceHandler_SMB_closeFile,
	(_fn_deinit)&deviceHandler_SMB_deinit,
	(_fn_emulated)NULL,
};
