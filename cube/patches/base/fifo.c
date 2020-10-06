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

#include <string.h>
#include "common.h"
#include "fifo.h"

static struct {
	int used, size;
	void *start_ptr, *end_ptr;
	void *read_ptr, *write_ptr;
} fifo = {0};

void fifo_init(void *buffer, int length)
{
	fifo.size = length;
	fifo.start_ptr = buffer;
	fifo.end_ptr = buffer + length;

	fifo_reset();
}

void fifo_reset(void)
{
	fifo.used = 0;
	fifo.read_ptr = 
	fifo.write_ptr = fifo.start_ptr;
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
	do {
		int size = MIN(length, fifo.end_ptr - fifo.read_ptr);

		memcpy(buffer, fifo.read_ptr, size);
		buffer += size;
		length -= size;

		fifo.read_ptr += size;
		if (fifo.read_ptr == fifo.end_ptr)
			fifo.read_ptr = fifo.start_ptr;
		fifo.used -= size;
	} while (length);
}

void fifo_write(void *buffer, int length)
{
	do {
		int size = MIN(length, fifo.end_ptr - fifo.write_ptr);

		memcpy(fifo.write_ptr, buffer, size);
		buffer += size;
		length -= size;

		fifo.write_ptr += size;
		if (fifo.write_ptr == fifo.end_ptr)
			fifo.write_ptr = fifo.start_ptr;
		fifo.used += size;
	} while (length);
}
