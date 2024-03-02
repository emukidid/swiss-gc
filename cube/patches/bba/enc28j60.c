/* 
 * Copyright (c) 2023-2024, Extrems <extrems@extremscorner.org>
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
#include "enc28j60.h"
#include "interrupt.h"

static struct {
	uint8_t bank;
	bba_page_t (*page)[8];
} _bba;

static void exi_select(void)
{
	EXI[EXI_CHANNEL_2][0] = (EXI[EXI_CHANNEL_2][0] & 0x405) | ((1 << EXI_DEVICE_0) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	EXI[EXI_CHANNEL_2][0] &= 0x405;
}

static uint32_t exi_imm_read(uint32_t len)
{
	EXI[EXI_CHANNEL_2][3] = ((len - 1) << 4) | (EXI_READ << 2) | 0b01;
	while (EXI[EXI_CHANNEL_2][3] & 0b01);
	return EXI[EXI_CHANNEL_2][4] >> ((4 - len) * 8);
}

static uint32_t exi_imm_read_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_2][4] = data;
	EXI[EXI_CHANNEL_2][3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_2][3] & 0b01);
	return EXI[EXI_CHANNEL_2][4] >> ((4 - len) * 8);
}

static void exi_dma_write(const void *buf, uint32_t len, bool sync)
{
	EXI[EXI_CHANNEL_2][1] = (uint32_t)buf;
	EXI[EXI_CHANNEL_2][2] = OSRoundUp32B(len);
	EXI[EXI_CHANNEL_2][3] = (EXI_WRITE << 2) | 0b11;
	while (sync && (EXI[EXI_CHANNEL_2][3] & 0b01));
}

static void exi_dma_read(void *buf, uint32_t len, bool sync)
{
	EXI[EXI_CHANNEL_2][1] = (uint32_t)buf;
	EXI[EXI_CHANNEL_2][2] = OSRoundUp32B(len);
	EXI[EXI_CHANNEL_2][3] = (EXI_READ << 2) | 0b11;
	while (sync && (EXI[EXI_CHANNEL_2][3] & 0b01));
}

static uint8_t enc28j60_read_cmd(uint32_t cmd)
{
	uint8_t data;

	exi_select();
	data = exi_imm_read_write(cmd, 2 + ENC28J60_EXI_DUMMY(cmd));
	exi_deselect();

	return data;
}

static void enc28j60_write_cmd(uint32_t cmd, uint8_t data)
{
	exi_select();
	exi_imm_read_write(cmd | data << 16, 2);
	exi_deselect();
}

static void enc28j60_set_bits(ENC28J60Reg addr, uint8_t data)
{
	enc28j60_write_cmd(ENC28J60_CMD_BFS(addr), data);
}

static void enc28j60_clear_bits(ENC28J60Reg addr, uint8_t data)
{
	enc28j60_write_cmd(ENC28J60_CMD_BFC(addr), data);
}

static void enc28j60_select_bank(uint8_t addr)
{
	uint8_t bank = addr >> 5;

	if (_bba.bank != bank && bank < 4) {
		enc28j60_clear_bits(ENC28J60_ECON1, ENC28J60_ECON1_BSEL(_bba.bank));
		enc28j60_set_bits(ENC28J60_ECON1, ENC28J60_ECON1_BSEL(bank));

		_bba.bank = bank;
	}
}

static uint8_t enc28j60_read_reg8(ENC28J60Reg addr)
{
	enc28j60_select_bank(addr);
	return enc28j60_read_cmd(ENC28J60_CMD_RCR(addr));
}

static void enc28j60_write_reg16(ENC28J60Reg16 addr, uint16_t data)
{
	enc28j60_select_bank(addr);
	enc28j60_write_cmd(ENC28J60_CMD_WCR(addr + 0), data);
	enc28j60_write_cmd(ENC28J60_CMD_WCR(addr + 1), data >> 8);
}

static void enc28j60_interrupt(void)
{
	enc28j60_clear_bits(ENC28J60_EIE, ENC28J60_EIE_INTIE);
	uint8_t eie = enc28j60_read_reg8(ENC28J60_EIE);
	uint8_t eir = enc28j60_read_reg8(ENC28J60_EIR);

	if (eir == 0) {
		uint8_t epktcnt = enc28j60_read_reg8(ENC28J60_EPKTCNT);
		if (epktcnt > 0) eir |= ENC28J60_EIR_PKTIF;
	}

	eir &= eie;

	if (eir & ENC28J60_EIR_PKTIF) {
		exi_select();
		exi_imm_read_write(ENC28J60_CMD_RBM, 1);

		uint16_t rsv[3];
		for (int i = 0; i < sizeof(rsv) / sizeof(uint16_t); i++)
			__sthbrx(rsv + i, exi_imm_read(sizeof(uint16_t)));
		uint16_t next_packet = rsv[0];
		uint16_t byte_count = rsv[1];
		//uint16_t status = rsv[2];

		DCInvalidateRange(__builtin_assume_aligned(_bba.page, 32), byte_count);
		exi_dma_read(_bba.page, byte_count, true);
		exi_deselect();

		eth_mac_receive(_bba.page, byte_count - 4);

		enc28j60_write_reg16(ENC28J60_ERDPT, next_packet);
		enc28j60_write_reg16(ENC28J60_ERXRDPT, next_packet == ENC28J60_INIT_ERXST ? ENC28J60_INIT_ERXND : next_packet - 1);

		enc28j60_set_bits(ENC28J60_ECON2, ENC28J60_ECON2_PKTDEC);
	}

	enc28j60_clear_bits(ENC28J60_EIR, eir);
	enc28j60_set_bits(ENC28J60_EIE, ENC28J60_EIE_INTIE);
}

static void exi_callback()
{
	if (EXILock(EXI_CHANNEL_2, EXI_DEVICE_0, exi_callback)) {
		enc28j60_interrupt();
		EXIUnlock(EXI_CHANNEL_2);
	}
}

static void debug_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	PI[0] = 1 << 12;
	exi_callback();
}

void bba_transmit_fifo(const void *data, size_t size)
{
	enc28j60_set_bits(ENC28J60_ECON1, ENC28J60_ECON1_TXRST);
	enc28j60_clear_bits(ENC28J60_ECON1, ENC28J60_ECON1_TXRST);

	enc28j60_write_reg16(ENC28J60_EWRPT, ENC28J60_INIT_ETXST);
	enc28j60_write_reg16(ENC28J60_ETXND, ENC28J60_INIT_ETXST + size);

	exi_select();
	exi_imm_read_write(ENC28J60_CMD_WBM, 2);

	DCStoreRange(__builtin_assume_aligned(data, 32), size);
	exi_dma_write(data, size, true);
	exi_deselect();

	enc28j60_set_bits(ENC28J60_ECON1, ENC28J60_ECON1_TXRTS);
}

void bba_init(void **arenaLo, void **arenaHi)
{
	*arenaHi -= sizeof(*_bba.page);  _bba.page  = *arenaHi;

	set_interrupt_handler(OS_INTERRUPT_PI_DEBUG, debug_interrupt_handler);
	unmask_interrupts(OS_INTERRUPTMASK_PI_DEBUG);
}
