#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure480p
VIConfigure480p:
	li			%r0, 2
	stw			%r0, 0 (%r3)
	lhz			%r0, 8 (%r3)
	cmpwi		%r0, 480
	ble			2f
	lhz			%r0, 6 (%r3)
	clrrwi		%r0, %r0, 1
	cmpwi		%r0, 480
	ble			1f
	li			%r0, 480
	sth			%r0, 6 (%r3)
1:	sth			%r0, 8 (%r3)
2:	sth			%r0, 16 (%r3)
	subfic		%r0, %r0, 480
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 0
	stw			%r0, 20 (%r3)
	mflr		%r0
	trap

.globl VIConfigure480p_length
VIConfigure480p_length:
.long (VIConfigure480p_length - VIConfigure480p)