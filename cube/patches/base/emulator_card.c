/* 
 * Copyright (c) 2020-2021, Extrems <extrems@extremscorner.org>
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
#include "emulator_card.h"
#include "frag.h"

#define carda_read_callback exi0_complete_transfer
#define cardb_read_callback exi1_complete_transfer

static void carda_write_callback(void *address, uint32_t length);
static void cardb_write_callback(void *address, uint32_t length);

static struct {
	int position;
	uint8_t command;
	uint8_t status;
	uint32_t offset;
	#ifdef ASYNC_READ
	frag_callback read_callback;
	frag_callback write_callback;
	#endif
} card[2] = {
	{
		.status = 0b11000001,
		#ifdef ASYNC_READ
		.read_callback  = carda_read_callback,
		.write_callback = carda_write_callback
		#endif
	}, {
		.status = 0b11000001,
		#ifdef ASYNC_READ
		.read_callback  = cardb_read_callback,
		.write_callback = cardb_write_callback
		#endif
	}
};

static void carda_write_callback(void *address, uint32_t length)
{
	if (length != 512) card[0].status |= 0b00001000;
	exi0_complete_transfer();
}

static void cardb_write_callback(void *address, uint32_t length)
{
	if (length != 512) card[1].status |= 0b00001000;
	exi1_complete_transfer();
}

uint8_t card_imm(unsigned chan, uint8_t data)
{
	uint8_t result = ~0;

	if (card[chan].position == 0)
		card[chan].command = data;

	switch (card[chan].command) {
		case 0x00:
		{
			if (card[chan].position >= 2) {
				switch ((card[chan].position - 2) % 4) {
					case 0: result = 0x00; break;
					case 1: result = 0x00; break;
					case 2: result = 0x00; break;
					case 3: result = VAR_CARD_IDS[chan]; break;
				}
			}
			break;
		}
		case 0x52:
		{
			switch (card[chan].position) {
				case 1: card[chan].offset = (card[chan].offset & ~0xFE0000) | ((data << 17) & 0xFE0000); break;
				case 2: card[chan].offset = (card[chan].offset & ~0x1FE00) | ((data << 9) & 0x1FE00); break;
				case 3: card[chan].offset = (card[chan].offset & ~0x180) | ((data << 7) & 0x180); break;
				case 4: card[chan].offset = (card[chan].offset & ~0x7F) | (data & 0x7F); break;
			}
			break;
		}
		case 0x81:
		{
			if (card[chan].position == 1)
				card[chan].status = (card[chan].status & ~0b00000010) | ((data << 1) & 0b00000010);
			break;
		}
		case 0x83:
		{
			if (card[chan].position >= 1)
				result = card[chan].status;
			break;
		}
		case 0x85:
		{
			if (card[chan].position >= 1)
				result = 0xEC;
			break;
		}
		case 0xF2:
		{
			switch (card[chan].position) {
				case 1: card[chan].offset = (card[chan].offset & ~0xFE0000) | ((data << 17) & 0xFE0000); break;
				case 2: card[chan].offset = (card[chan].offset & ~0x1FE00) | ((data << 9) & 0x1FE00); break;
				case 3: card[chan].offset = (card[chan].offset & ~0x180) | ((data << 7) & 0x180); break;
				case 4: card[chan].offset = (card[chan].offset & ~0x7F) | (data & 0x7F); break;
			}
			break;
		}
	}

	card[chan].position++;
	return result;
}

bool card_dma(unsigned chan, uint32_t address, uint32_t length, int type)
{
	void *buffer = OSPhysicalToUncached(address);

	switch (card[chan].command) {
		case 0x52:
		{
			if (card[chan].position >= 5 && type == EXI_READ) {
				#ifdef ASYNC_READ
				return frag_read_async(FRAGS_CARD(chan), buffer, length, card[chan].offset, card[chan].read_callback);
				#else
				frag_read(FRAGS_CARD(chan), buffer, length, card[chan].offset);
				#endif
			}
			break;
		}
		case 0xF2:
		{
			if (card[chan].position == 5 && type == EXI_WRITE) {
				if (card[chan].offset % 512 == 0) {
					#ifdef ASYNC_READ
					return frag_write_async(FRAGS_CARD(chan), buffer, 512, card[chan].offset, card[chan].write_callback);
					#else
					if (frag_write(FRAGS_CARD(chan), buffer, 512, card[chan].offset) != 512) card[chan].status |= 0b00001000;
					#endif
				}
			}
			break;
		}
	}

	card[chan].position += length;
	return false;
}

void card_select(unsigned chan)
{
}

void card_deselect(unsigned chan)
{
	switch (card[chan].command) {
		case 0x89:
		{
			card[chan].status &= ~0b00011100;
			break;
		}
		case 0xF1:
		case 0xF4:
		{
			if (card[chan].position >= 3)
				if (card[chan].status & 0b00000010)
					exi_interrupt(chan);
			break;
		}
		case 0xF2:
		{
			if (card[chan].position >= 5)
				if (card[chan].status & 0b00000010)
					exi_interrupt(chan);
			break;
		}
	}

	card[chan].position = 0;
}
