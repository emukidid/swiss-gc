#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXInitTexObjLODPre
GXInitTexObjLODPre:
	lwz			%r12, 20 (%r3)
	subi		%r0, %r12, 8
	cmplwi		%r0, 2
	ble			1f
	li			%r6, 1
	li			%r7, 1
	li			%r8, 2
1:	fcmpo		%cr0, %f3, %f0
	trap

.globl GXInitTexObjLODPre_length
GXInitTexObjLODPre_length:
.long (GXInitTexObjLODPre_length - GXInitTexObjLODPre)