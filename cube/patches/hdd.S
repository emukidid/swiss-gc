#***************************************************************************
# HDD Read code
#
# Patchcode to be at 0x80001800 and to be called for Read and DVDLowReadDiskId
#**************************************************************************

# memory map
# we have 0x1800 bytes to play with at 0x80001800 (code+data)
# This code is placed at 0x80001800
# 0x80002B00 is a 512 byte area used to read into
# 0x80002F00 is the base file sector for disk 1
# 0x80002F10 is the base file sector for disk 2
# 0x80002F20 is the currently selected disk sector
# 0x80002F30 is the SD card speed
# 0x80002F40 is the Video mode to patch
# 0x80002F80 is the Disc ID of the first disk
# 0x80002FA0 is the Disc ID of the second disk
# 0x80002FC0 is a 32 byte area to redirect reads to

# register map
# r16 is the 32-bit LBA (2TB Max)
# r17 is EXI speed
# r18 is constant 0xFFFFFFFF
# r19 is constant 0xCC000000
# r20 is saved dst (r3)
# r21 is saved len (r4)
# r22 is saved ofs (r5)
# r23,r24,r25,r26 used in ata_read_sector(s)
# r27,r28,r29 used in do_read
# r30 holds the Audio DMA status
# r31 is temp LR in various functions that are > 1 branch deep

# ATA register addresses
.set ATA_REG_DATA		,0x10
.set ATA_REG_COMMAND	,0x17 
.set ATA_REG_DEVICE		,0x16
.set ATA_REG_LBAHI		,0x15
.set ATA_REG_LBAMID		,0x14
.set ATA_REG_LBALO		,0x13
.set ATA_REG_SECCOUNT	,0x12
.set ATA_REG_STATUS		,0x17



# ATA commands
.set ATA_CMD_READSECT		,0x20
.set ATA_CMD_READSECTEXT	,0x24

# ATA Status register bits we care about
# ATA_SR_BSY		0x80
# ATA_SR_DRQ		0x08

# rlwinm args dest-register, source-register, rotate-left, mask-left, mask-right

# EXI Address defines
#SLOT A
#.set CHN_SR,		0x6800
#.set CHN_CR,		0x680C
#.set CHN_DATA,		0x6810

#SLOT B
.set CHN_SR,		0x6814
.set CHN_CR,		0x6820
.set CHN_DATA,		0x6824

.section .text
	.globl _start
_start:

	b		_do_read_v1				# read
	b		_do_readdiskid			# read a disk id
	b		_do_videomode			# set a different video mode to r3
	b		_do_seek				# make seek go to offset 0
	b		_do_read_v2				# patch read version 2

_do_read_v1:
	stwu		1,	-0x0028 (1)		# these are instructions we patched over, so do them here
	stw			30, 32 (1)
	mflr		31
	stw			31, 36(1)			# use this spot to save lr
	b		do_read_generic
	lwz			0, 36(1)
	mtlr		0
	stw			31, 36 (1)			# was patched over, do it now.
	li			0, 0				# was patched over, do it now.
	blr
  
_do_read_v2:
	stwu		1, 	-0x0020 (1)			# these 4 are instructions we patched over, so do them here
	stw 		31, 28 (1)
	# this read stores dst and len using:
	# stw 		3, 	8 (1)  
	# mr        31, 4    
	# so we'll set it later to ours.
	mflr		31
	stw			31, 8(1)
	bl			do_read_generic
	lwz			31, 8(1)
	mtlr		31
	stw 		3, 	8 (1)  
	mr        	31, 4  
	blr
	
_do_seek:
	stw 	0,		4(1)			# this is what we patched over, so do it here.
	li		3,		0				# make it go to offset 0
	blr

_do_videomode:
	stw 	0,		4(1)			# this is what we patched over, so do it here.
	lis 	3,		0x8000
	lwz 	3,		0x2F40(3)		# load our video mode to r3 ;)
	blr

