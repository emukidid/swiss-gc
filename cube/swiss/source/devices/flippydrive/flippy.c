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
#include <ogc/semaphore.h>
#include "flippy.h"

#define FLIPPY_CMD_BOOT(magic) \
	((dvdcmdbuf){(((0x12) << 24)), (0xABADBEEF), (magic)})
#define FLIPPY_CMD_BOOT_STATUS \
	((dvdcmdbuf){(((0xB4) << 24))})
#define FLIPPY_CMD_STATUS \
	((dvdcmdbuf){(((0xB5) << 24) | (0x00))})
#define FLIPPY_CMD_MOUNT(handle) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x01))})
#define FLIPPY_CMD_RESET \
	((dvdcmdbuf){(((0xB5) << 24) | (0x05)), (0xAA55F641)})
#define FLIPPY_CMD_MKDIR \
	((dvdcmdbuf){(((0xB5) << 24) | (0x07))})
#define FLIPPY_CMD_READ(handle, offset, length) \
	((dvdcmdbuf){(((0xB5) << 24) | (((handle) & 0xFF) << 16) | (0x08)), (offset), (length)})
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

typedef void (*dvdcallbacklow)(s32 result);

extern s32 DVD_LowWaitCoverClose(dvdcallbacklow cb);

static dvddrvinfo driveinfo ATTRIBUTE_ALIGN(32);
static flippybootstatus bootstatus ATTRIBUTE_ALIGN(32);

static lwpq_t queue = LWP_TQUEUE_NULL;
static sem_t semaphore = LWP_SEM_NULL;

static void cover_callback(s32 result)
{
	LWP_SemPost(semaphore);
	DVD_Resume();
}

static void reset_callback(s32 result, dvdcmdblk *block)
{
	DVD_Pause();
	DVD_LowWaitCoverClose(cover_callback);
}

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

static void nostatus_callback(s32 result, dvdcmdblk *block)
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

flippyresult flippy_boot(flippybootmode mode)
{
	dvdcmdblk block;
	u32 magic;

	switch (mode) {
		case FLIPPY_MODE_BOOT:
			magic = 0xCAFE6969;
			break;
		case FLIPPY_MODE_UPDATE:
			magic = 0xDABFED69;
			break;
		case FLIPPY_MODE_NOUPDATE:
			magic = 0xDECAF420;
			break;
		default:
			return FLIPPY_RESULT_INVALID_PARAMETER;
	}

	if (DVD_ReadImmPrio(&block, FLIPPY_CMD_BOOT(magic), NULL, 0, 0) < 0)
		return FLIPPY_RESULT_NOT_READY;

	return FLIPPY_RESULT_OK;
}

flippybootstatus *flippy_getbootstatus(void)
{
	dvdcmdblk block;

	if (DVD_ReadDmaPrio(&block, FLIPPY_CMD_BOOT_STATUS, &bootstatus, sizeof(flippybootstatus), 2) < 0)
		return NULL;

	return &bootstatus;
}

