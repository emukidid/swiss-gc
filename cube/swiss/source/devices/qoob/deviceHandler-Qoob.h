/* deviceHandler-Qoob.h
	- device interface for Qoob Flash
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_QOOB_H
#define DEVICE_HANDLER_QOOB_H

#include "../deviceHandler.h"

extern file_handle initial_Qoob;

int  deviceHandler_Qoob_readDir(file_handle*, file_handle**, unsigned int);
int  deviceHandler_Qoob_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_Qoob_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_Qoob_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_Qoob_init(file_handle* file);
int  deviceHandler_Qoob_deinit(file_handle* file);

#endif

