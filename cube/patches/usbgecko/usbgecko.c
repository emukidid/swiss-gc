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
#include "../base/common.h"
#include "../base/dolphin/exi.h"
#include "../base/dolphin/os.h"
#include "../base/emulator.h"

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool frag;
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

static void exi_select(void)
{
	EXI[EXI_CHANNEL_1][0] = (EXI[EXI_CHANNEL_1][0] & 0x405) | ((1 << EXI_DEVICE_0) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	EXI[EXI_CHANNEL_1][0] &= 0x405;
}

static uint32_t exi_imm_read_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_1][4] = data;
	EXI[EXI_CHANNEL_1][3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_1][3] & 0b01);
	return EXI[EXI_CHANNEL_1][4] >> ((4 - len) * 8);
}

static bool usb_receive_byte(uint8_t *data)
{
	uint16_t val;

	exi_select();
	val = exi_imm_read_write(0xA << 28, 2); *data = val;
	exi_deselect();

	return !(val & 0x800);
}

static bool usb_transmit_byte(const uint8_t *data)
{
	uint16_t val;

	exi_select();
	val = exi_imm_read_write(0xB << 28 | *data << 20, 2);
	exi_deselect();

	return !(val & 0x400);
}

static bool usb_transmit_check(void)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write(0xC << 28, 1);
	exi_deselect();

	return !(val & 0x4);
}

static bool usb_receive_check(void)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write(0xD << 28, 1);
	exi_deselect();

	return !(val & 0x4);
}

static int usb_transmit(const void *data, int size, int minsize)
{
	int i = 0, j = 0, check = 1;

	while (i < size) {
		if ((check && usb_transmit_check()) ||
			(check = usb_transmit_byte(data + i))) {
			j = i % 128;
			if (i < minsize)
				continue;
			else break;
		}

		i++;
		check = i % 128 == j;
	}

	return i;
}

static int usb_receive(void *data, int size, int minsize)
{
	int i = 0, j = 0, check = 1;

	while (i < size) {
		if ((check && usb_receive_check()) ||
			(check = usb_receive_byte(data + i))) {
			j = i % 64;
			if (i < minsize)
				continue;
			else break;
		}

		i++;
		check = i % 64 == j;
	}

	return i;
}

static void usb_request(uint32_t offset, uint32_t size)
{
	usb_request_t request = {offset & ~0x80000000, size};
	usb_transmit(&request, sizeof(request), sizeof(request));
}

static bool usb_serve_file(void)
{
	uint8_t val = ASK_SERVEFILE;

	usb_transmit(&val, 1, 1);
	usb_receive(&val, 1, 1);

	if (val == ANS_READY) {
		const char *file = (const char *)VAR_DISC_1_FN;
		uint8_t filelen = *(uint8_t *)VAR_DISC_1_FNLEN;

		if (*(uint32_t *)VAR_CURRENT_DISC) {
			file    =  (const char *)VAR_DISC_2_FN;
			filelen = *(uint8_t *)VAR_DISC_2_FNLEN;
		}

		usb_transmit(file, 1024, 1024);
	}

	return val == ANS_READY;
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
	usb_request(0, 0);
}

bool exi_probe(int32_t chan)
{
	if (chan == EXI_CHANNEL_2)
		return false;
	if (chan == *VAR_EXI_SLOT)
		return false;

	return true;
}

bool exi_try_lock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	if (chan == *VAR_EXI_SLOT && dev == EXI_DEVICE_0)
		return false;

	return true;
}

void schedule_read(OSTick ticks, bool request)
{
	OSCancelAlarm(&read_alarm);

	if (!dvd.length) {
		di_complete_transfer();
		return;
	}

	dvd.frag = is_frag_read(dvd.offset, dvd.length);

	if (!dvd.frag && request)
		usb_request(dvd.offset, dvd.length);

	OSSetAlarm(&read_alarm, ticks, (OSAlarmHandler)trickle_read);
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	dvd.buffer = OSPhysicalToCached(address);
	dvd.length = length;
	dvd.offset = offset | *(uint32_t *)VAR_CURRENT_DISC << 31;

	schedule_read(OSMicrosecondsToTicks(300), true);
}

void trickle_read(void)
{
	if (dvd.length) {
		OSTick start = OSGetTick();
		int size = dvd.frag ? read_frag(dvd.buffer, dvd.length, dvd.offset)
		                    : usb_receive(dvd.buffer, dvd.length, 0);
		DCStoreRangeNoSync(dvd.buffer, size);
		OSTick end = OSGetTick();

		dvd.buffer += size;
		dvd.length -= size;
		dvd.offset += size;

		schedule_read(OSDiffTick(end, start), dvd.frag);
	}
}

void device_reset(void)
{
	usb_unlock_file();
	end_read();
}

void change_disc(void)
{
	usb_unlock_file();

	*(uint32_t *)VAR_CURRENT_DISC = !*(uint32_t *)VAR_CURRENT_DISC;

	if (usb_serve_file() && usb_lock_file()) {
		(*DI_EMU)[1] &= ~0b001;
		(*DI_EMU)[1] |=  0b100;
		di_update_interrupts();
	}
}
