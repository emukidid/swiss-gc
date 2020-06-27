/* 
 * Copyright (c) 2019, Extrems <extrems@extremscorner.org>
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
	.globl memcpy
memcpy:
	cmplwi	r5, 0
	beqlr
	subi	r5, r5, 1
	srwi.	r6, r5, 5
	mtctr	r6
	clrlwi	r5, r5, 27
	addi	r5, r5, 1
	mtxer	r5
	mr		r0, r3
	beq		2f
1:	lswi	r5, r4, 32
	stswi	r5, r3, 32
	addi	r4, r4, 32
	addi	r3, r3, 32
	bdnz	1b
2:	lswi	r5, r4, 32
	stswx	r5, r0, r3
	mr		r3, r0
	blr
