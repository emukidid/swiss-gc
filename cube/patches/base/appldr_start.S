# appldr_start.S for Swiss
# Copyright (C) 2015 FIX94

#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

#define HID0_ICFI	(1<<11)
#define rHID0	1008

.extern RunAppldr

.section .text
.globl appldr_start

appldr_start:
	# OSDisableInterrupts
	mfmsr	r3
	rlwinm	r3,		r3,		0, 17, 15
	mtmsr	r3

	# Setup stack in safe ipl area
	lis		r1,		0x8130
	addi	r1,		r1,		0x4000
	li		r0,		0
	stwu	r0,		-0x40(r1)

	# Go!
	bl		RunAppldr
	mr		r31,	r3

	# ICFlashInvalidate
	mfspr	r3,		rHID0
	ori		r3,		r3,		HID0_ICFI
	mtspr	rHID0,	r3
	sync
	isync

	# goto entrypoint
	mtlr	r31
	blr
