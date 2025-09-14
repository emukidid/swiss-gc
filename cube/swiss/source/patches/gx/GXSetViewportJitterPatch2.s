#include "reservedarea.h"

.globl GXSetViewportJitterPatch2
GXSetViewportJitterPatch2:
	lwz		%r9, 0 (0)
	stfs	%f1, 1268 (%r9)
	stfs	%f2, 1272 (%r9)
	stfs	%f3, 1276 (%r9)
	stfs	%f4, 1280 (%r9)
	stfs	%f5, 1284 (%r9)
	stfs	%f6, 1288 (%r9)
	b		.

.globl GXSetViewportJitterPatch2Length
GXSetViewportJitterPatch2Length:
.long (. - GXSetViewportJitterPatch2) / 4
