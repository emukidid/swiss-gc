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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "exi.h"
#include "bba.h"

/* SMB Globals */
extern int net_initialized;
static struct smb2_context *smb2_context;

file_handle initial_SMB = {
	.name     = "smb:/",
	.fileType = IS_DIR,
	.device   = &__device_smb,
};

device_info initial_SMB_info;

device_info* deviceHandler_SMB_info(file_handle* file) {
	return &initial_SMB_info;
}

void readDeviceInfoSMB() {
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading filesystem info for smb:/"));
	struct smb2_statvfs buf;
	if(smb2_statvfs(smb2_context, "", &buf) == 0) {
		initial_SMB_info.freeSpace = (u64)buf.f_bfree * buf.f_bsize;
		initial_SMB_info.totalSpace = (u64)buf.f_blocks * buf.f_bsize;
		initial_SMB_info.metric = true;
	}
	DrawDispose(msgBox);
}

s32 deviceHandler_SMB_readDir(file_handle* ffile, file_handle** dir, u32 type) {

	struct smb2dir *dp = smb2_opendir(smb2_context, getRelativePath(ffile->name, initial_SMB.name));
	if(!dp) return -1;
	struct smb2dirent *entry;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileType = IS_SPECIAL;
	(*dir)[0].device   = ffile->device;
	
	// Read each entry of the directory
	while((entry = smb2_readdir(smb2_context, dp))) {
		if(!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) {
			continue;
		}
		// Do we want this one?
		if((type == -1 || ((entry->st.smb2_type == SMB2_TYPE_DIRECTORY) ? (type==IS_DIR) : (type==IS_FILE)))) {
			// Make sure we have room for this one
			if(num_entries == i) {
				num_entries *= 2;
				*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			if(concat_path((*dir)[i].name, ffile->name, entry->name) < PATHNAME_MAX && entry->st.smb2_size <= UINT32_MAX) {
				(*dir)[i].fileBase = entry->st.smb2_ino;
				(*dir)[i].size     = entry->st.smb2_size;
				(*dir)[i].fileType = entry->st.smb2_type == SMB2_TYPE_DIRECTORY ? IS_DIR : IS_FILE;
				(*dir)[i].device   = ffile->device;
				++i;
			}
		}
	}
	if(num_entries != i) {
		num_entries = i;
		*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
	}
	
	smb2_closedir(smb2_context, dp);
	return num_entries;
}

s32 deviceHandler_SMB_statFile(file_handle* file) {
	struct smb2_stat_64 st;
	int ret = smb2_stat(smb2_context, getRelativePath(file->name, initial_SMB.name), &st);
	if(ret == 0) {
		file->fileBase = st.smb2_ino;
		file->size     = st.smb2_size;
		file->fileType = st.smb2_type == SMB2_TYPE_DIRECTORY ? IS_DIR : IS_FILE;
	}
	return ret;
}

s64 deviceHandler_SMB_seekFile(file_handle* file, s64 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_SMB_readFile(file_handle* file, void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = smb2_open(smb2_context, getRelativePath(file->name, initial_SMB.name), O_RDONLY);
		if(!file->fp) return -1;
		if(!file->size) {
			if(deviceHandler_SMB_statFile(file)) {
				smb2_close(smb2_context, file->fp);
				file->fp = NULL;
				return -1;
			}
		}
	}
	
	s32 bytes_read = smb2_pread(smb2_context, file->fp, buffer, length, file->offset);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}

s32 deviceHandler_SMB_writeFile(file_handle* file, const void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = smb2_open(smb2_context, getRelativePath(file->name, initial_SMB.name), O_WRONLY | O_CREAT | O_TRUNC);
		if(!file->fp) return -1;
		file->size = 0;
		file->fileType = IS_FILE;
	}
	
	s32 bytes_written = smb2_pwrite(smb2_context, file->fp, buffer, length, file->offset);
	if(bytes_written > 0) file->offset += bytes_written;
	return bytes_written;
}

