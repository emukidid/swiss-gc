/* deviceHandler.h
	- devices interface (based on fileBrowser from wii64/wiiSX)
	by emu_kidid
 */

#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include <stdint.h>

typedef struct {
	char name[1024]; // File or Folder, absolute path goes here
	uint64_t fileBase;   // Only used on DVD/Wode/DVD-emulator
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size of the file
	int fileAttrib;        // IS_FILE or IS_DIR
	int status;            // is the device ok
} file_handle;

#define DEVICE_HANDLER_SEEK_SET 0
#define DEVICE_HANDLER_SEEK_CUR 1

extern file_handle* deviceHandler_initial;

/* Initialize the device */
extern int (*deviceHandler_init)(file_handle*);

/* read directory into array, return number of files found or error */
extern int (*deviceHandler_readDir)(file_handle*, file_handle**);

/* readFile returns the status of the read and reads if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_readFile)(file_handle*, void*, unsigned int);

/* writeFile returns the status of the write and writes if it can
     arguments: handle*, buffer, length */
extern int (*deviceHandler_writeFile)(file_handle*, void*, unsigned int);

/* seekFile returns the status of the seek and seeks if it can
     arguments: file*, offset, seek type */
extern int (*deviceHandler_seekFile)(file_handle*, unsigned int, unsigned int);

/* sets the offset and other device specific stuff */
extern void (*deviceHandler_setupFile)(file_handle*, file_handle*);

/* Shutdown the device */
extern int (*deviceHandler_deinit)();

#endif

