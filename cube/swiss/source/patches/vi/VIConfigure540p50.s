#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure540p50
VIConfigure540p50:
	lhz			%r0, 2 (%r3)
	srwi		%r6, %r0, 2
	lis			%r4, VAR_AREA
	stw			%r6, VAR_TVMODE (%r4)
	li			%r5, 480
	cmpwi		%r6, 1
	beq			1f
	cmpwi		%r6, 4
	bne			2f
1:	li			%r5, 574
2:	insrwi		%r0, %r6, 14, 0
	li			%r8, 0
	li			%r7, 0
	lhz			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
	subf.		%r5, %r6, %r5
	bge			3f
	oris		%r0, %r0, 2
	li			%r8, 1
	lhz			%r6, 8 (%r3)
	subfic		%r5, %r6, 540
3:	srawi		%r5, %r5, 1
	sth			%r5, 12 (%r3)
	sth			%r6, 16 (%r3)
	stb			%r7, 22 (%r3)
	stb			%r8, VAR_VFILTER_ON (%r4)
	stw			%r0, 0 (%r3)

.globl VIConfigure540p50_length
VIConfigure540p50_length:
.long (VIConfigure540p50_length - VIConfigure540p50)