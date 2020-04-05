/* 
 * Copyright (c) 2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
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
