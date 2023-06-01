/* 
 * Copyright (c) 2012-2023, Extrems <extrems@extremscorner.org>
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

.section .init
	.globl _start, __argv
_start:
	b		1f
	.ascii	"_arg"
__argv:
	.long	0	# argv magic
	.long	0	# command line
	.long	0	# command line length
	.long	0	# argc
	.long	0	# argv
	.long	0	# end address of argv

1:	li		r3, 0
	mtsrr1	r3
	lis		r3, startup@ha
	addi	r3, r3, startup@l
	mtsrr0	r3
	rfi

startup:
	lis		r0, 0x8000
	ori		r0, r0, 0x000C
	mtear	r0

	li		r3, 0
	lis		r0, reset@h + 0x48000002
	ori		r0, r0, reset@l + 0x48000002
1:	stwu	r0, 0x0100 (r3)
	dcbst	0, r3
	sync
	icbi	0, r3
	cmplwi	r3, 0x1700
	blt		1b

	li		r0, 0
	lis		sp, 0x0180
	stwu	r0, -8 (sp)

	bl		main

reset:
	lis		r3, 0x0C00
	li		r4, 0x3000 + 9*4
	li		r0, 0b000
	ecowx	r0, r4, r3
	b		.

.section .text
	.globl __eabi
__eabi:
	lis		r2, _SDA2_BASE_@ha
	lis		r13, _SDA_BASE_@ha
	addi	r2, r2, _SDA2_BASE_@l
	addi	r13, r13, _SDA_BASE_@l

	addi	r3, r13, __sbss_start@sdarel
	addi	r4, r13, __sbss_end@sdarel
	subf	r4, r3, r4
	b		memzero
