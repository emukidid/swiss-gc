/* deviceHandler-DVD.h
	- device interface for DVD (Discs)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_DVD_H
#define DEVICE_HANDLER_DVD_H

#include "../deviceHandler.h"

extern file_handle initial_DVD;

int  deviceHandler_DVD_readDir(file_handle*, file_handle**, unsigned int);
int  deviceHandler_DVD_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_DVD_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_DVD_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_DVD_init(file_handle* file);
int  deviceHandler_DVD_deinit(file_handle* file);

int gettype_disc();
int initialize_disc(u32 streaming);
char *dvd_error_str();

#endif

