# Patches into VI Configure to change the video mode to 480p just prior to video being configured
#include "asm.h"

# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_PROG_MODE,		-0xB8	# data/code to overwrite GXRMode obj with for 480p forcing

.globl ForceProgressive
ForceProgressive:
	.long (0x00000002)		# GXRModeObj 480p value
	.long (0x028001E0)		# GXRModeObj 480p value
	.long (0x01E00028)		# GXRModeObj 480p value
	.long (0x00000280)		# GXRModeObj 480p value
	.long (0x01E00000)		# GXRModeObj 480p value
	.long (0x00000000)		# GXRModeObj 480p value
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	nop 					# will be filled in with the instructions we wrote over
	li          %r4, 6
	mtctr       %r4
	lis         %r6, VAR_AREA
	addi        %r4, %r6, VAR_PROG_MODE-4
	lwz         %r5, 4 (%r4)
	srawi       %r5, %r5, 2
	lis		    %r6, 0x8000
	stw         %r5, 0x00CC (%r6)
	subi		%r3, %r3, 4
writemore:
	lwzu       	%r5, 4 (%r4)
	stwu       	%r5, 4 (%r3)
	bdnz+       writemore
	blr
	
   .globl ForceProgressive_length
   ForceProgressive_length:
   .long (ForceProgressive_length - ForceProgressive)