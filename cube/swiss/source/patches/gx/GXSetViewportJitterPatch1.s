#include "reservedarea.h"

.globl GXSetViewportJitterPatch1
GXSetViewportJitterPatch1:
	lwz		%r10, 0 (0)
	cmplwi	%r3, 1
	lis		%r9, 0
	stfs	%f1, 1084 (%r10)
	addi	%r9, %r9, 0
	stfs	%f2, 1088 (%r10)
	stfs	%f3, 1092 (%r10)
	stfs	%f4, 1096 (%r10)
	stfs	%f5, 1100 (%r10)
	stfs	%f6, 1104 (%r10)
	lfs		%f11, -12 (%r9)
	ble		2f
1:	fmuls	%f4, %f4, %f11
	lfs		%f12, -4 (%r9)
	lfs		%f0, -8 (%r9)
	lis		%r9, 6 - 1
	fmuls	%f3, %f3, %f11
	ori		%r8, %r9, 0x101A
	li		%r6, 0x10
	lis		%r9, 0xCC01
	fmuls	%f6, %f12, %f6
	li		%r7, 1
	stb		%r6, -0x8000 (%r9)
	fadds	%f11, %f0, %f3
	stw		%r8, -0x8000 (%r9)
	fadds	%f0, %f0, %f4
	stfs	%f3, -0x8000 (%r9)
	fneg	%f4, %f4
	fnmsubs	%f12, %f12, %f5, %f6
	fadds	%f11, %f11, %f1
	fadds	%f2, %f0, %f2
	stfs	%f4, -0x8000 (%r9)
	stfs	%f12, -0x8000 (%r9)
	stfs	%f11, -0x8000 (%r9)
	stfs	%f2, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	sth		%r7, 2 (%r10)
	blr
2:	fmadds	%f2, %f11, %f11, %f2
	beq		1b
	fsubs	%f2, %f2, %f11
	b		1b
	.long	0x3F000000
	.long	0x43AB0555
	.long	0x4B7FFFFF

.globl GXSetViewportJitterPatch1Length
GXSetViewportJitterPatch1Length:
.long (. - GXSetViewportJitterPatch1) / 4
