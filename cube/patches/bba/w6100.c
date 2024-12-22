/* 
 * Copyright (c) 2024, Extrems <extrems@extremscorner.org>
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

#include <ppu_intrinsics.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bba.h"
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator_eth.h"
#include "interrupt.h"
#include "w6100.h"

#define exi_cpr       (*VAR_EXI2_CPR)
#define exi_channel   ((*VAR_EXI_SLOT & 0x30) >> 4)
#define exi_device    ((*VAR_EXI_SLOT & 0xC0) >> 6)
#define exi_interrupt ((*VAR_EXI2_CPR & 0xC0) >> 6)
#define exi_regs      (*(volatile uint32_t **)VAR_EXI2_REGS)

static struct {
	bool sendok;
	bool interrupt;
	uint16_t wr;
	uint16_t rd;
	bba_page_t (*page)[8];
	struct {
		void *data;
		size_t size;
		bba_callback callback;
	} output;
} w6100;

static void exi_clear_interrupts(int32_t chan, bool exi, bool tc, bool ext)
{
	EXI[chan][0] = (EXI[chan][0] & (0x3FFF & ~0x80A)) | (ext << 11) | (tc << 3) | (exi << 1);
}

static void exi_select(void)
{
	exi_regs[0] = (exi_regs[0] & 0x405) | ((exi_cpr << 4) & 0x3F0);
}

static void exi_deselect(void)
{
	exi_regs[0] &= 0x405;
}

static void exi_imm_write(uint32_t data, uint32_t len)
{
	exi_regs[4] = data;
	exi_regs[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 0b01;
	while (exi_regs[3] & 0b01);
}

static uint32_t exi_imm_read(uint32_t len)
{
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ << 2) | 0b01;
	while (exi_regs[3] & 0b01);
	return exi_regs[4] >> ((4 - len) * 8);
}

static uint32_t exi_imm_read_write(uint32_t data, uint32_t len)
{
	exi_regs[4] = data;
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 0b01;
	while (exi_regs[3] & 0b01);
	return exi_regs[4] >> ((4 - len) * 8);
}

static void exi_dma_write(const void *buf, uint32_t len, bool sync)
{
	exi_regs[1] = (uint32_t)buf;
	exi_regs[2] = OSRoundUp32B(len);
	exi_regs[3] = (EXI_WRITE << 2) | 0b11;
	while (sync && (exi_regs[3] & 0b01));
}

static void exi_dma_read(void *buf, uint32_t len, bool sync)
{
	exi_regs[1] = (uint32_t)buf;
	exi_regs[2] = OSRoundUp32B(len);
	exi_regs[3] = (EXI_READ << 2) | 0b11;
	while (sync && (exi_regs[3] & 0b01));
}

static void w6100_read_cmd(uint32_t cmd, void *buf, uint32_t len)
{
	cmd &= ~W6100_RWB;
	cmd  = (cmd << 16) | (cmd >> 16);

	exi_select();
	exi_imm_write(cmd, 3);
	exi_dma_read(buf, len, true);
	exi_deselect();
}

static void w6100_write_cmd(uint32_t cmd, const void *buf, uint32_t len)
{
	cmd |= W6100_RWB;
	cmd  = (cmd << 16) | (cmd >> 16);

	exi_select();
	exi_imm_write(cmd, 3);
	exi_dma_write(buf, len, true);
	exi_deselect();
}

static uint8_t w6100_read_reg8(W6100Reg addr)
{
	uint8_t data;
	uint32_t cmd = W6100_OM(1) | addr;

	cmd = (cmd << 16) | (cmd >> 16);

	exi_select();
	data = exi_imm_read_write(cmd, 4);
	exi_deselect();

	return data;
}

static void w6100_write_reg8(W6100Reg addr, uint8_t data)
{
	uint32_t cmd = W6100_RWB | W6100_OM(1) | addr;

	cmd = (cmd << 16) | (cmd >> 16);

	exi_select();
	exi_imm_read_write(cmd | data, 4);
	exi_deselect();
}

static uint16_t w6100_read_reg16(W6100Reg16 addr)
{
	uint16_t data;
	uint32_t cmd = W6100_OM(2) | addr;

	cmd = (cmd << 16) | (cmd >> 16);

	exi_select();
	exi_imm_write(cmd, 3);
	data = exi_imm_read(2);
	exi_deselect();

	return data;
}

static void w6100_write_reg16(W6100Reg16 addr, uint16_t data)
{
	uint32_t cmd = W6100_RWB | W6100_OM(2) | addr;

	cmd = (cmd << 16) | (cmd >> 16);

	exi_select();
	exi_imm_write(cmd, 3);
	exi_imm_write(data << 16, 2);
	exi_deselect();
}

static void w6100_interrupt(void)
{
	w6100_write_reg8(W6100_SIMR, 0);
	uint8_t ir = w6100_read_reg8(W6100_S0_IR);

	if (ir & W6100_Sn_IR_SENDOK) {
		w6100_write_reg8(W6100_S0_IRCLR, W6100_Sn_IRCLR_SENDOK);
		w6100.sendok = true;
	}

	if (ir & W6100_Sn_IR_RECV) {
		w6100_write_reg8(W6100_S0_IRCLR, W6100_Sn_IRCLR_RECV);

		uint16_t rd = w6100.rd;
		uint16_t rs = w6100_read_reg16(W6100_RXBUF_S(0, rd));
		w6100.rd = rd + rs;

		rd += sizeof(rs);
		rs -= sizeof(rs);

		DCInvalidateRange(__builtin_assume_aligned(w6100.page, 32), rs);
		w6100_read_cmd(W6100_RXBUF_S(0, rd), w6100.page, rs);
		eth_mac_receive(w6100.page, rs);

		w6100_write_reg16(W6100_S0_RX_RD, w6100.rd);
		w6100_write_reg8(W6100_S0_CR, W6100_Sn_CR_RECV);
	}

	w6100_write_reg8(W6100_SIMR, W6100_SIMR_S(0));
}

static void exi_callback()
{
	if (EXILock(exi_channel, exi_device, exi_callback)) {
		if (w6100.interrupt) {
			w6100_interrupt();
			w6100.interrupt = false;
		}

		if (w6100.output.callback && w6100.sendok) {
			bba_output(w6100.output.data, w6100.output.size);
			w6100.output.callback();
			w6100.output.callback = NULL;
		}

		EXIUnlock(exi_channel);
	}
}

static void exi_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	int32_t chan = (interrupt - OS_INTERRUPT_EXI_0_EXI) / 3;
	exi_clear_interrupts(chan, true, false, false);
	w6100.interrupt = true;
	exi_callback();
}

static void debug_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	PI[0] = 1 << 12;
	w6100.interrupt = true;
	exi_callback();
}

void bba_output(const void *data, size_t size)
{
	DCStoreRange(__builtin_assume_aligned(data, 32), size);
	w6100_write_cmd(W6100_TXBUF_S(0, w6100.wr), data, size);
	w6100.wr += size;

	w6100_write_reg16(W6100_S0_TX_WR, w6100.wr);
	w6100_write_reg8(W6100_S0_CR, W6100_Sn_CR_SEND);
	w6100.sendok = false;
}

void bba_output_async(const void *data, size_t size, bba_callback callback)
{
	w6100.output.data = (void *)data;
	w6100.output.size = size;
	w6100.output.callback = callback;

	exi_callback();
}

void bba_init(void **arenaLo, void **arenaHi)
{
	w6100.sendok = w6100_read_reg16(W6100_S0_TX_FSR) == W6100_TX_BUFSIZE;

	w6100.wr = w6100_read_reg16(W6100_S0_TX_WR);
	w6100.rd = w6100_read_reg16(W6100_S0_RX_RD);

	*arenaHi -= sizeof(*w6100.page); w6100.page = *arenaHi;

	if (exi_interrupt < EXI_CHANNEL_MAX) {
		OSInterrupt interrupt = OS_INTERRUPT_EXI_0_EXI + (3 * exi_interrupt);
		set_interrupt_handler(interrupt, exi_interrupt_handler);
		unmask_interrupts(OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI));
	} else {
		set_interrupt_handler(OS_INTERRUPT_PI_DEBUG, debug_interrupt_handler);
		unmask_interrupts(OS_INTERRUPTMASK_PI_DEBUG);
	}
}
