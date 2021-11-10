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
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	uint32_t requested;
	bool read, patch;
} dvd = {0};

OSAlarm read_alarm = {0};

enum {
	ASK_READY = 0x15,
	ASK_OPENPATH,
	ASK_GETENTRY,
	ASK_SERVEFILE,
	ASK_LOCKFILE,
	ANS_READY = 0x25,
};

typedef struct {
	uint32_t offset;
	uint32_t size;
} __attribute((packed, scalar_storage_order("little-endian"))) usb_request_t;

#include "usbgecko_low.c"

static void usb_request(uint32_t offset, uint32_t size)
{
	usb_request_t request = {offset, size};
	usb_transmit(&request, sizeof(request), sizeof(request));
}

static bool usb_serve_file(void)
{
	uint8_t val = ASK_SERVEFILE;
	const frag_t *frag = NULL;

	if (frag_get_list(*VAR_CURRENT_DISC, &frag)) {
		usb_transmit(&val, 1, 1);
		usb_receive(&val, 1, 1);

		if (val == ANS_READY) {
			usb_transmit(frag->path, frag->pathlen, frag->pathlen);
			return true;
		}
	}

	return false;
}

static bool usb_lock_file(void)
{
	uint8_t val = ASK_LOCKFILE;

	usb_transmit(&val, 1, 1);
	usb_receive(&val, 1, 1);

	return val == ANS_READY;
}

static void usb_unlock_file(void)
{
	usb_receive(dvd.buffer, dvd.requested, dvd.requested);
	dvd.requested = 0;
	usb_request(0, 0);
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, frag_callback callback)
{
	if (dvd.read && buffer == dvd.buffer) {
		usb_request(offset, length);
		dvd.requested += length;
		return true;
	}

	return false;
}

void schedule_read(OSTick ticks)
{
	#ifdef ASYNC_READ
	void read_callback(void *address, uint32_t length)
	{
		DCStoreRangeNoSync(address, length);

		dvd.buffer += length;
		dvd.length -= length;
		dvd.offset += length;
		dvd.read = !!dvd.length;

		schedule_read(0);
	}
	#endif

	OSCancelAlarm(&read_alarm);

	if (!dvd.read) {
		di_complete_transfer();
		return;
	}

	#ifdef ASYNC_READ
	if (!dvd.requested)
		frag_read_async(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, read_callback);
	if (dvd.requested)
		OSSetAlarm(&read_alarm, ticks, trickle_read);
	#else
	if (!dvd.requested)
		dvd.patch = frag_read_patch(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset, NULL);

	OSSetAlarm(&read_alarm, ticks, trickle_read);
	#endif
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	if ((*VAR_IGR_TYPE & 0x80) && offset == 0x2440) {
		*VAR_CURRENT_DISC = FRAGS_APPLOADER;
		*VAR_SECOND_DISC = 0;
	}

	dvd.buffer = OSPhysicalToCached(address);
	dvd.length = length;
	dvd.offset = offset;
	dvd.read = true;

	schedule_read(0);
}

void trickle_read()
{
	if (dvd.read) {
		#ifdef ASYNC_READ
		if (dvd.requested) {
		#else
		if (dvd.patch) {
			OSTick start = OSGetTick();
			int size = frag_read(*VAR_CURRENT_DISC, dvd.buffer, dvd.length, dvd.offset);
			DCStoreRangeNoSync(dvd.buffer, size);
			OSTick end = OSGetTick();

			dvd.buffer += size;
			dvd.length -= size;
			dvd.offset += size;
			dvd.read = !!dvd.length;

			schedule_read(OSDiffTick(end, start));
		} else {
		#endif
			OSTick start = OSGetTick();
			int size = usb_receive(dvd.buffer, dvd.requested, 0);
			DCStoreRangeNoSync(dvd.buffer, size);
			OSTick end = OSGetTick();

			dvd.buffer += size;
			dvd.length -= size;
			dvd.offset += size;
			dvd.requested -= size;
			dvd.read = !!dvd.length;

			schedule_read(OSDiffTick(end, start));
		}
	}
}

bool change_disc(void)
{
	if (*VAR_SECOND_DISC) {
		usb_unlock_file();
		*VAR_CURRENT_DISC ^= 1;
		return usb_serve_file() && usb_lock_file();
	}

	return false;
}

void reset_device(void)
{
	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	usb_unlock_file();
	end_read();
}
