/* deviceHandler-SYS.h
        - virtual device interface for dumping ROMs
        by novenary
 */

#ifndef DEVICE_HANDLER_SYS_H
#define DEVICE_HANDLER_SYS_H

#include "../deviceHandler.h"

extern DEVICEHANDLER_INTERFACE __device_sys;

extern int read_rom_ipl_clear(unsigned int offset, void* buffer, unsigned int length);

#endif
