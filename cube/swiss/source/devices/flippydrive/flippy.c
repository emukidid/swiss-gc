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

#include <stdbool.h>
#include <string.h>
#include <ogc/dvd.h>
#include <ogc/irq.h>
#include <ogc/lwp.h>
#include <ogc/machine/processor.h>
#include "flippy.h"

#define FLIPPY_CMD_STATUS \
	((dvdcmdbuf){(((0xB5) << 24) | (0x00))})
#define FLIPPY_CMD_MOUNT(handle) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x01))})
#define FLIPPY_CMD_MKDIR \
	((dvdcmdbuf){(((0xB5) << 24) | (0x07))})
/* #define FLIPPY_CMD_READ(handle, offset, length) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x08)), (offset), (length)}) */
#define FLIPPY_CMD_READ(handle, offset, length) \
	((dvdcmdbuf){(((0xA8) << 24) | (((handle) & 0xFF) << 16)), ((offset) >> 2), (length)})
#define FLIPPY_CMD_WRITE(handle, offset, length) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x09)), (offset), (length)})
#define FLIPPY_CMD_OPEN \
	((dvdcmdbuf){(((0xB5) << 24) | (0x0A))})
#define FLIPPY_CMD_CLOSE(handle) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x0B))})
#define FLIPPY_CMD_UNLINK \
	((dvdcmdbuf){(((0xB5) << 24) | (0x0E))})
#define FLIPPY_CMD_READDIR(handle) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x0F))})
#define FLIPPY_CMD_FLASH_OPEN \
	((dvdcmdbuf){(((0xB5) << 24) | (0x11))})
#define FLIPPY_CMD_FLASH_UNLINK \
	((dvdcmdbuf){(((0xB5) << 24) | (0x12))})
#define FLIPPY_CMD_RENAME \
	((dvdcmdbuf){(((0xB5) << 24) | (0x13))})

static lwpq_t queue = LWP_TQUEUE_NULL;

static void status_callback(s32 result, dvdcmdblk *block)
{
	flippyfile *file = block->buf;

	if (result != sizeof(flippyfile)) {
		file->result = FLIPPY_RESULT_DISK_ERR;
	} else {
		file->result = bswap32(file->result);
		file->size   = bswap64(file->size);
	}

	LWP_ThreadBroadcast(queue);
}

static void read_callback(s32 result, dvdcmdblk *block)
{
	flippyfile *file = block->usrdata;

	if (result != block->currtxsize)
		file->result = FLIPPY_RESULT_DISK_ERR;
	else
		file->result = FLIPPY_RESULT_OK;

	LWP_ThreadBroadcast(queue);
}

static void readdir_callback(s32 result, dvdcmdblk *block)
{
	flippyfilestat *stat  = block->buf;
	flippyfilestat *entry = block->usrdata;

	if (result != sizeof(flippyfilestat)) {
		entry->result = FLIPPY_RESULT_DISK_ERR;
	} else {
		stat->size   = bswap64(stat->size);
		stat->date   = bswap32(stat->date);
		stat->time   = bswap32(stat->time);
		stat->result = bswap32(stat->result);

		memcpy(entry, stat, sizeof(flippyfilestat));
	}

	LWP_ThreadBroadcast(queue);
}

static void command_callback(s32 result, dvdcmdblk *block)
{
	flippyfile *file = block->usrdata;

	if (result != block->currtxsize) {
		file->result = FLIPPY_RESULT_DISK_ERR;
	} else {
		DVD_ReadDmaAsyncPrio(block, FLIPPY_CMD_STATUS, file, sizeof(flippyfile), status_callback, 0);
		return;
	}

	LWP_ThreadBroadcast(queue);
}

