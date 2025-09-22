#include "reservedarea.h"

.globl GXSetViewportPatch2
GXSetViewportPatch2:
	lwz		%r9, 0 (0)
	lis		%r10, VAR_AREA
	stfs	%f1, 1268 (%r9)
	stfs	%f2, 1272 (%r9)
	stfs	%f3, 1276 (%r9)
	stfs	%f4, 1280 (%r9)
	stfs	%f5, 1284 (%r9)
	stfs	%f6, 1288 (%r9)
	lfs		%f8, 1296 (%r9)
	lfs		%f7, 1292 (%r9)
	lbz		%r3, VAR_CURRENT_FIELD (%r10)
	b		.

.globl GXSetViewportPatch2Length
GXSetViewportPatch2Length:
.long (. - GXSetViewportPatch2) / 4
