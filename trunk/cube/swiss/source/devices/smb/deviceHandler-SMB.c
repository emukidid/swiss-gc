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
#include <fat.h>
#include "swiss.h"
#include "main.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "deviceHandler-SMB.h"

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
	TEX_SAMBA,
	0,
	0
};
	
device_info* deviceHandler_SMB_info() {
	return &initial_SMB_info;
}

void readDeviceInfoSMB() {
	struct statvfs buf;
	memset(&buf, 0, sizeof(statvfs));
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Reading filesystem info for smb:/");
	DrawFrameFinish();
	
	int res = statvfs("smb:/", &buf);
	initial_SMB_info.freeSpaceInKB = !res ? (u32)((uint64_t)((uint64_t)buf.f_bsize*(uint64_t)buf.f_bfree)/1024LL):0;
	initial_SMB_info.totalSpaceInKB = !res ? (u32)((uint64_t)((uint64_t)buf.f_bsize*(uint64_t)buf.f_blocks)/1024LL):0;
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

int deviceHandler_SMB_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	
   
	// We need at least a share name and ip addr in the settings filled out
	if(!strlen(&swissSettings.smbShare[0]) || !strlen(&swissSettings.smbServerIp[0])) {
		DrawFrameStart();
		sprintf(txtbuffer, "Check Samba Configuration");
		DrawMessageBox(D_FAIL,txtbuffer);
		DrawFrameFinish();
		wait_press_A();
		return SMB_SMBCFGERR;
	}

	if(!net_initialized) {       //Init if we have to
		DrawFrameStart();
		sprintf(txtbuffer, "Network has not been initialised");
		DrawMessageBox(D_FAIL,txtbuffer);
		DrawFrameFinish();
		wait_press_A();
		return SMB_NETINITERR;
	} 

	if(!smb_initialized) {       //Connect to the share
		init_samba();
		if(!smb_initialized) {
			DrawFrameStart();
			sprintf(txtbuffer, "Error initialising Samba");
			DrawMessageBox(D_FAIL,txtbuffer);
			DrawFrameFinish();
			wait_press_A();
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
		if((type == -1 || ((fstat.st_mode & S_IFDIR) ? (type==IS_DIR) : (type==IS_FILE))) && !endsWith(entry->d_name, ".patches")) {
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
			(*dir)[i].fileBase = 0;
			(*dir)[i].meta = 0;
			++i;
		}
	}
	
	closedir(dp);
	return num_entries;
}

int deviceHandler_SMB_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_SMB_readFile(file_handle* file, void* buffer, unsigned int length){
	if(!file->fp) {
		file->fp = fopen( file->name, "rb" );
	}
	if(!file->fp) return -1;
	
	fseek(file->fp, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}

int deviceHandler_SMB_init(file_handle* file){
	return 0;
}

extern char *getDeviceMountPath(char *str);

int deviceHandler_SMB_deinit(file_handle* file) {
	if(smb_initialized) {
		smbClose("smb");
		smb_initialized = 0;
	}
	initial_SMB_info.freeSpaceInKB = 0;
	initial_SMB_info.totalSpaceInKB = 0;
	if(file && file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	if(file) {
		char *mountPath = getDeviceMountPath(file->name);
		print_gecko("Unmounting [%s]\r\n", mountPath);
		fatUnmount(mountPath);
		free(mountPath);
	}
	return 0;
}

