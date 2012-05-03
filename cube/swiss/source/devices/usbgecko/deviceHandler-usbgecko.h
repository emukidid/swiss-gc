/* deviceHandler-USBGecko.h
	- device interface for USBGecko
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_USBGECKO_H
#define DEVICE_HANDLER_USBGECKO_H

#include "../deviceHandler.h"

extern file_handle initial_USBGecko;

int		deviceHandler_USBGecko_readDir(file_handle*, file_handle**, unsigned int);
int		deviceHandler_USBGecko_readFile(file_handle*, void*, unsigned int);
int		deviceHandler_USBGecko_writeFile(file_handle*, void*, unsigned int);
int		deviceHandler_USBGecko_seekFile(file_handle*, unsigned int, unsigned int);
void	deviceHandler_USBGecko_setupFile(file_handle* file, file_handle* file2);
int		deviceHandler_USBGecko_init(file_handle* file);
int		deviceHandler_USBGecko_deinit(file_handle* file);
int		deviceHandler_USBGecko_deleteFile(file_handle* file);

#endif

