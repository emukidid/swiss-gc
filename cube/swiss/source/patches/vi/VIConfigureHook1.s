#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigureHook1
VIConfigureHook1:
	lis			%r4, VAR_AREA
	lhz			%r5, VAR_SAR_WIDTH (%r4)
	lbz			%r6, VAR_SAR_HEIGHT (%r4)
	lhz			%r7, 4 (%r3)
	mullw		%r8, %r7, %r5
	divwuo.		%r8, %r8, %r6
	bns			1f
	mr			%r8, %r5
1:	cmpw		%r8, %r7
	bge			2f
	mr			%r8, %r7
2:	subfic		%r9, %r8, 720
	srwi		%r9, %r9, 1
	sth			%r9, 10 (%r3)
	sth			%r8, 14 (%r3)

.globl VIConfigureHook1_length
VIConfigureHook1_length:
.long (VIConfigureHook1_length - VIConfigureHook1)