/* deviceHandler-FSP.c
	- device implementation for File Sharing Protocol
	by Extrems
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "fsplib.h"
#include "exi.h"
#include "bba.h"
#include "patcher.h"

extern int net_initialized;
static FSP_SESSION *fsp_session;

file_handle initial_FSP = {
	.name     = "fsp:/",
	.fileType = IS_DIR,
	.device   = &__device_fsp,
};

device_info* deviceHandler_FSP_info(file_handle* file) {
	return NULL;
}

s32 deviceHandler_FSP_readDir(file_handle* ffile, file_handle** dir, u32 type) {

	FSP_DIR* dp = fsp_opendir(fsp_session, getDevicePath(ffile->name));
	if(!dp) return -1;
	FSP_RDENTRY entry;
	FSP_RDENTRY *result;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileType = IS_SPECIAL;
	(*dir)[0].device   = ffile->device;
	
	// Read each entry of the directory
	while( !fsp_readdir_native(dp, &entry, &result) && result == &entry ){
		if(!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) {
			continue;
		}
		// Do we want this one?
		if((type == -1 || ((entry.type == FSP_RDTYPE_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			if(concat_path((*dir)[i].name, ffile->name, entry.name) < PATHNAME_MAX) {
				(*dir)[i].size     = entry.size;
				(*dir)[i].fileType = (entry.type == FSP_RDTYPE_DIR) ? IS_DIR : IS_FILE;
				(*dir)[i].device   = ffile->device;
				++i;
			}
		}
	}
	fsp_closedir(dp);
	return i;
}

s32 deviceHandler_FSP_statFile(file_handle* file) {
	struct stat fstat;
	int ret = fsp_stat(fsp_session, getDevicePath(file->name), &fstat);
	if(ret == 0) {
		file->size     = fstat.st_size;
		file->fileType = S_ISDIR(fstat.st_mode) ? IS_DIR : IS_FILE;
	}
	return ret;
}

s64 deviceHandler_FSP_seekFile(file_handle* file, s64 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_FSP_readFile(file_handle* file, void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = fsp_fopen(fsp_session, getDevicePath(file->name), "rb");
		if(!file->fp) return -1;
		if(!file->size) {
			if(deviceHandler_FSP_statFile(file)) {
				fsp_fclose(file->fp);
				file->fp = NULL;
				return -1;
			}
		}
	}
	
	fsp_fseek(file->fp, file->offset, SEEK_SET);
	size_t bytes_read = fsp_fread(buffer, 1, length, file->fp);
	file->offset = fsp_ftell(file->fp);
	return bytes_read;
}

s32 deviceHandler_FSP_writeFile(file_handle* file, const void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = fsp_fopen(fsp_session, getDevicePath(file->name), "wb");
		if(!file->fp) return -1;
		file->size = 0;
		file->fileType = IS_FILE;
	}
	
	fsp_fseek(file->fp, file->offset, SEEK_SET);
	size_t bytes_written = fsp_fwrite(buffer, 1, length, file->fp);
	file->offset = fsp_ftell(file->fp);
	return bytes_written;
}

s32 deviceHandler_FSP_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0) {
		return 0;
	}
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_debug("Save Patch device found\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			if(!filesToPatch[i].patchFile) continue;
			if(!getFragments(DEVICE_PATCHES, filesToPatch[i].patchFile, &fragList, &numFrags, filesToPatch[i].file == file2, filesToPatch[i].offset, filesToPatch[i].size)) {
				free(fragList);
				return 0;
			}
		}
		
		if(swissSettings.igrType == IGR_APPLOADER || endsWith(file->name,".tgc")) {
			memset(&patchFile, 0, sizeof(file_handle));
			concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
			
			getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
		}
		
		if(devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			s32 exi_channel, exi_device;
			if(getExiDeviceByLocation(devices[DEVICE_PATCHES]->location, &exi_channel, &exi_device)) {
				exi_device = sdgecko_getDevice(exi_channel);
				// Card Type
				*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(exi_channel) ? 0:9;
				// Copy the actual freq
				*(vu8*)VAR_EXI_CPR = (exi_channel << 6) | ((1 << exi_device) << 3) | sdgecko_getSpeed(exi_channel);
				// Device slot (0, 1 or 2)
				*(vu8*)VAR_EXI_SLOT = (*(vu8*)VAR_EXI_SLOT & 0xF0) | (((exi_device << 2) | exi_channel) & 0x0F);
				*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[exi_channel];
			}
		}
	}
	
	if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, 0)) {
		free(fragList);
		return 0;
	}
	
	if(file2) {
		if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, 0)) {
			free(fragList);
			return 0;
		}
	}
	
	if(fragList) {
		print_frag_list(fragList, numFrags);
		*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
		free(fragList);
		fragList = NULL;
	}
	
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	net_getpeername(fsp_session->fd, (struct sockaddr *)&addr, &addrlen);
	
	u32 local_ip = bba_localip.s_addr;
	u32 netmask = bba_netmask.s_addr;
	u32 gateway = bba_gateway.s_addr;
	u32 host_ip = addr.sin_addr.s_addr;
	u16 port = addr.sin_port;
	
	gateway = (host_ip & netmask) == (local_ip & netmask) ? host_ip : gateway;
	addr.sin_addr.s_addr = gateway;
	addr.sin_port = 0;
	
	struct arpreq arpreq = {
		.arp_ha.sa_family = AF_UNSPEC,
		.arp_ha.sa_data = {0xff,0xff,0xff,0xff,0xff,0xff},
		.arp_dev = "en0"
	};
	memcpy(&arpreq.arp_pa, &addr, addrlen);
	net_ioctl(fsp_session->fd, SIOCGARP, &arpreq);
	
	memcpy(VAR_ROUTER_MAC, arpreq.arp_ha.sa_data, sizeof(VAR_ROUTER_MAC));
	*(vu32*)VAR_CLIENT_IP = local_ip;
	*(vu32*)VAR_ROUTER_IP = gateway;
	*(vu32*)VAR_SERVER_IP = host_ip;
	*(vu16*)VAR_SERVER_PORT = port;
	*(vu16*)VAR_SERVER_PMTU = in_range(swissSettings.fspPathMtu, 576, 2030) ? swissSettings.fspPathMtu : 1500;
	return 1;
}

s32 deviceHandler_FSP_init(file_handle* file) {
	if(!init_network()) {
		file->status = E_NONET;
		return EFAULT;
	}
	
	fsp_session = fsp_open_session(swissSettings.fspHostIp, swissSettings.fspPort, swissSettings.fspPassword);
	if(!fsp_session) {
		file->status = E_CHECKCONFIG;
		return EFAULT;
	}
	
	u8 dirpro;
	if(fsp_getpro(fsp_session, "swiss/patches", &dirpro) &&
		fsp_getpro(fsp_session, "swiss", &dirpro) &&
		fsp_getpro(fsp_session, "", &dirpro))
		dirpro = 0;
	
	if((dirpro & (FSP_DIR_MKDIR|FSP_DIR_ADD|FSP_DIR_DEL|FSP_DIR_GET)) == (FSP_DIR_MKDIR|FSP_DIR_ADD|FSP_DIR_DEL) ||
		(dirpro & FSP_DIR_OWNER)) {
		__device_fsp.features |=  FEAT_PATCHES;
		__device_fsp.emulable |=  EMU_ETHERNET;
	}
	else {
		__device_fsp.features &= ~FEAT_PATCHES;
		__device_fsp.emulable &= ~EMU_ETHERNET;
	}
	return 0;
}

s32 deviceHandler_FSP_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->fp) {
		ret = fsp_fclose(file->fp);
		file->fp = NULL;
	}
	return ret;
}

s32 deviceHandler_FSP_deinit(file_handle* file) {
	deviceHandler_FSP_closeFile(file);
	fsp_close_session(fsp_session);
	fsp_session = NULL;
	return 0;
}

s32 deviceHandler_FSP_deleteFile(file_handle* file) {
	deviceHandler_FSP_closeFile(file);
	if(file->fileType == IS_DIR)
		return fsp_rmdir(fsp_session, getDevicePath(file->name));
	else
		return fsp_unlink(fsp_session, getDevicePath(file->name));
}

s32 deviceHandler_FSP_renameFile(file_handle* file, char* name) {
	deviceHandler_FSP_closeFile(file);
	int ret = fsp_rename(fsp_session, getDevicePath(file->name), getDevicePath(name));
	if(ret == 0)
		strcpy(file->name, name);
	return ret;
}

s32 deviceHandler_FSP_makeDir(file_handle* dir) {
	return fsp_mkdir(fsp_session, getDevicePath(dir->name));
}

bool deviceHandler_FSP_test() {
	__device_fsp.hwName = bba_device_str;
	__device_fsp.location = bba_location;

	if (strcmp(bba_device_str, "Broadband Adapter")) {
		__device_fsp.features = FEAT_READ | FEAT_WRITE | FEAT_THREAD_SAFE;
		__device_fsp.emulable = EMU_NONE;
	}
	switch (bba_exists(LOC_ANY)) {
		case LOC_MEMCARD_SLOT_A:
			__device_fsp.deviceTexture = (textureImage){TEX_GCNET, 65, 84, 72, 88};
			return true;
		case LOC_MEMCARD_SLOT_B:
			if (sdgecko_getDevice(1) == EXI_DEVICE_0)
				__device_fsp.deviceTexture = (textureImage){TEX_GCNET, 65, 84, 72, 88};
			else
				__device_fsp.deviceTexture = (textureImage){TEX_ETH2GC, 64, 80, 64, 80};
			return true;
		case LOC_SERIAL_PORT_1:
			__device_fsp.deviceTexture = (textureImage){TEX_BBA, 140, 64, 140, 64};
			return true;
		case LOC_SERIAL_PORT_2:
			__device_fsp.deviceTexture = (textureImage){TEX_ETH2GC, 64, 80, 64, 80};
			return true;
	}
	return net_initialized;
}

u32 deviceHandler_FSP_emulated() {
	if (devices[DEVICE_PATCHES] && devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
		if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
			swissSettings.emulateAudioStream > 1)
			return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER | EMU_NO_PAUSING;
		else
			return EMU_READ | EMU_BUS_ARBITER | EMU_NO_PAUSING;
	} else {
		if ((swissSettings.emulateAudioStream == 1 && swissSettings.audioStreaming) ||
			swissSettings.emulateAudioStream > 1)
			return EMU_READ | EMU_AUDIO_STREAMING | EMU_BUS_ARBITER | EMU_NO_PAUSING;
		else if (swissSettings.emulateEthernet && (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
			return EMU_READ | EMU_ETHERNET | EMU_BUS_ARBITER | EMU_NO_PAUSING;
		else
			return EMU_READ | EMU_BUS_ARBITER | EMU_NO_PAUSING;
	}
}

char* deviceHandler_FSP_status(file_handle* file) {
	switch(file->status) {
		case E_NONET:
			return "Network has not been initialised";
		case E_CHECKCONFIG:
			return "Check FSP Configuration";
	}
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_fsp = {
	.deviceUniqueId = DEVICE_ID_E,
	.hwName = "Broadband Adapter",
	.deviceName = "File Service Protocol",
	.deviceDescription = "Configurable via the settings screen",
	.deviceTexture = {TEX_BBA, 140, 64, 140, 64},
	.features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_THREAD_SAFE|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	.emulable = EMU_READ|EMU_AUDIO_STREAMING|EMU_ETHERNET,
	.initial = &initial_FSP,
	.test = deviceHandler_FSP_test,
	.info = deviceHandler_FSP_info,
	.init = deviceHandler_FSP_init,
	.makeDir = deviceHandler_FSP_makeDir,
	.readDir = deviceHandler_FSP_readDir,
	.statFile = deviceHandler_FSP_statFile,
	.seekFile = deviceHandler_FSP_seekFile,
	.readFile = deviceHandler_FSP_readFile,
	.writeFile = deviceHandler_FSP_writeFile,
	.closeFile = deviceHandler_FSP_closeFile,
	.deleteFile = deviceHandler_FSP_deleteFile,
	.renameFile = deviceHandler_FSP_renameFile,
	.setupFile = deviceHandler_FSP_setupFile,
	.deinit = deviceHandler_FSP_deinit,
	.emulated = deviceHandler_FSP_emulated,
	.status = deviceHandler_FSP_status,
};
