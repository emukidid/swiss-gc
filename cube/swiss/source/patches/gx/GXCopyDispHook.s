#include "reservedarea.h"

.globl GXCopyDispHook
GXCopyDispHook:
	lis		%r4, VAR_AREA
	lbz		%r3, VAR_NEXT_FIELD (%r4)
	stb		%r3, VAR_CURRENT_FIELD (%r4)
	blr

.globl GXCopyDispHook_size
GXCopyDispHook_size:
.long (. - GXCopyDispHook)
