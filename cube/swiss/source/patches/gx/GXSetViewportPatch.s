#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetViewportPatch
GXSetViewportPatch:
	lis		%r9, VAR_AREA
	lis		%r8, 0
	lbz		%r10, VAR_CURRENT_FIELD (%r9)
	addi	%r9, %r8, 0
	lfs		%f11, -12 (%r9)
	cmpwi	%r10, 0
	bne		1f
	fadds	%f2, %f2, %f11
1:	lfs		%f0, -16 (%r9)
	fmuls	%f11, %f11, %f4
	lfs		%f12, -4 (%r9)
	lis		%r10, 5
	lfs		%f10, -8 (%r9)
	fmuls	%f3, %f0, %f3
	lis		%r9, 0xCC01
	addi	%r10, %r10, 0x101A
	fmuls	%f6, %f12, %f6
	li		%r8, 0x10
	stb		%r8, -0x8000 (%r9)
	fadds	%f1, %f10, %f1
	stw		%r10, -0x8000 (%r9)
	fmadds	%f0, %f0, %f4, %f10
	stfs	%f3, -0x8000 (%r9)
	fnmsubs	%f12, %f12, %f5, %f6
	stfs	%f11, -0x8000 (%r9)
	fadds	%f1, %f1, %f3
	fadds	%f0, %f0, %f2
	stfs	%f12, -0x8000 (%r9)
	stfs	%f1, -0x8000 (%r9)
	stfs	%f0, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	blr
	.long	0x3F000000
	.long	0xBF000000
	.long	0x43AB0AAB
	.long	0x4B7FFFFF

.globl GXSetViewportPatch_length
GXSetViewportPatch_length:
.long (GXSetViewportPatch_length - GXSetViewportPatch)