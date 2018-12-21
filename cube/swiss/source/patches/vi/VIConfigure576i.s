#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure576i
VIConfigure576i:
	lwz			%r0, 0 (%r3)
	lis			%r4, VAR_AREA
	stw			%r3, VAR_RMODE (%r4)
	li			%r4, 576
	clrrwi		%r0, %r0, 2
	cmpwi		%r0, 4
	beq			1f
	cmpwi		%r0, 16
	beq			1f
	li			%r4, 480
1:	li			%r6, 0
	lhz			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
	cmpw		%r4, %r5
	bge			2f
	li			%r6, 1
	lhz			%r5, 8 (%r3)
2:	subf		%r4, %r5, %r4
	srawi		%r4, %r4, 1
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	sth			%r6, 20 (%r3)
	sth			%r0, 0 (%r3)

.globl VIConfigure576i_length
VIConfigure576i_length:
.long (VIConfigure576i_length - VIConfigure576i)