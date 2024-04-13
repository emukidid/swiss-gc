#include "reservedarea.h"

.globl VIConfigureHook1RT4K
VIConfigureHook1RT4K:
	li			%r0, 0
	lhz			%r6, 4 (%r3)
	subfic		%r5, %r6, 720
	srwi		%r5, %r5, 1
	sth			%r0, 12 (%r3)
	sth			%r5, 18 (%r3)
	sth			%r6, 20 (%r3)
	lis			%r4, VAR_AREA
	stw			%r3, VAR_RMODE (%r4)
	lbz			%r0, VAR_VFILTER_ON (%r4)
	cmpwi		%r0, 0
	beq			1f
	addi		%r4, %r4, VAR_VFILTER
	addi		%r3, %r3, 50
	lswi		%r5, %r4, 7
	stswi		%r5, %r3, 7
1:	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	blr

.globl VIConfigureHook1RT4K_length
VIConfigureHook1RT4K_length:
.long (VIConfigureHook1RT4K_length - VIConfigureHook1RT4K)