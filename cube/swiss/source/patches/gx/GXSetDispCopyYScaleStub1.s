
.globl GXSetDispCopyYScaleStub1
GXSetDispCopyYScaleStub1:
	lwz		%r4, 0 (0)
	lwz		%r3, 484 (%r4)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScaleStub1_length
GXSetDispCopyYScaleStub1_length:
.long (GXSetDispCopyYScaleStub1_length - GXSetDispCopyYScaleStub1)