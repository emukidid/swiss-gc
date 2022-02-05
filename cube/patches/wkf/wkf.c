/* 
 * Copyright (c) 2019-2022, Extrems <extrems@extremscorner.org>
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
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 2
#endif

static struct {
	uint32_t base_sector;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t sector;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} wkf = {
	.base_sector = ~0
};

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, patch;
} dvd = {0};

static void wkf_read_queued(void)
{
	void *buffer = wkf.queued->buffer;
	uint32_t length = wkf.queued->length;
	uint32_t offset = wkf.queued->offset;
	uint32_t sector = wkf.queued->sector;

	if (wkf.base_sector != sector) {
		DI[2] = 0xDE000000;
		DI[3] = sector;
		DI[4] = 0x5A000000;
		DI[7] = 0b001;
		while (DI[7] & 0b001);
		wkf.base_sector = sector;
	}

	DI[0] = 0b0011000;
	DI[1] = 0;

	if (length) {
		DI[2] = DI_CMD_READ << 24;
		DI[3] = offset >> 2;
		DI[4] = length;
		DI[5] = (uint32_t)buffer;
		DI[6] = length;
		DI[7] = 0b011;
	} else {
		DI[2] = DI_CMD_SEEK << 24;
		DI[3] = offset >> 2;
		DI[7] = 0b001;
	}

	OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DI);
}

static void wkf_done_queued(void)
{
	void *buffer = wkf.queued->buffer;
	uint32_t length = wkf.queued->length;
	uint32_t offset = wkf.queued->offset;
	uint32_t sector = wkf.queued->sector;

	wkf.queued->callback(buffer, length);

	wkf.queued->callback = NULL;
	wkf.queued = NULL;

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (wkf.queue[i].callback != NULL && wkf.queue[i].length + wkf.queue[i].offset % 512 <= 512) {
			wkf.queued = &wkf.queue[i];
			wkf_read_queued();
			return;
		}
	}
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (wkf.queue[i].callback != NULL) {
			wkf.queued = &wkf.queue[i];
			wkf_read_queued();
			return;
		}
	}
}

void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSMaskInterrupts(OS_INTERRUPTMASK_PI_DI);

	wkf_done_queued();
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	length = MIN(length, 32768 - OSRoundUp32B(offset) % 32768);

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (wkf.queue[i].callback == NULL) {
			wkf.queue[i].buffer = buffer;
			wkf.queue[i].length = length;
			wkf.queue[i].offset = offset;
			wkf.queue[i].sector = frag->sector;
			wkf.queue[i].callback = callback;

			if (wkf.queued == NULL) {
				wkf.queued = &wkf.queue[i];
				wkf_read_queued();
			}
			return true;
		}
	}

	return false;
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
	#ifndef ASYNC_READ
	OSCancelAlarm(&read_alarm);
	#endif

	if (!dvd.read) {
		di_complete_transfer();
		return;
	}

	#ifdef ASYNC_READ
	frag_read_async(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, read_callback);
	#else
	dvd.patch = frag_read_patch(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, read_callback);

	if (dvd.patch)
		OSSetAlarm(&read_alarm, ticks, trickle_read);
	#endif
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

	schedule_read(0);
}

#ifndef ASYNC_READ
void trickle_read()
{
	if (dvd.read && dvd.patch) {
		OSTick start = OSGetTick();
		int size = frag_read(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset);
		OSTick end = OSGetTick();

		dvd.buffer += size;
		dvd.length -= size;
		dvd.offset += size;
		dvd.read = !!dvd.length;

		schedule_read(OSDiffTick(end, start));
	}
}
#endif

bool change_disc(void)
{
	if (*VAR_SECOND_DISC) {
		*VAR_CURRENT_DISC ^= 1;
		return true;
	}

	return false;
}

void reset_device(void)
{
	while (DI[7] & 0b001);

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	end_read();
}
