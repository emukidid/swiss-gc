#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl MTXPerspectiveHook
MTXPerspectiveHook:
	lis			%r12, VAR_AREA
	lfs			%f12, VAR_FLOAT9_16 (%r12)
	fneg		%f13, %f12
	fsel		%f29, %f29, %f12, %f13
	fmuls		%f1, %f4, %f29
	trap

.globl MTXPerspectiveHook_length
MTXPerspectiveHook_length:
.long (MTXPerspectiveHook_length - MTXPerspectiveHook)