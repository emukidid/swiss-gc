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
	.globl setjmp
setjmp:
	stmw	r14, 56 (r3)
	mfcr	r0
	stw		r0, 128 (r3)
	mflr	r0
	stw		r0, 132 (r3)
	stw		sp, 4 (r3)
	li		r3, 0
	blr

	.globl longjmp
longjmp:
	lmw		r14, 56 (r3)
	lwz		r0, 128 (r3)
	mtcr	r0
	lwz		r0, 132 (r3)
	mtlr	r0
	lwz		sp, 4 (r3)
	mr.		r3, r4
	bnelr
	li		r3, 1
	blr