_do_readdiskid:
	stw     0, 4 (1)      # this is what we patched over, so do it here.
	
	# determine if we need to swap current file base sector
	lis     5, 0x8000
	lwz     6, 0x2F00(5)  # r6 = file base for disk 1
	lwz     7, 0x2F20(5)  # r7 = current file file base
	cmpw    5, 6, 7       # cur base != disk 1 base?
	bne     5, _swap_file_base
	
	lwz     6, 0x2F10(5)  # r6 = file base for disk 2
	stw     6, 0x2F20(5)  # store disk 2 file base as current
	lis     5, 0x8000
	ori     5, 5, 0x2FA0  # src=0x80002FA0 (disk 2 id)
	b       _do_id_copy
  
_swap_file_base:  # so swap it
	stw     6, 0x2F20(5)  # store disk 1 file base as current
	lis     5, 0x8000
	ori     5, 5, 0x2F80  # src=0x80002F80 (disk 1 id)
  
_do_id_copy:
  li      6, 8          # rem=32/4 == 8;
loopReadId:
	lwz     7, 0(5)       # r7 = src[i]
	stw     7, 0(3)       # dst[i] = r7
	addi    3, 3, 4       # dst+=4;
	addi    6, 6, -1      # rem--;
	addi    5, 5, 4       # src+=4;
	cmpwi   5, 6, 0 
	bne     5, loopReadId   
	
	lis     3, 0x8000  
	ori     3, 3, 0x2FC0
	blr

# Animal Crossing NTSC first read
# r3  0x800F0C80	dst
# r4  0x00008000	len
# r5  0x00626D40	ofs
# r6  0x0000EB7F	lba base

do_read_generic:
	cmpwi	3,		0			# no dst? return
	beqlr
	cmpwi	4,		0			# no len? return
	beqlr
	stwu	1,	-128(1) 		# grow stack frame

	mflr	0					# move lr
	stw 	0,	 	0(1)		# save lr
	stw 	6,		12(1)		# save r6
	stw 	7,		16(1)		# save r7
	stw 	8,		20(1)		# save r8
	stw 	9,		24(1)		# save r9
	stw 	10, 	28(1)		# save r10
	stw 	11, 	32(1)		# save r11
	stmw	16, 	36(1)		# save r16->r31

# Keep a back up copy of dst,len,ofs in r20,r21,r22
	mr		20, 	3			# r20 is saved dst (r3)
	mr		21, 	4			# r21 is saved len (r4)
	mr		22, 	5			# r22 is saved ofs (r5)

# Setup our non-volatile registers
	lis 	3,		0x8000
	lwz 	16, 	0x2F20(3)	# load sector address from memory to r16
	lwz 	17, 	0x2F30(3)	# load hdd speed from memory to r17
	li		18, 	-1			# r18 is constant 0xFFFFFFFF
	lis 	19, 	0xCC00		# r19 is constant 0xCC000000

# Kill audio if it's a large read
#	lis		4,		0x4			# r4 = 0x40000
#	cmpw	5,		21,	4		# length < 256kb?
#	blt		5,		drS1		# yes, lets not disable audio
#	lhz		3,		0x5036(19)	# r3 = *(u16*)0xCC005036
#	mr		30,		3			# r30 = r3
#	rlwinm 	3, 3, 0, 17, 31
#	sth		3,		0x5036(19)	# *(u16*)0xCC005036 = ((*(u16*)0xCC005036)&~0x8000);
#	
## Reload dst,len,ofs	
#drS1:
	mr		3,		20			# load dst from r20

# Read from the HDD to the destination
	bl		do_read 			# do_read(dst,size, offset);

# Flush the destination - needed?
	mr		3,		20			# load dst from r20
	mr		4,		21			# load len from r21
	bl		dcache_flush_icache_inv
	
