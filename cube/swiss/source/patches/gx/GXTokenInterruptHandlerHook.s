#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXTokenInterruptHandlerHook
GXTokenInterruptHandlerHook:
	lis		%r4, 0xCC00
	li		%r0, 0
1:	stw		%r0, 0x404E (%r4)
	mftb	%r5
2:	mftb	%r6
	sub		%r6, %r6, %r5
	cmplwi	%r6, 10
	ble		2b
	lwz		%r0, 0x404E (%r4)
	cmplwi	%r0, 0
	bne		1b
	bctr

.globl GXTokenInterruptHandlerHook_length
GXTokenInterruptHandlerHook_length:
.long (GXTokenInterruptHandlerHook_length - GXTokenInterruptHandlerHook)