s32 deviceHandler_SMB_init(file_handle* file) {
	// We need at least a share name and ip addr in the settings filled out
	if(!strlen(swissSettings.smbShare) || !strlen(swissSettings.smbServerIp)) {
		file->status = E_CHECKCONFIG;
		return EFAULT;
	}
	
	if(!init_network()) {
		file->status = E_NONET;
		return EFAULT;
	}
	
	smb2_context = smb2_init_context();
	if(strlen(swissSettings.smbPassword)) smb2_set_password(smb2_context, swissSettings.smbPassword);
	if(smb2_connect_share(smb2_context, swissSettings.smbServerIp, swissSettings.smbShare, swissSettings.smbUser) < 0) {
		smb2_destroy_context(smb2_context);
		smb2_context = NULL;
		file->status = E_CONNECTFAIL;
		return EIO;
	}
	
	readDeviceInfoSMB();
	return 0;
}

s32 deviceHandler_SMB_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->fp) {
		ret = smb2_close(smb2_context, file->fp);
		file->fp = NULL;
	}
	return ret;
}

s32 deviceHandler_SMB_deinit(file_handle* file) {
	deviceHandler_SMB_closeFile(file);
	smb2_disconnect_share(smb2_context);
	smb2_destroy_context(smb2_context);
	smb2_context = NULL;
	return 0;
}

s32 deviceHandler_SMB_deleteFile(file_handle* file) {
	deviceHandler_SMB_closeFile(file);
	if(file->fileType == IS_DIR)
		return smb2_rmdir(smb2_context, getRelativePath(file->name, initial_SMB.name));
	else
		return smb2_unlink(smb2_context, getRelativePath(file->name, initial_SMB.name));
}

s32 deviceHandler_SMB_renameFile(file_handle* file, char* name) {
	deviceHandler_SMB_closeFile(file);
	int ret = smb2_rename(smb2_context, getRelativePath(file->name, initial_SMB.name), getRelativePath(name, initial_SMB.name));
	if(ret == 0)
		strcpy(file->name, name);
	return ret;
}

s32 deviceHandler_SMB_makeDir(file_handle* dir) {
	return smb2_mkdir(smb2_context, getRelativePath(dir->name, initial_SMB.name));
}

bool deviceHandler_SMB_test() {
	__device_smb.hwName = bba_device_str;
	__device_smb.location = bba_location;

	switch (bba_exists(LOC_ANY)) {
		case LOC_MEMCARD_SLOT_A:
			__device_smb.deviceTexture = (textureImage){TEX_GCNET, 65, 84, 72, 88};
			return true;
		case LOC_MEMCARD_SLOT_B:
			if (sdgecko_getDevice(1) == EXI_DEVICE_0)
				__device_smb.deviceTexture = (textureImage){TEX_GCNET, 65, 84, 72, 88};
			else
				__device_smb.deviceTexture = (textureImage){TEX_ETH2GC, 64, 80, 64, 80};
			return true;
		case LOC_SERIAL_PORT_1:
			__device_smb.deviceTexture = (textureImage){TEX_BBA, 140, 64, 140, 64};
			return true;
		case LOC_SERIAL_PORT_2:
			__device_smb.deviceTexture = (textureImage){TEX_ETH2GC, 64, 80, 64, 80};
			return true;
	}
	return net_initialized;
}

char* deviceHandler_SMB_status(file_handle* file) {
	switch(file->status) {
		case E_NONET:
			return "Network has not been initialised";
		case E_CHECKCONFIG:
			return "Check SMB Configuration";
		case E_CONNECTFAIL:
			return "Error connecting to SMB share";
	}
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_smb = {
	.deviceUniqueId = DEVICE_ID_8,
	.hwName = "Broadband Adapter",
	.deviceName = "Server Message Block",
	.deviceDescription = "Configurable via the settings screen",
	.deviceTexture = {TEX_BBA, 140, 64, 140, 64},
	.features = FEAT_READ|FEAT_WRITE,
	.initial = &initial_SMB,
	.test = deviceHandler_SMB_test,
	.info = deviceHandler_SMB_info,
	.init = deviceHandler_SMB_init,
	.makeDir = deviceHandler_SMB_makeDir,
	.readDir = deviceHandler_SMB_readDir,
	.statFile = deviceHandler_SMB_statFile,
	.seekFile = deviceHandler_SMB_seekFile,
	.readFile = deviceHandler_SMB_readFile,
	.writeFile = deviceHandler_SMB_writeFile,
	.closeFile = deviceHandler_SMB_closeFile,
	.deleteFile = deviceHandler_SMB_deleteFile,
	.renameFile = deviceHandler_SMB_renameFile,
	.deinit = deviceHandler_SMB_deinit,
	.status = deviceHandler_SMB_status,
};