## Re-enable audio if it was a large read
#	lis		4,		0x4			# r4 = 0x40000
#	cmpw	5,		21,	4		# length < 256kb?
#	blt		5,		drS2		# yes, lets not re-enable audio
#	rlwinm 	3, 30, 17, 30, 31	# r3 = *(u16*)0xCC005036 & 0x8000
#	cmpwi	5,		3, 1		# was the audio even playing before?
#	bne		5,		drS2		# No it was not, don't re-enable
#	lis		30,		0
#	ori		30,	30, 0x8000	
#	sth		30,		0x5036(19)	# *(u16*)0xCC005036 = previous state;
#	
#drS2:
# Reload regs from the game
	lwz 	6,		12(1)		# reload r6
	lwz 	7,		16(1)		# reload r7
	lwz 	8,		20(1)		# reload r8
	lwz 	9,		24(1)		# reload r9
	lwz 	10, 	28(1)		# reload r10
	lwz 	11, 	32(1)		# reload r11
	lmw 	16, 	36(1)		# reload r16->r31

# Setup registers for a small read from sector 0
	li		5,		0			# offset 0
	li		4,		32			# len 32
	lis 	3,		0x8000		# to this dest
	ori 	3,		3, 0x2FC0	# in memory
	lwz 	0,		0(1)		# load lr
	mtlr	0					# restore lr
	addi	1,		1, 128		# kill stack frame
	blr


# EXI Functions

# SPI - Returns the ATA status register (8 bits) - Returns in r3: Clobbers: r0
# u8 ataReadStatusReg()
ata_read_status_reg:
	lwz 	0,		CHN_SR(19)		# select EXI channel/freq/slot
	andi.	0,		0, 0x405    	
	or		0,		0, 17			# speed set by loader (192 or 208)
	stw 	0,		CHN_SR(19)  	
                                	
	lis 	3,		0x1700			# ATA command: read ATA_REG_STATUS | 0x00 (dummy)
	stw 	3,		CHN_DATA(19)	# write it out to the EXI data reg
	li		3,		0x15			# tell EXI we're doing a 2 byte write
	stw 	3,	 	CHN_CR(19) 		# exi[SD_CHANNEL * 5 + 3] = ((2-1) << 4) | (1 << 2) | 1;
                                	
loopATARB1: 						# wait while EXI writes
	lwz 	3,		CHN_CR(19)  	
	rlwinm	3,		3, 0, 30, 31	
	cmpwi	5,		3, 1        	
	beq 	5,		loopATARB1		# while (exi[SD_CHANNEL * 5 + 3] & 1);
                                	
	li		3,		1				# tell EXI we're doing a 1 byte read
	stw 	18, 	CHN_DATA(19)	# write out -1 to read data reg
	stw 	3,		CHN_CR(19)		# exi[SD_CHANNEL * 5 + 3] = ((1-1) << 4) | (0 << 2) | 1;
                                	
loopATARB2:                     	
	lwz 	3,		CHN_CR(19)  	
	rlwinm	3,		3, 0, 30, 31	
	cmpwi	5,		3, 1        	
	beq 	5,		loopATARB2		# while (exi[SD_CHANNEL * 5 + 3] & 1);
	lwz 	3,		CHN_DATA(19)	
    rlwinm	3,		3, 8, 24, 31	# return word >>24   
                       	
	lwz 	0,		CHN_SR(19)		# deselect EXI channel
	andi.	0,		0, 0x405    	
	stw 	0,		CHN_SR(19)                    	
	blr

# SPI - Writes r4 (8 bits) the ATA register given in r3 - Clobbers: r0,r3,r4,r5
# ataWriteByte(u8 addr, u8 data)
ata_write_byte:
	lwz 	0,		CHN_SR(19)		# select EXI channel/freq/slot
	andi.	0,		0, 0x405    	
	or		0,		0, 17			# speed set by loader (192 or 208)
	stw 	0,		CHN_SR(19)

	# 0x80000000 | (addr << 24) | (data<<16);
	lis 	5,		0x8000			# r5 = 0x80000000
	rlwinm	3,		3, 24, 0, 7		# r3 = addr<<24
	or		5,		5, 3			# r5 = 0x80000000 | addr<<24
	rlwinm	4,		4, 16, 8, 15	# r4 = data<<16
	or		3,		5, 4			# r3 = (0x80000000 | addr<<24) | data<<16

	stw 	3,		CHN_DATA(19)	# write it out to the EXI data reg
	li		3,		0x35			# tell EXI we're doing a 4 byte write
	stw 	3,		CHN_CR(19)		# exi[SD_CHANNEL * 5 + 3] = ((3-1) << 4) | (1 << 2) | 1;

