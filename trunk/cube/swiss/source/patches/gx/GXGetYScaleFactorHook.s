#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXGetYScaleFactorHook
GXGetYScaleFactorHook:
	cmpwi		%r4, 480
	ble			1f
	mr			%r4, %r3
1:	mflr		%r0
	trap

.globl GXGetYScaleFactorHook_length
GXGetYScaleFactorHook_length:
.long (GXGetYScaleFactorHook_length - GXGetYScaleFactorHook)