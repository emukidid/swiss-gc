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
	{ "aram:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

s32 deviceHandler_ARAM_init(file_handle* file) {
	if(aramfs != NULL) {
		f_unmount("aram:/");
		free(aramfs);
		aramfs = NULL;
		disk_shutdown(DEV_ARAM);
	}
	aramfs = (FATFS*)malloc(sizeof(FATFS));
	file->status = f_mount(aramfs, "aram:/", 1);
	if(file->status == FR_NO_FILESYSTEM) {
		file->status = f_mkfs("aram:/", &(MKFS_PARM){FM_EXFAT | FM_SFD}, aramfs->win, sizeof(aramfs->win));
		if(file->status == FR_OK) {
			file->status = f_mount(aramfs, "aram:/", 1);
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
	DEVICE_ID_I,
	"Memory Expansion Pak",
	"RAM Disk",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SYSTEM, 75, 48, 76, 48},
	FEAT_READ|FEAT_WRITE|FEAT_FAT_FUNCS,
	EMU_NONE,
	LOC_HSP,
	&initial_ARAM,
	(_fn_test)&deviceHandler_ARAM_test,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_ARAM_init,
	(_fn_makeDir)&deviceHandler_FAT_makeDir,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)deviceHandler_FAT_writeFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deleteFile)deviceHandler_FAT_deleteFile,
	(_fn_renameFile)&deviceHandler_FAT_renameFile,
	(_fn_setupFile)NULL,
	(_fn_deinit)&deviceHandler_ARAM_deinit,
	(_fn_emulated)NULL,
	(_fn_status)&deviceHandler_FAT_status,
};
