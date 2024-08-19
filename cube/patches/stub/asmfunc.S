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

.section .text
	.globl init_spi
init_spi:
	lwz		r3, EXIRegs (r0)
	li		r4, 3*4
1:	eciwx	r0, r4, r3
	andi.	r0, r0, 0b01
	bne		1b
	blr

	.globl select
select:
	lwz		r3, EXIRegs (r0)
	lhz		r4, EXICsb (r0)
	li		r0, (4 << 4)
	insrwi	r0, r4, 3, 22
	ecowx	r0, r0, r3
	b		rcv_spi

	.globl deselect
deselect:
	lwz		r3, EXIRegs (r0)
	li		r0, 0
	ecowx	r0, r0, r3

	.globl rcv_spi
rcv_spi:
	li		r3, 0xFF

	.globl xmit_spi
xmit_spi:
	lwz		r4, EXIRegs (r0)
	li		r5, 4*4
	slwi	r0, r3, 24
	ecowx	r0, r5, r4
	li		r6, 3*4
	li		r0, ((1 - 1) << 4) | 0b01
	ecowx	r0, r6, r4
1:	eciwx	r0, r6, r4
	andi.	r0, r0, 0b01
	bne		1b
	eciwx	r0, r5, r4
	srwi	r3, r0, 24
	blr

	.globl dvd_inquiry
dvd_inquiry:
	lis		r6, 0x1200
	li		r5, 0
	li		r4, 32
	b		dvd_readdma

	.globl dvd_read
dvd_read:
	lis		r6, 0xA800

dvd_readdma:
	addi	r4, r4, 31
	rlwinm	r3, r3, 0, 6, 26
	rlwinm.	r4, r4, 0, 6, 26
	srwi	r5, r5, 2
	beq		3f
	li		r7, 0
	srwi	r8, r4, 5
	mtctr	r8
1:	dcbi	r7, r3
	addi	r7, r7, 32
	bdnz	1b
	lis		r7, 0x0C00
	li		r8, 0x6000 + 7*4
	eciwx	r0, r8, r7
	andi.	r0, r0, 0b001
	bne		3f
	li		r8, 0x6000 + 2*4
	ecowx	r6, r8, r7
	li		r8, 0x6000 + 3*4
	ecowx	r5, r8, r7
	li		r8, 0x6000 + 4*4
	ecowx	r4, r8, r7
	li		r8, 0x6000 + 5*4
	ecowx	r3, r8, r7
	li		r9, 0x6000 + 6*4
	ecowx	r4, r9, r7
	li		r9, 0x6000 + 7*4
	li		r0, 0b011
	ecowx	r0, r9, r7
2:	eciwx	r0, r9, r7
	andi.	r0, r0, 0b001
	bne		2b
	eciwx	r0, r8, r7
	subf	r3, r3, r0
	blr
3:	li		r3, 0
	blr

	.globl flippy_reset
flippy_reset:
	lis		r3, 0x0C00
	li		r4, 0x6000 + 1*4
	li		r0, 0b100
	ecowx	r0, r4, r3
	li		r5, 0x6000 + 2*4
	lis		r0, 0xB500
	ori		r0, r0, 0x0005
	ecowx	r0, r5, r3
	li		r5, 0x6000 + 3*4
	lis		r0, 0xAA55
	ori		r0, r0, 0xF641
	ecowx	r0, r5, r3
	li		r5, 0x6000 + 7*4
	li		r0, 0b001
	ecowx	r0, r5, r3
1:	eciwx	r0, r5, r3
	andi.	r0, r0, 0b001
	bne		1b
2:	eciwx	r0, r4, r3
	andi.	r0, r0, 0b100
	beq		2b
	blr
