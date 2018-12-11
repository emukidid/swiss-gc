#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigureHook2
VIConfigureHook2:
	mfmsr	%r4
	insrwi	%r4, %r3, 1, 16
	mtmsr	%r4
	lis		%r3, VAR_AREA
	lwz		%r3, VAR_RMODE (%r3)
	lwz		%r4, 0 (%r3)
	lwz		%r6, 20 (%r3)
	clrlwi	%r5, %r4, 30
	clrrwi.	%r4, %r4, 2
	beq		1f
	cmpwi	%r5, 2
	beq		2f
1:	cmpwi	%r5, 1
	bnelr
	cmpwi	%r6, 1
	bnelr
2:	li		%r6, 1
	stw		%r6, 20 (%r3)
	stw		%r4, 0 (%r3)
	blr

.globl VIConfigureHook2_length
VIConfigureHook2_length:
.long (VIConfigureHook2_length - VIConfigureHook2)