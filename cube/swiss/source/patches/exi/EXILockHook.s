#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl EXILockHook
EXILockHook:
	cmpwi		0, 0
	bne			1f
	cmplwi		0, 2
	bne			1f
	li			%r3, 0
	lwz			%r0, 52 (%sp)
	lmw			%r27, 28 (%sp)
	addi		%sp, %sp, 48
	mtlr		%r0
	blr
1:	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	blr

.globl EXILockHook_length
EXILockHook_length:
.long (EXILockHook_length - EXILockHook)