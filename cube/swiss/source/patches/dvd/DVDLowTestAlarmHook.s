#include "reservedarea.h"

.globl DVDLowTestAlarmHook
DVDLowTestAlarmHook:
	subis	%r3, %r3, VAR_AREA
	subfic	%r3, %r3, VAR_PATCHES_BASE - 1
	addze	%r3, %r0
	blr

.globl DVDLowTestAlarmHook_size
DVDLowTestAlarmHook_size:
.long (. - DVDLowTestAlarmHook)
