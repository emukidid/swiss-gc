/* deviceHandler-WODE.h
	- device interface for WODE (GC Only)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_WODE_H
#define DEVICE_HANDLER_WODE_H

#include "../deviceHandler.h"

extern file_handle initial_WODE;

int deviceHandler_WODE_readDir(file_handle*, file_handle**, unsigned int);
int deviceHandler_WODE_readFile(file_handle*, void*, unsigned int);
int deviceHandler_WODE_seekFile(file_handle*, unsigned int, unsigned int);
int deviceHandler_WODE_setupFile(file_handle* file, file_handle* file2);
int deviceHandler_WODE_init(file_handle* file);
int deviceHandler_WODE_deinit(file_handle* file);

#endif

