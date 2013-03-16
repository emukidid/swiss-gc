#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXOrthoPre
MTXOrthoPre:
	fabs		%f12, %f2
	fabs		%f13, %f3
	fcmpu		%cr0, %f1, %f12
	fcmpu		%cr1, %f4, %f13
	crand		4*%cr0+eq, 4*%cr0+eq, 4*%cr1+eq
	beq			1f
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOATM_1 (%r12)
	fcmpu		%cr0, %f0, %f5
	fcmpu		%cr1, %f0, %f6
	creqv		4*%cr1+eq, 4*%cr0+eq, 4*%cr1+eq
	fcmpu		%cr0, %f1, %f3
	crand		4*%cr0+eq, 4*%cr0+eq, 4*%cr1+eq
	beq			1f
	fctiwz		%f12, %f2
	fctiwz		%f13, %f4
	stfd		%f12, -16 (%sp)
	stfd		%f13, -8 (%sp)
	lwz			%r10, -12 (%sp)
	lwz			%r11, -4 (%sp)
	subi		%r0, %r10, 1
	and.		%r0, %r0, %r10
	beq			1f
	subi		%r0, %r11, 1
	and.		%r0, %r0, %r11
	beq			1f
	lfs			%f0, VAR_FLOAT1_6 (%r12)
	fsubs		%f8, %f4, %f3
	fmuls		%f13, %f8, %f0
	fadds		%f4, %f4, %f13
	fsubs		%f3, %f3, %f13
1:	fsubs		%f8, %f4, %f3
	trap

.globl MTXOrthoPre_length
MTXOrthoPre_length:
.long (MTXOrthoPre_length - MTXOrthoPre)