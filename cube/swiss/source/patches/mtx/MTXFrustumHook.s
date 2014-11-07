#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXFrustumHook
MTXFrustumHook:
	lis			%r12, VAR_AREA
	lfs			%f0, VAR_FLOAT1_6 (%r12)
	fsubs		%f9, %f4, %f3
	fmuls		%f13, %f9, %f0
	fadds		%f4, %f4, %f13
	fsubs		%f3, %f3, %f13
	fsubs		%f9, %f4, %f3
	trap

.globl MTXFrustumHook_length
MTXFrustumHook_length:
.long (MTXFrustumHook_length - MTXFrustumHook)