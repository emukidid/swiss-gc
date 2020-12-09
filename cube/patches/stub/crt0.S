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

.section .init
	.globl _start
_start:
	mfmsr	r3
	b		1f
	.asciz	"STUBHAXX"
	.align 2

1:	li		r3, startup
	mtsrr0	r3
	li		r3, 0
	mtsrr1	r3
	rfi

startup:
	lis		r0, 0x8000
	ori		r0, r0, 0x000C
	mtear	r0

	li		r0, 0
	li		sp, 0x3000
	stwu	r0, -8 (sp)

	bl		_main

	li		r0, 0
	lis		r3, 0x0C00
	addi	r3, r3, 0x3000 + 9*4
	ecowx	r0, r0, r3
	b		.
