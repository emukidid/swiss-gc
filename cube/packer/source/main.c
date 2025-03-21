/* 
 * Copyright (c) 2012-2025, Extrems <extrems@extremscorner.org>
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

#include "xz/xz_crc32.c"
#include "xz/xz_dec_stream.c"
#include "xz/xz_dec_lzma2.c"
#include "xz/xz_dec_bcj.c"

#include <ogc/machine/processor.h>
#include "executable_xz.h"
#include "executable_bin.h"
#define EXECUTABLE_ADDR ((void *)0x00003100)
void (*entrypoint)(void) = EXECUTABLE_ADDR;
extern struct __argv __argv __attribute((section(".init")));
extern struct __argv __envp __attribute((section(".init")));

static struct xz_dec_bcj xz_dec_bcj;
static struct xz_dec_lzma2 xz_dec_lzma2;
static struct xz_dec xz_dec;

static struct xz_buf xz_buf = {
	.in = executable_xz,
	.in_size = executable_xz_size,
	.out = EXECUTABLE_ADDR,
	.out_size = executable_bin_size
};

static void memsync(void *buf, size_t size)
{
	uint8_t *b = buf;
	size_t i;

	for (i = 0; i < size; i += PPC_CACHE_ALIGNMENT)
		asm("dcbst %y0" :: "Z" (b[i]));
}

bool memeq(const void *a, const void *b, size_t size)
{
	const uint8_t *x = a;
	const uint8_t *y = b;
	size_t i;

	for (i = 0; i < size; ++i)
		if (x[i] != y[i])
			return false;

	return true;
}

void memzero(void *buf, size_t size)
{
	uint8_t *b = buf;
	uint8_t *e = b + size;

	while (b != e)
		*b++ = '\0';
}

void *memmove(void *dest, const void *src, size_t size)
{
	uint8_t *d = dest;
	const uint8_t *s = src;
	size_t i;

	if (d < s) {
		for (i = 0; i < size; ++i)
			d[i] = s[i];
	} else if (d > s) {
		i = size;
		while (i-- > 0)
			d[i] = s[i];
	}

	return dest;
}

void main(void)
{
	xz_crc32_init();

	xz_dec.bcj   = &xz_dec_bcj;
	xz_dec.lzma2 = &xz_dec_lzma2;

	if (xz_dec_run(&xz_dec, &xz_buf) != XZ_STREAM_END)
		return;

	memmove(entrypoint +  8, &__argv, sizeof(__argv));
	memmove(entrypoint + 40, &__envp, sizeof(__envp));
	memsync(xz_buf.out, xz_buf.out_pos);

	_sync();
	mthid0(mfhid0() | 0xC00);
	entrypoint();
}
