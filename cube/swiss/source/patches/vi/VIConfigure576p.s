#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure576p
VIConfigure576p:
	li			%r0, 4
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	subfic		%r4, %r5, 288
	cmpwi		%r5, 288
	ble			1f
	li			%r0, 6
	subfic		%r4, %r5, 576
	srwi		%r4, %r4, 1
1:	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	stw			%r6, 20 (%r3)
	stw			%r0, 0 (%r3)
	mflr		%r0
	trap

.globl VIConfigure576p_length
VIConfigure576p_length:
.long (VIConfigure576p_length - VIConfigure576p)