#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXAdjustForOverscanPatch
GXAdjustForOverscanPatch:
	lwz		%r0, 0 (%r3)
	stw		%r0, 0 (%r4)
	lhz		%r8, 6 (%r3)
	lhz		%r9, 8 (%r3)
	cmpwi	%r9, 480
	ble		2f
	clrrwi	%r6, %r6, 2
	sub		%r7, %r9, %r8
	cmpwi	%r8, 480
	mr		%r8, %r9
	ble		1f
	subi	%r7, %r8, 480
1:	srwi	%r7, %r7, 1
	add		%r6, %r6, %r7
2:	slwi	%r7, %r6, 1
	mullw	%r10, %r8, %r7
	divwu	%r10, %r10, %r9
	sub		%r8, %r8, %r10
	lwz		%r10, 20 (%r3)
	cmpwi	%r10, 0
	bne		3f
	andi.	%r0, %r0, 2
	beq		4f
3:	sub		%r9, %r9, %r6
4:	sub		%r9, %r9, %r6
	sth		%r8, 6 (%r4)
	sth		%r9, 8 (%r4)
	lhz		%r8, 12 (%r3)
	lhz		%r9, 16 (%r3)
	add		%r8, %r8, %r6
	sub		%r9, %r9, %r7
	sth		%r8, 12 (%r4)
	sth		%r9, 16 (%r4)
	stw		%r10, 20 (%r4)
	slwi	%r6, %r5, 1
	lhz		%r7, 4 (%r3)
	lhz		%r8, 10 (%r3)
	lhz		%r9, 14 (%r3)
	lwz		%r0, 24 (%r3)
	sub		%r7, %r7, %r6
	add		%r8, %r8, %r5
	sub		%r9, %r9, %r6
	sth		%r7, 4 (%r4)
	sth		%r8, 10 (%r4)
	sth		%r9, 14 (%r4)
	stw		%r0, 24 (%r4)
	addi	%r3, %r3, 28
	addi	%r4, %r4, 28
	lswi	%r5, %r3, 32
	stswi	%r5, %r4, 32
	blr

.globl GXAdjustForOverscanPatch_length
GXAdjustForOverscanPatch_length:
.long (GXAdjustForOverscanPatch_length - GXAdjustForOverscanPatch)