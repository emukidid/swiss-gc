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

file_handle* deviceHandler_initial = NULL;
device_info* (*deviceHandler_info)(void) = NULL;

static int device_availability[MAX_DEVICES];
static int statEnabled = 1;
void deviceHandler_setStatEnabled(int enable) {statEnabled = enable;}
int deviceHandler_getStatEnabled() {return statEnabled;}
int deviceHandler_getDeviceAvailable(int dev) { return device_availability[dev]; }
void deviceHandler_setDeviceAvailable(int dev, int a) { device_availability[dev] = a; }
void deviceHandler_setAllDevicesAvailable() {memset(&device_availability[0], 1, MAX_DEVICES);}

int  (*deviceHandler_init)(file_handle*) = NULL;
int  (*deviceHandler_readDir)(file_handle*, file_handle**, unsigned int) = NULL;
int  (*deviceHandler_readFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_writeFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_deleteFile)(file_handle*) = NULL;
int  (*deviceHandler_seekFile)(file_handle*, unsigned int, unsigned int) = NULL;
int  (*deviceHandler_setupFile)(file_handle*, file_handle*) = NULL;
int  (*deviceHandler_closeFile)(file_handle*) = NULL;
int  (*deviceHandler_deinit)() = NULL;

// Destination Device
file_handle* deviceHandler_dest_initial = NULL;

int  (*deviceHandler_dest_init)(file_handle*) = NULL;
int  (*deviceHandler_dest_readDir)(file_handle*, file_handle**, unsigned int) = NULL;
int  (*deviceHandler_dest_readFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_dest_writeFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_dest_deleteFile)(file_handle*) = NULL;
int  (*deviceHandler_dest_seekFile)(file_handle*, unsigned int, unsigned int) = NULL;
int  (*deviceHandler_dest_setupFile)(file_handle*, file_handle*) = NULL;
int  (*deviceHandler_dest_closeFile)(file_handle*) = NULL;
int  (*deviceHandler_dest_deinit)() = NULL;

// Temporary Device
file_handle* deviceHandler_temp_initial = NULL;

int  (*deviceHandler_temp_init)(file_handle*) = NULL;
int  (*deviceHandler_temp_readDir)(file_handle*, file_handle**, unsigned int) = NULL;
int  (*deviceHandler_temp_readFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_temp_writeFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_temp_deleteFile)(file_handle*) = NULL;
int  (*deviceHandler_temp_seekFile)(file_handle*, unsigned int, unsigned int) = NULL;
int  (*deviceHandler_temp_setupFile)(file_handle*, file_handle*) = NULL;
int  (*deviceHandler_temp_closeFile)(file_handle*) = NULL;
int  (*deviceHandler_temp_deinit)() = NULL;