flippyresult flippy_mount(flippyfileinfo *info)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	DVD_SetUserData(&block, file);
	if (!DVD_ReadImmAsyncPrio(&block, FLIPPY_CMD_MOUNT(file->handle), NULL, 0, nostatus_callback, 0)) {
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

flippyresult flippy_reset(void)
{
	dvdcmdblk block;

	LWP_SemInit(&semaphore, 0, 1);
	if (!DVD_ReadImmAsyncPrio(&block, FLIPPY_CMD_RESET, NULL, 0, reset_callback, 3)) {
		LWP_SemDestroy(semaphore);
		return FLIPPY_RESULT_NOT_READY;
	}

	LWP_SemWait(semaphore);
	LWP_SemDestroy(semaphore);
	return FLIPPY_RESULT_OK;
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

static flippyresult flippy_pread_dma(flippyfileinfo *info, void *buf, u32 len, u32 offset)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	DVD_SetUserData(&block, file);
	if (!DVD_ReadDmaAsyncPrio(&block, FLIPPY_CMD_READ(file->handle, offset, len), buf, (len + 31) & ~31, nostatus_callback, 2)) {
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
	u32 roundlen;
	s32 misalign;
	flippyresult result;
	flippyfile *file = &info->file;

	if (offset + len > file->size) {
		file->result = FLIPPY_RESULT_INVALID_PARAMETER;
		return file->result;
	}

	if (!len) {
		file->result = FLIPPY_RESULT_OK;
		return file->result;
	}

	if (len <= 32) {
		result = flippy_pread_dma(info, info->buffer, 32, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		memcpy(buf, info->buffer, len);
		return result;
	}

	if ((misalign = -(u32)buf & 31)) {
		result = flippy_pread_dma(info, info->buffer, 32, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		memcpy(buf, info->buffer, misalign);
		offset += misalign;
		len -= misalign;
		buf += misalign;
	}

	if ((roundlen = len & ~31)) {
		result = flippy_pread_dma(info, buf, roundlen, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		offset += roundlen;
		len -= roundlen;
		buf += roundlen;
	}

	if (len) {
		result = flippy_pread_dma(info, info->buffer, 32, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		memcpy(buf, info->buffer, len);
	}

	return result;
}

static flippyresult flippy_pwrite_dma(flippyfileinfo *info, const void *buf, u32 len, u32 offset)
{
	dvdcmdblk block;
	flippyfile *file = &info->file;

	do {
		u32 xlen = len;
		if (xlen > 16352)
			xlen = 16352;

		DVD_SetUserData(&block, file);
		if (!DVD_WriteDmaAsyncPrio(&block, FLIPPY_CMD_WRITE(file->handle, offset, xlen), buf, xlen ? (xlen + 31) & ~31 : 32, nostatus_callback, 2)) {
			file->result = FLIPPY_RESULT_NOT_READY;
			return file->result;
		}

		u32 level = IRQ_Disable();
		while (block.state != DVD_STATE_END
			&& block.state != DVD_STATE_FATAL_ERROR
			&& block.state != DVD_STATE_CANCELED)
			LWP_ThreadSleep(queue);
		IRQ_Restore(level);

		if (file->result != FLIPPY_RESULT_OK) break;
		offset += xlen;
		len -= xlen;
		buf += xlen;
	} while (len);

	return file->result;
}

flippyresult flippy_pwrite(flippyfileinfo *info, const void *buf, u32 len, u32 offset)
{
	u32 roundlen;
	s32 misalign;
	flippyresult result;
	flippyfile *file = &info->file;

	if (!(file->flags & FLIPPY_FLAG_WRITE)) {
		file->result = FLIPPY_RESULT_DENIED;
		return file->result;
	}

	if (!len && offset <= file->size) {
		file->result = FLIPPY_RESULT_OK;
		return file->result;
	}

	if (len <= 32) {
		memset(info->buffer, 0, 32);
		memcpy(info->buffer, buf, len);
		return flippy_pwrite_dma(info, info->buffer, len, offset);
	}

	if ((misalign = -(u32)buf & 31)) {
		memset(info->buffer, 0, 32);
		memcpy(info->buffer, buf, misalign);
		result = flippy_pwrite_dma(info, info->buffer, misalign, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		offset += misalign;
		len -= misalign;
		buf += misalign;
	}

	if ((roundlen = len & ~31)) {
		result = flippy_pwrite_dma(info, buf, roundlen, offset);
		if (result != FLIPPY_RESULT_OK) return result;
		offset += roundlen;
		len -= roundlen;
		buf += roundlen;
	}

	if (len) {
		memset(info->buffer, 0, 32);
		memcpy(info->buffer, buf, len);
		return flippy_pwrite_dma(info, info->buffer, len, offset);
	}

	return result;
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

flippyresult flippy_closeall(void)
{
	STACK_ALIGN(flippyfileinfo, info, 1, 32);

	for (int i = 0; i < FLIPPY_MAX_HANDLES; i++) {
		info->file.handle = i + 1;
		flippy_close(info);
	}

	info->file.handle = FLIPPY_FLASH_HANDLE;
	flippy_close(info);

	return FLIPPY_RESULT_OK;
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
	flippyversion *version = (flippyversion *)driveinfo.pad;

	if (initialized) return FLIPPY_RESULT_OK;
	DVD_Init();

	if (DVD_Inquiry(&block, &driveinfo) < 0 || driveinfo.rel_date != 0x20220426)
		return FLIPPY_RESULT_NOT_READY;

	if (FLIPPY_VERSION(version->major, version->minor, version->build) <
		FLIPPY_VERSION(FLIPPY_MINVER_MAJOR, FLIPPY_MINVER_MINOR, FLIPPY_MINVER_BUILD))
		return FLIPPY_RESULT_NOT_READY;

	LWP_InitQueue(&queue);
	initialized = true;
	return flippy_closeall();
}
