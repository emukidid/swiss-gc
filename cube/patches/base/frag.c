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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common.h"
#include "frag.h"

#define DEVICE_DISC 0
#ifndef DEVICE_PATCHES
#define DEVICE_PATCHES DEVICE_DISC
#endif

static const frag_t *frag_get(uint32_t offset, uint32_t size)
{
	frag_t *frags = (frag_t *)VAR_FRAG_LIST;

	for (int i = 0; i < sizeof(VAR_FRAG_LIST) / sizeof(*frags); i++) {
		if (!frags[i].size)
			break;
		if (offset < frags[i].offset || offset >= frags[i].offset + frags[i].size)
			continue;
		if (offset > frags[i].offset + frags[i].size - 32 && size >= 32)
			continue;

		return &frags[i];
	}

	return NULL;
}

bool is_frag_patch(uint32_t offset, uint32_t size)
{
	const frag_t *frag = frag_get(offset, size);
	return frag && frag->device == DEVICE_PATCHES;
}

bool frag_read_async(void *buffer, uint32_t length, uint32_t offset, frag_read_cb callback)
{
	const frag_t *frag = frag_get(offset, length);

	if (frag && frag->device == DEVICE_DISC) {
		offset = offset - frag->offset;
		length = MIN(length, frag->size - offset);
		return do_read_async(buffer, length, offset, frag->sector, callback);
	}

	return false;
}

void frag_read_complete(void *buffer, uint32_t length, uint32_t offset)
{
	while (length) {
		int size = frag_read(buffer, length, offset);
		buffer += size;
		length -= size;
		offset += size;
	}
}

int frag_read(void *buffer, uint32_t length, uint32_t offset)
{
	const frag_t *frag = frag_get(offset, length);

	if (frag && frag->device == DEVICE_PATCHES) {
		offset = offset - frag->offset;
		length = MIN(length, frag->size - offset);
		return do_read(buffer, length, offset, frag->sector);
	}

	return 0;
}

int frag_write(void *buffer, uint32_t length, uint32_t offset)
{
	const frag_t *frag = frag_get(offset, length);

	if (frag && frag->device == DEVICE_PATCHES) {
		offset = offset - frag->offset;
		length = MIN(length, frag->size - offset);
		return do_write(buffer, length, offset, frag->sector);
	}

	return 0;
}
