#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanHookDs
VIConfigurePanHookDs:
	lhz			%r3, 260 (%r29)
	add			%r4, %r23, %r25
	cmpw		%r4, %r3
	ble			1f
	subf		%r23, %r25, %r3
1:	srwi		%r25, %r25, 1
	srwi		%r23, %r23, 1
	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	mtmsr		%r4
	extrwi		%r3, %r3, 1, 16
	blr

.globl VIConfigurePanHookDs_length
VIConfigurePanHookDs_length:
.long (VIConfigurePanHookDs_length - VIConfigurePanHookDs)