#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl EXILockHook
EXILockHook:
	mr			0, %r3
	li			%r3, 0
	li			%r4, 0
	mflr		%r0
	stw			%r0, 4 (%sp)
	stwu		%sp, -8 (%sp)
	bl			0
	cmpwi		%r3, 0
	lwz			%r0, 12 (%sp)
	addi		%sp, %sp, 8
	mtlr		%r0
	bnelr
	mfmsr		%r4
	insrwi		%r4, 0, 1, 16
	mtmsr		%r4
	lwz			%r0, 52 (%sp)
	lmw			%r27, 28 (%sp)
	addi		%sp, %sp, 48
	mtlr		%r0
	blr

.globl EXILockHook_length
EXILockHook_length:
.long (EXILockHook_length - EXILockHook)