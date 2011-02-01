/* deviceHandler-DVD-Emu.h
	- device interface for DVD-Emulator
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_IDEEXI_H
#define DEVICE_HANDLER_IDEEXI_H

#include "../deviceHandler.h"

extern file_handle initial_IDEEXI0;
extern file_handle initial_IDEEXI1;

int  deviceHandler_IDEEXI_readDir(file_handle*, file_handle**);
int  deviceHandler_IDEEXI_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_IDEEXI_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_IDEEXI_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_IDEEXI_init(file_handle* file);
int  deviceHandler_IDEEXI_deinit(file_handle* file);

#endif

