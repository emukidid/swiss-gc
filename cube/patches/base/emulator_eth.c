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
#include <string.h>
#include "bba/bba.h"
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "emulator_eth.h"

static struct {
	struct {
		int position;
		uint8_t command;
		uint8_t imr;
		uint8_t isr;
	} exi;

	struct {
		uint16_t address;
		union {
			uint8_t (*data)[256 + 4096];
			uint8_t *regs;
		};

		int txfifocnt;
		uint8_t (*fifo)[BBA_TX_MAX_PACKET_SIZE];
	} mac;
} eth;

static void eth_update_interrupts(void)
{
	if (eth.mac.regs[BBA_IR])
		eth.exi.isr |= 0x80;

	if (eth.exi.isr & eth.exi.imr)
		exi_interrupt(EXI_CHANNEL_2);
}

void eth_mac_receive(const void *data, size_t size)
{
	if (!(eth.mac.regs[BBA_NCRA] & BBA_NCRA_SR))
		return;

	uint16_t bp = __lhbrx(&eth.mac.regs[BBA_BP]);
	uint16_t rwp = __lhbrx(&eth.mac.regs[BBA_RWP]);
	uint16_t rrp = __lhbrx(&eth.mac.regs[BBA_RRP]);
	uint16_t rhbp = __lhbrx(&eth.mac.regs[BBA_RHBP]);

	bba_page_t *page = (bba_page_t *)(*eth.mac.data);
	bba_header_t *bba = (bba_header_t *)page[rwp];
	uint8_t *dst = bba->data;

	bba->next = 0x000;
	bba->length = sizeof(bba_header_t) + size;
	bba->status = 0x00;

	while (size) {
		int page_size = MIN(page[rwp + 1] - dst, size);
		memcpy(dst, data, page_size);
		data += page_size;
		size -= page_size;

		if (rwp == rhbp) rwp = bp;
		else rwp++;
		dst = page[rwp];

		if (rwp == rrp)
			break;
	}

	if (!size) {
		bba->next = rwp;
		__sthbrx(&eth.mac.regs[BBA_RWP], rwp);

		if (eth.mac.regs[BBA_IMR] & BBA_IMR_RIM) {
			eth.mac.regs[BBA_IR] |= BBA_IR_RI;
			eth_update_interrupts();
		}
	}
}

static uint8_t eth_mac_read(void)
{
	uint16_t address = eth.mac.address % 8192;
	uint8_t value = (*eth.mac.data)[(address - 256) % 4096 + 256];

	if (eth.mac.address != BBA_WRTXFIFOD)
		eth.mac.address++;

	return value;
}

static void eth_mac_write(uint8_t value)
{
	uint16_t address = eth.mac.address % 8192;

	switch (address) {
		case BBA_NCRA:
		{
			eth.mac.regs[address] = value;

			if ((eth.mac.regs[BBA_NCRA] & (BBA_NCRA_ST0 | BBA_NCRA_ST1)) == BBA_NCRA_ST1) {
				bba_transmit_fifo(*eth.mac.fifo, eth.mac.txfifocnt);
				eth.mac.txfifocnt = 0;
				eth.mac.regs[BBA_NCRA] &= ~BBA_NCRA_ST1;

				if (eth.mac.regs[BBA_IMR] & BBA_IMR_TIM) {
					eth.mac.regs[BBA_IR] |= BBA_IR_TI;
					eth_update_interrupts();
				}
			}
			break;
		}
		case BBA_IR:
		{
			eth.mac.regs[address] = (value ^ eth.mac.regs[BBA_IR]) & eth.mac.regs[BBA_IR];
			break;
		}
		case BBA_WRTXFIFOD:
		{
			(*eth.mac.fifo)[eth.mac.txfifocnt++] = value;
			break;
		}
		#ifdef BBA
		case BBA_NAFR_MAR0 ... BBA_NAFR_MAR7:
		{
			bba_out8(address, value);
		}
		#endif
		default:
		{
			(*eth.mac.data)[(address - 256) % 4096 + 256] = value;
			break;
		}
	}

	if (eth.mac.address != BBA_WRTXFIFOD)
		eth.mac.address++;
}

