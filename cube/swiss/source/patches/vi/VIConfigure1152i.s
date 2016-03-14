#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure1152i
VIConfigure1152i:
	li			%r0, 6
	li			%r7, 1
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	subfic		%r4, %r5, 576
	srwi		%r4, %r4, 1
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	stw			%r6, 20 (%r3)
	stb			%r7, 24 (%r3)
	stw			%r0, 0 (%r3)
	mflr		%r0
	trap

.globl VIConfigure1152i_length
VIConfigure1152i_length:
.long (VIConfigure1152i_length - VIConfigure1152i)