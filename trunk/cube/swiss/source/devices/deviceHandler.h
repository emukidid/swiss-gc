/* deviceHandler.h
	- devices interface (based on fileBrowser from wii64/wiiSX)
	by emu_kidid
 */

#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include <gccore.h>
#include <stdio.h>

typedef struct {
	int fileTypeTexId;
	int regionTexId;
	u8 *banner;
	GXTexObj bannerTexObj;
	char description[128];
} file_meta;

typedef struct {
	char name[1024]; 		// File or Folder, absolute path goes here
	uint64_t fileBase;   	// Raw sector on device
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size of the file
	int fileAttrib;        	// IS_FILE or IS_DIR
	int status;            	// is the device ok
	FILE *fp;				// file pointer
	file_meta *meta;
} file_handle;

typedef struct {
	int textureId;
	u32 freeSpaceInKB;		// 4TB Max
	u32 totalSpaceInKB;		// 4TB Max
} device_info;

#define DEVICE_HANDLER_SEEK_SET 0
#define DEVICE_HANDLER_SEEK_CUR 1

#include "devices/dvd/deviceHandler-DVD.h"
#include "devices/fat/deviceHandler-FAT.h"
#include "devices/qoob/deviceHandler-Qoob.h"
#include "devices/wode/deviceHandler-WODE.h"
#include "devices/memcard/deviceHandler-CARD.h"
#include "devices/smb/deviceHandler-SMB.h"
#include "devices/wiikeyfusion/deviceHandler-wiikeyfusion.h"
#include "devices/usbgecko/deviceHandler-usbgecko.h"

extern file_handle* deviceHandler_initial;
extern device_info*	(*deviceHandler_info)(void);
extern void deviceHandler_setStatEnabled(int enable);
extern int deviceHandler_getStatEnabled();

/* Initialize the device */
extern int (*deviceHandler_init)(file_handle*);

/* read directory into array, return number of files found or error */
extern int (*deviceHandler_readDir)(file_handle*, file_handle**, unsigned int);

/* readFile returns the status of the read and reads if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_readFile)(file_handle*, void*, unsigned int);

/* writeFile returns the status of the write and writes if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_writeFile)(file_handle*, void*, unsigned int);

/* deleteFile returns the status of the delete - 0 for success, else error code */
extern int (*deviceHandler_deleteFile)(file_handle*);

/* seekFile returns the status of the seek and seeks if it can
     arguments: file*, offset, seek type */
extern int (*deviceHandler_seekFile)(file_handle*, unsigned int, unsigned int);

/* sets the offset and other device specific stuff */
extern int (*deviceHandler_setupFile)(file_handle*, file_handle*);

/* Shutdown the device */
extern int (*deviceHandler_deinit)();

// Destination Device
extern file_handle* deviceHandler_dest_initial;

/* Initialize the device */
extern int (*deviceHandler_dest_init)(file_handle*);

/* read directory into array, return number of files found or error */
extern int (*deviceHandler_dest_readDir)(file_handle*, file_handle**, unsigned int);

/* readFile returns the status of the read and reads if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_dest_readFile)(file_handle*, void*, unsigned int);

/* writeFile returns the status of the write and writes if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_dest_writeFile)(file_handle*, void*, unsigned int);

/* deleteFile returns the status of the delete - 0 for success, else error code */
extern int (*deviceHandler_dest_deleteFile)(file_handle*);

/* seekFile returns the status of the seek and seeks if it can
     arguments: file*, offset, seek type */
extern int (*deviceHandler_dest_seekFile)(file_handle*, unsigned int, unsigned int);

/* sets the offset and other device specific stuff */
extern int (*deviceHandler_dest_setupFile)(file_handle*, file_handle*);

/* Shutdown the device */
extern int (*deviceHandler_dest_deinit)();

#endif

