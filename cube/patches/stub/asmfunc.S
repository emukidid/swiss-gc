/* 
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
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
	lwz		r3, EXIRegs (r0)
	li		r4, 3*4
1:	eciwx	r0, r4, r3
	andi.	r0, r0, 0b01
	bne		1b
	blr

	.globl select
select:
	lwz		r3, EXIRegs (r0)
	li		r0, ((1 << 0) << 7) | (4 << 4)
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
