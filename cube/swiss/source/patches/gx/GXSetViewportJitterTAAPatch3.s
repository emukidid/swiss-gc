#include "reservedarea.h"

.globl GXSetViewportJitterTAAPatch3
GXSetViewportJitterTAAPatch3:
	cmplwi	%r3, 1
	lwz		%r9, 0 (0)
	lfs		%f12, 0 (0)
	lfs		%f11, 1292 (%r9)
	lfs		%f10, 1296 (%r9)
	stfs	%f1, 1268 (%r9)
	stfs	%f2, 1272 (%r9)
	stfs	%f3, 1276 (%r9)
	stfs	%f4, 1280 (%r9)
	stfs	%f5, 1284 (%r9)
	stfs	%f6, 1288 (%r9)
	ble		2f
1:	fmuls	%f4, %f4, %f12
	lfs		%f0, 0 (0)
	lis		%r10, 6 - 1
	lis		%r9, 0xCC01
	fmuls	%f3, %f3, %f12
	ori		%r10, %r10, 0x101A
	li		%r8, 0x10
	fmuls	%f6, %f6, %f10
	stb		%r8, -0x8000 (%r9)
	stw		%r10, -0x8000 (%r9)
	fadds	%f12, %f0, %f3
	stfs	%f3, -0x8000 (%r9)
	fadds	%f0, %f0, %f4
	fnmsubs	%f5, %f5, %f10, %f6
	fadds	%f11, %f11, %f6
	fadds	%f12, %f12, %f1
	fneg	%f4, %f4
	fadds	%f2, %f0, %f2
	stfs	%f4, -0x8000 (%r9)
	stfs	%f5, -0x8000 (%r9)
	stfs	%f12, -0x8000 (%r9)
	stfs	%f2, -0x8000 (%r9)
	stfs	%f11, -0x8000 (%r9)
	blr
2:	fmadds	%f1, %f12, %f12, %f1
	beq		1b
	fsubs	%f1, %f1, %f12
	b		1b

.globl GXSetViewportJitterTAAPatch3Length
GXSetViewportJitterTAAPatch3Length:
.long (. - GXSetViewportJitterTAAPatch3) / 4
