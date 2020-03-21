/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../alt/timer.h"
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/exi.h"
#include "../base/os.h"

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

bool exi_probe(int32_t chan)
{
	if (chan == EXI_CHANNEL_2)
		return false;
	if (chan == *VAR_EXI_SLOT)
		return false;
	return true;
}

bool exi_trylock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	if (chan == *VAR_EXI_SLOT && dev == EXI_DEVICE_0)
		return false;
	return true;
}

void schedule_read(void *address, uint32_t length, uint32_t offset, OSTick ticks)
{
	*(uint32_t *)VAR_LAST_OFFSET = offset;
	*(uint32_t *)VAR_TMP2 = length;
	*(uint8_t **)VAR_TMP1 = address;

	if (length) {
		if (!is_frag_read(offset, length))
			dvd_read(address, length, offset);
		else
			timer1_start(ticks);
		return;
	}

	dvd_read_diskid(NULL);
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	schedule_read(OSPhysicalToUncached(address), length, offset, 0);
}

void trickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		if (is_frag_read(position, remainder)) {
			OSTick start = OSGetTick();
			data_size = read_frag(data, remainder, position);
			OSTick end = OSGetTick();

			position  += data_size;
			remainder -= data_size;

			schedule_read(data + data_size, remainder, position, OSDiffTick(end, start));
		}
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
