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
#include "mix.h"

__attribute((always_inline))
static inline int_fast16_t clamp(int32_t x)
{
	if ((x + 0x8000) & ~0xFFFF)
		return x >> 31 ^ 0x7FFF;
	else
		return x;
}

void mix_samples(volatile sample_t *out, sample_t *in, bool _3to2, int count, uint8_t volume_l, uint8_t volume_r)
{
	for (int i = 0; i < count; i++) {
		sample_t sample = *in++;
		int_fast16_t r = sample.r;
		int_fast16_t l = sample.l;

		if (i & _3to2) {
			sample = *in++;
			r = (r + sample.r) >> 1;
			l = (l + sample.l) >> 1;
		}

		sample = *out;
		r = (r * volume_r >> 8) + sample.r;
		l = (l * volume_l >> 8) + sample.l;

		sample.r = clamp(r);
		sample.l = clamp(l);
		*out++ = sample;
	}
}
