/* 
 * Copyright (c) 2022-2025, Extrems <extrems@extremscorner.org>
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
	.globl memmove
memmove:
#ifndef BACKWARDS_MEMCPY
	sub		r0, r3, r4
	cmplw	r0, r5
	blt		3f
	srwi.	r0, r5, 2
	clrlwi	r6, r5, 30
	mtctr	r0
	mtxer	r6
	li		r5, 0
	beq		2f
1:	lwzx	r0, r5, r4
	stwx	r0, r5, r3
	addi	r5, r5, 4
	bdnz	1b
2:	lswx	r0, r5, r4
	stswx	r0, r5, r3
	add		r5, r5, r6
	blr
#endif
#ifndef MEMCPY
3:	srwi.	r0, r5, 2
	clrlwi	r6, r5, 30
	mtctr	r0
	mtxer	r6
	sub		r5, r5, r6
	lswx	r0, r5, r4
	stswx	r0, r5, r3
	beqlr
4:	subi	r5, r5, 4
	lwzx	r0, r5, r4
	stwx	r0, r5, r3
	bdnz	4b
	blr
#else
3:	b		backwards_memcpy
#endif

	.globl memcpy
	.set memcpy, memmove
