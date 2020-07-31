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
#include <stdbool.h>
#include "common.h"
#include "dolphin/os.h"
#include "emulator.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
} dvd = {0};

OSAlarm read_alarm = {0};

void schedule_read(OSTick ticks)
{
	#ifdef ASYNC_READ
	void read_callback(void *address, uint32_t length)
	{
		dvd.buffer += length;
		dvd.length -= length;
		dvd.offset += length;

		schedule_read(OSMicrosecondsToTicks(300));
	}
	#else
	OSCancelAlarm(&read_alarm);
	#endif

	if (!dvd.length) {
		di_complete_transfer();
		return;
	}

	#ifdef ASYNC_READ
	read_disc_frag(dvd.buffer, dvd.length, dvd.offset, read_callback);
	#else
	OSSetAlarm(&read_alarm, ticks, (OSAlarmHandler)trickle_read);
	#endif
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	dvd.buffer = OSPhysicalToUncached(address);
	dvd.length = length;
	dvd.offset = offset | *VAR_CURRENT_DISC << 31;

	schedule_read(OSMicrosecondsToTicks(300));
}

void trickle_read(void)
{
	#ifndef ASYNC_READ
	if (dtk_fill_buffer())
		return;

	if (dvd.length) {
		OSTick start = OSGetTick();
		int size = read_frag(dvd.buffer, dvd.length, dvd.offset);
		OSTick end = OSGetTick();

		dvd.buffer += size;
		dvd.length -= size;
		dvd.offset += size;

		schedule_read(OSDiffTick(end, start));
	}
	#endif
}

void device_reset(void)
{
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
