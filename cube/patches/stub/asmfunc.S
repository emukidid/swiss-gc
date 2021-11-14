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

.section .text
	.globl init_spi
init_spi:
#ifdef APPLOADER
	lis		r3, EXIRegs@ha
	lwz		r3, EXIRegs@l (r3)
#else
	lwz		r3, EXIRegs (r0)
#endif
	li		r4, 3*4
1:	eciwx	r0, r4, r3
	andi.	r0, r0, 0b01
	bne		1b
	blr

	.globl select
select:
#ifdef APPLOADER
	lis		r3, EXIRegs@ha
	lwz		r3, EXIRegs@l (r3)
#else
	lwz		r3, EXIRegs (r0)
#endif
	li		r0, ((1 << 0) << 7) | (4 << 4)
	ecowx	r0, r0, r3
	b		rcv_spi

	.globl deselect
deselect:
#ifdef APPLOADER
	lis		r3, EXIRegs@ha
	lwz		r3, EXIRegs@l (r3)
#else
	lwz		r3, EXIRegs (r0)
#endif
	li		r0, 0
	ecowx	r0, r0, r3

	.globl rcv_spi
rcv_spi:
	li		r3, 0xFF

	.globl xmit_spi
xmit_spi:
#ifdef APPLOADER
	lis		r4, EXIRegs@ha
	lwz		r4, EXIRegs@l (r4)
#else
	lwz		r4, EXIRegs (r0)
#endif
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

	.globl dvd_read
dvd_read:
	addi	r4, r4, 31
	rlwinm	r3, r3, 0, 6, 26
	rlwinm.	r4, r4, 0, 6, 26
	srwi	r5, r5, 2
	beq		3f
	li		r6, 0
	srwi	r7, r4, 5
	mtctr	r7
1:	dcbi	r6, r3
	addi	r6, r6, 32
	bdnz	1b
	lis		r0, 0xA800
	lis		r6, 0x0C00
	li		r7, 0x6000 + 2*4
	ecowx	r0, r7, r6
	li		r7, 0x6000 + 3*4
	ecowx	r5, r7, r6
	li		r7, 0x6000 + 4*4
	ecowx	r4, r7, r6
	li		r7, 0x6000 + 5*4
	ecowx	r3, r7, r6
	li		r7, 0x6000 + 6*4
	ecowx	r4, r7, r6
	li		r7, 0x6000 + 7*4
	li		r0, 0b011
	ecowx	r0, r7, r6
2:	eciwx	r0, r7, r6
	andi.	r0, r0, 0b001
	bne		2b
	li		r7, 0x6000 + 5*4
	eciwx	r0, r7, r6
	subf	r3, r3, r0
	blr
3:	li		r3, 0
	blr
