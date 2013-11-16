#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

#	CARDMountAsync - "Mounts a memory card asynchronously."
#	r3	Channel
#	r4	workarea	-	 5 * 8 * 1024 bytes
#	r5	detachCallback
#	r6	attachCallback

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDMountAsync
CARDMountAsync:
	b		skip_cb_clear
	li		%r6, 0
skip_cb_clear:
	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, 	-0x40(%sp)
	stw     %r6, 	0x24(%sp)

	#Disable external interrupts if they're not already enabled
	mfmsr	%r3
	stw		%r3, 	0x20(%sp)
	rlwinm	%r3,%r3,0,17,15
	mtmsr	%r3
	
	# Store the workarea ptr so that we can use it in our code
	lis		%r3, VAR_AREA
	stw		%r4, VAR_MEMCARD_WORK(%r3)
	
	# call our code to read the memory card "file table" into the work area once off
	lis		%r3,	0x8000
	ori		%r3,	%r3, 0x180C
	mtctr	%r3
	lis		%r3,	VAR_AREA
	li		%r0,	_CARD_SETUP
	stw		%r0,	VAR_MC_FUNC(%r3)
	bctrl
	
	#Re-enable external interrupts if they were previously
	lwz		%r3, 	0x20(%sp)		
	rlwinm	%r3, %r3, 17, 30,31
	cmpwi	%r3,	0
	beq		skip_setting_msr
	mfmsr	%r3
	ori		%r3,%r3,0x8000
	mtmsr	%r3
skip_setting_msr:
	# stash info about the callback somewhere as we'll call it later via our patched OSRestoreInterrupts
	lis		%r4,	VAR_AREA
	li		%r3,	0
	lwz		%r6,	0x24(%sp)
	cmpwi	%r6,	0
	beq		skip_cb
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG2(%r4)	# store callback arg 2
	stw		%r6,	VAR_MC_CB_ADDR(%r4)	# store callback address

skip_cb:
	li		%r3,	0
	li		%r4,	0
	
	lwz     %r0, 0x44(%sp)
	addi    %sp, %sp, 0x40 
	mtlr    %r0
	blr

	.globl CARDMountAsync_length
	CARDMountAsync_length:
	.long (CARDMountAsync_length -CARDMountAsync)