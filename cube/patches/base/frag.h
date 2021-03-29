/* 
 * Copyright (c) 2021, Extrems <extrems@extremscorner.org>
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

#ifndef FRAG_H
#define FRAG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	uint32_t offset;
	uint32_t device :  1;
	uint32_t size   : 31;
	uint32_t sector;
} frag_t;

typedef void (*frag_callback)(void *buffer, uint32_t length);

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, bool write, frag_callback callback);
bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, frag_callback callback);
int do_read_write(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, bool write);
void end_read(void);

int frag_get_list(uint32_t offset, const frag_t **frag);
bool is_frag_patch(uint32_t offset, size_t size);

bool frag_read_write_async(void *buffer, uint32_t length, uint32_t offset, bool write, frag_callback callback);
int frag_read_complete(void *buffer, uint32_t length, uint32_t offset);
int frag_read_write(void *buffer, uint32_t length, uint32_t offset, bool write);

#define frag_read_async(buffer, length, offset, callback) frag_read_write_async(buffer, length, offset, false, callback)
#define frag_write_async(buffer, length, offset, callback) frag_read_write_async(buffer, length, offset, true, callback)

#define frag_read(buffer, length, offset) frag_read_write(buffer, length, offset, false)
#define frag_write(buffer, length, offset) frag_read_write(buffer, length, offset, true)

#endif /* FRAG_H */
