#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXOrthoPre
MTXOrthoPre:
	fabs		%f12, %f2
	fneg		%f13, %f6
	fcmpu		%cr0, %f4, %f12
	fcmpu		%cr1, %f5, %f13
	crorc		4*%cr0+eq, 4*%cr0+eq, 4*%cr1+eq
	beq			1f
	lis			%r12, VAR_AREA
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