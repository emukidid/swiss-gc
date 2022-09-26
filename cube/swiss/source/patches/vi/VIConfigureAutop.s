#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigureAutop
VIConfigureAutop:
	lhz			%r0, 2 (%r3)
	insrwi		%r0, %r0, 16, 0
	li			%r6, 0
	lbz			%r5, 23 (%r3)
	extrwi		%r4, %r0, 2, 14
	cmpwi		%r4, 2
	beq			1f
	cmpwi		%r4, 3
	beq			1f
	cmpwi		%r5, 0
	beq			2f
	li			%r4, 2
	li			%r5, 0
1:	li			%r6, 1
2:	insrwi		%r0, %r4, 2, 14
	stb			%r5, 22 (%r3)
	lis			%r4, VAR_AREA
	stb			%r6, VAR_VFILTER_ON (%r4)
	stw			%r0, 0 (%r3)

.globl VIConfigureAutop_length
VIConfigureAutop_length:
.long (VIConfigureAutop_length - VIConfigureAutop)