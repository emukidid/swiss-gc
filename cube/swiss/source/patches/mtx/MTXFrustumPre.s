#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXFrustumPre
MTXFrustumPre:
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOAT7_6 (%r12)
	fmr			%f13, %f4
	fmuls		%f4, %f4, %f0
	fsubs		%f0, %f4, %f13
	fsubs		%f3, %f3, %f0
	fsubs		%f9, %f4, %f3
	trap

.globl MTXFrustumPre_length
MTXFrustumPre_length:
.long (MTXFrustumPre_length - MTXFrustumPre)