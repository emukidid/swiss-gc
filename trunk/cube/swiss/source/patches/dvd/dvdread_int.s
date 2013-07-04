#include "../asm.h"

# Issue read command synchronously - and return len.
#
#	r3	dvdstruct
#	r4	dst	
#	r5	len
#	r6	off

# TODO don't allow reading past the end of a file, games may depend on this.

.globl DVDReadInt
DVDReadInt:
	mflr    %r0
	stwu    %sp, -0x40(%sp)
	stw     %r0, 0x44(%sp)
	stw		%r7, 16(%sp)
	stw		%r3, 20(%sp)
	
#Update dvdstruct with "waiting to enter queue" status, immediately
	li		%r0,	2
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_WAITING (waiting to go in the queue)
	li		%r0,	1
	stw     %r0,	0x08(%r3)	# command : 1

#update dvdstruct with length and dst passed in
	li		%r7, 	0			# no cb
	stw     %r5,	0x14(%r3)	# length: passed in length
	stw     %r4,	0x18(%r3)	# dest: passed in dest
	stw		%r6,	0x10(%r3)	# offset = offset (passed in)
	stw     %r5,	0x1C(%r3)	# curTransferSize: length
	li		%r0,	0
	stw     %r0,	0x20(%r3)	# transferredSize: 0 so far
	stw     %r7,	0x38(%r3)	# Set the callback addr in the struct

#Call our method which will add this dvdstruct to our read queue. 
#Once added, it will be read in small pieces every time OSRestoreInterrupts is run.
#Finally, once complete, the struct state is updated to STATE_END (0) and the callback is called (if present).
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x183C
	mtctr	%r7
	bctrl
	
waitloop:
	lwz		%r3, 20(%sp)
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x1844
	mtctr	%r7
	bctrl
	lwz		%r3, 20(%sp)
	lwz		%r3, 0x0C(%r3)
	cmpwi	%r3, 0
	bne		waitloop

	li      %r3,	1
	lwz		%r7, 16(%sp)
	lwz     %r0, 0x44(%sp)
	addi    %sp, %sp, 0x40
	mtlr    %r0
	blr

.globl DVDReadInt_length
   DVDReadInt_length:
.long (DVDReadInt_length - DVDReadInt)
