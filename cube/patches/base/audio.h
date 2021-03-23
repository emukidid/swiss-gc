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

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "fifo.h"

typedef struct {
	int32_t l[2];
	int32_t r[2];
} adpcm_t;

typedef struct {
	int16_t r;
	int16_t l;
} sample_t;

void adpcm_reset(adpcm_t *adpcm);
void adpcm_decode(adpcm_t *adpcm, fifo_t *out, uint8_t *in, int count);

void mix_samples(volatile sample_t *out, fifo_t *in, int count, bool _3to2, uint8_t volume_l, uint8_t volume_r);

#endif /* AUDIO_H */