loopATAWB1: 						# wait while EXI writes
	lwz 	3,		CHN_CR(19)
	rlwinm	3,		3, 0, 30, 31
	cmpwi	5,		3, 1
	beq 	5,		loopATAWB1		# while (exi[SD_CHANNEL * 5 + 3] & 1);

	lwz 	0,		CHN_SR(19)		# deselect EXI channel
	andi.	0,		0, 0x405
	stw 	0,		CHN_SR(19)
	blr


# SPI - Initialises a 32-bit read mode for the given number of sectors - Clobbers: r0,r3,r4,r5
# ataRead_init_mult(u16 numSectors)
ata_read_init_mult:
	lwz 	0,		CHN_SR(19)		# select EXI channel/freq/slot
	andi.	0,		0, 0x405    	
	or		0,		0, 17			# speed set by loader (192 or 208)
	stw 	0,		CHN_SR(19)

	# u16 dwords = (numSectors<<7);
	slwi	3,		3, 7			# number of sectors is now number of 32bit words to read
	# 0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8);
	lis 	4,		0x7000			# r4 = 0x70000000
	rlwinm	5,		3, 16, 8, 15	# r5 = ((dwords&0xff) << 16)
	or		4,		4, 5			# r4 = 0x70000000 | ((dwords&0xff) << 16)
	rlwinm	5,		3, 0, 16, 23	# r5 = (((dwords>>8)&0xff) << 8)
	or		3,		4, 5			# r3 =  0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8)

	stw 	3,		CHN_DATA(19)	# write it out to the EXI data reg
	li		3,		0x35			# tell EXI we're doing a 4 byte write
	stw 	3,		CHN_CR(19)		# exi[SD_CHANNEL * 5 + 3] = ((4-1) << 4) | (1 << 2) | 1;

loopATARIM1:						# wait while EXI writes
	lwz 	3,		CHN_CR(19)
	rlwinm	3,		3, 0, 30, 31
	cmpwi	5,		3, 1
	beq 	5,		loopATARIM1 	# while (exi[SD_CHANNEL * 5 + 3] & 1);

	lwz 	0,		CHN_SR(19)		# deselect EXI channel
	andi.	0,		0, 0x405
	stw 	0,		CHN_SR(19)
	blr

# SPI - ATA 32 bit read (must call ataRead_init_mult first) - Returns in r3: Clobbers: r0
# u32 ataRead32_mult()
ata_read_u32:
	lwz 	0,		CHN_SR(19)		# select EXI channel/freq/slot
	andi.	0,		0, 0x405
	or		0,		0, 17			# speed set by sd-boot (192 or 208)
	stw 	0,		CHN_SR(19)

	li		3,		0x31			# tell EXI we're doing a 4 byte read
	stw 	18, 	CHN_DATA(19)	# write out -1 to read data reg
	stw 	3,		CHN_CR(19)		# exi[SD_CHANNEL * 5 + 3] = ((4-1) << 4) | (0 << 2) | 1;

loopATARU321:						# wait while EXI writes
	lwz 	3,		CHN_CR(19)
	rlwinm	3,		3, 0, 30, 31
	cmpwi	5,		3, 1
	beq 	5,		loopATARU321	# while (exi[SD_CHANNEL * 5 + 3] & 1);
	
	lwz 	3,		CHN_DATA(19)	# get the data back

	lwz 	0,		CHN_SR(19)		# deselect EXI channel
	andi.	0,		0, 0x405
	stw 	0,		CHN_SR(19)
	blr

