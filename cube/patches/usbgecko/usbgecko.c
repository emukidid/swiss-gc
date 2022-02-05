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
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 2
#endif

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

extern struct {
	int requested;
	intptr_t buffer;
	intptr_t registers;
} _usb;

static struct {
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} usb = {0};

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, patch;
} dvd = {0};

static OSInterruptHandler TCIntrruptHandler = NULL;
static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context);

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
	usb_receive(OSPhysicalToCached(_usb.buffer), _usb.requested, _usb.requested);
	_usb.requested = 0;
	usb_request(0, 0);
}

static void usb_read_queued(void)
{
	if (!EXILock(EXI_CHANNEL_1, EXI_DEVICE_0, (EXICallback)usb_read_queued))
		return;

	void *buffer = usb.queued->buffer;
	uint32_t length = usb.queued->length;
	uint32_t offset = usb.queued->offset;

	_usb.buffer = OSCachedToPhysical(buffer);
	_usb.requested = length;
	usb_request(offset, length);

	exi_select();
	exi_clear_interrupts(0, 1, 0);
	exi_imm_read_write(0xD << 28, 1, 0);

	TCIntrruptHandler = OSSetInterruptHandler(OS_INTERRUPT_EXI_1_TC, tc_interrupt_handler);
	OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_1_TC);
}

static void usb_done_queued(void)
{
	void *buffer = usb.queued->buffer;
	uint32_t length = usb.queued->length;
	uint32_t offset = usb.queued->offset;

	usb.queued->callback(buffer, length);

	EXIUnlock(EXI_CHANNEL_1);

	usb.queued->callback = NULL;
	usb.queued = NULL;

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (usb.queue[i].callback != NULL) {
			usb.queued = &usb.queue[i];
			usb_read_queued();
			return;
		}
	}
}

static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	if (_usb.requested)
		return;

	OSMaskInterrupts(OS_INTERRUPTMASK_EXI_1_TC);
	OSSetInterruptHandler(OS_INTERRUPT_EXI_1_TC, TCIntrruptHandler);
	exi_deselect();

	usb_done_queued();
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	if (!length) {
		callback(buffer, length);
		return true;
	}

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (usb.queue[i].callback == NULL) {
			usb.queue[i].buffer = buffer;
			usb.queue[i].length = length;
			usb.queue[i].offset = offset;
			usb.queue[i].callback = callback;

			if (usb.queued == NULL) {
				usb.queued = &usb.queue[i];
				usb_read_queued();
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
		DCStoreRangeNoSync(address, length);

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

	dvd.buffer = OSPhysicalToCached(address);
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
		DCStoreRangeNoSync(dvd.buffer, size);
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
