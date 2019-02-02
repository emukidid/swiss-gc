# appldr_start.S for Swiss
# Copyright (C) 2015 FIX94

#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

#define HID0_ICFI	(1<<11)
#define rHID0	1008

.extern RunAppldr

.section .text
	.globl _start, __main
_start:
	# OSDisableInterrupts
	mfmsr	r4
	rlwinm	r4,		r4,		0, 17, 15
	mtmsr	r4

	# Setup stack in safe ipl area
	lis		r1,		0x8130
	addi	r1,		r1,		0x4000
	li		r0,		0
	stwu	r0,		-0x40(r1)

	# Go!
	bl		RunAppldr
	mr		r31,	r3

	# ICFlashInvalidate
	mfspr	r4,		rHID0
	ori		r4,		r4,		HID0_ICFI
	mtspr	rHID0,	r4
	sync
	isync

	# goto entrypoint
	mtlr	r31
	blr

	.globl dcache_store
dcache_store:
	addi	r4, r4, 0x1F
	srwi	r4, r4, 5
	mtctr	r4
	li		r0, 0
dcache_store_loop:
	dcbst	r0, r3
	addi	r3, r3, 0x20
	bdnz	dcache_store_loop
	sync
	blr
	
	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		r4, r4, 0x20 
1:
	addi 		r4, r4, 0x1f
	srwi 		r4, r4, 5
	mtctr 		r4
2:
	dcbf 		r0, r3
	icbi		r0, r3		#todo kill this off
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr