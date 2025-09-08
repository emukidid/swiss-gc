
.globl GXPeekARGBPatch
GXPeekARGBPatch:
	lis		%r6, 0xC800
	insrwi	%r6, %r3, 10, 20
	insrwi	%r6, %r4, 10, 10
	sync
	lwz		%r0, 0 (%r6)
	stw		%r0, 0 (%r5)
	blr

.globl GXPeekARGBPatchLength
GXPeekARGBPatchLength:
.long (. - GXPeekARGBPatch) / 4
