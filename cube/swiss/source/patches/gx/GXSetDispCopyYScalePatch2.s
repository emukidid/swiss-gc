#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetDispCopyYScalePatch2
GXSetDispCopyYScalePatch2:
	mr		%r9, %r13
	lwz		%r3, 580 (%r9)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScalePatch2_length
GXSetDispCopyYScalePatch2_length:
.long (GXSetDispCopyYScalePatch2_length - GXSetDispCopyYScalePatch2)