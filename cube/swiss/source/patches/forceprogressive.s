# Patches into VI Configure to change the video mode to 480p just prior to video being configured
#include "asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../reservedarea.h"


.globl ForceProgressive
ForceProgressive:
	.long (0x00000000)
	.long (0x00000606)
	.long (0x06060606)
	.long (0x06060606)
	.long (0x06060606)
	.long (0x06060606)
	.long (0x06060606)
	.long (0x06060000)
	.long (0x15161500)
	.long (0x00000000)
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	li			%r0, 0
	lis			%r12, 0x8000
	stw			%r0, 0x00CC (%r12)
	li			%r0, 2
	stw			%r0, 0 (%r3)
	lhz			%r0, 8 (%r3)
	cmplwi		%r0, 480
	ble			1f
	li			%r0, 480
	sth			%r0, 6 (%r3)
	sth			%r0, 8 (%r3)
	sth			%r0, 16 (%r3)
1:	lhz			%r0, 16 (%r3)
	subfic		%r0, %r0, 480
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 10
	lis			%r12, VAR_AREA
	mtctr		%r0
	addi		%r12, %r12, VAR_PROG_MODE-4
	addi		%r3, %r3, 16
2:	lwzu		%r0, 4 (%r12)
	stwu		%r0, 4 (%r3)
	bdnz		2b
	blr

   .globl ForceProgressive_length
   ForceProgressive_length:
   .long (ForceProgressive_length - ForceProgressive)