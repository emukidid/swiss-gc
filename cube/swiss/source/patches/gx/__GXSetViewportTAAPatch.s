#include "reservedarea.h"

.globl __GXSetViewportTAAPatch
__GXSetViewportTAAPatch:
	cmplwi	%r3, 1
	lis		%r9, 0
	addi	%r9, %r9, 0
	lfs		%f11, -12 (%r9)
	ble		2f
1:	fmuls	%f3, %f3, %f11
	lfs		%f12, -4 (%r9)
	lfs		%f0, -8 (%r9)
	lis		%r9, 6 - 1
	fmuls	%f4, %f4, %f11
	ori		%r10, %r9, 0x101A
	li		%r8, 0x10
	lis		%r9, 0xCC01
	fmuls	%f6, %f12, %f6
	stb		%r8, -0x8000 (%r9)
	stw		%r10, -0x8000 (%r9)
	fadds	%f2, %f0, %f2
	stfs	%f3, -0x8000 (%r9)
	fadds	%f0, %f0, %f3
	fneg	%f11, %f4
	fnmsubs	%f12, %f12, %f5, %f6
	fadds	%f2, %f2, %f4
	fadds	%f1, %f0, %f1
	stfs	%f11, -0x8000 (%r9)
	stfs	%f12, -0x8000 (%r9)
	stfs	%f1, -0x8000 (%r9)
	stfs	%f2, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	blr
2:	fmadds	%f1, %f11, %f11, %f1
	beq		1b
	fsubs	%f1, %f1, %f11
	b		1b
	.long	0x3F000000
	.long	0x43AB0555
	.long	0x4B7FFFFF

.globl __GXSetViewportTAAPatchLength
__GXSetViewportTAAPatchLength:
.long (. - __GXSetViewportTAAPatch) / 4
