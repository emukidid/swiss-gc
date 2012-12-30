#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.globl _start, __main
_start:
	b		setup_read_type1v1		# low style read version 1
	b		setup_read_type1v2		# low style read version 2
	b		setup_read_type1v3		# low style read version 3
	b		setup_read_type2		# read hi style, queued with callback
	b		os_restore_interrupts	# custom os restore interrupts which will call the dvd callback
	b		card_open				# s32 card_open(char* fileName, CARDFileInfo* fileInfo)
	b		card_fastopen			# s32 card_fastopen(s32 fileNo, CARDFileInfo* fileInfo)
	b		card_close				# void card_close(CARDFileInfo* fileInfo)
	b		card_create				# s32 card_create(char* fileName, u32 size, CARDFileInfo* fileInfo)
	b		card_delete				# s32 card_delete(char* fileName)
	b		card_read				# s32 card_read(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset)
	b		card_write				# s32 card_write(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset)
	b		card_getstatus			# s32 card_getstatus(s32 fileNo, CARDStat* stat)
	b		card_setstatus			# s32 card_setstatus(s32 fileNo, CARDStat* stat)
	b		card_setup				# void card_setup(); // Read our virtual memcard file table into the work area
	b		add_read_to_queue		# void add_read_to_queue(DVDFileInfo* struct)

	.globl perform_read
perform_read:
	mflr    	0             		# move lr  
	stwu    	1,  -64(1)   		# grow stack frame 
	stw     	0,   68(1)    		# save lr
	stw			3,	12(1)
	stw			4,	16(1)
	stw			5,	20(1)
	stw			6,	24(1)
	bl			device_frag_read
	lwz			3,	12(1)
	lwz			4,	16(1)
	lwz			5,	20(1)
	lwz			6,	24(1)
	lwz     	0,	68(1)    		# load lr
	addi    	1,  1, 64   		# grow stack frame 
	mtlr    	0             		# move lr  
	blr
	
# Low level read
	.globl setup_read_type1v1
setup_read_type1v1:
	stwu		1, -0x0028 (1)		# these are instructions we patched over, so do them here
	stw			30, 32 (1)
	mflr		0
	stw			0, 36(1)			# use this spot to save lr
	bl			perform_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li     		5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz			0, 36(1)
	mtlr		0
	stw			31, 36 (1)			# was patched over, do it now.
	li			0, 0				# was patched over, do it now.
	blr

	.globl setup_read_type1v2
setup_read_type1v2:
	stwu		1, -0x0020 (1)		# these 4 are instructions we patched over, so do them here
	stw 		31, 28 (1)
	# this read stores dst and len using:
	# stw 		3, 8 (1)  
	# mr        31, 4    
	# so we'll set it later to ours.
	mflr		31
	stw			31, 8(1)
	bl			perform_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li      	5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz			31, 8(1)
	mtlr		31
	stw 		3, 8 (1)  			# was patched over, do it now.
	mr        	31, 4  				# was patched over, do it now.
	blr

	.globl setup_read_type1v3
setup_read_type1v3:
	stwu    	1, -0x0028 (1)		# these are instructions we patched over, so do them here
	stw     	30, 32 (1)
	mflr	  	0
	stw	  		0, 36(1)			# use this spot to save lr
	bl	  		perform_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li      	5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz	  		0, 36(1)
	mtlr    	0
	stw     	31, 36 (1)			# was patched over, do it now.
	li      	0, 1				# was patched over, do it now.
	blr
	
# FST style read (immediate, no callback) - so we need to add the FP.offset to the File.offset
	.globl setup_read_type2
setup_read_type2:
	mflr    	0             		# move lr  
	stwu    	1,  -64(1)   		# grow stack frame 
	stw     	0,   68(1)    		# save lr
	stw			3,	12(1)
	stw			4,	16(1)
	stw			5,	20(1)
	stw			6,	24(1)
	lwz			0,	0x30(3)			# read offset and add it to file offset (r6)
	add			6, 6, 0
	mr			3,	4				# dst
	mr			4,	5				# len
	mr			5,	6				# ofs
	bl			perform_read
	lwz			3,	12(1)
	lwz			4,	16(1)
	lwz			5,	20(1)
	lwz			6,	24(1)
	lwz     	0,	68(1)    		# load lr
	addi    	1,  1, 64   		# grow stack frame 
	mtlr    	0             		# move lr  
	blr

	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		4, 4, 0x20 
1:
	addi 		4, 4, 0x1f
	srwi 		4, 4, 5
	mtctr 		4
