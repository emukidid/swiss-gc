/* 
 * Copyright (c) 2022, Extrems <extrems@extremscorner.org>
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
	.globl memmove
memmove:
#ifndef BACKWARDS_MEMCPY
	sub		r0, r3, r4
	cmplw	r0, r5
	blt		3f
	srwi.	r6, r5, 5
	clrlwi	r5, r5, 27
	mtctr	r6
	mtxer	r5
	mr		r0, r3
	beq		2f
1:	lswi	r5, r4, 32
	stswi	r5, r3, 32
	addi	r4, r4, 32
	addi	r3, r3, 32
	bdnz	1b
2:	lswx	r5, r0, r4
	stswx	r5, r0, r3
	mr		r3, r0
	blr
#endif
#ifndef MEMCPY
3:	add		r4, r4, r5
	add		r3, r3, r5
	srwi.	r6, r5, 5
	clrlwi	r5, r5, 27
	mtctr	r6
	mtxer	r5
	sub		r4, r4, r5
	sub		r3, r3, r5
	lswx	r5, r0, r4
	stswx	r5, r0, r3
	beqlr
4:	subi	r4, r4, 32
	subi	r3, r3, 32
	lswi	r5, r4, 32
	stswi	r5, r3, 32
	bdnz	4b
	blr
#else
3:	b		backwards_memcpy
#endif

	.globl memcpy
	.set memcpy, memmove
