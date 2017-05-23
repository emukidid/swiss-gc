/* deviceHandler-DVD.h
	- device interface for DVD (Discs)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_DVD_H
#define DEVICE_HANDLER_DVD_H

#include "../deviceHandler.h"

extern DEVICEHANDLER_INTERFACE __device_dvd;

int gettype_disc();
int initialize_disc(u32 streaming);
char *dvd_error_str();

#endif

