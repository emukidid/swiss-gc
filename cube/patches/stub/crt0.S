/* 
 * Copyright (c) 2020-2021, Extrems <extrems@extremscorner.org>
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

#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .init
	.globl _start
_start:
#ifdef APPLOADER
	lis		r3, 0x8000
	li		r4, 0x3100 / 32
	mtctr	r4
1:	dcbz	r0, r3
	addi	r3, r3, 32
	bdnz	1b

	li		r3, 0
	mtsrr1	r3
	lis		r3, startup@ha
	addi	r3, r3, startup@l
	mtsrr0	r3
	rfi
#else
	mfmsr	r3
	b		1f
	.asciz	"STUBHAXX"
	.align 2

1:	li		r3, 0
	mtsrr1	r3
	li		r3, startup
	mtsrr0	r3
	rfi
#endif

startup:
	lis		r0, 0x8000
	ori		r0, r0, 0x000C
	mtear	r0

	li		r3, 0
	lis		r0, reset@h + 0x48000002
	ori		r0, r0, reset@l + 0x48000002
1:	stwu	r0, 0x0100 (r3)
	dcbst	r0, r3
	sync
	icbi	r0, r3
	cmplwi	r3, 0x1700
	blt		1b

	li		r0, 0
#ifdef APPLOADER
	lis		sp, 0x0180
#else
	li		sp, 0x3000
#endif
	stwu	r0, -8 (sp)

	bl		pf_main
	bl		dvd_main

reset:
	li		r0, 0
	lis		r3, 0x0C00
	li		r4, 0x3000 + 9*4
	ecowx	r0, r4, r3
	b		.

	.globl run
run:
	oris	r3, r3, 0x8000
	mtsrr0	r3
	li		r3, 0x30
	mtsrr1	r3
	rfi