2:
	dcbf 		0, 3
	addi 		3, 3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr

	.globl os_restore_interrupts
os_restore_interrupts:
	mflr		0					# move lr
	stwu		1,	-0x20(1) 		# grow stack frame
	stw 		0,	 0x24(1)		# save lr
	
	cmpwi	3, 0
	mfmsr	4
	beq		disable_interrupts
	
	ori		5, 4, 0x8000		# enable interrupts
	b		write_msr
disable_interrupts:
	rlwinm	5, 4, 0, 17, 15		# disable interrupts
write_msr:
	stw		5, 0x10(1)			# store msr

	cmpwi	3, 0
	beq		skip_cb					# Some games like this and play with no stutter, others hang/crash..
	bl		process_read_queue		# process some of the dvd read queue
		
check_mc_cb:
	# Execute Memcard Callback if any was pending
	lis		6,	VAR_AREA
	lwz		7,	VAR_MC_CB_ADDR(6)	# load callback addr
	cmpwi	7, 0
	beq		skip_cb
	
	lwz		3,	VAR_MC_CB_ARG1(6)	# load cb arg 1
	lwz		4,	VAR_MC_CB_ARG2(6)	# load cb arg 2
	lis		5, 0
	stw		5,	VAR_MC_CB_ADDR(6)	# clear callback
	stw		5,	VAR_MC_CB_ARG1(6)	# clear cb arg 1
	stw		5,	VAR_MC_CB_ARG2(6)	# clear cb arg 2
	mtctr	7
	bctrl					# call the callback	
skip_cb:
	lwz		5, 0x10(1)			# load new msr
	mtmsr	5					# put MSR in place
	rlwinm	3, 5, 17, 31, 31	# return if they're now enabled or not
	mr		4, 3
	lwz 	0,	0x24(1)		# load lr
	addi	1,	1, 0x20		# kill stack frame
	mtlr	0				# restore lr
	blr

	.globl process_read_queue
process_read_queue:
	mflr		0					# move lr
	stwu		1,	-0x40(1) 		# grow stack frame
	stw 		0,	 0x44(1)		# save lr

# If there is a pending DVD read, execute some of it before enabling interrupts again
	bl		get_queued_read
	cmpwi	3,	0
	beq		common_exit			# nothing to read, return

	li		0,	1
	stw		0,	0x0C(3)			# this struct has entered "busy" state

	lwz		7,	0x1C(3)			# load curTransferSize
	cmpwi	7,	4096			# is this a small < 4096 byte read?
	blt		reduced_read		# then read it
	li		7,	4096			# else, 4096 byte read for now
reduced_read:
	stw		3,	0x18(1)			# store the struct we're processing
	stw		7,	0x14(1)			# store the amount we're about to read
	lwz		4,	0x30(3)			# File base to read from
	lwz		5,	0x10(3)			# Offset to read from
	add		5,	5,	4			# offset = startAddr + offset
	mr		4,	7				# Amount to read
	lwz		6,	0x20(3)			# transferredSize
	lwz		3,	0x18(3)			# Destination base to read to
	add		3,	3,	6			# Destination to read to = dest + transferredSize
	bl		perform_read		# perform a small read
# update DVD struct with how much we've transferred now
	lwz		3,	0x18(1)			# re-load the struct we read for
	lwz		7,	0x14(1)			# re-load the amount we read
	lwz		6,	0x20(3)			# transferredSize
	add		6,	6,	7			# transferredSize += read amount
	stw     6,	0x20(3)			# store it
	lwz		6,	0x10(3)			# offset
	add		6,	6,	7			# offset += read amount
	stw     6,	0x10(3)			# store it
	lwz		6,	0x1C(3)			# curTransferSize
	sub		6,	6, 7			# curTransferSize -= read amount
	stw     6,	0x1C(3)			# store it

	cmpwi	6,	0
	bne		common_exit			# exit now
	
update_dvd_struct:
	# Update the DVD struct to indicate the DVD read is complete
	li		0,	0
	stw     0,	0x0C(3)			# state: DVD_STATE_END (read complete)
	
	# Execute DVD Callback if this struct had one
	lwz		7,	0x38(3)			# load callback addr
	cmpwi	7, 0
	beq		common_exit
	
	mr		4,	3				# load dvdstruct addr
	lwz		3,	0x20(3)			# load read length
	li		5, 0
	mtctr	7
	bctrl						# call the callback

common_exit:
	lwz 	0,	0x44(1)			# load lr
	addi	1,	1, 0x40			# kill stack frame
	mtlr	0					# restore lr
	blr
