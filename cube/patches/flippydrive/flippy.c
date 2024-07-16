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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"
#include "interrupt.h"
#include "ipl.h"

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 2
#endif

static struct {
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t command;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} flippy;

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read;
} dvd;

static void flippy_reset(void)
{
	DI[1] = 0b100;

	DI[2] = DI_CMD_FLIPPY_IPC << 24 | 0x05;
	DI[3] = 0xAA55F641;
	DI[7] = 0b001;
	while (DI[7] & 0b001);

	while (!(DI[1] & 0b100));
}

static void di_interrupt_handler(OSInterrupt interrupt, OSContext *context);

static void flippy_done_queued(void);
static void flippy_read_queued(void)
{
	void *buffer = flippy.queued->buffer;
	uint32_t length = flippy.queued->length;
	uint32_t offset = flippy.queued->offset;
	uint32_t command = flippy.queued->command;

	DI[0] = 0b0011000;
	DI[1] = 0;

	switch (command >> 24) {
		case DI_CMD_READ:
			DI[2] = command;
			DI[3] = offset;
			DI[4] = length;
			DI[5] = (uint32_t)buffer;
			DI[6] = length;
			DI[7] = 0b011;
			break;
		case DI_CMD_SEEK:
			DI[2] = command;
			DI[3] = offset;
			DI[7] = 0b001;
			break;
		case DI_CMD_FLIPPY_IPC:
			switch (command & 0xFF) {
				case 0x01:
					DI[2] = command;
					DI[7] = 0b001;
					break;
				case 0x08:
					DI[2] = command;
					DI[3] = offset;
					DI[4] = length;
					DI[5] = (uint32_t)buffer;
					DI[6] = length;
					DI[7] = 0b011;
					break;
				case 0x09:
					DI[2] = command;
					DI[3] = offset;
					DI[4] = length;
					DI[5] = (uint32_t)buffer;
					DI[6] = length;
					DI[7] = 0b111;
					break;
			}
			break;
		case DI_CMD_AUDIO_STREAM:
			DI[2] = command;
			DI[3] = offset;
			DI[4] = length;
			DI[7] = 0b001;
			break;
		case DI_CMD_REQUEST_AUDIO_STATUS:
			DI[2] = command;
			DI[7] = 0b001;
			break;
	}

	set_interrupt_handler(OS_INTERRUPT_PI_DI, di_interrupt_handler);
	unmask_interrupts(OS_INTERRUPTMASK_PI_DI);
}

static void flippy_done_queued(void)
{
	void *buffer = flippy.queued->buffer;
	uint32_t length = flippy.queued->length;
	uint32_t offset = flippy.queued->offset;
	uint32_t command = flippy.queued->command;

	switch (command >> 24) {
		case DI_CMD_FLIPPY_IPC:
			switch (command & 0xFF) {
				case 0x01:
					const frag_t *frag = buffer;
					*VAR_CURRENT_DISC = frag->file;
					break;
			}
			break;
		case DI_CMD_AUDIO_STREAM:
		case DI_CMD_REQUEST_AUDIO_STATUS:
			*(uint32_t *)buffer = DI[8];
			break;
	}

	flippy.queued->callback(buffer, length);

	flippy.queued->callback = NULL;
	flippy.queued = NULL;

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (flippy.queue[i].callback != NULL) {
			flippy.queued = &flippy.queue[i];
			flippy_read_queued();
			return;
		}
	}
}

static void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	mask_interrupts(OS_INTERRUPTMASK_PI_DI);

	flippy_done_queued();
}

bool flippy_push_queue(void *buffer, uint32_t length, uint32_t offset, uint32_t command, frag_callback callback)
{
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (flippy.queue[i].callback == NULL) {
			flippy.queue[i].buffer = buffer;
			flippy.queue[i].length = length;
			flippy.queue[i].offset = offset;
			flippy.queue[i].command = command;
			flippy.queue[i].callback = callback;

			if (flippy.queued == NULL) {
				flippy.queued = &flippy.queue[i];
				flippy_read_queued();
			}
			return true;
		}
	}

	return false;
}

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback)
{
	uint32_t command;
	uint8_t handle = sector >> 32;
	uint32_t filepos = sector;
	offset += filepos;

	if (write)
		command = DI_CMD_FLIPPY_IPC << 24 | handle << 16 | 0x09;
	else
		command = DI_CMD_FLIPPY_IPC << 24 | handle << 16 | 0x08;

	if (!length) {
		callback(buffer, length);
		return true;
	}

	return flippy_push_queue(buffer, length, offset, command, callback);
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	offset += frag->filepos;

	if (length)
		return flippy_push_queue(buffer, length, offset >> 2, DI_CMD_READ << 24, callback);
	else
		return flippy_push_queue(buffer, length, offset >> 2, DI_CMD_SEEK << 24, callback);
}

void schedule_read(OSTick ticks)
{
	void read_callback(void *address, uint32_t length)
	{
		dvd.buffer += length;
		dvd.length -= length;
		dvd.offset += length;
		dvd.read = !!dvd.length;

		schedule_read(0);
	}

	if (!dvd.read) {
		di_complete_transfer();
		return;
	}

	frag_read_async(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, read_callback);
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	if ((*VAR_IGR_TYPE & 0x80) && offset == 0x2440) {
		*VAR_CURRENT_DISC = FRAGS_APPLOADER;
		*VAR_SECOND_DISC = 0;
	}

	dvd.buffer = OSPhysicalToUncached(address);
	dvd.length = length;
	dvd.offset = offset;
	dvd.read = true;

	#ifdef DVD_MATH
	void alarm_handler(OSAlarm *alarm, OSContext *context)
	{
		schedule_read(0);
	}

	if (*VAR_EMU_READ_SPEED) {
		dvd_schedule_read(offset, length, alarm_handler);
		return;
	}
	#endif

	schedule_read(0);
}

bool change_disc(void)
{
	void callback(void *address, uint32_t length)
	{
		OSSetAlarm(&cover_alarm, OSSecondsToTicks(1.5), di_close_cover);
	}

	if (*VAR_SECOND_DISC) {
		const frag_t *frag = NULL;
		int fragnum = frag_get_list(*VAR_CURRENT_DISC ^ 1, &frag);

		if (fragnum)
			return flippy_push_queue((void *)frag, fragnum, 0, DI_CMD_FLIPPY_IPC << 24 | frag->handle << 16 | 0x01, callback);
	}

	return false;
}

void reset_devices(void)
{
	while (DI[7] & 0b001);

	if (AI[0] & 0b0000001) {
		AI[1] = 0;

		DI[2] = DI_CMD_AUDIO_STREAM << 24 | 0x01 << 16;
		DI[7] = 0b001;
		while (DI[7] & 0b001);

		AI[0] &= ~0b0000001;
	}

	flippy_reset();

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	EXI[EXI_CHANNEL_0][0] = 0;
	EXI[EXI_CHANNEL_1][0] = 0;
	EXI[EXI_CHANNEL_2][0] = 0;

	reset_device();
	ipl_set_config(1);
}
