#include "../asm.h"

#	CARDCheckExAsync - "Checks for the file system consistency of the memory card and repairs it if possible."
#	r3	Channel
#	r4	*xfer bytes used to repair the FS
#	r5	cb

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

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
	
	mtctr	%r5
	li		%r4, -3
	bctrl

NoCardNoCB:
	li      %r3, -3
	b		end

CARDPresent:

	cmpwi	%r5, 0
	beq		CardZeroPtr

	li		%r6, 0
	stw		%r6, 0(%r5)

CardZeroPtr:
	cmpwi	%r5, 0
	beq		CardNoCB

	mtctr	%r5
	li		%r4, 0
	bctrl

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
