#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanHook
VIConfigurePanHook:
	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	lhz			%r0, 260 (%r29)
	sub			%r0, %r0, %r25
	sub.		%r0, %r0, %r23
	bgelr
	add			%r23, %r23, %r0
	blr

.globl VIConfigurePanHook_length
VIConfigurePanHook_length:
.long (VIConfigurePanHook_length - VIConfigurePanHook)