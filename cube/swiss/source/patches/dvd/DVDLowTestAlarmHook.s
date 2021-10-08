#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl DVDLowTestAlarmHook
DVDLowTestAlarmHook:
	subis	%r3, %r3, VAR_AREA
	subfic	%r3, %r3, VAR_PATCHES_BASE - 1
	addze	%r3, %r0
	blr

.globl DVDLowTestAlarmHook_length
DVDLowTestAlarmHook_length:
.long (DVDLowTestAlarmHook_length - DVDLowTestAlarmHook)