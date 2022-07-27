/* deviceHandler-SYS.h
        - virtual device interface for dumping ROMs
        by novenary
 */

#ifndef DEVICE_HANDLER_SYS_H
#define DEVICE_HANDLER_SYS_H

#include "../deviceHandler.h"

extern DEVICEHANDLER_INTERFACE __device_sys;

extern bool load_rom_ipl(DEVICEHANDLER_INTERFACE* device, void** buffer, unsigned int* length);

#endif
