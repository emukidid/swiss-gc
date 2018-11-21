#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetViewportJitterPatch
GXSetViewportJitterPatch:
	lwz		%r10, 0 (0)
	lis		%r8, VAR_AREA
	lis		%r7, 0
	stfs	%f1, 1084 (%r10)
	addi	%r9, %r7, 0
	stfs	%f2, 1088 (%r10)
	stfs	%f3, 1092 (%r10)
	stfs	%f4, 1096 (%r10)
	stfs	%f5, 1100 (%r10)
	stfs	%f6, 1104 (%r10)
	lfs		%f10, -12 (%r9)
	lbz		%r8, VAR_CURRENT_FIELD (%r8)
	cmpwi	%r8, 0
	bne		1f
	fadds	%f2, %f2, %f10
1:	lfs		%f0, -16 (%r9)
	fmuls	%f10, %f10, %f4
	lfs		%f11, -4 (%r9)
	lis		%r8, 5
	fmuls	%f3, %f0, %f3
	lfs		%f12, -8 (%r9)
	addi	%r8, %r8, 0x101A
	lis		%r9, 0xCC01
	fmuls	%f6, %f11, %f6
	li		%r6, 0x10
	li		%r7, 1
	stb		%r6, -0x8000 (%r9)
	fmadds	%f0, %f0, %f4, %f12
	stw		%r8, -0x8000 (%r9)
	fadds	%f12, %f12, %f3
	stfs	%f3, -0x8000 (%r9)
	fnmsubs	%f11, %f11, %f5, %f6
	stfs	%f10, -0x8000 (%r9)
	fadds	%f0, %f0, %f2
	fadds	%f12, %f12, %f1
	stfs	%f11, -0x8000 (%r9)
	stfs	%f12, -0x8000 (%r9)
	stfs	%f0, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	sth		%r7, 2 (%r10)
	blr
	.long	0x3F000000
	.long	0xBF000000
	.long	0x43AB0AAB
	.long	0x4B7FFFFF

.globl GXSetViewportJitterPatch_length
GXSetViewportJitterPatch_length:
.long (GXSetViewportJitterPatch_length - GXSetViewportJitterPatch)