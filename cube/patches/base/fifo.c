/* 
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
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
#include "common.h"
#include "dolphin/ar.h"
#include "fifo.h"

static struct {
	uint32_t used, size;
	uint32_t start, end;
	uint32_t read, write;
} fifo = {
	.size  = 0x4000 - 0x0020,
	.start = 0x0020,
	.end   = 0x4000,
	.read  = 0x0020,
	.write = 0x0020
};

void fifo_reset(void)
{
	fifo.used = 0;
	fifo.read = 
	fifo.write = fifo.start;
}

int fifo_space(void)
{
	return fifo.size - fifo.used;
}

int fifo_size(void)
{
	return fifo.used;
}

void fifo_read(void *buffer, int length)
{
	dcache_flush_icache_inv(buffer, length);

	while (ARGetDMAStatus());
	bool status = ARGetInterruptStatus();

	do {
		int size = MIN(length, fifo.end - fifo.read);

		ARStartDMARead((intptr_t)buffer, fifo.read, size);
		while (ARGetDMAStatus());

		buffer += size;
		length -= size;

		fifo.read += size;
		if (fifo.read == fifo.end)
			fifo.read = fifo.start;
		fifo.used -= size;
	} while (length);

	if (!status) ARClearInterrupt();
}

void fifo_write(void *buffer, int length)
{
	dcache_flush_icache_inv(buffer, length);

	while (ARGetDMAStatus());
	bool status = ARGetInterruptStatus();

	do {
		int size = MIN(length, fifo.end - fifo.write);

		ARStartDMAWrite((intptr_t)buffer, fifo.write, size);
		while (ARGetDMAStatus());

		buffer += size;
		length -= size;

		fifo.write += size;
		if (fifo.write == fifo.end)
			fifo.write = fifo.start;
		fifo.used += size;
	} while (length);

	if (!status) ARClearInterrupt();
}
