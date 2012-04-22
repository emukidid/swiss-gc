# Patches into VI Configure to change the video mode to 576p just prior to video being configured
#include "asm.h"

# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_PROG_MODE,		-0xB8	# data/code to overwrite GXRMode obj with for 576p forcing

.globl ForceProgressive576p
ForceProgressive576p:
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
	lhz			%r0, 16 (%r3)
	subfic		%r0, %r0, 576
	srwi		%r0, %r0, 1
	sth			%r0, 12 (%r3)
	li			%r0, 10
	lis			%r12, VAR_AREA
	mtctr		%r0
	addi		%r12, %r12, VAR_PROG_MODE-4
	addi		%r3, %r3, 16
1:	lwzu		%r0, 4 (%r12)
	stwu		%r0, 4 (%r3)
	bdnz		1b
	blr

   .globl ForceProgressive576p_length
   ForceProgressive576p_length:
   .long (ForceProgressive576p_length - ForceProgressive576p)