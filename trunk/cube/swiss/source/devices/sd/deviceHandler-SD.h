/* deviceHandler-SD.h
	- device interface for SD (via SDGecko)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_SD_H
#define DEVICE_HANDLER_SD_H

#include "../deviceHandler.h"

extern file_handle initial_SD0;
extern file_handle initial_SD1;

extern DWORD mclust2sect (DWORD clst);

int  deviceHandler_SD_readDir(file_handle*, file_handle**);
int  deviceHandler_SD_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_SD_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_SD_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_SD_init(file_handle* file);
int  deviceHandler_SD_deinit(file_handle* file);

#endif

