#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl getTimingPatch
getTimingPatch:
	subi	%r3, %r3, 24
	lis		%r4, 0
	addi	%r4, %r4, 0
	slwi	%r3, %r3, 2
	lwzx	%r3, %r4, %r3
	blr
	.long	38*0
	.long	38*1
	.long	38*6
	.long	38*6
	.long	38*2
	.long	38*3
	.long	38*7
	.long	38*7
	.long	38*4
	.long	38*5
	.long	38*6
	.long	38*6
	.long	38*0
	.long	38*1
	.long	38*6
	.long	38*6
	.long	38*2
	.long	38*3
	.long	38*7
	.long	38*7
	.long	38*0
	.long	38*1
	.long	38*6
	.long	38*6

.globl getTimingPatch_length
getTimingPatch_length:
.long (getTimingPatch_length - getTimingPatch)