/* deviceHandler.c
	- devices interface (based on fileBrowser from wii64/wiiSX)
	by emu_kidid
 */

#include <stdio.h>
#include <ogcsys.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include "swiss.h"
#include "patcher.h"
#include "deviceHandler.h"

DEVICEHANDLER_INTERFACE* allDevices[MAX_DEVICES];	// All devices registered in Swiss
DEVICEHANDLER_INTERFACE* devices[MAX_DEVICE_SLOTS];	// Currently used devices


// Device stat global disable status
static int statEnabled = 1;
void deviceHandler_setStatEnabled(int enable) {statEnabled = enable;}
int deviceHandler_getStatEnabled() {return statEnabled;}


// Device availability
typedef struct {
	DEVICEHANDLER_INTERFACE* device;
	bool available;
} _device_availability;
static _device_availability device_availability[MAX_DEVICES];

bool deviceHandler_getDeviceAvailable(DEVICEHANDLER_INTERFACE *dev) {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(device_availability[i].device == dev) {
			return device_availability[i].available;
		}
	}
	return false; 
}

void deviceHandler_setDeviceAvailable(DEVICEHANDLER_INTERFACE *dev, bool availability) { 
	int i, empty_slot = -1;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(device_availability[i].device == dev) {
			device_availability[i].available = availability;
			return;
		}
		if(device_availability[i].device == NULL && empty_slot < 0) {
			empty_slot = i;
		}
	}
	device_availability[empty_slot].device = dev;
	device_availability[empty_slot].available = availability;
}

void deviceHandler_setAllDevicesAvailable() {
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL) {
			device_availability[i].device = allDevices[i];
			device_availability[i].available = true;
		}
	}
}

DEVICEHANDLER_INTERFACE* getDeviceByUniqueId(u8 id) {
	for(int i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && allDevices[i]->deviceUniqueId == id) {
			return allDevices[i];
		}
	}
	return NULL;
}

DEVICEHANDLER_INTERFACE* getDeviceByLocation(u32 location) {
	for(int i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && allDevices[i]->location == location && deviceHandler_getDeviceAvailable(allDevices[i])) {
			return allDevices[i];
		}
	}
	return NULL;
}

DEVICEHANDLER_INTERFACE* getDeviceFromPath(char *path) {
	char *devpos = getDevicePath(path);
	if(devpos == path) {
		return NULL;	// garbage
	}
	for(int i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && !strncmp(&allDevices[i]->initial->name[0], path, devpos-path) && deviceHandler_getDeviceAvailable(allDevices[i])) {
			return allDevices[i];
		}
	}	
	return NULL;
}

const char* getHwNameByLocation(u32 location) {
	DEVICEHANDLER_INTERFACE *device = getDeviceByLocation(location);
	if(device != NULL) {
		return device->hwName;
	}
	u32 type;
	switch(location) {
		case LOC_MEMCARD_SLOT_A:
			if(EXI_GetType(EXI_CHANNEL_0, EXI_DEVICE_0, &type) && ~type) return EXI_GetTypeString(type);
			break;
		case LOC_MEMCARD_SLOT_B:
			if(EXI_GetType(EXI_CHANNEL_1, EXI_DEVICE_0, &type) && ~type) return EXI_GetTypeString(type);
			break;
		case LOC_SERIAL_PORT_1:
			if(EXI_GetType(EXI_CHANNEL_0, EXI_DEVICE_2, &type) && ~type) return EXI_GetTypeString(type);
			break;
		case LOC_SERIAL_PORT_2:
			if(EXI_GetType(EXI_CHANNEL_2, EXI_DEVICE_0, &type) && ~type) return EXI_GetTypeString(type);
			break;
		case LOC_SYSTEM:
			if(EXI_GetType(EXI_CHANNEL_0, EXI_DEVICE_1, &type) && ~type) return EXI_GetTypeString(type);
			break;
	}
	return "Empty";
}

