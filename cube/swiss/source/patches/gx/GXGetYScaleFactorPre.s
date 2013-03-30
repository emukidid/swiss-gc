#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXGetYScaleFactorPre
GXGetYScaleFactorPre:
	cmpwi		%r4, 480
	ble			1f
	mr			%r4, %r3
1:	mflr		%r0
	trap

.globl GXGetYScaleFactorPre_length
GXGetYScaleFactorPre_length:
.long (GXGetYScaleFactorPre_length - GXGetYScaleFactorPre)