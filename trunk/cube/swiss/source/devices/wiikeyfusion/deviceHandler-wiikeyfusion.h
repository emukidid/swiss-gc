/* deviceHandler-wiikeyfusion.h
	- device implementation for Wiikey Fusion
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_WKF_H
#define DEVICE_HANDLER_WKF_H

#include "../deviceHandler.h"

extern file_handle initial_WKF;
extern int wkfFragSetupReq;
int deviceHandler_WKF_readDir(file_handle*, file_handle**, unsigned int);
int deviceHandler_WKF_readFile(file_handle*, void*, unsigned int);
int deviceHandler_WKF_seekFile(file_handle*, unsigned int, unsigned int);
int deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2);
int deviceHandler_WKF_init(file_handle* file);
int deviceHandler_WKF_deinit(file_handle* file);

#endif

