/* deviceHandler-FAT.h
	- device interface for FAT (via SDGecko/IDE-EXI)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_SD_H
#define DEVICE_HANDLER_SD_H

#include "../deviceHandler.h"

extern DEVICEHANDLER_INTERFACE __device_sd_a;
extern DEVICEHANDLER_INTERFACE __device_sd_b;
extern DEVICEHANDLER_INTERFACE __device_sd_c;
extern DEVICEHANDLER_INTERFACE __device_ata_a;
extern DEVICEHANDLER_INTERFACE __device_ata_b;
extern DEVICEHANDLER_INTERFACE __device_ata_c;

extern s32 deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir, u32 type);
extern s32 deviceHandler_FAT_readFile(file_handle* file, void* buffer, u32 length);
extern s32 deviceHandler_FAT_writeFile(file_handle* file, void* buffer, u32 length);
extern s32 getFragments(file_handle* file, u32 (*fragList)[4], s32 maxFrags, u8 fileNum, u32 forceBaseOffset, u32 forceSize, u32 dev);

#endif

