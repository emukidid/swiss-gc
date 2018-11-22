#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIRetraceHandlerHook
VIRetraceHandlerHook:
	lis		%r4, VAR_AREA
	stb		%r3, VAR_NEXT_FIELD (%r4)
	mtlr	%r0
	blr

.globl VIRetraceHandlerHook_length
VIRetraceHandlerHook_length:
.long (VIRetraceHandlerHook_length - VIRetraceHandlerHook)