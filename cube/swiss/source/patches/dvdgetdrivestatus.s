#include "asm.h"

# Get drive status
# return value in r3 is 
# 0 DVD_STATE_END 	Request finished/no request.
# 1 DVD_STATE_BUSY 	The drive is busy with processing a request.

# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_READ_DVDSTRUCT,-0x120	# Current read DVDFileInfo

.globl DVDGetDriveStatus
DVDGetDriveStatus:
	lis		%r4,	VAR_AREA			# check if there's some read in progress
	lwz		%r3,	VAR_READ_DVDSTRUCT(%r4)
	cmpwi	%r3,	0
	bne		check_busy					# a read is queued, but is it complete?
	blr									# all OK here, return DVD_STATE_END
check_busy:
	lwz		%r3,	0x0C(%r3)			# read the state
	cmpwi	%r3,	0
	bne		return_isbusy
	blr
return_isbusy:
	li		%r3,	1
	blr
	
.globl DVDGetDriveStatus_length
   DVDGetDriveStatus_length:
.long (DVDGetDriveStatus_length - DVDGetDriveStatus)