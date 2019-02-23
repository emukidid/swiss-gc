#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl EXILockHookD
EXILockHookD:
	cmpwi		0, 0
	bne			1f
	cmplwi		0, 2
	beq			2f
1:	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	lis			%r4, VAR_AREA
	lbz			%r0, VAR_EXI_SLOT (%r4)
	cmpw		0, %r0
	bnelr
	mulli		%r0, %r0, 20
	lis			%r4, 0xCC00
	addi		%r4, %r4, 0x6800
	lwzx		%r0, %r4, %r0
	extrwi.		%r0, %r0, 3, 22
	beqlr
	mfmsr		%r4
	insrwi		%r4, %r3, 1, 16
	mtmsr		%r4
2:	li			%r3, 0
	lwz			%r0, 60 (%sp)
	lmw			%r25, 28 (%sp)
	addi		%sp, %sp, 56
	mtlr		%r0
	blr

.globl EXILockHookD_length
EXILockHookD_length:
.long (EXILockHookD_length - EXILockHookD)