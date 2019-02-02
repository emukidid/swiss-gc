#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIGetRetraceCountHook
VIGetRetraceCountHook:
	lis			%r4, VAR_AREA
	lbz			%r4, VAR_CURRENT_FIELD (%r4)
	sub			%r3, %r3, %r4
	blr

.globl VIGetRetraceCountHook_length
VIGetRetraceCountHook_length:
.long (VIGetRetraceCountHook_length - VIGetRetraceCountHook)