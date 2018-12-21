#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure1080i50
VIConfigure1080i50:
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
1:	li			%r7, 1
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
	subf.		%r4, %r5, %r4
	bge			2f
	ori			%r0, %r0, 2
	lhz			%r5, 8 (%r3)
	subfic		%r4, %r5, 540
2:	srawi		%r4, %r4, 1
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	sth			%r6, 20 (%r3)
	stb			%r7, 24 (%r3)
	sth			%r0, 0 (%r3)

.globl VIConfigure1080i50_length
VIConfigure1080i50_length:
.long (VIConfigure1080i50_length - VIConfigure1080i50)