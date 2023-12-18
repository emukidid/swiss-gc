/* deviceHandler-ARAM.c
	- device implementation for Memory Expansion Pak
	by Extrems
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "aram.h"

static FATFS *aramfs = NULL;

file_handle initial_ARAM =
	{ "ram:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

s32 deviceHandler_ARAM_init(file_handle* file) {
	if(aramfs != NULL) {
		f_unmount("ram:/");
		free(aramfs);
		aramfs = NULL;
		disk_shutdown(DEV_ARAM);
	}
	aramfs = (FATFS*)malloc(sizeof(FATFS));
	file->status = f_mount(aramfs, "ram:/", 1);
	if(file->status == FR_NO_FILESYSTEM) {
		file->status = f_mkfs("ram:/", &(MKFS_PARM){FM_EXFAT | FM_SFD}, aramfs->win, sizeof(aramfs->win));
		if(file->status == FR_OK) {
			file->status = f_mount(aramfs, "ram:/", 1);
		}
	}
	return file->status == FR_OK ? 0 : EIO;
}

s32 deviceHandler_ARAM_deinit(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	if(file) {
		f_unmount(file->name);
		free(aramfs);
		aramfs = NULL;
		disk_shutdown(DEV_ARAM);
	}
	return 0;
}

bool deviceHandler_ARAM_test() {
	return __io_aram.startup() && __io_aram.isInserted();
}

DEVICEHANDLER_INTERFACE __device_aram = {
	.deviceUniqueId = DEVICE_ID_I,
	.hwName = "Memory Expansion Pak",
	.deviceName = "RAM Disk",
	.deviceDescription = "Supported File System(s): FAT16, FAT32, exFAT",
	.deviceTexture = {TEX_SYSTEM, 75, 48, 76, 48},
	.features = FEAT_READ|FEAT_WRITE|FEAT_FAT_FUNCS,
	.location = LOC_HSP,
	.initial = &initial_ARAM,
	.test = deviceHandler_ARAM_test,
	.info = deviceHandler_FAT_info,
	.init = deviceHandler_ARAM_init,
	.makeDir = deviceHandler_FAT_makeDir,
	.readDir = deviceHandler_FAT_readDir,
	.seekFile = deviceHandler_FAT_seekFile,
	.readFile = deviceHandler_FAT_readFile,
	.writeFile = deviceHandler_FAT_writeFile,
	.closeFile = deviceHandler_FAT_closeFile,
	.deleteFile = deviceHandler_FAT_deleteFile,
	.renameFile = deviceHandler_FAT_renameFile,
	.deinit = deviceHandler_ARAM_deinit,
	.status = deviceHandler_FAT_status,
};
