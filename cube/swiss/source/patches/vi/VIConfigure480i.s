#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure480i
VIConfigure480i:
	li			%r0, 0
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
	cmpwi		%r5, 480
	ble			2f
	li			%r6, 1
	lhz			%r5, 8 (%r3)
	cmpwi		%r5, 480
	ble			2f
	lhz			%r5, 6 (%r3)
	clrrwi		%r5, %r5, 1
	cmpwi		%r5, 480
	ble			1f
	li			%r5, 480
	sth			%r5, 6 (%r3)
1:	sth			%r5, 8 (%r3)
2:	subfic		%r4, %r5, 480
	srwi		%r4, %r4, 1
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	stw			%r6, 20 (%r3)
	stw			%r0, 0 (%r3)
	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	mtmsr		%r4
	extrwi		%r3, %r3, 1, 16
	blr

.globl VIConfigure480i_length
VIConfigure480i_length:
.long (VIConfigure480i_length - VIConfigure480i)