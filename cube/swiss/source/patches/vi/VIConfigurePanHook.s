#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanHook
VIConfigurePanHook:
	lhz			%r3, 260 (%r29)
	add			%r4, %r23, %r25
	cmpw		%r4, %r3
	ble			1f
	subf		%r23, %r25, %r3
1:	mfmsr		%r3
	rlwinm		%r4, %r3, 0, 17, 15
	extrwi		%r3, %r3, 1, 16
	mtmsr		%r4
	blr

.globl VIConfigurePanHook_length
VIConfigurePanHook_length:
.long (VIConfigurePanHook_length - VIConfigurePanHook)