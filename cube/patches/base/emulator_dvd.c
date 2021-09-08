/* 
 * Copyright (c) 2008, Dolphin Emulator Project
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
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
#include <math.h>
#include "common.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "DVDMath.h"
#include "emulator.h"

#define DVD_SECTOR_SIZE    2048
#define DVD_ECC_BLOCK_SIZE (16 * DVD_SECTOR_SIZE)

#define BUFFER_TRANSFER_RATE (13.5 * 1000 * 1000)

static struct {
	union {
		struct {
			uint32_t start_offset;
			uint32_t end_offset;
		};

		struct {
			uint32_t start_block        : 17;
			uint32_t start_block_offset : 15;
			uint32_t end_block          : 17;
			uint32_t end_block_offset   : 15;
		};
	};

	OSTime start_time;
	OSTime end_time;
} read_buffer = {0};

static uint32_t dvd_buffer_size(void)
{
	DVDDiskID *id = (DVDDiskID *)VAR_AREA;
	return (*VAR_EMU_READ_SPEED == 1 ? id->streaming ? 5 : 15 : 32) * DVD_ECC_BLOCK_SIZE;
}

__attribute((noinline))
void dvd_schedule_read(uint32_t offset, uint32_t length, OSAlarmHandler handler)
{
	OSTime current_time = OSGetTime();

	uint32_t head_position;

	uint32_t buffer_start, buffer_end;
	uint32_t buffer_size = dvd_buffer_size();

	uint32_t dvd_offset = DVDRoundDown32KB(offset);

	if (read_buffer.start_time == read_buffer.end_time) {
		buffer_start = buffer_end = head_position = 0;
	} else {
		if (current_time >= read_buffer.end_time) {
			head_position = read_buffer.end_offset;
		} else {
			head_position = read_buffer.start_offset +
				OSDiffTick(current_time, read_buffer.start_time) *
				(read_buffer.end_block - read_buffer.start_block) /
				OSDiffTick(read_buffer.end_time, read_buffer.start_time) *
				DVD_ECC_BLOCK_SIZE;
		}

		buffer_start = read_buffer.end_offset >= buffer_size ? read_buffer.end_offset - buffer_size : 0;

		if (dvd_offset < buffer_start)
			buffer_start = buffer_end = 0;
		else buffer_end = head_position;
	}

	OSTick ticks_until_execution = 0;
	OSTick ticks_until_completion = COMMAND_LATENCY_TICKS;

	do {
		OSTick ticks = 0;

		uint32_t chunk_length = MIN(length, DVD_ECC_BLOCK_SIZE - offset % DVD_ECC_BLOCK_SIZE);

		if (dvd_offset >= buffer_start && dvd_offset < buffer_end) {
			ticks += COMMAND_LATENCY_TICKS;
			ticks += OSSecondsToTicks(chunk_length / BUFFER_TRANSFER_RATE);
		} else {
			if (dvd_offset != head_position) {
				ticks += OSSecondsToTicks(CalculateSeekTime(head_position, dvd_offset));
				ticks += OSSecondsToTicks(CalculateRotationalLatency(dvd_offset,
					OSTicksToSeconds((double)(current_time + ticks_until_completion + ticks))));

				#if READ_SPEED_TIER == 2
				ticks_until_execution += ticks;
				#endif
			} else {
				ticks += OSSecondsToTicks(CalculateRawDiscReadTime(dvd_offset, DVD_ECC_BLOCK_SIZE));
			}

			#if READ_SPEED_TIER == 1
			ticks_until_execution += ticks;
			#endif
			head_position = dvd_offset + DVD_ECC_BLOCK_SIZE;
		}

		#if READ_SPEED_TIER == 0
		ticks_until_execution += ticks;
		#endif
		ticks_until_completion += ticks;

		offset += chunk_length;
		length -= chunk_length;
		dvd_offset += DVD_ECC_BLOCK_SIZE;
	} while (length);

	if (dvd_offset != buffer_start + DVD_ECC_BLOCK_SIZE || buffer_start == buffer_end) {
		read_buffer.start_offset = dvd_offset >= buffer_end ? dvd_offset : buffer_end;
		read_buffer.end_offset = dvd_offset + buffer_size - DVD_ECC_BLOCK_SIZE;

		read_buffer.start_time = current_time + ticks_until_completion;
		read_buffer.end_time = read_buffer.start_time +
			(OSTick)OSSecondsToTicks(CalculateRawDiscReadTime(read_buffer.start_offset,
			read_buffer.end_offset - read_buffer.start_offset));
	}

	OSSetAlarm(&read_alarm, ticks_until_execution, handler);
}

__attribute((always_inline))
double fmod(double x, double y)
{
	return x - floor(x / y) * y;
}
