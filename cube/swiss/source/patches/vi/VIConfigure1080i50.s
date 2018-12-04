#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure1080i50
VIConfigure1080i50:
	li			%r0, 20
	li			%r7, 1
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
	subic.		%r4, %r5, 480
	ble			2f
	li			%r0, 4
	subic.		%r4, %r5, 576
	ble			2f
	li			%r0, 22
	lhz			%r5, 8 (%r3)
	cmpwi		%r5, 480
	ble			1f
	li			%r0, 6
1:	subic		%r4, %r5, 540
2:	srawi		%r4, %r4, 1
	addze		%r4, %r4
	neg			%r4, %r4
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	stw			%r6, 20 (%r3)
	stb			%r7, 24 (%r3)
	stw			%r0, 0 (%r3)
	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	mtmsr		%r4
	extrwi		%r3, %r3, 1, 16
	blr

.globl VIConfigure1080i50_length
VIConfigure1080i50_length:
.long (VIConfigure1080i50_length - VIConfigure1080i50)