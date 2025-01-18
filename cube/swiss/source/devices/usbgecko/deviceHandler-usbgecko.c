/* deviceHandler-USBGecko.c
	- device implementation for USBGecko
	by emu_kidid
 */

#include <stdlib.h>
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

#define NO_PC (-1)

file_handle initial_USBGecko =
	{ "./",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

device_info* deviceHandler_USBGecko_info(file_handle* file) {
	return NULL;
}
	
s32 deviceHandler_USBGecko_readDir(file_handle* ffile, file_handle** dir, u32 type){	
  
	// Set everything up to read
	s32 num_entries = 1, i = 1;
	file_handle *entry = NULL;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileType = IS_SPECIAL;
	
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading directory"));
	// Read each entry of the directory
	s32 res = usbgecko_open_dir(ffile->name);
	if(!res) return -1;
	while( (entry = usbgecko_get_entry()) != NULL ){
		// Make sure we have room for this one
		if(i == num_entries) {
			++num_entries;
			*dir = reallocarray(*dir, num_entries, sizeof(file_handle));
		}
		memset(&(*dir)[i], 0, sizeof(file_handle));
		if(strlcpy((*dir)[i].name, entry->name, PATHNAME_MAX) < PATHNAME_MAX) {
			(*dir)[i].size     = entry->size;
			(*dir)[i].fileType = entry->fileType;
			++i;
		}
	}
	DrawDispose(msgBox);
	return i;
}

s64 deviceHandler_USBGecko_seekFile(file_handle* file, s64 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
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

s32 deviceHandler_USBGecko_writeFile(file_handle* file, const void* buffer, u32 length) {
	s32 bytes_written = usbgecko_write_file(buffer, length, file->offset, file->name);
	if(bytes_written > 0) file->offset += bytes_written;
	
	return bytes_written;
}

s32 deviceHandler_USBGecko_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0) {
		return 0;
	}
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_gecko("Save Patch device found\r\n");
		
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
	return 1;
}

s32 deviceHandler_USBGecko_init(file_handle* file) {
	s32 res = 0;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Waiting for PC\205"));
	if(usb_isgeckoalive(1)) {
		s32 retries = 1000;
		
		usb_flush(1);
		usbgecko_lock_file(0);
		// Wait for the PC and retry 1000 times
		while(!usbgecko_pc_ready() && retries) {
			VIDEO_WaitVSync();
			retries--;
		}
		if(!retries) {
			file->status = NO_PC;
		}
	}
	else {
		res = ENODEV;
	}
	DrawDispose(msgBox);
	return res;
}

s32 deviceHandler_USBGecko_deinit(file_handle* file) {
	return 0;
}

s32 deviceHandler_USBGecko_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_USBGecko_test() {
	return usb_isgeckoalive(1);
}

u32 deviceHandler_USBGecko_emulated() {
	return EMU_READ | EMU_BUS_ARBITER;
}

char* deviceHandler_USBGecko_status(file_handle* file) {
	if(file->status == NO_PC)
		return "Couldn't connect to PC";
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_usbgecko = {
	.deviceUniqueId = DEVICE_ID_A,
	.hwName = "USB Gecko",
	.deviceName = "USB Gecko - Slot B only",
	.deviceDescription = "Requires PC application to be up",
	.deviceTexture = {TEX_USBGECKO, 60, 84, 64, 84},
	.features = FEAT_READ|FEAT_BOOT_GCM|FEAT_HYPERVISOR,
	.emulable = EMU_READ,
	.location = LOC_MEMCARD_SLOT_B,
	.initial = &initial_USBGecko,
	.test = deviceHandler_USBGecko_test,
	.info = deviceHandler_USBGecko_info,
	.init = deviceHandler_USBGecko_init,
	.readDir = deviceHandler_USBGecko_readDir,
	.seekFile = deviceHandler_USBGecko_seekFile,
	.readFile = deviceHandler_USBGecko_readFile,
	.closeFile = deviceHandler_USBGecko_closeFile,
	.setupFile = deviceHandler_USBGecko_setupFile,
	.deinit = deviceHandler_USBGecko_deinit,
	.emulated = deviceHandler_USBGecko_emulated,
	.status = deviceHandler_USBGecko_status,
};