# Waits for the status to fill the ATA_SR_DRQ bit - Returns in r3
# u8 ataStatusWaitForData
ata_status_wait_for_data:
	mflr	31						# r31 = old LR
loopATASWFD1:
	bl		ata_read_status_reg		# r3 = status

	rlwinm	3,		3, 29, 30, 31	# r3 = status & ATA_SR_DRQ
	cmpwi	5, 		3, 0
	beq 	5,		loopATASWFD1	# while(!(ataReadByte() & ATA_SR_DRQ));

	mtlr	31						# restore LR
	blr

# Waits for the status to clear the ATA_SR_BSY bit - Returns in r3
# u8 ataStatusWaitBusy
ata_status_wait_busy:
	mflr	31						# r31 = old LR
loopATASWB1:
	bl		ata_read_status_reg		# r3 = status

	rlwinm	3,		3, 25, 30, 31	# r3 = status & ATA_SR_BSY
	cmpwi	5,		3, 1
	beq 	5, 		loopATASWB1		# while(ataReadByte() & ATA_SR_BSY);

	mtlr	31						# restore LR
	blr

# Sets up the ATA registers for a read - returns the amount of 32 bit words to read - clobbers r0,r3,r4,r24
# u32 ata_setup_read(numSectors)
ata_setup_read:
	mflr	0
	stwu	1,		-32(1)			# grow stack frame
	stw 	0,		0(1)
	mr		24, 	3				# r24 = numSectors

	bl		ata_status_wait_busy	# Wait for drive to be ready (BSY to clear)

	# Fill out the ATA registers

	# 1. Select the device (Drive 0, LBA mode)
	li		3, 	ATA_REG_DEVICE
	li		4, 	0x40
	bl 		ata_write_byte			# ataWriteByte(ATA_REG_DEVICE, 0x40);
	
	# Sec Cnt : Sect 15:8 then write Sect 7:0
	# LBA Low : LBA 31:24 then write LBA 7:0
	# LBA Mid : LBA 39:32 then write LBA 15:8
	# LBA High: LBA 47:40 then write LBA 23:16
	
	# 2. Normal
	# LBA Lo
	li		3, 	ATA_REG_LBALO
	rlwinm		4, 	16, 8, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBALO, (u8)((lbaLO>>24)& 0xFF));
	eieio
	# LBA Lo
	li		3, 	ATA_REG_LBALO
	rlwinm		4, 	16, 0, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBALO, (u8)(lbaLO & 0xFF));
	eieio
	# LBA Mid
	li		3, 	ATA_REG_LBAMID
	li		4,	0
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBAMID, (u8)((lbaHI) & 0xFF));
	eieio
	# LBA Mid
	li		3, 	ATA_REG_LBAMID
	rlwinm		4, 	16, 24, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBAMID, (u8)((lbaLO>>8) & 0xFF))
	eieio
	# LBA Hi
	li		3, 	ATA_REG_LBAHI
	li		4, 0
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBAHI, (u8)((lbaHI>>8) & 0xFF));
	eieio
	# LBA Hi
	li		3, 	ATA_REG_LBAHI
	rlwinm		4, 	16, 16, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_LBAHI, (u8)((lbaLO>>16) & 0xFF));
	eieio
	# Sector Count
	li		3, 	ATA_REG_SECCOUNT
	rlwinm		4, 	24, 24, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_SECCOUNT, (u8)((numsectors>>8) & 0xFF));
	eieio
	# Sector Count
	li		3, 	ATA_REG_SECCOUNT
	rlwinm		4, 	24, 0, 24, 31
	bl 		ata_write_byte			#ataWriteByte(ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));
		
	
	# Issue read sector command...
	li		3,	ATA_REG_COMMAND
	li		4,	ATA_CMD_READSECTEXT	# add 24bit support
	bl 		ata_write_byte			# ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECTEXT);

	bl		ata_status_wait_busy		# Wait for BSY bit to clear

	bl		ata_status_wait_for_data	# Wait for DRQ bit to set

	# Initialise the 32 bit read mode
	mr		3,		24					# numSectors
	bl		ata_read_init_mult			# ataRead_init_mult(numsectors);
	eieio
	
	slwi	3,		24, 7				# numsectors = number of 32 bit words to read
	
	lwz 	0,		0(1)
	addi	1,		1, 32				# kill stack frame
	mtlr	0
	blr
	
