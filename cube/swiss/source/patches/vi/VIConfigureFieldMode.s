
.globl VIConfigureFieldMode
VIConfigureFieldMode:
	lbz			%r0, 24 (%r3)
	cmpwi		%r0, 0
	beq			1f
	lbz			%r0, 25 (%r3)
	cmpwi		%r0, 0
	beq			1f
	addi		%r4, %r3, 26
	lis			%r5, 0x0304
	addi		%r5, %r5, 0x0906
	lis			%r6, 0x0308
	stswi		%r5, %r4, 6
	addi		%r4, %r4, 6
	stswi		%r5, %r4, 6
	addi		%r4, %r4, 6
	lis			%r5, 0x0904
	addi		%r5, %r5, 0x0306
	lis			%r6, 0x0908
	stswi		%r5, %r4, 6
	addi		%r4, %r4, 6
	stswi		%r5, %r4, 6
1:

.globl VIConfigureFieldMode_size
VIConfigureFieldMode_size:
.long (. - VIConfigureFieldMode)
