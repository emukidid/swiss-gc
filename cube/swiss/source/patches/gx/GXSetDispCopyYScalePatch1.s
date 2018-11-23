#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetDispCopyYScalePatch1
GXSetDispCopyYScalePatch1:
	lwz		%r4, 0 (0)
	lwz		%r3, 484 (%r4)
	extrwi	%r3, %r3, 10, 12
	addi	%r3, %r3, 1
	blr

.globl GXSetDispCopyYScalePatch1_length
GXSetDispCopyYScalePatch1_length:
.long (GXSetDispCopyYScalePatch1_length - GXSetDispCopyYScalePatch1)