#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
#**************************************************************************

# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_DISC_1_LBA, 	-0x100	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 	-0xFC	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 	-0xF8	# is the currently selected disk sector
.set VAR_EXI_BUS_SPD, 	-0xF4	# is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
.set VAR_SD_TYPE, 		-0xF0	# is the Card Type (SDHC=0, SD=1)
.set VAR_EXI_FREQ, 		-0xDC	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 		-0xD8	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_TMP1,  		-0xD4	# space for a variable if required
.set VAR_TMP2,  		-0xD0	# space for a variable if required
.set VAR_TMP3,  		-0xCC	# space for a variable if required
.set VAR_TMP4,  		-0xC8	# space for a variable if required
.set VAR_CB_ADDR,		-0xC4	# high level read callback addr
.set VAR_CB_ARG1,		-0xC0	# high level read callback r3
.set VAR_CB_ARG2,		-0xBC	# high level read callback r4
.set VAR_PROG_MODE,		-0xB8	# data/code to overwrite GXRMode obj with for 480p forcing
.set VAR_MUTE_AUDIO,	-0x20	# does the user want audio muted during reads?
.set VAR_ASPECT_FLOAT,	-0x1C	# Aspect ratio multiply float (4 bytes)
.set VAR_MEMCARD_LBA,	-0x18	# Memory card file base on SD
.set VAR_MEMCARD_WORK,	-0x14	# Memory card work area 40960 bytes big
.set VAR_MEMCARD_RESULT,-0x10	# Last memory card status from a CARD func
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

.section .text
	.globl main
main:
	b		setup_read_type1v1		# low style read version 1
	b		setup_read_type1v2		# low style read version 2
	b		setup_read_type1v3		# low style read version 3
	b		setup_read_type2		# read hi style
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
	
# Read from the device to the destination (device specific code called)
	lwz			3,		12(1)		# load r3 - dst
	lwz			4,		16(1)		# load r4 - len
	lwz			5,		20(1)		# load r5 - offset
	bl			do_read 			# do_read(dst,size, offset);

# Flush the destination - needed?
	lwz			3,		12(1)		# load dst
	lwz			4,		16(1)		# load len
	bl			dcache_flush_icache_inv
	
	bl			resume_audio
	
# Reload regs from the game
	lwz			3,		12(1)		# load r3
	lwz			4,		16(1)		# load r4
	lwz			5,		20(1)		# load r5
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

os_restore_interrupts:
	stwu		1,	-0x20(1) 		# grow stack frame
	mflr		0					# move lr
	stw 		0,	 	0(1)		# save lr
	
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
	beq		skip_cb
	
	# Execute DVD Callback if any was pending
	lis		6,	VAR_AREA
	lwz		7,	VAR_CB_ADDR(6)	# load callback addr
	cmpwi	7, 0
	beq		skip_cb
	
	lwz		3,	VAR_CB_ARG1(6)	# load read length
	lwz		4,	VAR_CB_ARG2(6)	# load dvdstruct addr
	lis		5, 0
	stw		5,	VAR_CB_ADDR(6)	# clear callback
	stw		5,	VAR_CB_ARG1(6)	# clear read length
	stw		5,	VAR_CB_ARG2(6)	# clear dvdstruct addr
	mtctr	7
	bctrl					# call the callback
	
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
	lwz		5, 0x10(1)		# load new msr
	mtmsr	5
	rlwinm	3, 5, 17, 31, 31	# return if they're now enabled or not
	mr		4, 3
	lwz 	0,	0(1)		# load lr
	mtlr	0				# restore lr
	addi	1,	1, 0x20		# kill stack frame
	blr

	.globl pause_audio
pause_audio:
	lis		3,	VAR_AREA
	lhz		4,	VAR_MUTE_AUDIO+2(3)	# load user option
	cmpwi	5, 	4, 0
	beq		5, 	pause_audio_end		# if the user doesn't want to mute audio, return
	lis		3,	0xCC00
	lhz		4,	0x503A(3)
	rlwinm	4,	4, 0, 17, 31
	cmpwi	5,	4, 0
	bne		5,	pause_audio_end		# we weren't playing anything, so return
	li		4,	0
	sth		4,	0x5036(3)			# stop audio
	lis		3,	VAR_AREA
	li		4,	1
	sth		4,	VAR_MUTE_AUDIO(3)	# save that we just disabled audio
pause_audio_end:
	blr

	.globl resume_audio
resume_audio:
	lis		3,	VAR_AREA
	lhz		4,	VAR_MUTE_AUDIO(3)	# restore previous playing state
	cmpwi	5,	4, 0
	beq		5,	resume_audio_end	# no need to resume playback
	li		5,	0
	sth		5,	VAR_MUTE_AUDIO(3)	# clear previous playing state
	lis		3,	0xCC00
	ori		5,	5, 0x8000
	sth		5,	0x5036(3) 			# 0xCC005036|(0x8000)
resume_audio_end:
	blr
	