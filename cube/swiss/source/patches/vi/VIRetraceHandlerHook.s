#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIRetraceHandlerHook
VIRetraceHandlerHook:
	srwi	%r3, %r3, 31
	lis		%r4, VAR_AREA
	xori	%r3, %r3, 1
	stb		%r3, VAR_NEXT_FIELD (%r4)
	mtlr	%r0
	blr

.globl VIRetraceHandlerHook_length
VIRetraceHandlerHook_length:
.long (VIRetraceHandlerHook_length - VIRetraceHandlerHook)