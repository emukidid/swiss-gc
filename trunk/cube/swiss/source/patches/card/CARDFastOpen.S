#include "../asm.h"

#	CARDFastOpen - "Opens a file using a file number."
#	r3	chan
#	r4	FileNo
#	r5	FileInfo

#	return slot 0 CARD_RESULT_NOFILE or CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDFastOpen
CARDFastOpen:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x40(%sp)	

	cmpwi	%r3,	0
	bne		NoCard
			
# Call our code instead - return value in r3 of CARD_RESULT_NOFILE OR CARD_RESULT_READY
	lis		%r3,	0x8000
	ori		%r3,	%r3, 0x1818
	mtctr	%r3
	bctrl
	
	#Re-enable external interrupts if they were previously
	lwz		%r4, 0x20(%sp)		
	rlwinm	%r4, %r4, 17, 30,31
	cmpwi	%r4,	0
	beq		skip_setting_msr
	mfmsr	%r4
	ori		%r4,%r4,0x8000
	mtmsr	%r4
skip_setting_msr:
	b 		end
	
NoCard:
	li		%r3,	-3
end:

	lwz     %r0,	0x44(%sp)
	addi    %sp,	%sp,	0x40
	mtlr    %r0
	blr

	.globl CARDFastOpen_length
	CARDFastOpen_length:
	.long (CARDFastOpen_length - CARDFastOpen)