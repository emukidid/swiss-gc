#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure288p
VIConfigure288p:
	li			%r0, 5
	stw			%r0, 0 (%r3)
	lhz			%r0, 8 (%r3)
	srwi		%r0, %r0, 1
	sth			%r0, 16 (%r3)
	subfic		%r0, %r0, 288
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 0
	stw			%r0, 20 (%r3)
	mflr		%r0
	trap

.globl VIConfigure288p_length
VIConfigure288p_length:
.long (VIConfigure288p_length - VIConfigure288p)