flippyresult flippy_mount(flippyfileinfo *info)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	DVD_SetUserData(&block, file);
	if (!DVD_ReadImmAsyncPrio(&block, FLIPPY_CMD_MOUNT(file->handle), NULL, 0, read_callback, 0)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_mkdir(const char *path)
{
	dvdcmdblk block;
	STACK_ALIGN(flippyfile,     file, 1, 32);
	STACK_ALIGN(flippyfilestat, stat, 1, 32);

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_MKDIR, stat, sizeof(flippyfilestat), command_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_pread(flippyfileinfo *info, void *buf, u32 len, u32 offset)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	if (offset % 4 || offset + len > file->size) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	if (!len) {
		file->result = FLIPPY_RESULT_OK;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_ReadDmaAsyncPrio(&block, FLIPPY_CMD_READ(file->handle, offset, len), buf, len, read_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_pwrite(flippyfileinfo *info, const void *buf, u32 len, u32 offset)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	if (!len) {
		file->result = FLIPPY_RESULT_OK;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_WRITE(file->handle, offset, len), buf, len, command_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_open(flippyfileinfo *info, const char *path, u32 flags)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;
	STACK_ALIGN(flippyfilestat, stat, 1, 32);

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	stat->type  = FLIPPY_TYPE_FILE;
	stat->flags = flags;

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_OPEN, stat, sizeof(flippyfilestat), command_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_opendir(flippydirinfo *info, const char *path)
{
	dvdcmdblk block;
	flippyfile *dir = &info->dir;
	flippyfilestat *stat = &info->stat;

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		dir->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return dir->result;
	}

	stat->type = FLIPPY_TYPE_DIR;

	DVD_SetUserData(&block, dir);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_OPEN, stat, sizeof(flippyfilestat), command_callback, 2)) {
		dir->result = FLIPPY_RESULT_NOT_READY;
		return dir->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return dir->result;
}

flippyresult flippy_close(flippyfileinfo *info)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	DVD_SetUserData(&block, file);
	if (!DVD_ReadImmAsyncPrio(&block, FLIPPY_CMD_CLOSE(file->handle), NULL, 0, command_callback, 3)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_closedir(flippydirinfo *info)
{
	dvdcmdblk block;
	flippyfile *dir = &info->dir;

	DVD_SetUserData(&block, dir);
	if (!DVD_ReadImmAsyncPrio(&block, FLIPPY_CMD_CLOSE(dir->handle), NULL, 0, command_callback, 3)) {
		dir->result = FLIPPY_RESULT_NOT_READY;
		return dir->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return dir->result;
}

flippyresult flippy_unlink(const char *path)
{
	dvdcmdblk block;
	STACK_ALIGN(flippyfile,     file, 1, 32);
	STACK_ALIGN(flippyfilestat, stat, 1, 32);

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_UNLINK, stat, sizeof(flippyfilestat), command_callback, 3)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_readdir(flippydirinfo *info, flippyfilestat *entry)
{
	dvdcmdblk block;
	flippyfile *dir = &info->dir;
	flippyfilestat *stat = &info->stat;

	DVD_SetUserData(&block, entry);
	if (!DVD_ReadDmaAsyncPrio(&block, FLIPPY_CMD_READDIR(dir->handle), stat, sizeof(flippyfilestat), readdir_callback, 2)) {
		entry->result = FLIPPY_RESULT_NOT_READY;
		return entry->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return entry->result;
}

flippyresult flippy_flash_open(flippyfileinfo *info, const char *path, u32 flags)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;
	STACK_ALIGN(flippyfilestat, stat, 1, 32);

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	stat->type  = FLIPPY_TYPE_FILE;
	stat->flags = flags;

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_FLASH_OPEN, stat, sizeof(flippyfilestat), command_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_flash_opendir(flippydirinfo *info, const char *path)
{
	dvdcmdblk block;
	flippyfile *dir = &info->dir;
	flippyfilestat *stat = &info->stat;

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		dir->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return dir->result;
	}

	stat->type = FLIPPY_TYPE_DIR;

	DVD_SetUserData(&block, dir);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_FLASH_OPEN, stat, sizeof(flippyfilestat), command_callback, 2)) {
		dir->result = FLIPPY_RESULT_NOT_READY;
		return dir->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return dir->result;
}

flippyresult flippy_flash_unlink(const char *path)
{
	dvdcmdblk block;
	STACK_ALIGN(flippyfile,     file, 1, 32);
	STACK_ALIGN(flippyfilestat, stat, 1, 32);

	memset(stat, 0, sizeof(flippyfilestat));

	if (strlcpy(stat->name, path, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_FLASH_UNLINK, stat, sizeof(flippyfilestat), command_callback, 3)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_rename(const char *old, const char *new)
{
	dvdcmdblk block;
	STACK_ALIGN(flippyfile,     file,  1, 32);
	STACK_ALIGN(flippyfilestat, stats, 2, 32);

	memset(stats, 0, sizeof(flippyfilestat) * 2);

	if (strlcpy(stats[0].name, old, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	if (strlcpy(stats[1].name, new, FLIPPY_MAX_PATH) >= FLIPPY_MAX_PATH) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	DVD_SetUserData(&block, file);
	if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_RENAME, stats, sizeof(flippyfilestat) * 2, command_callback, 2)) {
		file->result = FLIPPY_RESULT_NOT_READY;
		return file->result;
	}

	u32 level = IRQ_Disable();
	while (block.state != DVD_STATE_END
		&& block.state != DVD_STATE_FATAL_ERROR
		&& block.state != DVD_STATE_CANCELED)
		LWP_ThreadSleep(queue);
	IRQ_Restore(level);

	return file->result;
}

flippyresult flippy_init(void)
{
	static bool initialized;
	dvdcmdblk block;
	STACK_ALIGN(dvddrvinfo,     driveinfo, 1, 32);
	STACK_ALIGN(flippyfileinfo, fileinfo,  1, 32);

	if (initialized) return FLIPPY_RESULT_OK;
	DVD_Init();

	if (DVD_Inquiry(&block, driveinfo) < 0 || driveinfo->rel_date != 0x20220426)
		return FLIPPY_RESULT_NOT_READY;

	LWP_InitQueue(&queue);

	for (int i = 0; i < FLIPPY_MAX_HANDLE; i++) {
		fileinfo->file.handle = i + 1;
		flippy_close(fileinfo);
	}

	initialized = true;
	return FLIPPY_RESULT_OK;
}