uint8_t eth_exi_imm(uint8_t data)
{
	uint8_t result = 0x00;

	if (eth.exi.position == 0)
		eth.exi.command = data;

	if (eth.exi.command & 0x80) {
		if (eth.exi.command & 0x40) {
			switch (eth.exi.position) {
				case 1: eth.mac.address = (eth.mac.address & ~0xFF00) | ((data << 8) & 0xFF00); break;
				case 2: eth.mac.address = (eth.mac.address & ~0xFF) | (data & 0xFF); break;
			}
			if (eth.exi.position >= 4)
				eth_mac_write(data);
		} else {
			switch (eth.exi.position) {
				case 1: eth.mac.address = (eth.mac.address & ~0xFF00) | ((data << 8) & 0xFF00); break;
				case 2: eth.mac.address = (eth.mac.address & ~0xFF) | (data & 0xFF); break;
			}
			if (eth.exi.position >= 4)
				result = eth_mac_read();
		}
	} else {
		switch (eth.exi.command) {
			case 0x00:
			{
				switch (eth.exi.position % 32) {
					case 2: result = 0x04; break;
					case 3: result = 0x02; break;
					case 4: result = 0x02; break;
					case 5: result = 0x00; break;
				}
				break;
			}
			case 0x02:
			{
				if (eth.exi.position >= 2)
					result = eth.exi.imr;
				break;
			}
			case 0x03:
			{
				if (eth.exi.position >= 2)
					result = eth.exi.isr;
				break;
			}
			case 0x42:
			{
				if (eth.exi.position >= 2) {
					eth.exi.imr = data;
					eth_update_interrupts();
				}
				break;
			}
			case 0x43:
			{
				if (eth.exi.position >= 2) {
					eth.exi.isr = (data ^ eth.exi.isr) & eth.exi.isr;
					eth_update_interrupts();
				}
				break;
			}
		}
	}

	eth.exi.position++;
	return result;
}

void eth_exi_dma(uint32_t address, uint32_t length, int type)
{
	void *buffer = OSPhysicalToUncached(address);

	if (eth.exi.command & 0x80) {
		if (eth.exi.command & 0x40) {
			if (eth.exi.position >= 4 && type == EXI_WRITE) {
				if (eth.mac.address == BBA_WRTXFIFOD) {
					memcpy(*eth.mac.fifo + eth.mac.txfifocnt, buffer, length);
					eth.mac.txfifocnt += length;
				}
			}
		} else {
			if (eth.exi.position >= 4 && type == EXI_READ) {
				memcpy(buffer, *eth.mac.data + eth.mac.address, length);
				eth.mac.address += length;
			}
		}
	}

	eth.exi.position += length;
}

void eth_exi_select(void)
{
}

void eth_exi_deselect(void)
{
	eth.exi.position = 0;
}

void eth_init(void **arenaLo, void **arenaHi)
{
	eth.mac.txfifocnt = 0;

	*arenaHi -= sizeof(*eth.mac.fifo); eth.mac.fifo = *arenaHi;
	*arenaHi -= sizeof(*eth.mac.data); eth.mac.data = *arenaHi;

	DCZeroRange(*eth.mac.data, sizeof(*eth.mac.data));

	eth.mac.regs[BBA_NAFR_PAR0] = VAR_CLIENT_MAC[0];
	eth.mac.regs[BBA_NAFR_PAR1] = VAR_CLIENT_MAC[1];
	eth.mac.regs[BBA_NAFR_PAR2] = VAR_CLIENT_MAC[2];
	eth.mac.regs[BBA_NAFR_PAR3] = VAR_CLIENT_MAC[3];
	eth.mac.regs[BBA_NAFR_PAR4] = VAR_CLIENT_MAC[4];
	eth.mac.regs[BBA_NAFR_PAR5] = VAR_CLIENT_MAC[5];

	eth.mac.regs[BBA_NWAYS] = BBA_NWAYS_LS100 | BBA_NWAYS_LPNWAY | BBA_NWAYS_ANCLPT | BBA_NWAYS_100TXF;
}
