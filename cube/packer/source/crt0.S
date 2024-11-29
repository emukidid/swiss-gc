/* 
 * Copyright (c) 2012-2024, Extrems <extrems@extremscorner.org>
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

1:	mfdabr	r3
	xoris	r0, r3, 0x8000
	cmpwi	r0, 0x00E8 | 0b110
	bne		2f
	lis		r0, 0xfee1
	ori		r0, r0, 0xdead
	stw		r0, -0b110 (r3)

2:	li		r3, 0
	mtsrr1	r3
	lis		r3, startup@ha
	addi	r3, r3, startup@l
	mtsrr0	r3
	rfi

startup:
	lis		r0, 0x8000
	ori		r0, r0, 0x000C
	mtear	r0

	lis		r3, 0x0C00
	li		r4, 0x3000
	li		r0, 1
	ecowx	r0, r4, r3
	eciwx	r0, r4, r3
	lis		r5, 0x1000
	eciwx	r0, 0, r5
	eciwx	r0, r4, r3
	andi.	r0, r0, 1
	ecowx	r0, r4, r3
	beq		reload
	li		r4, 0x6000 + 5*4
	eciwx	r0, r4, r3
	oris	r0, r0, 0x1000
	ecowx	r0, r4, r3
	eciwx	r0, r4, r3
	andis.	r0, r0, 0x1000
	beq		premain

	li		r3, 0x30F8
	lis		r0, 0xdead
	ori		r0, r0, 0xbeef
	ecowx	r0, 0, r3
1:	eciwx	r0, 0, r3
	cmpwi	r0, 0
	bne		1b

	mftb	r3
2:	mftb	r4
	sub		r0, r4, r3
	cmplwi	r0, 44550
	blt		2b

	li		r3, 0x1800
	lis		r0, 0x4E80
	ori		r0, r0, 0x0020
	stw		r0, 0 (r3)
	dcbst	0, r3
	sync
	icbi	0, r3

premain:
	li		r3, 0
	lis		r0, reload@h + 0x48000002
	ori		r0, r0, reload@l + 0x48000002
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

reload:
	li		r3, 0x1800
	lwzu	r0, 4 (r3)
	xoris	r0, r0, 0x5354
	cmpwi	r0, 0x5542
	beq		1f
	lwzu	r0, 4 (r3)
	xoris	r0, r0, 0x5354
	cmpwi	r0, 0x5542
	bne		2f
1:	lwz		r0, 4 (r3)
	xoris	r0, r0, 0x4841
	cmpwi	r0, 0x5858
	beqla	0x1800

2:	lis		r3, 0x0C00
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
