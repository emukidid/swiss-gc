/* 
 * Copyright (c) 2020-2024, Extrems <extrems@extremscorner.org>
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

.section .text.mmc_interrupt_vector,"ax",@progbits
	.balign 32
	.globl _mmc
_mmc:
_mmc_transferred:
	.long	512
#ifndef DMA
_mmc_data:
	.long	0
#endif
_mmc_buffer:
	.long	0
_mmc_registers:
	.long	0x0C006800

	.globl mmc_interrupt_vector
mmc_interrupt_vector:
	mtsprg	0, r4
	mtsprg	1, r5
	mtsprg	2, r6
	mfcr	r6
	mtsprg	3, r6
	lwz		r4, _mmc_registers - 0x80000000 (r0)
	eciwx	r5, r0, r4
	rlwinm	r6, r5, 1, 28, 28
	and.	r6, r5, r6
	lwz		r6, _mmc_transferred - 0x80000000 (r0)
	cmpwi	cr7, r6, 512 - 4
	cmpwi	cr6, r6, 0
	beq		7f
	bgt		cr7, 7f
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	eciwx	r6, r5, r4
	blt		cr6, 5f
#ifndef DMA
	stw		r6, _mmc_data - 0x80000000 (r0)
	li		r6, ~0
	ecowx	r6, r5, r4
	bnl		cr7, 4f
1:	li		r5, 3*4
	li		r6, ((4 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	li		r5, 0
	blt		cr6, 3f
2:	lwz		r4, _mmc_buffer - 0x80000000 (r0)
	lwz		r5, _mmc_transferred - 0x80000000 (r0)
	lwz		r6, _mmc_data - 0x80000000 (r0)
	ecowx	r6, r5, r4
	addi	r5, r5, 4
3:	stw		r5, _mmc_transferred - 0x80000000 (r0)
	b		6f
4:	li		r5, 3*4
	li		r6, ((2 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		2b
#else
	li		r6, ~0
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((2 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	li		r6, 512
	stw		r6, _mmc_transferred - 0x80000000 (r0)
	b		6f
1:	li		r5, 1*4
	lwz		r6, _mmc_buffer - 0x80000000 (r0)
	ecowx	r6, r5, r4
	li		r5, 2*4
	li		r6, 512
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, 0b11
	ecowx	r6, r5, r4
	li		r6, 0
	stw		r6, _mmc_transferred - 0x80000000 (r0)
	b		6f
#endif
5:	srwi	r6, r6, 24
	cmplwi	r6, 0xFE
	li		r6, ~0
	ecowx	r6, r5, r4
	beq		1b
	li		r5, 3*4
	li		r6, ((1 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
6:	lis		r4, 0x0C00
	li		r5, 0x3000
	eciwx	r5, r5, r4
	li		r6, 0x3000 + 1*4
	eciwx	r6, r6, r4
	and.	r6, r6, r5
	bne		7f
	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	rfi
7:	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	ba		0x00000504
