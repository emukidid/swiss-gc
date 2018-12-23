#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure576i
VIConfigure576i:
	lwz			%r0, 0 (%r3)
	lis			%r4, VAR_AREA
	stw			%r3, VAR_RMODE (%r4)
	li			%r5, 576
	clrrwi		%r0, %r0, 2
	cmpwi		%r0, 4
	beq			1f
	cmpwi		%r0, 16
	beq			1f
	li			%r5, 480
1:	li			%r7, 0
	lhz			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
	cmpw		%r5, %r6
	bge			2f
	li			%r7, 1
	lhz			%r6, 8 (%r3)
2:	subf		%r5, %r6, %r5
	srawi		%r5, %r5, 1
	sth			%r5, 12 (%r3)
	sth			%r6, 16 (%r3)
	sth			%r7, 20 (%r3)
	stb			%r7, VAR_VFILTER_ON (%r4)
	sth			%r0, 0 (%r3)

.globl VIConfigure576i_length
VIConfigure576i_length:
.long (VIConfigure576i_length - VIConfigure576i)