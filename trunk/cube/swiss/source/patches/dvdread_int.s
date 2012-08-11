#include "asm.h"

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

#Disable external interrupts
	mfmsr	%r7
	stw		%r7, 0x20(%sp)		# store old MSR
	rlwinm	%r7,%r7,0,17,15
	mtmsr	%r7

#Call our code to do the load instead
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x180C
	mtctr	%r7
	bctrl

#Update dvdstruct to show that we've completed the read
	li		%r0,	0
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_END (read complete)
	li		%r0,	1
	stw     %r0,	0x08(%r3)	# command : 1
	stw     %r5,	0x14(%r3)	# length: passed in length
	stw     %r4,	0x18(%r3)	# dest: passed in dest
	stw     %r5,	0x1C(%r3)	# curTransferSize: passed in length
	stw     %r5,	0x20(%r3)	# transferredSize: passed in length
	add		%r7, 	%r5, %r6
	stw		%r7,	0x10(%r3)	# offset = offset+length

#Enable external interrupts if we disabled them earlier
	lwz		%r7, 0x20(%sp)		# load old MSR
	rlwinm	%r7, %r7, 17, 30,31
	cmpwi	%r7,	0
	beq		skip_setting_msr
	mfmsr	%r7
	ori		%r7,%r7,0x8000
	mtmsr	%r7

skip_setting_msr:		
	mr      %r3,	%r5			# TODO: round up to nearest 32 byte??
	lwz		%r7, 16(%sp)
	lwz     %r0, 0x44(%sp)
	addi    %sp, %sp, 0x40
	mtlr    %r0
	blr

.globl DVDReadInt_length
   DVDReadInt_length:
.long (DVDReadInt_length - DVDReadInt)
