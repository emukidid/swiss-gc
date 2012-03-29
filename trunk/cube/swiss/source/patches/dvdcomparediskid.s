.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_DISC_1_LBA, 	-0x100	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 	-0xF0	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 	-0xE0	# is the currently selected disk sector

.globl DVDCompareDiskId
DVDCompareDiskId:

	# determine if we need to swap current file base sector
	lis     	3, VAR_AREA
	lwz     	6, VAR_DISC_1_LBA(3)	# r6 = file base for disk 1
	lwz     	7, VAR_CUR_DISC_LBA(3)	# r7 = current file file base
	cmpw    	5, 6, 7       			# cur base != disk 1 base?
	bne     	5, _swap_file_base
	
	lwz     	6, VAR_DISC_2_LBA(3)	# r6 = file base for disk 2
	stw     	6, VAR_CUR_DISC_LBA(3)	# store disk 2 file base as current
	b			DVDCompareDiskId_end
	
_swap_file_base:  # so swap it back to disc 1
	stw     	6, VAR_CUR_DISC_LBA(3)  # store disk 1 file base as current
  
DVDCompareDiskId_end:
	li      	3, 1          # return that all is good..
	blr
	
   .globl DVDCompareDiskId_length
   DVDCompareDiskId_length:
   .long (DVDCompareDiskId_length - DVDCompareDiskId)