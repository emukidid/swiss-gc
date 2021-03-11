#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetCopyFilterPatch
GXSetCopyFilterPatch:
	cmpwi	%cr5, %r5, 0
	li		%r5, 64
	beq		%cr5, 2f
	li		%r5, 0
	li		%r0, 7
	mtctr	%r0
1:	lbz		%r0, 0 (%r6)
	add		%r5, %r5, %r0
	addi	%r6, %r6, 1
	bdnz	1b
2:	lis		%r12, 0xCC01
	li		%r0, 0x61
	li		%r6, 0x6666
	oris	%r9, %r6, 0x0466
	oris	%r8, %r6, 0x0366
	oris	%r7, %r6, 0x0266
	oris	%r6, %r6, 0x0166
	cmpwi	%r3, 0
	beq		3f
	lwz		%r10,  0 (%r4)
	lwz		%r11, 12 (%r4)
	rlwimi	%r6, %r10,  8, 28, 31
	rlwimi	%r8, %r11,  8, 28, 31
	rlwimi	%r6, %r10, 20, 24, 27
	rlwimi	%r8, %r11, 20, 24, 27
	rlwimi	%r6, %r10,  0, 20, 23
	rlwimi	%r8, %r11,  0, 20, 23
	rlwimi	%r6, %r10, 12, 16, 19
	rlwimi	%r8, %r11, 12, 16, 19
	lwz		%r10,  4 (%r4)
	lwz		%r11, 16 (%r4)
	rlwimi	%r6, %r10, 24, 12, 15
	rlwimi	%r8, %r11, 24, 12, 15
	rlwimi	%r6, %r10,  4,  8, 11
	rlwimi	%r8, %r11,  4,  8, 11
	rlwimi	%r7, %r10, 24, 28, 31
	rlwimi	%r9, %r11, 24, 28, 31
	rlwimi	%r7, %r10,  4, 24, 27
	rlwimi	%r9, %r11,  4, 24, 27
	lwz		%r10,  8 (%r4)
	lwz		%r11, 20 (%r4)
	rlwimi	%r7, %r10, 16, 20, 23
	rlwimi	%r9, %r11, 16, 20, 23
	rlwimi	%r7, %r10, 28, 16, 19
	rlwimi	%r9, %r11, 28, 16, 19
	rlwimi	%r7, %r10,  8, 12, 15
	rlwimi	%r9, %r11,  8, 12, 15
	rlwimi	%r7, %r10, 20,  8, 11
	rlwimi	%r9, %r11, 20,  8, 11
3:	stb		%r0, -0x8000 (%r12)
	stw		%r6, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r7, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r8, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r9, -0x8000 (%r12)
	li		%r10, 0x1516
	lis		%r11, 0x1500
	beq		%cr5, 4f
	lis		%r4, VAR_AREA
	lbz		%r3, VAR_VFILTER_ON (%r4)
	cmpwi	%r3, 0
	beq		4f
	addi	%r4, %r4, VAR_VFILTER
	lswi	%r10, %r4, 7
4:	lis		%r3, 0x5300
	lis		%r4, 0x5400
	rlwinm	%r6, %r10, 24, 10, 15
	rlwinm	%r8, %r11, 24, 10, 15
	rlwinm	%r7, %r10,  8, 10, 15
	rlwinm	%r9, %r11,  8, 10, 15
	rlwimi	%r6, %r10, 16, 26, 31
	rlwimi	%r8, %r11, 16, 26, 31
	rlwimi	%r7, %r10,  0, 26, 31
	mullw	%r6, %r6, %r5
	mullw	%r8, %r8, %r5
	mullw	%r7, %r7, %r5
	mullw	%r9, %r9, %r5
	addis	%r6, %r6, 32
	addis	%r8, %r8, 32
	addi	%r6, %r6, 32
	addi	%r8, %r8, 32
	addis	%r7, %r7, 32
	addis	%r9, %r9, 32
	addi	%r7, %r7, 32
	rlwimi	%r3, %r6, 10, 26, 31
	rlwimi	%r4, %r8, 10, 26, 31
	rlwimi	%r3, %r6,  0, 20, 25
	rlwimi	%r4, %r8,  0, 20, 25
	rlwimi	%r3, %r7, 22, 14, 19
	rlwimi	%r4, %r9, 22, 14, 19
	rlwimi	%r3, %r7, 12,  8, 13
	stb		%r0, -0x8000 (%r12)
	stw		%r3, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r4, -0x8000 (%r12)
	blr

.globl GXSetCopyFilterPatch_length
GXSetCopyFilterPatch_length:
.long (GXSetCopyFilterPatch_length - GXSetCopyFilterPatch)