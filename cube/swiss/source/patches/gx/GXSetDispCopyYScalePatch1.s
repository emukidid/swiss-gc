#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetDispCopyYScalePatch1
GXSetDispCopyYScalePatch1:
	fctiwz	%f1, %f1
	stwu	%sp, -16 (%sp)
	stfd	%f1, 8 (%sp)
	lwz		%r5, 12 (%sp)
	li		%r4, 0x100
	divw.	%r4, %r4, %r5
	lwz		%r9, 0 (0)
	lwz		%r8, 492 (%r9)
	rlwinm	%r8, %r8, 0, 22, 20
	lwz		%r3, 484 (%r9)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	ble		1f
	slwi	%r3, %r3, 8
	divwu	%r3, %r3, %r4
	lis		%r7, 0xCC01
	li		%r6, 0x61
	oris	%r5, %r4, 0x4E00
	stb		%r6, -0x8000 (%r7)
	stw		%r5, -0x8000 (%r7)
	ori		%r8, %r8, 0x400
1:	stw		%r8, 492 (%r9)
	addi	%sp, %sp, 16
	blr

.globl GXSetDispCopyYScalePatch1_length
GXSetDispCopyYScalePatch1_length:
.long (GXSetDispCopyYScalePatch1_length - GXSetDispCopyYScalePatch1)