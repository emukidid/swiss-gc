#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl EXILockHookD
EXILockHookD:
	cmpwi		0, 0
	bne			1f
	cmplwi		0, 2
	bne			1f
	li			%r3, 0
	lwz			%r0, 60 (%sp)
	lmw			%r25, 28 (%sp)
	addi		%sp, %sp, 56
	mtlr		%r0
	blr
1:	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	blr

.globl EXILockHookD_length
EXILockHookD_length:
.long (EXILockHookD_length - EXILockHookD)