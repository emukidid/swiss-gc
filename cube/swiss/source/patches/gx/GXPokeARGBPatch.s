#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXPokeARGBPatch
GXPokeARGBPatch:
	lis		%r6, 0xC800
	insrwi	%r6, %r3, 10, 20
	insrwi	%r6, %r4, 10, 10
	sync
	stw		%r5, 0 (%r6)
	blr

.globl GXPokeARGBPatch_length
GXPokeARGBPatch_length:
.long (GXPokeARGBPatch_length - GXPokeARGBPatch)