#include "reservedarea.h"

.globl GXSetViewportPatch
GXSetViewportPatch:
	lis		%r10, VAR_AREA
	lis		%r9, 0
	lbz		%r10, VAR_CURRENT_FIELD (%r10)
	addi	%r9, %r9, 0
	lfs		%f0, -12 (%r9)
	cmpwi	%r10, 0
	fmadds	%f2, %f0, %f0, %f2
	bne		1f
	fsubs	%f2, %f2, %f0
1:	fmuls	%f4, %f0, %f4
	lfs		%f11, -4 (%r9)
	lfs		%f12, -8 (%r9)
	lis		%r9, 6 - 1
	fmuls	%f0, %f0, %f3
	ori		%r10, %r9, 0x101A
	li		%r8, 0x10
	lis		%r9, 0xCC01
	fmuls	%f6, %f11, %f6
	stb		%r8, -0x8000 (%r9)
	stw		%r10, -0x8000 (%r9)
	fadds	%f1, %f12, %f1
	stfs	%f0, -0x8000 (%r9)
	fadds	%f12, %f12, %f4
	fnmsubs	%f11, %f11, %f5, %f6
	fneg	%f4, %f4
	fadds	%f1, %f1, %f0
	fadds	%f12, %f12, %f2
	stfs	%f4, -0x8000 (%r9)
	stfs	%f11, -0x8000 (%r9)
	stfs	%f1, -0x8000 (%r9)
	stfs	%f12, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	blr
	.long	0x3F000000
	.long	0x43AB0555
	.long	0x4B7FFFFF

.globl GXSetViewportPatchLength
GXSetViewportPatchLength:
.long (. - GXSetViewportPatch) / 4
