#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXInitTexObjLODPre
GXInitTexObjLODPre:
	lbz			%r0, 31 (%r3)
	andi.		%r0, %r0, 2
	beq			1f
	li			%r6, 1
	li			%r7, 1
	li			%r8, 2
1:	fcmpo		%cr0, %f3, %f0
	trap

.globl GXInitTexObjLODPre_length
GXInitTexObjLODPre_length:
.long (GXInitTexObjLODPre_length - GXInitTexObjLODPre)