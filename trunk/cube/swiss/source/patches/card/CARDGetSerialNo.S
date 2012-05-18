#include "../asm.h"

#	CARDGetSerialNo - "Gets the card format serial number."

#	r3	Channel
#	r4	serialNo

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDGetSerialNo
CARDGetSerialNo:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	-3
	blr

DoCode:

	mflr	%r0
	
	lis		%r3,	0xc7bd
	subi	%r3,	%r3,	0x26C
	stw		%r3,	0x00(%r4)

	lis		%r3,	0xf47f
	subi	%r3,	%r3,	0x3924
	stw		%r3,	0x04(%r4)

	li		%r3,	0

	mtlr    %r0
	blr

	.globl CARDGetSerialNo_length
	CARDGetSerialNo_length:
	.long (CARDGetSerialNo_length -CARDGetSerialNo)