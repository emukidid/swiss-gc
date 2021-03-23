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
#include "audio.h"
#include "fifo.h"

__attribute((always_inline))
static inline int_fast16_t clamp_s16(int32_t x)
{
	if ((x + 0x8000) & ~0xFFFF)
		return x >> 31 ^ 0x7FFF;
	else
		return x;
}

__attribute((always_inline))
static inline int32_t clamp_s22(int32_t x)
{
	if ((x + 0x200000) & ~0x3FFFFF)
		return x >> 31 ^ 0x1FFFFF;
	else
		return x;
}

static int_fast16_t decode_sample(uint8_t header, int sample, int32_t prev[2])
{
	static uint8_t table[][4] = {
		{ 0, 60, 115, 98 },
		{ 0,  0,  52, 55 }
	};

	int index = (header & 0x30) >> 4;
	int shift = header & 0xF;

	int32_t curr = prev[0] * table[0][index] -
	               prev[1] * table[1][index];

	curr = clamp_s22((curr + 32) >> 6) + ((int16_t)(sample << 12) >> shift << 6);

	prev[1] = prev[0];
	prev[0] = curr;

	return clamp_s16((curr + 32) >> 6);
}

static void pop_sample(fifo_t *fifo, sample_t *sample)
{
	if (fifo_size(fifo) < sizeof(sample_t)) return;
	*sample = *(sample_t *)fifo->read_ptr;

	fifo->read_ptr += sizeof(sample_t);
	if (fifo->read_ptr == fifo->end_ptr)
		fifo->read_ptr = fifo->start_ptr;
	fifo->used -= sizeof(sample_t);
}

static void push_sample(fifo_t *fifo, sample_t sample)
{
	if (fifo_space(fifo) < sizeof(sample_t)) return;
	*(sample_t *)fifo->write_ptr = sample;

	fifo->write_ptr += sizeof(sample_t);
	if (fifo->write_ptr == fifo->end_ptr)
		fifo->write_ptr = fifo->start_ptr;
	fifo->used += sizeof(sample_t);
}

void adpcm_reset(adpcm_t *adpcm)
{
	adpcm->l[0] = adpcm->l[1] = 0;
	adpcm->r[0] = adpcm->r[1] = 0;
}

void adpcm_decode(adpcm_t *adpcm, fifo_t *out, uint8_t *in, int count)
{
	for (int j = 0; j < count; j += 28, in += 32) {
		for (int i = 0; i < 28; i++) {
			sample_t sample;
			sample.l = decode_sample(in[0], in[4 + i] & 0xF, adpcm->l);
			sample.r = decode_sample(in[1], in[4 + i] >>  4, adpcm->r);
			push_sample(out, sample);
		}
	}
}

void mix_samples(volatile sample_t *out, fifo_t *in, int count, bool _3to2, uint8_t volume_l, uint8_t volume_r)
{
	for (int i = 0; i < count; i++) {
		sample_t sample = {0};
		pop_sample(in, &sample);
		int_fast16_t r = sample.r;
		int_fast16_t l = sample.l;

		if (i & _3to2) {
			pop_sample(in, &sample);
			r = (r + sample.r) >> 1;
			l = (l + sample.l) >> 1;
		}

		sample = *out;
		r = (r * volume_r >> 8) + sample.r;
		l = (l * volume_l >> 8) + sample.l;

		sample.r = clamp_s16(r);
		sample.l = clamp_s16(l);
		*out++ = sample;
	}
}
