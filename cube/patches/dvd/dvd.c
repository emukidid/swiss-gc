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

OSAlarm read_alarm = {0};

static void dvd_read(void *address, uint32_t length, uint32_t offset)
{
	DI[2] = 0xA8000000;
	DI[3] = offset >> 2;
	DI[4] = length;
	DI[5] = (uint32_t)address;
	DI[6] = length;
	DI[7] = 0b011;
}

static void dvd_read_diskid(DVDDiskID *diskID)
{
	DI[2] = 0xA8000040;
	DI[3] = 0;
	DI[4] = sizeof(DVDDiskID);
	DI[5] = (uint32_t)diskID;
	DI[6] = sizeof(DVDDiskID);
	DI[7] = 0b011;
}

#ifdef GCODE
static void gcode_set_disc_frags(uint32_t disc, const frag_t *frag, uint32_t count)
{
	DI[2] = 0xB3000001;
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
		if (DI[8]) return;
		frag++;
	}
}

static void gcode_set_disc_number(uint32_t disc)
{
	DI[2] = 0xB3000002;
	DI[3] = disc;
	DI[4] = 0;
	DI[7] = 0b001;
	while (DI[7] & 0b001);
}
#endif

void schedule_read(OSTick ticks)
{
	OSCancelAlarm(&read_alarm);

	if (!dvd.read) {
		di_complete_transfer();
		return;
	}

	dvd.patch = is_frag_patch(dvd.offset, dvd.length);

	if (!dvd.patch)
		dvd_read(dvd.buffer, dvd.length, dvd.offset);
	else
		OSSetAlarm(&read_alarm, ticks, (OSAlarmHandler)trickle_read);
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	dvd.buffer = OSPhysicalToUncached(address);
	dvd.length = length;
	dvd.offset = offset;
	dvd.read = true;

	schedule_read(COMMAND_LATENCY_TICKS);
}

void trickle_read(void)
{
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
}

void device_reset(void)
{
	DVDDiskID *id = (DVDDiskID *)VAR_AREA;

	while (DI[7] & 0b001);

	if (id->streaming) {
		AI[1] = 0;

		DI[2] = 0xE1010000;
		DI[7] = 0b001;
		while (DI[7] & 0b001);

		AI[0] &= ~0b0000001;
	}

	#ifdef GCODE
	const frag_t *frag = NULL;
	int count = frag_get_list(FRAGS_DISC_1, &frag);
	gcode_set_disc_frags(0, frag, count);
	gcode_set_disc_number(0);
	#endif

	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	EXI[EXI_CHANNEL_0][0] = 0;
	EXI[EXI_CHANNEL_1][0] = 0;
	EXI[EXI_CHANNEL_2][0] = 0;

	end_read(-1);
}