bool getFragments(int deviceSlot, file_handle *file, file_frag **fragList, u32 *totFrags, u8 fileNum, u32 forceBaseOffset, u32 forceSize) {
	file_frag *frags = *fragList;
	u32 numFrags = *totFrags;
	
	// open the file (should be open already)
	if(devices[deviceSlot]->readFile(file, NULL, 0) != 0) {
		return false;
	}
	if(forceSize == 0) {
		forceSize = file->size;
	}
	if(!file->status && file->ffsFp) {
		FATFS* fatfs = file->ffsFp->obj.fs;
		// fatfs - Cluster link table map buffer
		DWORD clmt[(MAX_FRAGS+1)*2];
		file->ffsFp->cltbl = clmt;
		*file->ffsFp->cltbl = sizeof(clmt)/sizeof(DWORD);
		if(f_lseek(file->ffsFp, CREATE_LINKMAP) != FR_OK) {
			file->ffsFp->cltbl = NULL;
			return false;	// Too many fragments for our buffer
		}
		file->ffsFp->cltbl = NULL;
		
		print_gecko("getFragments [%s] - found %i fragments [%i arr]\r\n", file->name, (clmt[0]/2)-1, clmt[0]);
		
		frags = realloc(frags, sizeof(file_frag) * (numFrags + (clmt[0]/2)));
		if(frags == NULL) {
			return false;
		}
		for(int i = 1; forceSize && clmt[i]; i+=2) {
			FSIZE_t size = (FSIZE_t)clmt[i] * fatfs->csize * fatfs->ssize;
			LBA_t sector = clst2sect(fatfs, clmt[i+1]);
			// this frag offset in the file is the last frag offset+size
			size = forceSize < size ? forceSize : size;
			frags[numFrags].offset = forceBaseOffset;
			frags[numFrags].size = size;
			frags[numFrags].fileNum = fileNum;
			frags[numFrags].devNum = deviceSlot == DEVICE_PATCHES;
			frags[numFrags].fileBase = sector;
			forceBaseOffset += size;
			forceSize -= size;
			numFrags++;
		}
		file->status = 1;
	}
	else if(devices[deviceSlot] == &__device_fsp) {
		frags = realloc(frags, sizeof(file_frag) * (numFrags + 2));
		if(frags == NULL) {
			return false;
		}
		char *path = NULL;
		int pathlen = asprintf(&path, "%s\n%s", getDevicePath(file->name), swissSettings.fspPassword) + 1;
		frags[numFrags].offset = forceBaseOffset;
		frags[numFrags].size = forceSize;
		frags[numFrags].fileNum = fileNum;
		frags[numFrags].devNum = 0;
		frags[numFrags].fileBase = (u32)installPatch2(path, pathlen) | ((u64)pathlen << 32);
		free(path);
		numFrags++;
	}
	else if(devices[deviceSlot] == &__device_usbgecko) {
		frags = realloc(frags, sizeof(file_frag) * (numFrags + 2));
		if(frags == NULL) {
			return false;
		}
		frags[numFrags].offset = forceBaseOffset;
		frags[numFrags].size = forceSize;
		frags[numFrags].fileNum = fileNum;
		frags[numFrags].devNum = 0;
		frags[numFrags].fileBase = (u32)installPatch2(file->name, PATHNAME_MAX) | ((u64)PATHNAME_MAX << 32);
		numFrags++;
	}
	else {
		frags = realloc(frags, sizeof(file_frag) * (numFrags + 2));
		if(frags == NULL) {
			return false;
		}
		frags[numFrags].offset = forceBaseOffset;
		frags[numFrags].size = forceSize;
		frags[numFrags].fileNum = fileNum;
		frags[numFrags].devNum = 0;
		frags[numFrags].fileBase = file->fileBase;
		numFrags++;
	}
	*fragList = frags;
	*totFrags = numFrags;
	memset(&frags[numFrags], 0, sizeof(file_frag));
	return true;
}

void print_frag_list(file_frag *fragList, u32 totFrags) {
	print_gecko("== Fragments List ==\r\n");
	for(int i = 0; i < totFrags; i++) {
		print_gecko("Frag %i: ofs in file: [0x%08X] len [0x%08X] LBA on disk [0x%012llX]\r\n",
					i, fragList[i].offset, fragList[i].size, fragList[i].fileBase);
	}
	print_gecko("== Fragments End ==\r\n");
}

FILE* openFileStream(int deviceSlot, file_handle *file)
{
	if(devices[deviceSlot]->readFile(file, NULL, 0) != 0 || !file->size) {
		return NULL;
	}
	return funopen((const void *)file,
		(int (*)(void *, char *, int))devices[deviceSlot]->readFile,
		(int (*)(void *, const char *, int))NULL,
		(fpos_t (*)(void *, fpos_t, int))devices[deviceSlot]->seekFile,
		(int (*)(void *))devices[deviceSlot]->closeFile);
}
