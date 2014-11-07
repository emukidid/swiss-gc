#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetScissorHook
GXSetScissorHook:
	nop
	nop			#r0 = gx->dispCopyDst
	clrlslwi	%r12, %r0, 22, 4
	cmpwi		%r3, 0
	beq			1f
	add			%r0, %r3, %r5
	cmpw		%r0, %r12
	beq			1f
	srwi		%r0, %r12, 1
	sub			%r3, %r3, %r0
	mulli		%r3, %r3, 3
	mulli		%r5, %r5, 3
	srawi		%r3, %r3, 2
	srwi		%r5, %r5, 2
	addze		%r3, %r3
	add			%r3, %r3, %r0
1:	trap

.globl GXSetScissorHook_length
GXSetScissorHook_length:
.long (GXSetScissorHook_length - GXSetScissorHook)