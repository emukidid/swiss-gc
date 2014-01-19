#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigurePanPre
VIConfigurePanPre:
	nop
	lhz			%r0, 260 (%r29)
	add			%r12, %r23, %r25
	cmpw		%r12, %r0
	ble			1f
	subf		%r23, %r25, %r0
1:	trap

.globl VIConfigurePanPre_length
VIConfigurePanPre_length:
.long (VIConfigurePanPre_length - VIConfigurePanPre)