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
#include "deviceHandler.h"

#ifndef NULL
#define NULL 0
#endif

DEVICEHANDLER_INTERFACE* allDevices[MAX_DEVICES];	// All devices registered in Swiss
DEVICEHANDLER_INTERFACE* devices[MAX_DEVICES];		// Currently used devices


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

int deviceHandler_test(DEVICEHANDLER_INTERFACE *device) {
	deviceHandler_setStatEnabled(0);
	int ret = device->init(device->initial);
	deviceHandler_setStatEnabled(1);
	return ret;
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
	char *devpos = strstr(path,"/");
	if(!path) {
		return NULL;	// garbage
	}
	if(devpos == path) {
		return &__device_fsp;	// naughty device, doesn't have a path name identifier.
	}
	
	for(int i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && !strncmp(&allDevices[i]->initial->name[0], path, devpos-path) && deviceHandler_getDeviceAvailable(allDevices[i])) {
			return allDevices[i];
		}
	}	
	return NULL;
}
