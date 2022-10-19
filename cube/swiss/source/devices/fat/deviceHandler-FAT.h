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

extern device_info* deviceHandler_FAT_info(file_handle* file);
extern s32 deviceHandler_FAT_makeDir(file_handle* dir);
extern s32 deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir, u32 type);
extern s64 deviceHandler_FAT_seekFile(file_handle* file, s64 where, u32 type);
extern s32 deviceHandler_FAT_readFile(file_handle* file, void* buffer, u32 length);
extern s32 deviceHandler_FAT_writeFile(file_handle* file, void* buffer, u32 length);
extern s32 deviceHandler_FAT_closeFile(file_handle* file);
extern s32 deviceHandler_FAT_deleteFile(file_handle* file);
extern s32 deviceHandler_FAT_renameFile(file_handle* file, char* name);
extern char* deviceHandler_FAT_status(file_handle* file);

#endif

