#include "../asm.h"

#	CARDCheckExAsync - "Checks for the file system consistency of the memory card and repairs it if possible."
#	r3	Channel
#	r4	*xfer bytes used to repair the FS
#	r5	cb

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

.globl CARDCheckExAsync
CARDCheckExAsync:
	b		skip_cb_clear
	li		%r5, 0
skip_cb_clear:
	mflr	%r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

	cmpwi	%r3, 0
	beq		CARDPresent

	cmpwi	%r5, 0
	beq		NoCardZeroPtr

	li		%r6, 0
	stw		%r6, 0(%r5)

NoCardZeroPtr:
	cmpwi	%r5, 0
	beq		NoCardNoCB
	
	# stash info about the callback somewhere as we'll call it later via our patched OSRestoreInterrupts
	lis		%r4,	VAR_AREA
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	li		%r3, 	-3
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2
	stw		%r5,	VAR_MC_CB_ADDR(%r4)	# store callback address

NoCardNoCB:
	li      %r3, -3
	b		end

CARDPresent:

	cmpwi	%r5, 0
	beq		CardZeroPtr

CardZeroPtr:
	cmpwi	%r5, 0
	beq		CardNoCB

	# stash info about the callback somewhere as we'll call it later via our patched OSRestoreInterrupts
	lis		%r4,	VAR_AREA
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2
	stw		%r5,	VAR_MC_CB_ADDR(%r4)	# store callback address

CardNoCB:
	li      %r3, 0
end:
	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDCheckExAsync_length
	CARDCheckExAsync_length:
	.long (CARDCheckExAsync_length - CARDCheckExAsync)
