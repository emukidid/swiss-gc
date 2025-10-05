
.globl GXSetDispCopyYScaleStub1
GXSetDispCopyYScaleStub1:
	li		%r0, 0
	lwz		%r4, 0 (0)
	lis		%r5, 0xCC01
	stb		%r0, -0x8000 (%r5)
	stw		%r0, -0x8000 (%r5)
	sth		%r0, 2 (%r4)
	lwz		%r3, 484 (%r4)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScaleStub1Length
GXSetDispCopyYScaleStub1Length:
.long (. - GXSetDispCopyYScaleStub1) / 4
