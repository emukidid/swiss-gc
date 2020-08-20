/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
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
#include "../base/common.h"
#include "../base/dolphin/dvd.h"
#include "../base/dolphin/os.h"
#include "../base/emulator.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, frag;
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

void schedule_read(OSTick ticks)
{
	OSCancelAlarm(&read_alarm);

	if (!dvd.read) {
		dvd_read_diskid(NULL);
		return;
	}

	dvd.frag = is_frag_read(dvd.offset, dvd.length);

	if (!dvd.frag)
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

	if (*VAR_EMU_READ_SPEED)
		di_defer_transfer(dvd.offset, dvd.length);

	schedule_read(OSMicrosecondsToTicks(300));
}

void trickle_read(void)
{
	if (dvd.read && dvd.frag) {
		OSTick start = OSGetTick();
		int size = read_frag(dvd.buffer, dvd.length, dvd.offset);
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

	if (id->streaming) {
		AI[1] = 0;

		DI[2] = 0xE1010000;
		DI[7] = 0b001;
		while (DI[7] & 0b001);

		AI[0] &= ~0b0000001;
	}

	end_read();
}
