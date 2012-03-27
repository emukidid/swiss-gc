#include "asm.h"

# issue read command
#
#	r3	dvdstruct
#	r4	dst	
#	r5	len
#	r6	off

.globl DVDRead
DVDRead:

	stwu    %sp, -0x40(%sp)
	mflr    %r0
	stw     %r0, 8(%sp)
	stw		%r7, 16(%sp)
	
#Call our code to do the load instead

	mfmsr	%r7
	stw		%r7, 0x20(%sp)		# store old MSR
	rlwinm	%r7,%r7,0,17,15
	mtmsr	%r7
	
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

	lwz		%r7, 0x20(%sp)		# load old MSR
	rlwinm	%r7, %r7, 17, 30,31
	cmpwi	%r7,	0
	beq		skip_setting_msr
	mfmsr	%r7
	ori		%r7,%r7,0x8000
	mtmsr	%r7

skip_setting_msr:		
	mr      %r3,	%r5

	lwz		%r7, 16(%sp)
	lwz     %r0, 8(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x40
	blr
	
.globl DVDRead_length
   DVDRead_length:
.long (DVDRead_length - DVDRead)