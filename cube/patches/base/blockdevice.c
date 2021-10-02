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
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read;
} dvd = {0};

OSAlarm read_alarm = {0};

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, frag_callback callback)
{
	return do_read_write_async(buffer, length, offset, sector, false, callback);
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
	frag_read_async(dvd.buffer, dvd.length, dvd.offset, read_callback);
	#else
	OSSetAlarm(&read_alarm, ticks, (OSAlarmHandler)trickle_read);
	#endif
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	dvd.buffer = OSPhysicalToUncached(address);
	dvd.length = length;
	dvd.offset = offset | *VAR_CURRENT_DISC << 31;
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

void trickle_read(void)
{
	#ifndef ASYNC_READ
	#ifdef DTK
	if (dtk_fill_buffer())
		return;
	#endif

	if (dvd.read) {
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
