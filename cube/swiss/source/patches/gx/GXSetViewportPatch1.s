#include "reservedarea.h"

.globl GXSetViewportPatch1
GXSetViewportPatch1:
	lis		%r9, VAR_AREA
	lbz		%r3, VAR_CURRENT_FIELD (%r9)
	b		.

.globl GXSetViewportPatch1Length
GXSetViewportPatch1Length:
.long (. - GXSetViewportPatch1) / 4
