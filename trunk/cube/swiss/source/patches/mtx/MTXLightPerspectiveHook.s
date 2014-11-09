#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXLightPerspectiveHook
MTXLightPerspectiveHook:
	lis			%r12, VAR_AREA
	lfs			%f12, VAR_FLOAT9_16 (%r12)
	fneg		%f13, %f12
	fsel		%f27, %f27, %f12, %f13
	fmuls		%f1, %f4, %f27
	trap

.globl MTXLightPerspectiveHook_length
MTXLightPerspectiveHook_length:
.long (MTXLightPerspectiveHook_length - MTXLightPerspectiveHook)