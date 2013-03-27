# Patches into VI Configure to change the video mode to 576p just prior to video being configured
#include "asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../reservedarea.h"

.globl ForceProgressive576p
ForceProgressive576p:
	li			%r0, 6
	stw			%r0, 0 (%r3)
	lhz			%r0, 16 (%r3)
	subfic		%r0, %r0, 576
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 0
	stw			%r0, 20 (%r3)
	mflr		%r0
	trap

.globl ForceProgressive576p_length
ForceProgressive576p_length:
.long (ForceProgressive576p_length - ForceProgressive576p)