# Swap replacement read disk lba base between disc 1 and disc 1 (or disc 1 and disc 1 if no disc 2) and return ok status
#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl DVDCompareDiskId
DVDCompareDiskId:

	# determine if we need to swap current file base sector
	lis     	%r3, VAR_AREA
	lwz     	%r6, VAR_DISC_1_LBA(%r3)	# r6 = file base for disk 1
	lwz     	%r7, VAR_CUR_DISC_LBA(%r3)	# r7 = current file file base
	cmpw    	5, %r6, %r7       			# cur base != disk 1 base?
	bne     	5, _swap_file_base
	
	lwz     	%r6, VAR_DISC_2_LBA(%r3)	# r6 = file base for disk 2
	stw     	%r6, VAR_CUR_DISC_LBA(%r3)	# store disk 2 file base as current
	b			DVDCompareDiskId_end
	
_swap_file_base:  # so swap it back to disc 1
	stw     	%r6, VAR_CUR_DISC_LBA(%r3)  # store disk 1 file base as current
  
DVDCompareDiskId_end:
	li      	%r3, 1          # return that all is good..
	blr
	
   .globl DVDCompareDiskId_length
   DVDCompareDiskId_length:
   .long (DVDCompareDiskId_length - DVDCompareDiskId)