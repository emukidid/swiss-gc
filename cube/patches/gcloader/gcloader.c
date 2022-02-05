/* 
 * Copyright (c) 2021-2022, Extrems <extrems@extremscorner.org>
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

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 2
#endif
#define SECTOR_SIZE 512

static struct {
	char (*buffer)[SECTOR_SIZE];
	uint32_t last_sector;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t sector;
		uint32_t command;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} gcode = {
	.buffer = OSCachedToUncached(&VAR_SECTOR_BUF),
	.last_sector = ~0
};

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read;
} dvd = {0};

static void gcode_set_disc_frags(uint32_t disc, const frag_t *frag, uint32_t count)
{
	DI[2] = DI_CMD_GCODE_SET_DISC_FRAGS << 24 | 0x01;
	DI[3] = disc;
	DI[4] = count;
	DI[7] = 0b001;
	while (DI[7] & 0b001);
	if (DI[8]) return;

	while (count--) {
		DI[2] = frag->offset;
		DI[3] = frag->size;
		DI[4] = frag->sector;
		DI[7] = 0b001;
		while (DI[7] & 0b001);
		if (!DI[8]) break;
		frag++;
	}
}

static void gcode_set_disc_number(uint32_t disc)
{
	DI[2] = DI_CMD_GCODE_SET_DISC_FRAGS << 24 | 0x02;
	DI[3] = disc;
	DI[7] = 0b001;
	while (DI[7] & 0b001);
}

static void gcode_done_queued(void);
static void gcode_read_queued(void)
{
	void *buffer = gcode.queued->buffer;
	uint32_t length = gcode.queued->length;
	uint32_t offset = gcode.queued->offset;
	uint32_t sector = gcode.queued->sector;
	uint32_t command = gcode.queued->command;

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
		case DI_CMD_GCODE_READ:
			switch (command & 0xFF) {
				default:
					DI[2] = command;
					DI[3] = sector;
					DI[4] = length;
					DI[5] = (uint32_t)buffer;
					DI[6] = length;
					DI[7] = 0b011;
					break;
				case 0x01:
					if (sector == gcode.last_sector) {
						gcode_done_queued();
						return;
					} else {
						gcode.last_sector = sector;

						DI[2] = DI_CMD_GCODE_READ << 24;
						DI[3] = sector;
						DI[4] = SECTOR_SIZE;
						DI[5] = (uint32_t)gcode.buffer;
						DI[6] = SECTOR_SIZE;
						DI[7] = 0b011;
					}
					break;
			}
			break;
		case DI_CMD_GCODE_SET_DISC_FRAGS:
			DI[2] = command;
			DI[3] = offset;
			DI[4] = length;
			DI[7] = 0b001;
			break;
		case DI_CMD_GCODE_WRITE_BUFFER:
			DI[2] = command;
			DI[5] = (uint32_t)buffer;
			DI[6] = length;
			DI[7] = 0b111;
			break;
		case DI_CMD_GCODE_WRITE:
			if (gcode.last_sector == sector)
				gcode.last_sector = ~0;

			DI[2] = command;
			DI[3] = sector;
			DI[7] = 0b001;
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

	OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DI);
}

static void gcode_done_queued(void)
{
	void *buffer = gcode.queued->buffer;
	uint32_t length = gcode.queued->length;
	uint32_t offset = gcode.queued->offset;
	uint32_t sector = gcode.queued->sector;
	uint32_t command = gcode.queued->command;

	switch (command >> 24) {
		case DI_CMD_GCODE_READ:
			switch (command & 0xFF) {
				case 0x01:
					buffer = memcpy(buffer, *gcode.buffer + offset, length);
					break;
			}
			break;
		case DI_CMD_GCODE_SET_DISC_FRAGS:
			switch (command & 0xFF) {
				case 0x01:
					if (DI[8]) {
						length = 0;
					} else {
						const frag_t *frag = buffer;

						for (int i = 0; i < length; i++) {
							DI[2] = frag[i].offset;
							DI[3] = frag[i].size;
							DI[4] = frag[i].sector;
							DI[7] = 0b001;
							while (DI[7] & 0b001);
							if (!DI[8]) {
								*VAR_CURRENT_DISC = frag[i].file;
								break;
							}
						}
					}
					break;
				case 0x02:
					if (!DI[8])
						*VAR_CURRENT_DISC = offset;
					break;
			}
			break;
		case DI_CMD_GCODE_WRITE_BUFFER:
			gcode.queued->command = DI_CMD_GCODE_WRITE << 24;
			gcode_read_queued();
			return;
		case DI_CMD_GCODE_WRITE:
			if (DI[8])
				length = 0;
			break;
		case DI_CMD_AUDIO_STREAM:
		case DI_CMD_REQUEST_AUDIO_STATUS:
			*(uint32_t *)buffer = DI[8];
			break;
	}

	gcode.queued->callback(buffer, length);

	gcode.queued->callback = NULL;
	gcode.queued = NULL;

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (gcode.queue[i].callback != NULL) {
			gcode.queued = &gcode.queue[i];
			gcode_read_queued();
			return;
		}
	}
}

void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSMaskInterrupts(OS_INTERRUPTMASK_PI_DI);

	gcode_done_queued();
}

bool gcode_push_queue(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, uint32_t command, frag_callback callback)
{
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (gcode.queue[i].callback == NULL) {
			gcode.queue[i].buffer = buffer;
			gcode.queue[i].length = length;
			gcode.queue[i].offset = offset;
			gcode.queue[i].sector = sector;
			gcode.queue[i].command = command;
			gcode.queue[i].callback = callback;

			if (gcode.queued == NULL) {
				gcode.queued = &gcode.queue[i];
				gcode_read_queued();
			}
			return true;
		}
	}

	return false;
}

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback)
{
	uint32_t command;
	sector = offset / SECTOR_SIZE + sector;
	offset = offset % SECTOR_SIZE;

	if (write) {
		length = SECTOR_SIZE;
		command = DI_CMD_GCODE_WRITE_BUFFER << 24;
	} else if ((intptr_t)buffer % 32 + offset) {
		length = MIN(length, SECTOR_SIZE - offset);
		command = DI_CMD_GCODE_READ << 24 | 0x01;
	} else
		command = DI_CMD_GCODE_READ << 24;

	return gcode_push_queue(buffer, length, offset, sector, command, callback);
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	if (length)
		return gcode_push_queue(buffer, length, offset >> 2, frag->sector, DI_CMD_READ << 24, callback);
	else
		return gcode_push_queue(buffer, length, offset >> 2, frag->sector, DI_CMD_SEEK << 24, callback);
}

int do_read_write(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write)
{
	return 0;
}

void end_read(void) {}

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
		int count = frag_get_list(*VAR_CURRENT_DISC ^ 1, &frag);
		if (count)
			return gcode_push_queue((void *)(frag + 1), count - 1, 0, 0, DI_CMD_GCODE_SET_DISC_FRAGS << 24 | 0x01, callback);
	}

	return false;
}

void reset_device(void)
{
	DVDDiskID *id = (DVDDiskID *)VAR_AREA;

	while (DI[7] & 0b001);

	if (id->streaming) {
		AI[1] = 0;

		DI[2] = DI_CMD_AUDIO_STREAM << 24 | 0x01 << 16;
		DI[7] = 0b001;
		while (DI[7] & 0b001);

		AI[0] &= ~0b0000001;
	}

	const frag_t *frag = NULL;
	int count = frag_get_list(FRAGS_BOOT_GCM, &frag);
	gcode_set_disc_frags(0, frag, count);
	gcode_set_disc_number(0);

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	end_read();
}
