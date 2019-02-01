#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetCopyFilterPatch
GXSetCopyFilterPatch:
	lis		%r12, 0xCC01
	li		%r0, 0x61
	li		%r6, 0x6666
	oris	%r9, %r6, 0x0466
	oris	%r8, %r6, 0x0366
	oris	%r7, %r6, 0x0266
	oris	%r6, %r6, 0x0166
	cmpwi	%r3, 0
	beq		1f
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
1:	stb		%r0, -0x8000 (%r12)
	stw		%r6, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r7, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r8, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r9, -0x8000 (%r12)
	li		%r10, 0x1516
	lis		%r11, 0x1500
	cmpwi	%r5, 0
	beq		2f
	lis		%r4, VAR_AREA
	lbz		%r3, VAR_VFILTER_ON (%r4)
	cmpwi	%r3, 0
	beq		2f
	addi	%r4, %r4, VAR_VFILTER
	lswi	%r10, %r4, 7
2:	lis		%r9, 0x5400
	lis		%r8, 0x5300
	rlwimi	%r8, %r10,  8, 26, 31
	rlwimi	%r9, %r11,  8, 26, 31
	rlwimi	%r8, %r10, 22, 20, 25
	rlwimi	%r9, %r11, 22, 20, 25
	rlwimi	%r8, %r10,  4, 14, 19
	rlwimi	%r9, %r11,  4, 14, 19
	rlwimi	%r8, %r10, 18,  8, 13
	stb		%r0, -0x8000 (%r12)
	stw		%r8, -0x8000 (%r12)
	stb		%r0, -0x8000 (%r12)
	stw		%r9, -0x8000 (%r12)
	blr

.globl GXSetCopyFilterPatch_length
GXSetCopyFilterPatch_length:
.long (GXSetCopyFilterPatch_length - GXSetCopyFilterPatch)