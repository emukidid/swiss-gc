#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXLightFrustumPre
MTXLightFrustumPre:
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOAT1_6 (%r12)
	fsubs		%f10, %f4, %f3
	fmuls		%f13, %f10, %f0
	fadds		%f4, %f4, %f13
	fsubs		%f3, %f3, %f13
	fsubs		%f10, %f4, %f3
	trap

.globl MTXLightFrustumPre_length
MTXLightFrustumPre_length:
.long (MTXLightFrustumPre_length - MTXLightFrustumPre)