# Reads the contents of a single sector, and copies them to the dst specified with the start offset and for the total length specified
# Returns nothing in r3, clobbers r3,r4,r5,r6,r7,r23,r24
# ataReadSector(u32 *dst, u32 startByte, u32 length)
ata_read_sector:
	mflr	0
	stwu	1,		-32(1)				# grow stack frame
	stw 	0,		0(1)
	mr		23, 	3					# r23 = dst
	mr		25,		4					# r25 = startByte
	mr		26,		5					# r26 = len
	
	li		3,		1					# setup for 1 sector
	bl		ata_setup_read				# setup the ATA registers for our read

	# invalidate the temp sector buffer
	lis		7, 		0x8000
	ori		7, 		7, 0x2B00			# r7 = 0x80002B00 temp sector location
	li		4,		512
loopATARS1:
	bl		ata_read_u32				# 32 bits of data returned
	stw		3,		0(7)				# *tempsector = ata_read_u32();
	addi	7,		7, 4				# temp sector location+=4;
	addi	4,		4, -4				# bytesRemain-=4
	cmpwi	5,		4, 0    			# bytesRemain != 0 ?
	bne		5,	loopATARS1				# yes, keep reading
	
	subi	7, 		7, 512				# reset temp sector location
	add		7,		7, 25				# where in the temp sector location to read from
loopATARS2:
	lbz		3,		0(7)				# r3 = tmpBuf[i+startByte]
	stb		3,		0(23)				# dst[i] = r3
	addi	23,		23, 1				# dst++;
	addi	7,		7, 1				# tmpBuf++;
	addi	26,		26, -1				# len-=1
	cmpwi	5,		26, 0				# len == 0?
	bne		5,	loopATARS2				# no, keep copying

	# Read 1 32bit word to end the read - needed?
	bl		ata_read_u32

	lwz 	0,		0(1)
	addi	1,		1, 32				# kill stack frame
	mtlr	0
	blr
	
# Reads X sectors specified in r6 from the u32 sector in r16 to the dst in r3
# Returns nothing in r3, clobbers r3,r4,r5,r6,r7,r23,r24
# ataReadSectors(u32 *dst, u32 numSectors)
ata_read_sectors:
	mflr	0
	stwu	1,		-32(1)				# grow stack frame
	stw 	0,		0(1)
	mr		23, 	3					# r23 = dst
	
	mr		3,		4					# r3 = numSectors
	bl		ata_setup_read				# setup the ATA registers for our read
	mr		24,		3					# r24 = num u32 words to read
loopATARSR1:
	bl		ata_read_u32				# 32 bits of data returned
	stw 	3,		0(23)				# *Buffer = ataRead32_mult();
	addi	23, 	23, 4				# Buffer+=4;
	addi	24,		24, -1				# wordsRemaining--
	cmpwi	5,		24, 0				# wordsRemaining == 0?
	bne 	5,		loopATARSR1			# no, keep reading

	# Read 1 32bit word to end the read - needed?
	bl		ata_read_u32

	lwz 	0,		0(1)
	addi	1,		1, 32				# kill stack frame
	mtlr	0
	blr

# Wraps all read cases - Unaligned, etc.
# do_read(dst,size, offset);
do_read:
	mflr	0
	stwu	1,		-32(1)			# grow stack frame
	stw 	0,	  	0(1)

	andi.	27, 	5, 511			# r27 = startByte = (offset%512)
	mr		28, 	4				# r28 = numbytes = len
	mr		29, 	3				# r29 = dst
	srwi	3,		5, 9			# r3 = offset >> 9
	add		16, 	16, 3			# lba += offset>>9

	cmpwi	5,		27, 0
	beq 	5,		loopF1			# if(!startByte), go to aligned reads section

	li		3,		512
	sub 	3,		3, 27			# r3 = (512-startByte)
	cmpw	5,		28, 3			# len > (512-startByte)
	bgt 	5,	loopF2				# if(len > (512-startByte)) we need to read from a mis-aligned sector, and then at least one after that

