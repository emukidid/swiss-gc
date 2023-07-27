/* 
 * Copyright (c) 2023, Extrems <extrems@extremscorner.org>
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

#ifndef EMULATOR_ETH_H
#define EMULATOR_ETH_H

#include <stdint.h>

void eth_mac_receive(const void *data, size_t size);

uint8_t eth_exi_imm(uint8_t data);
void eth_exi_dma(uint32_t address, uint32_t length, int type);
void eth_exi_select(void);
void eth_exi_deselect(void);

void eth_init(void **arenaLo, void **arenaHi);

#endif /* EMULATOR_ETH_H */
