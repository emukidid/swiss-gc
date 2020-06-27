/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

union mmcr0 {
	uint32_t val;
	struct {
		uint32_t dis            : 1;
		uint32_t dp             : 1;
		uint32_t du             : 1;
		uint32_t dms            : 1;
		uint32_t dmr            : 1;
		uint32_t enint          : 1;
		uint32_t discount       : 1;
		uint32_t rtcselect      : 2;
		uint32_t intonbittrans  : 1;
		uint32_t threshold      : 6;
		uint32_t pmc1intcontrol : 1;
		uint32_t pmcintcontrol  : 1;
		uint32_t pmctrigger     : 1;
		uint32_t pmc1select     : 7;
		uint32_t pmc2select     : 6;
	};
};

union mmcr1 {
	uint32_t val;
	struct {
		uint32_t pmc3select : 5;
		uint32_t pmc4select : 5;
	};
};

union pmc {
	uint32_t val;
	struct {
		uint32_t ov :  1;
		uint32_t    : 30;
	};
};

static inline void timer1_start(uint32_t ticks)
{
	union mmcr0 mmcr0;
	union pmc pmc1;

	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	mmcr0.pmc1select = 0;
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));

	mmcr0.dis = false;
	mmcr0.dp = false;
	mmcr0.du = false;
	mmcr0.dms = false;
	mmcr0.dmr = false;
	mmcr0.enint = true;
	mmcr0.discount = false;
	mmcr0.rtcselect = 0;
	mmcr0.intonbittrans = false;
	mmcr0.threshold = 0;
	mmcr0.pmc1intcontrol = true;
	mmcr0.pmctrigger = false;
	mmcr0.pmc1select = 3;

	pmc1.val = 0x80000000 - ticks / 2;

	asm volatile("mtpmc1 %0" :: "r" (pmc1.val));
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
}

static inline bool timer1_interrupt(void)
{
	union pmc pmc1;
	asm volatile("mfpmc1 %0" : "=r" (pmc1.val));
	return pmc1.ov;
}

static inline void timer1_stop(void)
{
	union mmcr0 mmcr0;
	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	mmcr0.pmc1select = 0;
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
	asm volatile("mtpmc1 %0" :: "r" (0));
}

static inline void timer2_start(uint32_t ticks)
{
	union mmcr0 mmcr0;
	union pmc pmc2;

	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	mmcr0.pmc2select = 0;
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));

	mmcr0.dis = false;
	mmcr0.dp = false;
	mmcr0.du = false;
	mmcr0.dms = false;
	mmcr0.dmr = false;
	mmcr0.enint = true;
	mmcr0.discount = false;
	mmcr0.rtcselect = 0;
	mmcr0.intonbittrans = false;
	mmcr0.threshold = 0;
	mmcr0.pmcintcontrol = true;
	mmcr0.pmctrigger = false;
	mmcr0.pmc2select = 3;

	pmc2.val = 0x80000000 - ticks / 2;

	asm volatile("mtpmc2 %0" :: "r" (pmc2.val));
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
}

static inline bool timer2_interrupt(void)
{
	union pmc pmc2;
	asm volatile("mfpmc2 %0" : "=r" (pmc2.val));
	return pmc2.ov;
}

static inline void timer2_stop(void)
{
	union mmcr0 mmcr0;
	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	mmcr0.pmc2select = 0;
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
	asm volatile("mtpmc2 %0" :: "r" (0));
}

static inline void timer3_start(uint32_t ticks)
{
	union mmcr0 mmcr0;
	union mmcr1 mmcr1;
	union pmc pmc3;

	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	asm volatile("mfmmcr1 %0" : "=r" (mmcr1.val));
	mmcr1.pmc3select = 0;
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));

	mmcr0.dis = false;
	mmcr0.dp = false;
	mmcr0.du = false;
	mmcr0.dms = false;
	mmcr0.dmr = false;
	mmcr0.enint = true;
	mmcr0.discount = false;
	mmcr0.rtcselect = 0;
	mmcr0.intonbittrans = false;
	mmcr0.threshold = 0;
	mmcr0.pmcintcontrol = true;
	mmcr0.pmctrigger = false;
	mmcr1.pmc3select = 3;

	pmc3.val = 0x80000000 - ticks / 2;

	asm volatile("mtpmc3 %0" :: "r" (pmc3.val));
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
}

static inline bool timer3_interrupt(void)
{
	union pmc pmc3;
	asm volatile("mfpmc3 %0" : "=r" (pmc3.val));
	return pmc3.ov;
}

static inline void timer3_stop(void)
{
	union mmcr1 mmcr1;
	asm volatile("mfmmcr1 %0" : "=r" (mmcr1.val));
	mmcr1.pmc3select = 0;
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));
	asm volatile("mtpmc3 %0" :: "r" (0));
}

static inline void timer4_start(uint32_t ticks)
{
	union mmcr0 mmcr0;
	union mmcr1 mmcr1;
	union pmc pmc4;

	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	asm volatile("mfmmcr1 %0" : "=r" (mmcr1.val));
	mmcr1.pmc4select = 0;
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));

	mmcr0.dis = false;
	mmcr0.dp = false;
	mmcr0.du = false;
	mmcr0.dms = false;
	mmcr0.dmr = false;
	mmcr0.enint = true;
	mmcr0.discount = false;
	mmcr0.rtcselect = 0;
	mmcr0.intonbittrans = false;
	mmcr0.threshold = 0;
	mmcr0.pmcintcontrol = true;
	mmcr0.pmctrigger = false;
	mmcr1.pmc4select = 3;

	pmc4.val = 0x80000000 - ticks / 2;

	asm volatile("mtpmc4 %0" :: "r" (pmc4.val));
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
}

static inline bool timer4_interrupt(void)
{
	union pmc pmc4;
	asm volatile("mfpmc4 %0" : "=r" (pmc4.val));
	return pmc4.ov;
}

static inline void timer4_stop(void)
{
	union mmcr1 mmcr1;
	asm volatile("mfmmcr1 %0" : "=r" (mmcr1.val));
	mmcr1.pmc4select = 0;
	asm volatile("mtmmcr1 %0" :: "r" (mmcr1.val));
	asm volatile("mtpmc4 %0" :: "r" (0));
}

static inline void restore_timer_interrupts(void)
{
	union mmcr0 mmcr0;
	union mmcr1 mmcr1;
	asm volatile("mfmmcr0 %0" : "=r" (mmcr0.val));
	asm volatile("mfmmcr1 %0" : "=r" (mmcr1.val));
	mmcr0.enint = mmcr0.pmc1select || mmcr0.pmc2select || mmcr1.pmc3select || mmcr1.pmc4select;
	asm volatile("mtmmcr0 %0" :: "r" (mmcr0.val));
}

static inline void clear_timers(void)
{
	asm volatile("mtmmcr0 %0" :: "r" (0));
	asm volatile("mtmmcr1 %0" :: "r" (0));
	asm volatile("mtpmc1 %0" :: "r" (0));
	asm volatile("mtpmc2 %0" :: "r" (0));
	asm volatile("mtpmc3 %0" :: "r" (0));
	asm volatile("mtpmc4 %0" :: "r" (0));
}

#endif /* TIMER_H */
