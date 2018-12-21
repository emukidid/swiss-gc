#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXAdjustForOverscan2
GXAdjustForOverscan2:
	cmplw	%r3, %r4
	clrlwi	%r0, %r5, 16
	clrlwi	%r7, %r6, 16
	rlwinm	%r5, %r5, 1, 16, 30
	rlwinm	%r6, %r6, 1, 16, 30
	beq		1f
	lwz		%r9, 0 (%r3)
	lwz		%r8, 4 (%r3)
	stw		%r9, 0 (%r4)
	stw		%r8, 4 (%r4)
	lwz		%r9, 8 (%r3)
	lwz		%r8, 12 (%r3)
	stw		%r9, 8 (%r4)
	stw		%r8, 12 (%r4)
	lwz		%r9, 16 (%r3)
	lwz		%r8, 20 (%r3)
	stw		%r9, 16 (%r4)
	stw		%r8, 20 (%r4)
	lwz		%r9, 24 (%r3)
	lwz		%r8, 28 (%r3)
	stw		%r9, 24 (%r4)
	stw		%r8, 28 (%r4)
	lwz		%r9, 32 (%r3)
	lwz		%r8, 36 (%r3)
	stw		%r9, 32 (%r4)
	stw		%r8, 36 (%r4)
	lwz		%r9, 40 (%r3)
	lwz		%r8, 44 (%r3)
	stw		%r9, 40 (%r4)
	stw		%r8, 44 (%r4)
	lwz		%r9, 48 (%r3)
	lwz		%r8, 52 (%r3)
	stw		%r9, 48 (%r4)
	stw		%r8, 52 (%r4)
	lwz		%r8, 56 (%r3)
	stw		%r8, 56 (%r4)
1:	lhz		%r8, 4 (%r3)
	lwz		%r9, 0 (%r3)
	sub		%r8, %r8, %r5
	sth		%r8, 4 (%r4)
	clrlwi	%r11, %r9, 30
	lhz		%r10, 6 (%r3)
	lhz		%r8, 8 (%r3)
	mullw	%r9, %r6, %r10
	divwu	%r8, %r9, %r8
	sub		%r8, %r10, %r8
	sth		%r8, 6 (%r4)
	lwz		%r8, 20 (%r3)
	cmpwi	%r8, 0
	bne		2f
	cmplwi	%r11, 0
	bne		2f
	srawi	%r9, %r6, 1
	lhz		%r8, 8 (%r3)
	addze	%r9, %r9
	sub		%r8, %r8, %r9
	sth		%r8, 8 (%r4)
	b		3f
2:	lhz		%r8, 8 (%r3)
	sub		%r8, %r8, %r6
	sth		%r8, 8 (%r4)
3:	lhz		%r8, 14 (%r3)
	cmplwi	%r11, 1
	sub		%r5, %r8, %r5
	sth		%r5, 14 (%r4)
	bne		4f
	lhz		%r5, 16 (%r3)
	slwi	%r6, %r6, 1
	sub		%r5, %r5, %r6
	sth		%r5, 16 (%r4)
	b		5f
4:	lhz		%r5, 16 (%r3)
	sub		%r5, %r5, %r6
	sth		%r5, 16 (%r4)
5:	lhz		%r5, 10 (%r3)
	add		%r0, %r5, %r0
	sth		%r0, 10 (%r4)
	lhz		%r0, 12 (%r3)
	add		%r0, %r0, %r7
	sth		%r0, 12 (%r4)
	blr

.globl GXAdjustForOverscan2_length
GXAdjustForOverscan2_length:
.long (GXAdjustForOverscan2_length - GXAdjustForOverscan2)