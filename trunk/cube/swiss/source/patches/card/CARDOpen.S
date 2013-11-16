#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

#	CARDOpen - "Opens a file."
#	r3	chan
#	r4	FileName
#	r5	FileInfo

#	return slot 0 CARD_RESULT_NOFILE OR CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDOpen
CARDOpen:

	mflr    %r0
	stw     %r0,	4(%sp)
	stwu    %sp,	-0x40(%sp)	

	cmpwi	%r3,	0
	bne		NoCard

	#Disable external interrupts if they're not already enabled
	mfmsr	%r3
	stw		%r3, 0x20(%sp)
	rlwinm	%r3,%r3,0,17,15
	mtmsr	%r3
	
# Call our code instead - return value in r3 of CARD_RESULT_NOFILE OR CARD_RESULT_READY
	lis		%r3,	0x8000
	ori		%r3,	%r3, 0x180C
	mtctr	%r3
	lis		%r3,	VAR_AREA
	li		%r0,	_CARD_OPEN
	stw		%r0,	VAR_MC_FUNC(%r3)
	bctrl
	stw		%r3,	0x1C(%sp)
	
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
	stw		%r3,	0x1C(%sp)
end:
	lwz		%r3,	0x1C(%sp)
	mr		%r4,	%r3

	lwz     %r0,	0x44(%sp)
	addi    %sp,	%sp,	0x40
	mtlr    %r0
	blr

	.globl CARDOpen_length
	CARDOpen_length:
	.long (CARDOpen_length -CARDOpen)