#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetScissorPre
GXSetScissorPre:
	lis			%r12, VAR_AREA
	or.			%r0, %r3, %r4
	beq			1f
	lwz			%r0, VAR_SCREEN_WD (%r12)
	sub			%r3, %r3, %r0
	mulli		%r3, %r3, 3
	mulli		%r5, %r5, 3
	srawi		%r3, %r3, 2
	srwi		%r5, %r5, 2
	add			%r3, %r3, %r0
	b			2f
1:	srwi		%r0, %r5, 1
	stw			%r0, VAR_SCREEN_WD (%r12)
2:	nop
	trap

.globl GXSetScissorPre_length
GXSetScissorPre_length:
.long (GXSetScissorPre_length - GXSetScissorPre)