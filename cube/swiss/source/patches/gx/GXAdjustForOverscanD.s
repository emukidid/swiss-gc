#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXAdjustForOverscanD
GXAdjustForOverscanD:
	stwu	%sp, -40 (%sp)
	stmw	%r29, 28 (%sp)
	slwi	%r30, %r5, 1
	slwi	%r31, %r6, 1
	cmplw	%r3, %r4
	beq		2f
	subi	%r9, %r4, 8
	subi	%r8, %r3, 8
	li		%r0, 7
	mtctr	%r0
1:	lwzu	%r7, 8 (%r8)
	lwz		%r0, 4 (%r8)
	stwu	%r7, 8 (%r9)
	stw		%r0, 4 (%r9)
	bdnz	1b
	lwz		%r0, 8 (%r8)
	stw		%r0, 8 (%r9)
2:	lhz		%r0, 4 (%r3)
	sub		%r0, %r0, %r30
	sth		%r0, 4 (%r4)
	clrlwi	%r7, %r31, 16
	lhz		%r0, 6 (%r3)
	mullw	%r7, %r7, %r0
	lhz		%r0, 8 (%r3)
	divwu	%r29, %r7, %r0
	lhz		%r0, 6 (%r3)
	sub		%r0, %r0, %r29
	sth		%r0, 6 (%r4)
	lwz		%r0, 20 (%r3)
	cmpwi	%r0, 0
	bne		3f
	lwz		%r0, 0 (%r3)
	rlwinm	%r0, %r0, 0, 30, 30
	cmpwi	%r0, 2
	beq		3f
	lhz		%r0, 8 (%r3)
	sub		%r0, %r0, %r6
	sth		%r0, 8 (%r4)
	b		4f
3:	lhz		%r0, 8 (%r3)
	sub		%r0, %r0, %r31
	sth		%r0, 8 (%r4)
4:	lhz		%r0, 14 (%r3)
	sub		%r0, %r0, %r30
	sth		%r0, 14 (%r4)
	lhz		%r0, 16 (%r3)
	sub		%r0, %r0, %r31
	sth		%r0, 16 (%r4)
	lhz		%r0, 10 (%r3)
	add		%r0, %r0, %r5
	sth		%r0, 10 (%r4)
	lhz		%r0, 12 (%r3)
	add		%r0, %r0, %r6
	sth		%r0, 12 (%r4)
	lmw		%r29, 28 (%sp)
	addi	%sp, %sp, 40
	blr

.globl GXAdjustForOverscanD_length
GXAdjustForOverscanD_length:
.long (GXAdjustForOverscanD_length - GXAdjustForOverscanD)