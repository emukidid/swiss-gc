#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetScissorPre
GXSetScissorPre:
	or.			%r0, %r3, %r4
	beq			1f
	add			%r0, %r3, %r5
	cmpwi		%r0, 640
	beq			1f
	subi		%r3, %r3, 320
	mulli		%r3, %r3, 3
	mulli		%r5, %r5, 3
	srawi		%r3, %r3, 2
	srwi		%r5, %r5, 2
	addi		%r3, %r3, 320
1:	nop
	trap

.globl GXSetScissorPre_length
GXSetScissorPre_length:
.long (GXSetScissorPre_length - GXSetScissorPre)