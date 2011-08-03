#include "asm.h"

# issue read command
#
#	r3	dvdstruct
#	r4	dst	
#	r5	len
#	r6	off

.globl DVDRead
DVDRead:

	stwu    %sp, -0x20(%sp)
	mflr    %r0
	stw     %r0, 8(%sp)
	stw		%r7, 16(%sp)
	
#Call our code to do the load instead

	mfmsr	%r7
	rlwinm	%r7,%r7,0,17,15
	mtmsr	%r7
	
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x180C
	mtctr	%r7
	bctrl
	
	mfmsr	%r7
	ori		%r7,%r7,0x8000
	mtmsr	%r7
	
#update dvdstruct

	li		%r0,	0
	stw     %r0,	0x00(%r3)
	stw     %r0,	0x04(%r3)
	stw     %r0,	0x24(%r3)
	stw     %r0,	0x2C(%r3)
	stw     %r0,	0x28(%r3)
	stw     %r0,	0x0C(%r3)

	li		%r0,	1
	stw     %r0,	0x08(%r3)

#offset
	lwz     %r0,	0x30(%r3)
	stw     %r0,	0x10(%r3)

#size
	#lwz     %r0,	0x2F40(%r7)
	stw     %r5,	0x14(%r3)

#ptr
	stw     %r4,	0x18(%r3)

#TransferSize
	stw     %r5,	0x1C(%r3)
	stw     %r5,	0x20(%r3)
	
	mr		%r3,	%r5
	
	lwz		%r7, 16(%sp)
	lwz     %r0, 8(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x20
	blr
	
.globl DVDRead_length
   DVDRead_length:
.long (DVDRead_length - DVDRead)