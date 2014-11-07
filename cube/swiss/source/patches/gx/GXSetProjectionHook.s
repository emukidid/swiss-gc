#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetProjectionHook
GXSetProjectionHook:
	lfs			%f0, 0 (%r3)
	beq			1f
	lis			%r12, VAR_AREA
	lfs			%f1, VAR_FLOAT3_4 (%r12)
	fmuls		%f0, %f0, %f1
1:	trap

.globl GXSetProjectionHook_length
GXSetProjectionHook_length:
.long (GXSetProjectionHook_length - GXSetProjectionHook)