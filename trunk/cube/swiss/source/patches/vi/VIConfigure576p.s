#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure576p
VIConfigure576p:
	li			%r0, 6
	stw			%r0, 0 (%r3)
	lhz			%r0, 8 (%r3)
	sth			%r0, 16 (%r3)
	subfic		%r0, %r0, 576
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 0
	stw			%r0, 20 (%r3)
	mflr		%r0
	trap

.globl VIConfigure576p_length
VIConfigure576p_length:
.long (VIConfigure576p_length - VIConfigure576p)