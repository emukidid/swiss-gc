#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl setFbbRegsHook
setFbbRegsHook:
	stwu	%sp, -8 (%sp)
	mflr	%r0
	stw		%r0, 12 (%sp)
	nop
	lis		%r4, VAR_AREA
	stb		%r3, VAR_CURRENT_FIELD (%r4)
	lwz		%r0, 12 (%sp)
	addi	%sp, %sp, 8
	mtlr	%r0
	blr

.globl setFbbRegsHook_length
setFbbRegsHook_length:
.long (setFbbRegsHook_length - setFbbRegsHook)