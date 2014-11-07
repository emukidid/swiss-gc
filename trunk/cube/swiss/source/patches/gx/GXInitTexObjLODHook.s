#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXInitTexObjLODHook
GXInitTexObjLODHook:
	lbz			%r0, 31 (%r3)
	andi.		%r0, %r0, 2
	beq			1f
	li			%r6, 1
	li			%r7, 1
	li			%r8, 2
1:	nop
	trap

.globl GXInitTexObjLODHook_length
GXInitTexObjLODHook_length:
.long (GXInitTexObjLODHook_length - GXInitTexObjLODHook)