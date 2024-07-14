/* 
 * Copyright (c) 2024, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __FLIPPY_H__
#define __FLIPPY_H__

#include <gctypes.h>
#include <gcutil.h>

#define FLIPPY_FLAG_DISABLE_CACHE    0x01
#define FLIPPY_FLAG_DISABLE_FASTSEEK 0x02
#define FLIPPY_FLAG_DISABLE_DVDSPEED 0x04
#define FLIPPY_FLAG_WRITE            0x08
#define FLIPPY_FLAG_DEFAULT          (FLIPPY_FLAG_DISABLE_CACHE | FLIPPY_FLAG_DISABLE_FASTSEEK | FLIPPY_FLAG_DISABLE_DVDSPEED)

#define FLIPPY_FLASH_HANDLE 64

#define FLIPPY_MAX_HANDLES  32
#define FLIPPY_MAX_PATH     256

#define FLIPPY_MINVER_MAJOR 1
#define FLIPPY_MINVER_MINOR 0
#define FLIPPY_MINVER_BUILD 0

#define FLIPPY_VERSION(major, minor, build) (((major) << 24) | ((minor) << 16) | ((build) << 1))

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FLIPPY_RESULT_OK = 0,
	FLIPPY_RESULT_DISK_ERR,
	FLIPPY_RESULT_INT_ERR,
	FLIPPY_RESULT_NOT_READY,
	FLIPPY_RESULT_NO_FILE,
	FLIPPY_RESULT_NO_PATH,
	FLIPPY_RESULT_INVALID_NAME,
	FLIPPY_RESULT_DENIED,
	FLIPPY_RESULT_EXIST,
	FLIPPY_RESULT_INVALID_OBJECT,
	FLIPPY_RESULT_WRITE_PROTECTED,
	FLIPPY_RESULT_INVALID_DRIVE,
	FLIPPY_RESULT_NOT_ENABLED,
	FLIPPY_RESULT_NO_FILESYSTEM,
	FLIPPY_RESULT_MKFS_ABORTED,
	FLIPPY_RESULT_TIMEOUT,
	FLIPPY_RESULT_LOCKED,
	FLIPPY_RESULT_NOT_ENOUGH_CORE,
	FLIPPY_RESULT_TOO_MANY_OPEN_FILES,
	FLIPPY_RESULT_INVALID_PARAMETER
} flippyresult;

typedef enum {
	FLIPPY_TYPE_FILE = 0,
	FLIPPY_TYPE_DIR,
	FLIPPY_TYPE_BAD = 0xFF
} flippyfiletype;

typedef struct {
	u32 result;
	u64 size;
	u8 handle;
	u8 flags;
	u8 padding[18];
} ATTRIBUTE_PACKED flippyfile;

typedef struct {
	flippyfile file;
	u8 buffer[32];
} flippyfileinfo;

typedef struct {
	char name[FLIPPY_MAX_PATH];
	u8 type;
	u8 flags;
	u64 size;
	u32 date;
	u32 time;
	u32 result;
	u8 padding[10];
} ATTRIBUTE_PACKED flippyfilestat;

typedef struct {
	flippyfile dir;
	flippyfilestat stat;
} flippydirinfo;

typedef struct {
	u32 flags;
	u8 major;
	u8 minor;
	u16 build : 15;
	u16 dirty : 1;
	u8 padding[16];
} ATTRIBUTE_PACKED flippyversion;

typedef enum {
	FLIPPY_MODE_BOOT = 0,
	FLIPPY_MODE_UPDATE,
	FLIPPY_MODE_NOUPDATE
} flippybootmode;

typedef struct {
	u32 magic;
	u8 show_video;
	u8 show_progress_bar;
	u16 current_progress;
	char text[64];
	char subtext[64];
	u8 padding[120];
} ATTRIBUTE_PACKED flippybootstatus;

flippyresult flippy_boot(flippybootmode mode);
flippybootstatus *flippy_getbootstatus(void);
flippyresult flippy_mount(flippyfileinfo *info);
flippyresult flippy_reset(void);
flippyresult flippy_mkdir(const char *path);
flippyresult flippy_pread(flippyfileinfo *info, void *buf, u32 len, u32 offset);
flippyresult flippy_pwrite(flippyfileinfo *info, const void *buf, u32 len, u32 offset);
flippyresult flippy_open(flippyfileinfo *info, const char *path, u32 flags);
flippyresult flippy_opendir(flippydirinfo *info, const char *path);
flippyresult flippy_closeall(void);
flippyresult flippy_close(flippyfileinfo *info);
flippyresult flippy_closedir(flippydirinfo *info);
flippyresult flippy_unlink(const char *path);
flippyresult flippy_readdir(flippydirinfo *info, flippyfilestat *entry);
flippyresult flippy_flash_open(flippyfileinfo *info, const char *path, u32 flags);
flippyresult flippy_flash_opendir(flippydirinfo *info, const char *path);
flippyresult flippy_flash_unlink(const char *path);
flippyresult flippy_rename(const char *old, const char *new);
flippyresult flippy_init(void);

#ifdef __cplusplus
}
#endif

#endif
