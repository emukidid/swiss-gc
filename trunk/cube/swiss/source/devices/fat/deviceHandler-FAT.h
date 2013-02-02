/* deviceHandler-FAT.h
	- device interface for FAT (via SDGecko/IDE-EXI)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_SD_H
#define DEVICE_HANDLER_SD_H

#include "../deviceHandler.h"

extern file_handle initial_SD0;
extern file_handle initial_SD1;
extern file_handle initial_IDE0;
extern file_handle initial_IDE1;

int  deviceHandler_FAT_readDir(file_handle*, file_handle**, unsigned int);
int  deviceHandler_FAT_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_FAT_writeFile(file_handle*, void*, unsigned int);
int  deviceHandler_FAT_seekFile(file_handle*, unsigned int, unsigned int);
int  deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_FAT_init(file_handle* file);
int  deviceHandler_FAT_deinit(file_handle* file);
int  deviceHandler_FAT_deleteFile(file_handle* file);

#endif

