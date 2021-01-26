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
#include "dolphin/os.h"
#include "frag.h"

#define DEVICE_DISC 0
#ifndef DEVICE_PATCHES
#define DEVICE_PATCHES DEVICE_DISC
#endif

static bool frag_get(uint32_t offset, size_t size, frag_t *frag)
{
	const frag_t *frags = (frag_t *)VAR_FRAG_LIST;

	for (int i = 0; i < sizeof(VAR_FRAG_LIST) / sizeof(*frags); i++) {
		if (!frags[i].size)
			break;
		if (offset < frags[i].offset || offset >= frags[i].offset + frags[i].size) {
			size = MIN(size, OSRoundUp32B(frags[i].offset - offset));
			continue;
		}

		frag->offset = offset - frags[i].offset;
		frag->size = MIN(size, OSRoundUp32B(frags[i].size) - frag->offset);
		frag->device = frags[i].device;
		frag->sector = frags[i].sector;
		return true;
	}

	return false;
}

int frag_get_list(uint32_t offset, const frag_t **frag)
{
	const frag_t *frags = (frag_t *)VAR_FRAG_LIST;
	int count = 0;

	for (int i = 0; i < sizeof(VAR_FRAG_LIST) / sizeof(*frags); i++) {
		if (!frags[i].size)
			break;
		if (frags[i].offset != offset) {
			if (!count)
				continue;
			else break;
		}

		if (!count) *frag = frags + i;
		offset += frags[i].size;
		count++;
	}

	return count;
}

bool is_frag_patch(uint32_t offset, size_t size)
{
	frag_t frag;
	return frag_get(offset, size, &frag) && frag.device == DEVICE_PATCHES;
}

bool frag_read_async(void *buffer, uint32_t length, uint32_t offset, frag_read_cb callback)
{
	frag_t frag;

	if (frag_get(offset, length, &frag)) {
		if (frag.device == DEVICE_PATCHES)
			return do_read_async(buffer, frag.size, frag.offset, frag.sector, callback);
		else
			return do_read_disc(buffer, frag.size, frag.offset, frag.sector, callback);
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

int frag_read_write(void *buffer, uint32_t length, uint32_t offset, bool write)
{
	frag_t frag;

	if (frag_get(offset, length, &frag) && frag.device == DEVICE_PATCHES)
		return do_read_write(buffer, frag.size, frag.offset, frag.sector, write);

	return 0;
}
