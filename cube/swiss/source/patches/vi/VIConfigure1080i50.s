#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure1080i50
VIConfigure1080i50:
	lwz			%r0, 0 (%r3)
	srwi		%r6, %r0, 2
	lis			%r4, VAR_AREA
	stw			%r6, VAR_TVMODE (%r4)
	li			%r5, 574
	cmpwi		%r6, 1
	beq			1f
	cmpwi		%r6, 4
	beq			1f
	li			%r5, 480
1:	clrrwi		%r0, %r0, 2
	li			%r8, 1
	li			%r7, 0
	lhz			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
	subf.		%r5, %r6, %r5
	bge			2f
	ori			%r0, %r0, 2
	lhz			%r6, 8 (%r3)
	subfic		%r5, %r6, 540
2:	srawi		%r5, %r5, 1
	sth			%r5, 12 (%r3)
	sth			%r6, 16 (%r3)
	sth			%r7, 20 (%r3)
	stb			%r8, 24 (%r3)
	sth			%r0, 0 (%r3)

.globl VIConfigure1080i50_length
VIConfigure1080i50_length:
.long (VIConfigure1080i50_length - VIConfigure1080i50)