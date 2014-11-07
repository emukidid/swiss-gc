#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanHook
VIConfigurePanHook:
	nop
	lhz			%r0, 260 (%r29)
	add			%r12, %r23, %r25
	cmpw		%r12, %r0
	ble			1f
	subf		%r23, %r25, %r0
1:	trap

.globl VIConfigurePanHook_length
VIConfigurePanHook_length:
.long (VIConfigurePanHook_length - VIConfigurePanHook)