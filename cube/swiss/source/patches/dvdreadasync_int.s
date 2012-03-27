#include "asm.h"

# issue read command
#
#	r3	dvdstruct
#	r4	dst	
#	r5	len
#	r6	off
#	r7	cb

.globl DVDReadAsyncInt
DVDReadAsyncInt:

	stwu    %sp, -0x10(%sp)
	mflr    %r0
	stw     %r0, 8(%sp)

	stw     %r7,	0x38(%r3)	# Set the callback addr
	
#Call our code to do the load instead
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x180C
	mtctr	%r7
	bctrl
	
#update dvdstruct
	li		%r0,	0
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_END (read complete)
	
	li		%r0,	1
	stw     %r0,	0x08(%r3)	# command : 1
	stw     %r5,	0x14(%r3)	# length: passed in length
	stw     %r4,	0x18(%r3)	# dest: passed in dest
	stw     %r5,	0x1C(%r3)	# curTransferSize: passed in length
	stw     %r5,	0x20(%r3)	# transferredSize: passed in length
# set current file offset.. if it's empty, then set it first to startAddr then add length to it
	lwz		%r0,	0x10(%r3)
	cmpwi	%r0,	0
	bne		skip_set_base_offset
	lwz     %r7,	0x30(%r3)
	stw		%r7,	0x10(%r3)	# offset = startAddr
skip_set_base_offset:
	lwz     %r7,	0x10(%r3)	# read the current offset
	add		%r7, 	%r7, %r5	# offset += length;
	stw     %r7,	0x10(%r3)	# offset is incremented with the read length

#determine if we need to call the callback
	lwz     %r7,	0x38(%r3)
	cmpwi	%r7,	0
	beq		skip_cb
	mtctr	%r7
	mr		%r4,	%r3
	mr		%r3,	%r5
	bctrl
	
skip_cb:
	li      %r3,	1

	lwz     %r0, 8(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x10
	blr
   .globl DVDReadAsyncInt_length
   DVDReadAsyncInt_length:
   .long (DVDReadAsyncInt_length - DVDReadAsyncInt)