/* 
 * Copyright (c) 2017-2024, Extrems <extrems@extremscorner.org>
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
#include <string.h>
#include "bba.h"
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "emulator_eth.h"
#include "frag.h"
#include "interrupt.h"
#include "ipl.h"

static struct {
	bool locked;
	uint8_t rwp;
	uint8_t rrp;
	bba_page_t (*page)[8];
	ppc_context_t *entry;
	ppc_context_t *exit;
	void (*callback)(void);
} _bba;

static struct {
	void *buffer;
	uint32_t length;
	uint32_t offset;
	bool read, patch;
} dvd;

static void exi_callback();

#include "tcpip.c"

static void exi_clear_interrupts(int32_t chan, bool exi, bool tc, bool ext)
{
	EXI[chan][0] = (EXI[chan][0] & (0x3FFF & ~0x80A)) | (ext << 11) | (tc << 3) | (exi << 1);
}

static void exi_select(void)
{
	EXI[EXI_CHANNEL_0][0] = (EXI[EXI_CHANNEL_0][0] & 0x405) | ((1 << EXI_DEVICE_2) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	EXI[EXI_CHANNEL_0][0] &= 0x405;
}

static void exi_imm_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_0][4] = data;
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
}

static uint32_t exi_imm_read(uint32_t len)
{
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_READ << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
	return EXI[EXI_CHANNEL_0][4] >> ((4 - len) * 8);
}

static uint32_t exi_imm_read_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_0][4] = data;
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
	return EXI[EXI_CHANNEL_0][4] >> ((4 - len) * 8);
}

static void exi_immex_write(const uint8_t *buf __attribute((vector_size(4))), uint32_t len)
{
	uint32_t xlen;

	do {
		xlen = MIN(len, 4);
		exi_imm_write((uint32_t)*buf++, xlen);
	} while (len -= xlen);
}

static void exi_dma_write(const void *buf, uint32_t len, bool sync)
{
	EXI[EXI_CHANNEL_0][1] = (uint32_t)buf;
	EXI[EXI_CHANNEL_0][2] = OSRoundUp32B(len);
	EXI[EXI_CHANNEL_0][3] = (EXI_WRITE << 2) | 0b11;
	while (sync && (EXI[EXI_CHANNEL_0][3] & 0b01));
}

static void exi_dma_read(void *buf, uint32_t len, bool sync)
{
	EXI[EXI_CHANNEL_0][1] = (uint32_t)buf;
	EXI[EXI_CHANNEL_0][2] = OSRoundUp32B(len);
	EXI[EXI_CHANNEL_0][3] = (EXI_READ << 2) | 0b11;
	while (sync && (EXI[EXI_CHANNEL_0][3] & 0b01));
}

uint8_t bba_cmd_in8(uint8_t reg)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write((reg & 0x3F) << 24, 4);
	exi_deselect();

	return val;
}

void bba_cmd_out8(uint8_t reg, uint8_t val)
{
	exi_select();
	exi_imm_read_write(0x40 << 24 | (reg & 0x3F) << 24 | val, 4);
	exi_deselect();
}

uint8_t bba_in8(uint16_t reg)
{
	uint8_t val;

	exi_select();
	exi_imm_read_write(0x80 << 24 | reg << 8, 4);
	val = exi_imm_read(1);
	exi_deselect();

	return val;
}

void bba_out8(uint16_t reg, uint8_t val)
{
	exi_select();
	exi_imm_read_write(0xC0 << 24 | reg << 8, 4);
	exi_imm_write(val << 24, 1);
	exi_deselect();
}

void bba_ins(uint16_t reg, void *val, uint32_t len)
{
	exi_select();
	exi_imm_read_write(0x80 << 24 | reg << 8, 4);
	exi_clear_interrupts(EXI_CHANNEL_0, false, true, false);
	exi_dma_read(val, len, false);

	_bba.locked = false;
	if (!setjmp(_bba.entry))
		longjmp(_bba.exit, 2);
	_bba.locked = true;

	exi_deselect();
}

void bba_outs(uint16_t reg, const void *val, uint32_t len)
{
	exi_select();
	exi_imm_read_write(0xC0 << 24 | reg << 8, 4);
	#ifdef ETH_EMULATOR
	if (!_bba.locked) {
		exi_immex_write(val, len);
		exi_deselect();
		return;
	}
	#endif

	exi_clear_interrupts(EXI_CHANNEL_0, false, true, false);
	exi_dma_write(val, len, false);

	_bba.locked = false;
	if (!setjmp(_bba.entry))
		longjmp(_bba.exit, 2);
	_bba.locked = true;

	exi_deselect();
}

void bba_transmit_fifo(const void *data, size_t size)
{
	if (!size) return;
	DCStoreRange(__builtin_assume_aligned(data, 32), size);

	while (bba_in8(BBA_NCRA) & (BBA_NCRA_ST0 | BBA_NCRA_ST1));
	bba_outs(BBA_WRTXFIFOD, data, size);
	bba_out8(BBA_NCRA, (bba_in8(BBA_NCRA) & ~BBA_NCRA_ST0) | BBA_NCRA_ST1);
}

void bba_receive_dma(void *data, size_t size, uint8_t offset)
{
	if (!size) return;
	bba_ins(_bba.rrp << 8 | offset, data, size);
}

static void bba_receive(void)
{
	_bba.rwp = bba_in8(BBA_RWP);
	_bba.rrp = bba_in8(BBA_RRP);

	while (_bba.rrp != _bba.rwp) {
		bba_page_t *page = *_bba.page;
		bba_header_t *bba = (bba_header_t *)page;
		size_t size = sizeof(bba_header_t) + MIN_FRAME_SIZE;

		DCInvalidateRange(__builtin_assume_aligned(bba, 32), size);
		bba_receive_dma(bba, size, 0);
		bba_out8(BBA_RRP, bba->next);

		size = bba->length - sizeof(*bba);
		#ifdef ETH_EMULATOR
		if (!eth_input(page, (void *)bba->data, size)) {
			DCInvalidateRange(__builtin_assume_aligned(bba->data + MIN_FRAME_SIZE, 32), size - MIN_FRAME_SIZE);
			bba_receive_dma(bba->data + MIN_FRAME_SIZE, size - MIN_FRAME_SIZE, sizeof(*bba) + MIN_FRAME_SIZE);
			eth_mac_receive(bba->data, size);
		}
		#else
		eth_input(page, (void *)bba->data, size);
		#endif

		_bba.rwp = bba_in8(BBA_RWP);
		_bba.rrp = bba->next;
	}
}

static void bba_interrupt(void)
{
	uint8_t ir = bba_in8(BBA_IR);

	if (ir & BBA_IR_RI) bba_receive();

	bba_out8(BBA_IR, ir);
}

static void exi_callback()
{
	switch (setjmp(_bba.exit)) {
		case 0:
			longjmp(_bba.entry, 1);
			break;
		case 1:
			EXIUnlock(EXI_CHANNEL_0);
			break;
	}
}

static void exi_coroutine()
{
	if (EXILock(EXI_CHANNEL_0, EXI_DEVICE_2, exi_callback)) {
		_bba.locked = true;

		set_interrupt_handler(OS_INTERRUPT_EXI_0_TC, exi_callback);
		unmask_interrupts(OS_INTERRUPTMASK_EXI_0_TC);

		if (_bba.callback) {
			_bba.callback();
			_bba.callback = NULL;
		}

		uint8_t status = bba_cmd_in8(0x03);
		bba_cmd_out8(0x02, BBA_CMD_IRMASKALL);

		if (status & 0x80) bba_interrupt();

		bba_cmd_out8(0x03, status);
		bba_cmd_out8(0x02, BBA_CMD_IRMASKNONE);

		mask_interrupts(OS_INTERRUPTMASK_EXI_0_TC);

		_bba.locked = false;
		if (!setjmp(_bba.entry))
			longjmp(_bba.exit, 1);
	} else {
		if (!setjmp(_bba.entry))
			longjmp(_bba.exit, 2);
	}
}

static void exi_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	exi_clear_interrupts(EXI_CHANNEL_2, true, false, false);
	exi_callback();
}

void bba_init(void **arenaLo, void **arenaHi)
{
	*arenaHi -= sizeof(*_bba.exit);  _bba.exit  = *arenaHi;
	*arenaHi -= sizeof(*_bba.entry); _bba.entry = *arenaHi;

	void **sp = *arenaHi; sp[-2] = sp;
	_bba.entry->gpr[1] = (intptr_t)sp - 8;
	_bba.entry->lr = (intptr_t)exi_coroutine;

	*arenaHi -= sizeof(*_bba.page);  _bba.page  = *arenaHi;

	set_interrupt_handler(OS_INTERRUPT_EXI_2_EXI, exi_interrupt_handler);
	unmask_interrupts(OS_INTERRUPTMASK_EXI_2_EXI);
}

void schedule_read(OSTick ticks)
{
	void read_callback(void *address, uint32_t length)
	{
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

	dvd.buffer = OSPhysicalToUncached(address);
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
		*VAR_CURRENT_DISC ^= 1;
		return true;
	}

	return false;
}

void reset_devices(void)
{
	while (EXI[EXI_CHANNEL_0][3] & 0b000001);
	while (EXI[EXI_CHANNEL_1][3] & 0b000001);
	while (EXI[EXI_CHANNEL_2][3] & 0b000001);

	reset_device();
	ipl_set_config(0);
}
