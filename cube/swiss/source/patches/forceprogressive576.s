# Patches into VI Configure to change the video mode to 576p just prior to video being configured
#include "asm.h"

# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_PROG_MODE,		-0xB8	# data/code to overwrite GXRMode obj with for 576p forcing

.globl ForceProgressive576p
ForceProgressive576p:
	.long (0x00000006)		# GXRModeObj 576p value
	.long (0x028001E0)		# GXRModeObj 576p value
	.long (0x02400028)		# GXRModeObj 576p value
	.long (0x00000280)		# GXRModeObj 576p value
	.long (0x02400000)		# GXRModeObj 576p value
	.long (0x00000000)		# GXRModeObj 576p value
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
	
   .globl ForceProgressive576p_length
   ForceProgressive576p_length:
   .long (ForceProgressive576p_length - ForceProgressive576p)