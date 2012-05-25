#include "../asm.h"

#	CardWriteAsync - "Writes data to a file asynchronously."
#	r3	file *
#	r4	buffer (32 bytes aligned)
#	r5	length (multiple of 8192)
#	r6	offset (multiple of 8192)
#	r7	cb

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	Function will never be called for Slot 1 so don't deal with that case

.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

.globl CARDWriteAsync
CARDWriteAsync:
	b		skip_cb_clear
	li		%r7, 0
skip_cb_clear:
	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x40(%sp)
	stw		%r7, 0x24(%sp)

	#Disable external interrupts if they're not already enabled
	mfmsr	%r7
	stw		%r7, 0x20(%sp)
	rlwinm	%r7,%r7,0,17,15
	mtmsr	%r7
	
# Call our code instead - return value in r3 of CARD_RESULT_NOFILE OR CARD_RESULT_READY
	lis		%r7,	0x8000
	ori		%r7,	%r7, 0x182C
	mtctr	%r7
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
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 1
	stw		%r3,	VAR_MC_CB_ARG1(%r4)	# store callback arg 2
	stw		%r7,	VAR_MC_CB_ADDR(%r4)	# store callback address

skip_cb:
	mr      %r4,	%r3
	lwz     %r0,	0x44(%sp)
	addi    %sp,	%sp,	0x40
	mtlr    %r0
	blr

	.globl CARDWriteAsync_length
	CARDWriteAsync_length:
	.long (CARDWriteAsync_length -CARDWriteAsync)