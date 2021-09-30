/* 
 * Copyright (c) 2019-2021, Extrems <extrems@extremscorner.org>
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
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, patch;
} dvd = {0};

static struct {
	uint32_t base_sector;
	int items;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t sector;
		frag_callback callback;
	} queue[QUEUE_SIZE];
} wkf = {
	.base_sector = ~0
};

OSAlarm read_alarm = {0};

static void wkf_read_queued(void)
{
	void *buffer = wkf.queue[0].buffer;
	uint32_t length = wkf.queue[0].length;
	uint32_t offset = wkf.queue[0].offset;
	uint32_t sector = wkf.queue[0].sector;

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
		DI[2] = 0xA8000000;
		DI[3] = offset >> 2;
		DI[4] = length;
		DI[5] = (uint32_t)buffer;
		DI[6] = length;
		DI[7] = 0b011;
	} else {
		DI[2] = 0xAB000000;
		DI[3] = offset >> 2;
		DI[7] = 0b001;
	}

	OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DI);
}

void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSMaskInterrupts(OS_INTERRUPTMASK_PI_DI);

	void *buffer = wkf.queue[0].buffer;
	uint32_t length = wkf.queue[0].length;
	uint32_t offset = wkf.queue[0].offset;
	uint32_t sector = wkf.queue[0].sector;

	wkf.queue[0].callback(buffer, length);

	if (--wkf.items) {
		memcpy(wkf.queue, wkf.queue + 1, wkf.items * sizeof(*wkf.queue));
		wkf_read_queued();
	}
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, frag_callback callback)
{
	length = MIN(length, 32768 - OSRoundUp32B(offset) % 32768);

	wkf.queue[wkf.items].buffer = buffer;
	wkf.queue[wkf.items].length = length;
	wkf.queue[wkf.items].offset = offset;
	wkf.queue[wkf.items].sector = sector;
	wkf.queue[wkf.items].callback = callback;
	if (wkf.items++) return true;

	wkf_read_queued();
	return true;
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
	frag_read_async(dvd.buffer, dvd.length, dvd.offset, read_callback);
	#else
	dvd.patch = is_frag_patch(dvd.offset, dvd.length);

	if (!dvd.patch)
		frag_read_async(dvd.buffer, dvd.length, dvd.offset, read_callback);
	else
		OSSetAlarm(&read_alarm, ticks, (OSAlarmHandler)trickle_read);
	#endif
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	dvd.buffer = OSPhysicalToUncached(address);
	dvd.length = length;
	dvd.offset = offset | *VAR_CURRENT_DISC << 31;
	dvd.read = true;

	schedule_read(0);
}

void trickle_read(void)
{
	#ifndef ASYNC_READ
	if (dvd.read && dvd.patch) {
		OSTick start = OSGetTick();
		int size = frag_read(dvd.buffer, dvd.length, dvd.offset);
		OSTick end = OSGetTick();

		dvd.buffer += size;
		dvd.length -= size;
		dvd.offset += size;
		dvd.read = !!dvd.length;

		schedule_read(OSDiffTick(end, start));
	}
	#endif
}

void device_reset(void)
{
	while (DI[7] & 0b001);

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	EXI[EXI_CHANNEL_0][0] = 0;
	EXI[EXI_CHANNEL_1][0] = 0;
	EXI[EXI_CHANNEL_2][0] = 0;

	end_read();
}

bool change_disc(void)
{
	if (*VAR_SECOND_DISC) {
		*VAR_CURRENT_DISC ^= 1;
		return true;
	}

	return false;
}
