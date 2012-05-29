#include "../asm.h"

#	CARDCheckAsync - "Verifies the file system consistency of the memory card and repairs it if necessary (and possible)."
#	r3	Channel
#	r4	cb( chan, res )

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

.globl CARDCheckAsync
CARDCheckAsync:
	b		skip_cb_clear
	li		%r4, 0
skip_cb_clear:
	mflr	%r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

	cmpwi	%r3, 0
	beq		CARDPresent

	cmpwi	%r4, 0
	beq		NoCardNoCB
	
	mtctr	%r4
	li		%r4, -3
	bctrl

NoCardNoCB:
	li      %r3, -3
	b		end

CARDPresent:

	cmpwi	%r4, 0
	beq		CardNoCB

	# stash info about the callback somewhere as we'll call it later via our patched OSRestoreInterrupts
	mtctr	%r4
	lis		%r4,	VAR_AREA
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2
	mfctr	%r3
	stw		%r3,	VAR_MC_CB_ADDR(%r4)	# store callback address

CardNoCB:
	li      %r3, 0
end:
	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

   .globl CARDCheckAsync_length
   CARDCheckAsync_length:
   .long (CARDCheckAsync_length - CARDCheckAsync)