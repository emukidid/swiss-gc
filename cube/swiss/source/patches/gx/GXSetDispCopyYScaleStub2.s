
.globl GXSetDispCopyYScaleStub2
GXSetDispCopyYScaleStub2:
	mr		%r9, %r13
	lwz		%r3, 580 (%r9)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScaleStub2_length
GXSetDispCopyYScaleStub2_length:
.long (GXSetDispCopyYScaleStub2_length - GXSetDispCopyYScaleStub2)