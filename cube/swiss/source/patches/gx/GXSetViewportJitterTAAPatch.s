#include "reservedarea.h"

.globl GXSetViewportJitterTAAPatch
GXSetViewportJitterTAAPatch:
	lwz		%r10, 0 (0)
	lis		%r8, VAR_AREA
	lis		%r9, 0
	stfs	%f1, 1084 (%r10)
	addi	%r9, %r9, 0
	stfs	%f2, 1088 (%r10)
	stfs	%f3, 1092 (%r10)
	stfs	%f4, 1096 (%r10)
	stfs	%f5, 1100 (%r10)
	stfs	%f6, 1104 (%r10)
	lbz		%r8, VAR_CURRENT_FIELD (%r8)
	lfs		%f0, -12 (%r9)
	cmpwi	%r8, 0
	fmadds	%f1, %f0, %f0, %f1
	bne		1f
	fsubs	%f1, %f1, %f0
1:	fmuls	%f3, %f0, %f3
	lfs		%f11, -4 (%r9)
	lfs		%f12, -8 (%r9)
	lis		%r9, 6 - 1
	fmuls	%f0, %f0, %f4
	ori		%r8, %r9, 0x101A
	li		%r6, 0x10
	lis		%r9, 0xCC01
	fmuls	%f6, %f11, %f6
	li		%r7, 1
	stb		%r6, -0x8000 (%r9)
	fadds	%f10, %f12, %f3
	stw		%r8, -0x8000 (%r9)
	fadds	%f12, %f12, %f0
	stfs	%f3, -0x8000 (%r9)
	fneg	%f0, %f0
	fnmsubs	%f11, %f11, %f5, %f6
	fadds	%f10, %f10, %f1
	fadds	%f2, %f12, %f2
	stfs	%f0, -0x8000 (%r9)
	stfs	%f11, -0x8000 (%r9)
	stfs	%f10, -0x8000 (%r9)
	stfs	%f2, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	sth		%r7, 2 (%r10)
	blr
	.long	0x3F000000
	.long	0x43AB0555
	.long	0x4B7FFFFF

.globl GXSetViewportJitterTAAPatchLength
GXSetViewportJitterTAAPatchLength:
.long (. - GXSetViewportJitterTAAPatch) / 4
