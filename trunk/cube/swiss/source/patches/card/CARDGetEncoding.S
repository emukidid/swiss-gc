#include "../asm.h"

#	CARDGetEncoding - "Returns the character encoding that is used by the specified memory card."
#	r3	Channel
#	r4	endcoding(u16)

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDGetEncoding
CARDGetEncoding:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

#0 is USA/EUR, 1 is JAP
	li		%r0,	0
	sth		%r0,	0(%r4)

end:
	li		%r3,	0

	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDGetEncoding_length
	CARDGetEncoding_length:
	.long (CARDGetEncoding_length - CARDGetEncoding)