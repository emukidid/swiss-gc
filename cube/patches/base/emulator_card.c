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

static struct {
	int position;
	uint8_t command;
	uint8_t status;
	bool interrupt;
	uint32_t offset;
	void *write_buffer;
	uint32_t write_offset;
} card[2] = {
	{
		.status = 0xC1,
		.offset = FRAGS_CARD_A
	}, {
		.status = 0xC1,
		.offset = FRAGS_CARD_B
	}
};

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
				case 1: card[chan].offset = (card[chan].offset & ~0x1FE0000) | ((data << 17) & 0x1FE0000); break;
				case 2: card[chan].offset = (card[chan].offset & ~0x1FE00) | ((data << 9) & 0x1FE00); break;
				case 3: card[chan].offset = (card[chan].offset & ~0x180) | ((data << 7) & 0x180); break;
				case 4: card[chan].offset = (card[chan].offset & ~0x7F) | (data & 0x7F); break;
			}
			break;
		}
		case 0x81:
		{
			if (card[chan].position == 1)
				card[chan].interrupt = data & 1;
			break;
		}
		case 0x83:
		{
			if (card[chan].position >= 1)
				result = card[chan].status;
			break;
		}
		case 0xF2:
		{
			switch (card[chan].position) {
				case 1: card[chan].offset = (card[chan].offset & ~0x1FE0000) | ((data << 17) & 0x1FE0000); break;
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

void card_dma(unsigned chan, uint32_t address, uint32_t length, int type)
{
	void *buffer = OSPhysicalToUncached(address);

	switch (card[chan].command) {
		case 0x52:
		{
			if (card[chan].position >= 5 && type == EXI_READ)
				frag_read(buffer, length, card[chan].offset);
			break;
		}
		case 0xF2:
		{
			if (card[chan].position == 5 && type == EXI_WRITE) {
				if (!card[chan].write_buffer && card[chan].offset % 512 == 0) {
					card[chan].write_buffer = buffer;
					card[chan].write_offset = card[chan].offset;
				}
			}
			break;
		}
	}

	card[chan].position += length;
}

void card_select(unsigned chan)
{
}

void card_deselect(unsigned chan)
{
	switch (card[chan].command) {
		case 0xF1:
		case 0xF4:
		{
			if (card[chan].position >= 3)
				if (card[chan].interrupt)
					exi_interrupt(chan);
			break;
		}
		case 0xF2:
		{
			if (card[chan].position >= 5) {
				bool success = card[chan].offset % 128 == 0;

				if (card[chan].write_buffer && (card[chan].offset + 128 - card[chan].write_offset) == 512) {
					success = frag_write(card[chan].write_buffer, 512, card[chan].write_offset) == 512;
					card[chan].write_buffer = NULL;
				}

				if (card[chan].interrupt && success)
					exi_interrupt(chan);
			}
			break;
		}
	}

	card[chan].position = 0;
}
