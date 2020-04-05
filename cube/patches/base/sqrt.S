/* 
 * Copyright (c) 2012, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .rodata.cst4,"aM",@progbits,4
	.align 2
.LC0:	/* 0.5 */
	.long 0x3f000000

.section .text
	.globl sqrt
sqrt:
	frsqrte	fr13, fr1
	lis		r9, .LC0@ha
	lfs		fr0, .LC0@l (r9)
	fres	fr10, fr13
	fmul	fr11, fr1, fr0
	fmul	fr12, fr13, fr13
	fnmsubs	fr12, fr12, fr11, fr0
	fmadds	fr13, fr13, fr12, fr13
	fmul	fr12, fr13, fr13
	fnmsub	fr11, fr12, fr11, fr0
	fmadd	fr13, fr13, fr11, fr13
	fmul	fr12, fr13, fr1
	fnmsub	fr11, fr12, fr12, fr1
	fmul	fr0, fr11, fr0
	fmadd	fr13, fr0, fr13, fr12
	fsel	fr1, fr13, fr13, fr10
	blr
