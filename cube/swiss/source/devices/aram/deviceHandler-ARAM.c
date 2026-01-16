/* deviceHandler-ARAM.c
	- device implementation for Memory Expansion Pak
	by Extrems
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/aram.h>
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"

file_handle initial_ARAM = {
	.name     = "ram:/",
	.fileType = IS_DIR,
	.device   = &__device_aram,
};

s32 deviceHandler_ARAM_init(file_handle* file) {
	file->status = fatFs_Mount(file);
	if(file->status == FR_NO_FILESYSTEM) {
		FATFS* fatfs = file->device->context;
		file->status = f_mkfs(file->name, &(MKFS_PARM){FM_EXFAT | FM_SFD}, fatfs->win, sizeof(fatfs->win));
		if(file->status == FR_OK) {
			file->status = f_mount(fatfs, file->name, 1);
		}
	}
	return file->status == FR_OK ? 0 : EIO;
}

bool deviceHandler_ARAM_test() {
	return __io_aram.startup(&__io_aram)
		&& __io_aram.isInserted(&__io_aram)
		&& __io_aram.shutdown(&__io_aram);
}

DEVICEHANDLER_INTERFACE __device_aram = {
	.deviceUniqueId = DEVICE_ID_I,
	.hwName = "Memory Expansion Pak",
	.deviceName = "RAM Disk",
	.deviceDescription = "Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_SYSTEM, 75, 48, 76, 48},
	.features = FEAT_READ|FEAT_WRITE|FEAT_THREAD_SAFE,
	.quirks = QUIRK_NO_DEINIT,
	.location = LOC_HSP,
	.initial = &initial_ARAM,
	.test = deviceHandler_ARAM_test,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_ARAM_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.statFile = deviceHandler_FAT_statFile,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.hideFile = deviceHandler_FAT_hideFile,
	.deinit = deviceHandler_FAT_deinit,
	.status = deviceHandler_FAT_status,
};
