
.globl GXSetDispCopyYScaleStub2
GXSetDispCopyYScaleStub2:
	li		%r9, 0
	lis		%r10, 0xCC01
	stb		%r9, -0x8000 (%r10)
	stw		%r9, -0x8000 (%r10)
	mr		%r10, %r13
	lwz		%r3, 580 (%r10)
	sth		%r9, 2 (%r10)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScaleStub2_length
GXSetDispCopyYScaleStub2_length:
.long (GXSetDispCopyYScaleStub2_length - GXSetDispCopyYScaleStub2)