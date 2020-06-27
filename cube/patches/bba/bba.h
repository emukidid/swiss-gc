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

#ifndef BBA_H
#define BBA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint32_t next   : 12;
	uint32_t length : 12;
	uint32_t status : 8;
	uint8_t data[];
} __attribute((packed, scalar_storage_order("little-endian"))) bba_header_t;

typedef uint8_t bba_page_t[256] __attribute((aligned(32)));

void bba_transmit(const void *data, size_t size);
void bba_receive_end(bba_page_t page, void *data, size_t size);

#endif /* BBA_H */
