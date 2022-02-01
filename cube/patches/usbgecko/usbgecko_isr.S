/* 
 * Copyright (c) 2020-2022, Extrems <extrems@extremscorner.org>
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
	.globl _usb
_usb:
_usb_requested:
	.long	0
_usb_buffer:
	.long	0
_usb_registers:
	.long	0x0C006800 + 5*4

#ifdef MMC
	.globl _mmc
_mmc:
_mmc_transferred:
	.long	512
_mmc_registers:
	.long	0x0C006800
#endif

	.globl external_interrupt_vector
external_interrupt_vector:
	mtsprg	0, r4
	mtsprg	1, r5
	mtsprg	2, r6
	mfcr	r6
	mtsprg	3, r6
	lwz		r4, _usb_registers - 0x80000000 (r0)
	eciwx	r5, r0, r4
	rlwinm	r6, r5, 1, 28, 28
	and.	r6, r5, r6
	lwz		r5, _usb_requested - 0x80000000 (r0)
	lwz		r6, _usb_buffer - 0x80000000 (r0)
	cmpwi	cr7, r5, 1
	cmpwi	cr6, r6, 0
#ifdef MMC
	beq		3f
	blt		cr7, 3f
#else
	beq		8f
	blt		cr7, 8f
#endif
	li		r5, 4*4
	eciwx	r5, r5, r4
	bnl		cr6, 2f
	clrlwi	r6, r6, 1
	extrwi	r5, r5, 8, 8
	stb		r5, 0 (r6)
	addi	r6, r6, 1
	lwz		r5, _usb_requested - 0x80000000 (r0)
	stw		r6, _usb_buffer - 0x80000000 (r0)
	subi	r5, r5, 1
	stw		r5, _usb_requested - 0x80000000 (r0)
	beq		cr7, 8f
1:	eciwx	r5, r0, r4
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	lis		r6, 0xD000
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((1 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		7f
2:	rlwimi.	r6, r5, 5, 0, 0
	stw		r6, _usb_buffer - 0x80000000 (r0)
	bnl		1b
	eciwx	r5, r0, r4
	andi.	r6, r5, 0x405
	ecowx	r6, r0, r4
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	lis		r6, 0xA000
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((2 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
#ifdef MMC
	b		7f
3:	lwz		r4, _mmc_registers - 0x80000000 (r0)
	eciwx	r5, r0, r4
	rlwinm	r6, r5, 1, 28, 28
	and.	r6, r5, r6
	lwz		r6, _mmc_transferred - 0x80000000 (r0)
	cmpwi	cr7, r6, 512
	cmpwi	cr6, r6, 0
	beq		8f
	bnl		cr7, 8f
	andi.	r5, r5, (0x3FFF & ~0x80A) | (1 << 3)
	ecowx	r5, r0, r4
	li		r5, 4*4
	eciwx	r5, r5, r4
	blt		cr6, 6f
	stw		r5, VAR_SECTOR_BUF (r6)
	addi	r6, r6, 4
4:	cmpwi	cr7, r6, 512
	stw		r6, _mmc_transferred - 0x80000000 (r0)
	li		r5, 4*4
	li		r6, ~0
	ecowx	r6, r5, r4
	bnl		cr7, 5f
	li		r5, 3*4
	li		r6, ((4 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		7f
5:	li		r5, 3*4
	li		r6, ((2 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
	b		7f
6:	srwi	r5, r5, 24
	cmplwi	r5, 0xFE
	li		r6, 0
	beq		4b
	li		r5, 4*4
	li		r6, ~0
	ecowx	r6, r5, r4
	li		r5, 3*4
	li		r6, ((1 - 1) << 4) | 0b01
	ecowx	r6, r5, r4
#endif
7:	lis		r4, 0x0C00
	li		r5, 0x3000
	eciwx	r5, r5, r4
	rlwinm.	r6, r5, 0, 16, 14
	bne		8f
	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	rfi
8:	mfsprg	r6, 3
	mtcr	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	ba		0x00000504
