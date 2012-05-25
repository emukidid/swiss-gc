#include "../asm.h"

#	CARDSetStatusAsync - "Sets a file status using a file number asynchronously."
#	r3	chan
#	r4	fileNo
#	r5	stat
#	r6	cb

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

.globl CARDSetStatusAsync
CARDSetStatusAsync:
	b		skip_cb_clear
	li		%r6, 0
skip_cb_clear:
	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	0
	li		%r4,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x40(%sp)
	stw		%r6, 0x24(%sp)

	#Disable external interrupts if they're not already enabled
	mfmsr	%r3
	stw		%r3, 0x20(%sp)
	rlwinm	%r3,%r3,0,17,15
	mtmsr	%r3
	
# Call our code instead - return value in r3 of CARD_RESULT_NOFILE OR CARD_RESULT_READY
	lis		%r3,	0x8000
	ori		%r3,	%r3, 0x1834
	mtctr	%r3
	bctrl
	stw     %r3,	0x1C(%sp)
	
	#Re-enable external interrupts if they were previously
	lwz		%r4, 0x20(%sp)		
	rlwinm	%r4, %r4, 17, 30,31
	cmpwi	%r4,	0
	beq		skip_setting_msr
	mfmsr	%r4
	ori		%r4,%r4,0x8000
	mtmsr	%r4
skip_setting_msr:
	# stash info about the callback somewhere as we'll call it later via our patched OSRestoreInterrupts
	lis		%r4,	VAR_AREA
	lwz     %r3,	0x1C(%sp)
	lwz		%r6,	0x24(%sp)
	cmpwi	%r6,	0
	beq		skip_cb
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2
	stw		%r6,	VAR_MC_CB_ADDR(%r4)	# store callback address

skip_cb:
	mr      %r4,	%r3
	lwz     %r0, 0x44(%sp)
	addi    %sp, %sp, 0x40
	mtlr    %r0
	blr

	.globl CARDSetStatusAsync_length
	CARDSetStatusAsync_length:
	.long (CARDSetStatusAsync_length -CARDSetStatusAsync)