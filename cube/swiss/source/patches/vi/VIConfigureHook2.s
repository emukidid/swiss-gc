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
	lhz		%r0, 2 (%r3)
	lbz		%r4, 23 (%r3)
	stw		%r4, 20 (%r3)
	stw		%r0, 0 (%r3)
	blr

.globl VIConfigureHook2_length
VIConfigureHook2_length:
.long (VIConfigureHook2_length - VIConfigureHook2)