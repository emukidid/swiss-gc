#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

#	CARDCreateAsync - "Creates a file asynchronously."
#	r3	chan
#	r4	fileName	-	32 bytes long
#	r5	size		-	Multiple of card sector size
#	r6	fileInfo
#	r7	cb

#	532 bytes to play with here (133 instructions)

#	return slot 0 CARD_RESULT_EXIST or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDCreateAsync
CARDCreateAsync:
	b		skip_cb_clear
	li		%r7, 0
skip_cb_clear:
	cmpwi	%r3, 0
	beq		DoCode
	li		%r3,	-3
	li		%r4,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x40(%sp)	
	stw		%r7,	0x24(%sp)
	#Disable external interrupts if they're not already enabled
	mfmsr	%r3
	stw		%r3, 0x20(%sp)
	rlwinm	%r3,%r3,0,17,15
	mtmsr	%r3
	
# Call our code instead - return value in r3 of CARD_RESULT_EXIST OR CARD_RESULT_READY
	lis		%r3,	0x8000
	ori		%r3,	%r3, 0x1820
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
	lwz		%r7,	0x24(%sp)
	cmpwi	%r7,	0
	beq		skip_cb
	stw		%r7,	VAR_MC_CB_ADDR(%r4)	# store callback address
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2

skip_cb:
	mr      %r4,	%r3
	lwz     %r0,	0x44(%sp)
	addi    %sp,	%sp,	0x40
	mtlr    %r0
	blr

	.globl CARDCreateAsync_length
	CARDCreateAsync_length:
	.long (CARDCreateAsync_length - CARDCreateAsync)