# This case will just read from a single un-aligned sector and then return
	mr		3, 		29				# move dst
	mr		4, 		27				# move startByte
	mr		5, 		28				# move len
	bl		ata_read_sector			# ata_read_sector(u32 *dst, startbyte, len);
	b		loopFend				# return

# This case will read from one un-aligned sector, and will flow through to read more sectors
loopF2:
	mr		5,		3				# 512-startByte
	mr		3,		29				# move dst
	mr		4,		27				# move startByte
	bl		ata_read_sector			# ata_read_sector(u32 *dst, startbyte, len);
	li		3,		512
	sub 	3,		3, 27			# 512-startByte
	sub 	28, 	28, 3			# numBytes-=(512-startByte);
	add 	29, 	29, 3			# dst+=(512-startByte);
	addi	16, 	16, 1			# lba ++;

# Check to see if we'll need to read from many full blocks or just a single aligned sector < 512b
# We re-use r27 now since startByte is always 0 at this section
loopF1:
	srwi	27, 	28, 9			# r27 = numFullBlocks = numBytes/512;
	cmpwi	5,		27, 0
	beq 	5,		loopF4			# no numFullBlocks, so skip the aligned 512b length block reading part

# This case reads 512 byte aligned sectors and batches on every 511 sectors
#loopF3:
## we can only do 127 sectors at a time in one go
#	cmpwi	5,		27, 127			# if we have < 127 whole sectors to read
#	blt 	5,		loopF3A			# we'll do it in single loops, otherwise 127 per go
#	mr		3,		29				# move dst
#	li		4,		127				# offset 0 in the sector
#	bl		ata_read_sectors		# ata_read_sectors(u32 *dst, numSectors);
#	addi	16, 	16, 127			# lbalo+=127;
#	li		3,		127
#	subi	27, 	27, 127			# numFullBlocks-=127;
#	slwi	3,		3,	9
#	add 	29, 	29, 3			# dst+=(261,632 bytes);
#	sub 	28, 	28, 3			# numBytes-=(261,632 bytes);
#	b		loopF3
	
loopF3A:
	cmpwi	5,		27, 0			# if !numFullBlocks
	beq 	5,		loopF4			# exit if there are no whole sectors left
	mr		3,		29				# move dst
	li		4,		1				# 1 block to read
	bl		ata_read_sectors		# ata_read_sectors(dst,numFullBlocks);
	addi	16, 	16, 1			# lba += 1;
	subi	27, 	27, 1			# number of sectors read--
	addi 	29, 	29, 512			# dst+=(number of sectors read *512);
	subi 	28, 	28, 512			# numBytes-=(number of sectors read *512);
	b		loopF3A
	
loopF4:
	cmpwi	5,		28, 0
	beq 	5,		loopFend		# if(!numBytes), finish up

# This case reads from a single sector, with an aligned start
	mr		3,		29				# move dst
	li		4,		0				# 0 startByte
	mr		5,		28				# move numBytes
	bl		ata_read_sector			# ata_read_sector(dst,0, numBytes);

loopFend:
	lwz 		0,	0(1)
	addi		1,	1, 32			# kill stack frame
	mtlr		0
	blr
# End of ATA functions

# Cache functions
dcache_flush_icache_inv:
	clrlwi. 5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 4, 4, 0x20 
1:
	addi 4, 4, 0x1f
	srwi 4, 4, 5
	mtctr 4
2:
	dcbf 0, 3
	icbi 0, 3
	addi 3, 3, 0x20
	bdnz 2b
	sc
	sync
	isync
	blr

# End of Cache functions

