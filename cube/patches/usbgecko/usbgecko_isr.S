/* 
 * Copyright (c) 2020-2023, Extrems <extrems@extremscorner.org>
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
#include "asm.h"

.section .text.usb_interrupt_vector,"ax",@progbits
	.balign 32
	.globl _usb
_usb:
_usb_requested:
	.long	0
_usb_data:
	.long	0
_usb_buffer:
	.long	0
_usb_registers:
	.long	0x0C006800 + 5*4

	.globl usb_interrupt_vector
usb_interrupt_vector:
	mtsprg	0, r4
	mtsprg	1, r5
	mtsprg	2, r6
	mfcr	r6
	mtsprg	3, r6
	lwz		r4, _usb_registers - 0x80000000 (r0)
	eciwx	r5, r0, r4
	rlwinm	r6, r5, 1, 28, 28
	and.	r6, r5, r6
	lwz		r6, _usb_requested - 0x80000000 (r0)
	cmpwi	cr7, r6, 0
	cmpwi	cr6, r6, -1
	beq		6f
	beq		cr7, 6f
	beq		cr6, 1f
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
1:	li		r5, 4*4
	eciwx	r5, r5, r4
	andis.	r6, r5, 0x0400
	lwz		r6, _usb_requested - 0x80000000 (r0)
	blt		cr7, 2f
	beq		3f
	neg		r6, r6
	stw		r6, _usb_requested - 0x80000000 (r0)
	li		r5, 4*4
	lis		r6, 0xA000
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((2 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		5f
2:	not		r6, r6
	stw		r6, _usb_requested - 0x80000000 (r0)
	extrwi	r6, r5, 8, 8
	lwz		r5, _usb_data - 0x80000000 (r0)
	insrwi	r6, r5, 24, 0
	stw		r6, _usb_data - 0x80000000 (r0)
	beq		cr6, 4f
3:	li		r5, 4*4
	lis		r6, 0xD000
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((1 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	bgt		cr7, 5f
4:	lwz		r4, _usb_buffer - 0x80000000 (r0)
	addi	r5, r4, 1
	clrrwi	r4, r4, 2
	stw		r5, _usb_buffer - 0x80000000 (r0)
	clrlwi.	r5, r5, 30
	bne		5f
	lwz		r6, _usb_data - 0x80000000 (r0)
	ecowx	r6, r5, r4
5:	lis		r4, 0x0C00
	li		r5, 0x3000
	eciwx	r5, r5, r4
	li		r6, 0x3000 + 1*4
	eciwx	r6, r6, r4
	and.	r6, r6, r5
	bne		6f
	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	rfi
6:	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	ba		0x00000504
