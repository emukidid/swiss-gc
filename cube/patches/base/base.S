#***************************************************************************
# HDD/SD Read code launch pad
#
# Patchcode to be at 0x80001800
# To be used for Read and DVDLowReadDiskId, or DVDReadPrio/DVDReadPrioAsync
#**************************************************************************

# memory map
# we have 0x1800 bytes to play with at 0x80001800 (code+data)
# This code is placed at 0x80001800 and calls a C code read implementation for a specific device
# 0x80002F00 is the base file sector for disk 1
# 0x80002F10 is the base file sector for disk 2
# 0x80002F20 is the currently selected disk sector
# 0x80002F30 is the EXI bus speed (16mhz vs 32mhz)
# 0x80002F80 is the Disc ID of the first disk
# 0x80002FA0 is the Disc ID of the second disk
# 0x80002FC0 is a 32 byte area to redirect reads to

.section .text
	.globl main
main:
	b		setup_read_type1v1	# low style read version 1
	b		setup_read_type1v2	# low style read version 2
	b		setup_read_type1v3	# low style read version 3
	b		setup_read_type2	# read hi style
	b		do_readdiskid		# read a disk id
	
# Low level read
setup_read_type1v1:
	stwu		1, -0x0028 (1)		# these are instructions we patched over, so do them here
	stw			30, 32 (1)
	mflr		0
	stw			0, 36(1)			# use this spot to save lr
	bl			setup_for_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li     		5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz			0, 36(1)
	mtlr		0
	stw			31, 36 (1)			# was patched over, do it now.
	li			0, 0				# was patched over, do it now.
	blr

setup_read_type1v2:
	stwu		1, -0x0020 (1)		# these 4 are instructions we patched over, so do them here
	stw 		31, 28 (1)
	# this read stores dst and len using:
	# stw 		3, 8 (1)  
	# mr        31, 4    
	# so we'll set it later to ours.
	mflr		31
	stw			31, 8(1)
	bl			setup_for_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li      	5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz			31, 8(1)
	mtlr		31
	stw 		3, 8 (1)  			# was patched over, do it now.
	mr        	31, 4  				# was patched over, do it now.
	blr
  
setup_read_type1v3:
	stwu    	1, -0x0028 (1)		# these are instructions we patched over, so do them here
	stw     	30, 32 (1)
	mflr	  	0
	stw	  		0, 36(1)			# use this spot to save lr
	bl	  		setup_for_read
	# clear everything that'll go into DI regs for a 0xE0 cmd
	li      	5,	0         		# offset 0
	li      	4,	0         		# len 0
	li			3,	0  				# to this dest
	lwz	  		0, 36(1)
	mtlr    	0
	stw     	31, 36 (1)			# was patched over, do it now.
	li      	0, 1				# was patched over, do it now.
	blr
	
# FST style read - so we need to add the FP.offset to the File.offset
setup_read_type2:
	stwu    	1,  -64(1)   		# grow stack frame 
	mflr    	0             		# move lr  
	stw     	0,    0(1)    		# save lr
	stw			3,	12(1)
	stw			4,	16(1)
	stw			5,	20(1)
	stw			6,	24(1)
	lwz			0,	0x30(3)			# read offset and add it to file offset (r6)
	add			6, 6, 0
	mr			3,	4				# dst
	mr			4,	5				# len
	mr			5,	6				# ofs
	bl			setup_for_read
	lwz     	0,    0(1)    		# save lr
	lwz			3,	12(1)
	lwz			4,	16(1)
	lwz			5,	20(1)
	lwz			6,	24(1)
	addi    	1,  1, 64   		# grow stack frame 
	mtlr    	0             		# move lr  
	blr
  
setup_for_read:
	cmpwi		3,		0			# no dst? return
	beqlr
	cmpwi		4,		0			# no len? return
	beqlr
	stwu		1,	-128(1) 		# grow stack frame

	mflr		0					# move lr
	stw 		0,	 	0(1)		# save lr
	stw			3,		12(1)		# save r3 - dst
	stw			4,		16(1)		# save r4 - len
	stw			5,		20(1)		# save r5 - offset
	stw 		6,		24(1)		# save r6
	stw 		7,		28(1)		# save r7
	stw 		8,		32(1)		# save r8
	stw 		9,		36(1)		# save r9
	stw 		10, 	40(1)		# save r10
	stw 		11, 	44(1)		# save r11
	stmw		16, 	48(1)		# save r16->r31

# Read from the device to the destination (C code called)
	bl			do_read 			# do_read(dst,size, offset);

# Flush the destination - needed?
	lwz			3,		12(1)		# load dst
	lwz			4,		16(1)		# load len
	bl			dcache_flush_icache_inv
	
# Reload regs from the game
	lwz			3,		12(1)		# load r3
	lwz			4,		16(1)		# load r4
	lwz			5,		20(1)		# load r5
	lwz 		6,		24(1)		# load r6
	lwz 		7,		28(1)		# load r7
	lwz 		8,		32(1)		# load r8
	lwz 		9,		36(1)		# load r9
	lwz 		10, 	40(1)		# load r10
	lwz 		11, 	44(1)		# load r11
	lmw			16, 	48(1)		# load r16->r31
	lwz 		0,		0(1)		# load lr
	mtlr		0					# restore lr
	addi		1,		1, 128		# kill stack frame
	blr

# Cache function
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
#	icbi 		0, 3
	addi 		3, 3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr

# End of Cache function

do_readdiskid:
	stw     	0, 4 (1)      # this is what we patched over, so do it here.
	
	# determine if we need to swap current file base sector
	lis     	5, 0x8000
	lwz     	6, 0x2F00(5)  # r6 = file base for disk 1
	lwz     	7, 0x2F20(5)  # r7 = current file file base
	cmpw    	5, 6, 7       # cur base != disk 1 base?
	bne     	5, _swap_file_base
	
	lwz     	6, 0x2F10(5)  # r6 = file base for disk 2
	stw     	6, 0x2F20(5)  # store disk 2 file base as current
	lis     	5, 0x8000
	ori     	5, 5, 0x2FA0  # src=0x80002FA0 (disk 2 id)
	b       	_do_id_copy
  
_swap_file_base:  # so swap it
	stw     	6, 0x2F20(5)  # store disk 1 file base as current
	lis     	5, 0x8000
	ori     	5, 5, 0x2F80  # src=0x80002F80 (disk 1 id)
  
_do_id_copy:
  li      		6, 8          # rem=32/4 == 8;
loopReadId:
	lwz     	7, 0(5)       # r7 = src[i]
	stw     	7, 0(3)       # dst[i] = r7
	addi    	3, 3, 4       # dst+=4;
	addi    	6, 6, -1      # rem--;
	addi    	5, 5, 4       # src+=4;
	cmpwi   	5, 6, 0 
	bne     	5, loopReadId   
	
	lis     	3, 0x8000  
	ori     	3, 3, 0x2FC0
	blr
