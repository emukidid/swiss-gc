/* 
 * Copyright (c) 2021-2022, Extrems <extrems@extremscorner.org>
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

.section .text.external_interrupt_vector,"ax",@progbits
	.balign 32
	.globl _ata
_ata:
_ata_transferred:
	.long	512
_ata_registers:
	.long	0x0C006800

	.globl external_interrupt_vector
external_interrupt_vector:
	mtsprg	0, r4
	mtsprg	1, r5
	mtsprg	2, r6
	mfcr	r6
	mtsprg	3, r6
	lwz		r4, _ata_registers - 0x80000000 (r0)
	eciwx	r5, r0, r4
	rlwinm	r6, r5, 1, 28, 28
	and.	r6, r5, r6
	lwz		r6, _ata_transferred - 0x80000000 (r0)
	cmpwi	cr7, r6, 512
	cmpwi	cr6, r6, 0
	beq		7f
	bnl		cr7, 7f
#ifdef DMA
	bnl		cr6, 6f
#endif
	li		r5, 4*4
	eciwx	r5, r5, r4
#ifndef DMA
	blt		cr6, 2f
	stw		r5, VAR_SECTOR_BUF (r6)
	addi	r6, r6, 4
	stw		r6, _ata_transferred - 0x80000000 (r0)
1:	eciwx	r5, r0, r4
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 3*4
	li		r6, ((4 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		5f
#endif
2:	andi.	r6, r5, 0x0800
	bne		3f
	eciwx	r5, r0, r4
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	lis		r6, 0x1700
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((3 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		5f
3:	eciwx	r5, r0, r4
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	lis		r6, 0x7000 | 128
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((4 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
4:	eciwx	r6, r5, r4
	andi.	r6, r6, 0b01
	bne		4b
	li		r6, 0
	stw		r6, _ata_transferred - 0x80000000 (r0)
#ifndef DMA
	b		1b
#else
	eciwx	r5, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 1*4
	li		r6, VAR_SECTOR_BUF
	ecowx	r6, r5, r4
	li		r5, 2*4
	li		r6, 512
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, 0b11
	ecowx	r6, r5, r4
#endif
5:	lis		r4, 0x0C00
	li		r5, 0x3000
	eciwx	r5, r5, r4
	rlwinm.	r6, r5, 0, 16, 14
	bne		7f
	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	rfi
#ifdef DMA
6:	li		r6, 512
	stw		r6, _ata_transferred - 0x80000000 (r0)
#endif
7:	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	ba		0x00000504
