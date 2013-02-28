#include "asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../reservedarea.h"


.globl ForceWidescreen
ForceWidescreen:
#C_MTXOrtho
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOAT1_1 (%r12)
	fneg		%f12, %f6
	fcmpu		%cr0, %f5, %f12
	fcmpu		%cr7, %f6, %f0
	cror		4*%cr0+gt, 4*%cr0+gt, 4*%cr7+gt
	bgt			1f
	lfs			%f0, VAR_FLOAT7_6 (%r12)
	fmr			%f12, %f4
	fmuls		%f4, %f4, %f0
	fsubs		%f0, %f4, %f12
	fsubs		%f3, %f3, %f0
1:	fsubs		%f8, %f4, %f3
	trap

#C_MTXFrustum
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOAT7_6 (%r12)
	fmr			%f12, %f4
	fmuls		%f4, %f4, %f0
	fsubs		%f0, %f4, %f12
	fsubs		%f3, %f3, %f0
	fsubs		%f9, %f4, %f3
	trap

#GXSetProjection
	lfs			%f0, 0 (%r3)
	beq			2f
	lis			%r12, VAR_AREA
	lfs			%f1, VAR_FLOAT3_4 (%r12)
	fmuls		%f0, %f0, %f1
2:	trap

#GXSetScissor
	lis			%r12, VAR_AREA
	or.			%r0, %r3, %r4
	beq			3f
	lwz			%r0, VAR_SCREEN_WD (%r12)
	sub			%r3, %r3, %r0
	mulli		%r3, %r3, 3
	mulli		%r5, %r5, 3
	srawi		%r3, %r3, 2
	srwi		%r5, %r5, 2
	add			%r3, %r3, %r0
	b			4f
3:	srwi		%r0, %r5, 1
	stw			%r0, VAR_SCREEN_WD (%r12)
4:	nop
	trap

.globl ForceWidescreen_length
ForceWidescreen_length:
.long (ForceWidescreen_length - ForceWidescreen)