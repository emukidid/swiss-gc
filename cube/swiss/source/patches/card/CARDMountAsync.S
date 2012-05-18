#include "../asm.h"

#	CARDMountAsync - "Mounts a memory card asynchronously."
#	r3	Channel
#	r4	workarea	-	 5 * 8 * 1024 bytess
#	r5	detachCallback
#	r6	attachCallback

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDMountAsync
CARDMountAsync:

	cmpwi	%r3,	0
	beq		DoCode
	li		%r3,	-3
	blr

DoCode:

	mflr    %r0
	stw     %r0, 4(%sp)
	stwu    %sp, -0x10(%sp)	

	li		%r3,	0
	li		%r4,	0

	cmpwi	%r6,	0
	beq		end
	
	mtctr	%r6
	bctrl

end:
	li		%r3,	0
	li		%r4,	0

# TODO - Store the workarea ptr so that we can use it in our code
	
	lwz     %r0, 0x14(%sp)
	addi    %sp, %sp, 0x10
	mtlr    %r0
	blr

	.globl CARDMountAsync_length
	CARDMountAsync_length:
	.long (CARDMountAsync_length -CARDMountAsync)