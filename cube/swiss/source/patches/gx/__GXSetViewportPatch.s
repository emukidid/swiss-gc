#include "reservedarea.h"

.globl __GXSetViewportPatch
__GXSetViewportPatch:
	cmplwi	%r3, 1
	lfs		%f12, 0 (0)
	ble		2f
1:	fmuls	%f4, %f4, %f12
	lfs		%f0, 0 (0)
	lis		%r10, 6 - 1
	lis		%r9, 0xCC01
	fmuls	%f3, %f3, %f12
	ori		%r10, %r10, 0x101A
	li		%r8, 0x10
	fmuls	%f6, %f8, %f6
	stb		%r8, -0x8000 (%r9)
	stw		%r10, -0x8000 (%r9)
	fadds	%f1, %f0, %f1
	stfs	%f3, -0x8000 (%r9)
	fadds	%f0, %f0, %f4
	fnmsubs	%f5, %f5, %f8, %f6
	fadds	%f1, %f1, %f3
	fadds	%f6, %f6, %f7
	fneg	%f4, %f4
	fadds	%f2, %f0, %f2
	stfs	%f4, -0x8000 (%r9)
	stfs	%f5, -0x8000 (%r9)
	stfs	%f1, -0x8000 (%r9)
	stfs	%f2, -0x8000 (%r9)
	stfs	%f6, -0x8000 (%r9)
	blr
2:	fmadds	%f2, %f12, %f12, %f2
	beq		1b
	fsubs	%f2, %f2, %f12
	b		1b

.globl __GXSetViewportPatchLength
__GXSetViewportPatchLength:
.long (. - __GXSetViewportPatch) / 4
