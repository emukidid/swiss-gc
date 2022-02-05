/* 
 * Copyright (c) 2021-2022, Extrems <extrems@extremscorner.org>
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
	uint32_t size;
	union {
		struct {
			uint64_t file   :  8;
			uint64_t device :  8;
			uint64_t sector : 48;
		};
		struct {
			uint16_t        : 16;
			uint16_t pathlen;
			const char *path;
		};
		uint64_t data;
	};
} frag_t;

typedef void (*frag_callback)(void *buffer, uint32_t length);

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback);
bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback);
int do_read_write(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write);
void end_read(void);

int frag_get_list(int file, const frag_t **frag);
bool is_frag_patch(int file, uint32_t offset, size_t size);

bool frag_read_write_async(int file, void *buffer, uint32_t length, uint32_t offset, bool write, frag_callback callback);
int frag_read_complete(int file, void *buffer, uint32_t length, uint32_t offset);
bool frag_read_patch(int file, void *buffer, uint32_t length, uint32_t offset, frag_callback callback);
int frag_read_write(int file, void *buffer, uint32_t length, uint32_t offset, bool write);

#define frag_read_async(file, buffer, length, offset, callback) frag_read_write_async(file, buffer, length, offset, false, callback)
#define frag_write_async(file, buffer, length, offset, callback) frag_read_write_async(file, buffer, length, offset, true, callback)

#define frag_read(file, buffer, length, offset) frag_read_write(file, buffer, length, offset, false)
#define frag_write(file, buffer, length, offset) frag_read_write(file, buffer, length, offset, true)

#endif /* FRAG_H */
