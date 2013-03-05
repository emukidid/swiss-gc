#include "../asm.h"

# Issue cancel command synchronously - and return 0.
#	s32 DVDCancel(DVDCommandBlock* block);
#	r3	dvdstruct

.globl DVDCancel
DVDCancel:
	stwu    %sp, -0x40(%sp)
	mflr    %r0
	stw     %r0, 8(%sp)

#Update block with "cancelled" status, if it hasn't been completed
	lwz     %r0,	0x0C(%r3)	# get state
	cmpwi	%r0,	0			# if this read is already completed, just call the callback and return true
	beq		skip_cancel
	li		%r0,	10
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_CANCELED
skip_cancel:
	li		%r3, 0
	lwz     %r0, 8(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x40
	blr

.globl DVDCancel_length
   DVDCancel_length:
.long (DVDCancel_length - DVDCancel)
