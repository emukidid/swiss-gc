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
#include "common.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, patch;
} dvd = {0};

static void dvd_read(void *address, uint32_t length, uint32_t offset)
{
	DI[2] = DI_CMD_READ << 24;
	DI[3] = offset >> 2;
	DI[4] = length;
	DI[5] = (uint32_t)address;
	DI[6] = length;
	DI[7] = 0b011;
}

static void dvd_read_diskid(DVDDiskID *diskID)
{
	DI[2] = DI_CMD_READ << 24 | 0x40;
	DI[3] = 0;
	DI[4] = sizeof(DVDDiskID);
	DI[5] = (uint32_t)diskID;
	DI[6] = sizeof(DVDDiskID);
	DI[7] = 0b011;
}

#ifdef GCODE
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
#endif

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	if (dvd.read && buffer == dvd.buffer) {
		dvd_read(buffer, length, offset);
		dvd.read = false;
		return true;
	}

	return false;
}

void schedule_read(OSTick ticks)
{
	#ifdef ASYNC_READ
	void read_callback(void *address, uint32_t length)
	{
		dvd.buffer += length;
		dvd.length -= length;
		dvd.offset += length;
		dvd.read = !!dvd.length;

		schedule_read(0);
	}
	#else
	OSCancelAlarm(&read_alarm);
	#endif

	if (!dvd.read) {
		di_complete_transfer();
		return;
	}

	#ifdef ASYNC_READ
	frag_read_async(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, read_callback);
	#else
	dvd.patch = frag_read_patch(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, NULL);

	if (dvd.patch)
		OSSetAlarm(&read_alarm, ticks, trickle_read);
	#endif
}

static bool memeq(const void *a, const void *b, size_t size)
{
	const uint8_t *x = a;
	const uint8_t *y = b;
	size_t i;

	for (i = 0; i < size; ++i)
		if (x[i] != y[i])
			return false;

	return true;
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	if ((*VAR_IGR_TYPE & 0x80) && offset == 0x2440) {
		*VAR_CURRENT_DISC = FRAGS_APPLOADER;
		*VAR_SECOND_DISC = 0;
	} else {
		switch (*VAR_CURRENT_DISC) {
			case FRAGS_DISC_1:
				if (memeq(VAR_AREA, VAR_DISC_2_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_DISC_2;
				else if (!memeq(VAR_AREA, VAR_DISC_1_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_NULL;
				break;
			case FRAGS_DISC_2:
				if (memeq(VAR_AREA, VAR_DISC_1_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_DISC_1;
				else if (!memeq(VAR_AREA, VAR_DISC_2_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_NULL;
				break;
			case FRAGS_NULL:
				if (memeq(VAR_AREA, VAR_DISC_1_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_DISC_1;
				else if (memeq(VAR_AREA, VAR_DISC_2_ID, 8))
					*VAR_CURRENT_DISC = FRAGS_DISC_2;
				break;
		}
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

	#ifdef GCODE
	const frag_t *frag = NULL;
	int count = frag_get_list(FRAGS_BOOT_GCM, &frag);
	gcode_set_disc_frags(0, frag, count);
	gcode_set_disc_number(0);
	#endif

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	end_read();
}
