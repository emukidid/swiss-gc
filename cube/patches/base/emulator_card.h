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

#ifndef EMULATOR_CARD_H
#define EMULATOR_CARD_H

#include <stdint.h>
#include <stdbool.h>

uint8_t card_imm(unsigned chan, uint8_t data);
bool card_dma(unsigned chan, uint32_t address, uint32_t length, int type);
void card_select(unsigned chan);
void card_deselect(unsigned chan);

#endif /* EMULATOR_CARD_H */
