#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanHookD
VIConfigurePanHookD:
	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	lhz			%r0, 260 (%r31)
	lhz			%r4, 18 (%sp)
	sub			%r0, %r0, %r4
	sub.		%r0, %r0, %r29
	bgelr
	add			%r29, %r29, %r0
	blr

.globl VIConfigurePanHookD_length
VIConfigurePanHookD_length:
.long (VIConfigurePanHookD_length - VIConfigurePanHookD)