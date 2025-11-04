#include "reservedarea.h"

.globl OSSetArenaHiHook
OSSetArenaHiHook:
	lis		%r5, 0x8000
	lwz		%r0, 0x0038 (%r5)
	cmpwi	%r0, 0
	bne		1f
	lwz		%r0, 0x00EC (%r5)
1:	cmplw	%r0, %r3
	bge		2f
	mr		%r3, %r0
2:	stw		%r3, 0 (%r4)
	blr

.globl OSSetArenaHiHook_size
OSSetArenaHiHook_size:
.long (. - OSSetArenaHiHook)
