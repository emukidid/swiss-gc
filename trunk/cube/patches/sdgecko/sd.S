#***************************************************************************
# SD Read code for GC/Wii via SDGecko
# emu_kidid 2007-2012
#**************************************************************************

# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
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

# register map
# r11 is constant reg base 0xCC0068xx (either 00 or 14 depending on slot)

#SLOT A
.set CHN_SR,      0
.set CHN_CR,      0xC
.set CHN_DATA,    0x10

  .section .text  

# SD Functions 

# u8 rcvr_spi() (clobbers r3, r4, r0)
rcvr_spi:  
  li	  4,  -1
  li      3,  1
  stw     4, CHN_DATA(11)   #write out 0xFFFFFFFF to read data reg
  stw     3, CHN_CR(11)     #set to 1 to start xfer of 1 byte

loopA1:                     
  lwz     0, CHN_CR(11)     #load the word if word&1, loop until xfer bit is gone
  andi.   0, 0, 1           
  bne     0, loopA1         
  lwz     0, CHN_DATA(11)	  	
  rlwinm  3, 0, 8, 24, 31   #return word >>24
 blr

# xmit_spi(u32 byte) (clobbers r3, r0)
xmit_spi:
  slwi    3, 3, 24          
  stw     3, CHN_DATA(11)   # exi[SD_CHANNEL * 5 + 4] = ((snd[d]) << 24); 
  li      0, 5        
  stw     0, CHN_CR(11)     # exi[SD_CHANNEL * 5 + 3] = ((0) << 4) | (1 << 2) | 1; 
loopxmit:
  lwz     0, CHN_CR(11)
  andi.   0, 0, 1       
  bne     0, loopxmit         # while (exi[SD_CHANNEL * 5 + 3] & 1); 
 blr

# u32 rcvr_spi_word() (clobbers r3,r4,r0)
rcvr_spi_word:  
  li	  4, -1
  li	  3, 49
  stw     4, CHN_DATA(11)  #write out 0xFFFFFFFF to read data reg
  stw     3, CHN_CR(11)    #set to 49 to start xfer of 4 bytes

loopA2:                     
  lwz     0, CHN_CR(11)     #load the word if word&1, loop until xfer bit is gone
  andi.   0, 0, 1           
  bne     0, loopA2         
  lwz     3, CHN_DATA(11)	  #return word
 blr

# send_cmd (u32 cmd, u32 sector) (clobbers r3, r4, r5)
send_cmd:
  mflr		0					# move lr
  stwu		1,	-0x20(1) 		# grow stack frame
  stw 		0,	 0x24(1)		# save lr
  stw		3,		0x10(1)
  stw		4,		0x14(1)
  
  lis     3, VAR_AREA   
  lwz     4, VAR_EXI_BUS_SPD(3)	# load sd speed from memory to r4
  
  lwz     0, CHN_SR(11)     # deselect SDCard
  andi.   0, 0, 0x405 
  stw     0, CHN_SR(11)
  
  lwz     0, CHN_SR(11)     # select SDCard 
  andi.   0, 0, 0x405 
  or      0, 0, 4           # speed set by sd-boot (192 or 208)
  stw     0, CHN_SR(11)
         
  bl      rcvr_spi          # waste one read via rcvr_spi 
loopC1:                     
  bl      rcvr_spi          # while (res != 0xFF); 
  cmpwi   3, 0xFF  
  bne     loopC1   
  lwz		3,		0x10(1)
  lwz		4,		0x14(1)
  
  slwi    3, 3, 24          # send cmd 
  stw     3, CHN_DATA(11)   # exi[SD_CHANNEL * 5 + 4] = ((snd[d]) << 24); 

  li      0, 5        
  stw     0, CHN_CR(11)     # exi[SD_CHANNEL * 5 + 3] = ((0) << 4) | (1 << 2) | 1; 
loopD1:
  lwz     0, CHN_CR(11)
  andi.   0, 0, 1       
  bne     0, loopD1         # while (exi[SD_CHANNEL * 5 + 3] & 1); 

  stw     4, CHN_DATA(11)   # send data (sector number) 
  li      0, 53       
  stw     0, CHN_CR(11)     # exi[SD_CHANNEL * 5 + 3] = ((3) << 4) | (1 << 2) | 1; 
loopD21:
  lwz     0, CHN_CR(11)
  andi.   0, 0, 1     
  bne     0, loopD21   # while (exi[SD_CHANNEL * 5 + 3] & 1); 

  li      0, 1         # send 0x01 
  slwi    0, 0, 24    
  stw     0, CHN_DATA(11) # exi[SD_CHANNEL * 5 + 4] = ((snd[d]) << 24); 
  li      0, 5        
  stw     0, CHN_CR(11) # exi[SD_CHANNEL * 5 + 3] = ((0) << 4) | (1 << 2) | 1; 
loopD3:
  lwz     0, CHN_CR(11)
  andi.   0, 0, 1       
  bne     0, loopD3    # while (exi[SD_CHANNEL * 5 + 3] & 1); 
  
  srwi    3, 3, 24    
  cmpwi   3, 0x4C   # if (cmd == CMD12) rcvr_spi(); 
  bne     loopD4   
  bl      rcvr_spi    
loopD4:

  li      5, 10        # timeout=10; 
  
loopD5:
  cmpwi   5, 0      
  beq     loopD6    # timeout == 0, bail out 
  bl      rcvr_spi     # res = rcvr_spi(); 
  addi    5, 5, -1     # timeout-- 
  andi.   0, 3, 0x80   # (res & 0x80) 
  bne     0, loopD5   
  
loopD6:
  lwz 	0,	0x24(1)		# load lr
  addi	1,	1, 0x20		# kill stack frame
  mtlr	0				# restore lr
  blr

# rcvr_datablock(void *dest, u32 start_byte, u32 bytes_to_read, u32 total_read)
rcvr_datablock:
	mflr		0					# move lr
	stwu		1,	 -0x40(1) 		# grow stack frame
	stw 		0,	  0x44(1)		# save lr
	stw			6,		12(1)
	stw			7,		16(1)
	stw			8,		20(1)
	stw			9,		24(1)
  
	mr      6, 3        			# r6 = *buff 
	mr      7, 4        			# r7 = start_byte  
	mr      8, 5        			# r8 = bytes to read 
	add     9, 4, 5     			# r9 = total_read = start_byte+btr 
  
loopE1:
	bl      rcvr_spi    
	cmpwi   3, 0xFF  
	beq     loopE1    				# while (token == 0xFF); 
  
loopE2:
	cmpwi   7, 0     				# if(start_byte) 
	beq     loopE3    				# get out if no start_byte 
	bl      rcvr_spi    
	addi    7, 7, -1   				# start_byte-- 
	b       loopE2      
  
loopE3:
	cmpwi    8, 4     				# if(btr<4) 
	blt      loopE4    				# go read the rest 
	bl       rcvr_spi_word
	stw      3, 0(6)     			# else, write one word 
	addi     6, 6, 4    			# buff+=4 
	addi     8, 8, -4   			# btr-=4 
	b        loopE3      
  
loopE4:
	cmpwi    8, 0     				# anything left? 
	beq      loopE5    				# no, continue 
	bl       rcvr_spi     			# else, yes, read bytes til done 
	stb      3, 0(6)    
	addi     6, 6, 1   
	addi     8, 8, -1  
	b        loopE4      
  
loopE5:
	cmpwi    9, 512   				# if(total_read<512) 
	blt      loopE6   
	b        loopE8       			# finish 
loopE6:
	li       7, 512       			# r7 = 512 
	sub      7, 7, 9     			# r7 = remainder = 512-total_read 
loopE7:
	cmpwi    7, 0      				# if(!remainder) 
	beq      loopE8    				# bail out 
	bl       rcvr_spi     			# read out and discard 
	addi     7, 7, -1     			# remainder-- 
	b        loopE7      
  
loopE8:
	bl      rcvr_spi     			# discard CRC 
	bl      rcvr_spi     			# discard CRC  
	lwz			6,		12(1)
	lwz			7,		16(1)
	lwz			8,		20(1)
	lwz			9,		24(1)
	lwz 		0,		0x44(1)		# load lr
	addi		1,		1, 0x40		# kill stack frame
	mtlr		0					# restore lr
	blr

 	.globl do_read   
do_read:
	# save registers
	mflr		0					# move lr
	stwu		1,	 -0x40(1) 		# grow stack frame
	stw 		0,	  0x44(1)		# save lr
	stw			3,		12(1)		# save r3 - dst
	stw			4,		16(1)		# save r4 - len
	stw			5,		20(1)		# save r5 - offset
	stw 		11, 	24(1)		# save r11
	
  # setup registers for our code
  lis     11, 0xCC00    			# r11 is constant 0xCC0068xx
  ori     11, 11, 0x6800			# slot A by default

  lis     6, VAR_AREA
  lwz	  7, VAR_EXI_SLOT(6)		# setup our reg base for the slot
  cmpwi   7, 0
  beq     skip_slot_b_adj
  addi    11, 11, 0x14				# make exi reg base 0xCC006814 for slot 2
skip_slot_b_adj:
  lwz     6, VAR_CUR_DISC_LBA(6)	# load sector address from memory to r6
  
  andi.   7, 5, 511   				# r7 = startByte = (offset%512) 
  mr      8, 4        				# r8 = numbytes = len 
  mr      9, 3        				# r9 = dst 
  srwi    4, 5, 9      				# offset >> 9 
  add     4, 4, 6      				# r4 = startSector = base_sector+offset>>9 

  lis	  3, VAR_AREA
  lwz	  3, VAR_SD_TYPE(3)			# SD Card Type (SDHC=0, SD=1)
  cmpwi	  5, 3, 0
  beq	  5, _skip_shift
  slwi    4, 4,9      				# startSector<<=9; if normal SDcard use byteaddr 
_skip_shift:
  li      3, 0x52      				# CMD18 - Read multiple block command 
  bl      send_cmd     				# send_cmd(CMD18, startSector); 
  
  cmpwi   7, 0    
  beq     loopF1    				# if(!startByte), go to main loop read 
  
  li      0, 512      
  sub     0, 0, 7					# (512-startByte) 
  cmpw    8, 0    
  bgt     loopF2    				# if(len > (512-startByte)) we need to read from mis-aligned multiple sectors 
  
  mr      3, 9        				# move dst 
  mr      4, 7        				# move startByte 
  mr      5, 8        				# move len 
  bl      rcvr_datablock			# rcvr_datablock(dst,startByte, len); 
  b       loopFend     				# return 

loopF2:
  mr      3, 9        				# move dst 
  mr      4, 7        				# move startByte 
  li      0, 512      
  sub     5, 0, 7     				# move 512-startByte (which in this read is the length) 
  bl    rcvr_datablock 				# rcvr_datablock(dst,startByte, 512-startByte); 
  li      0, 512      
  sub     0, 0, 7     				# 512-startByte 
  sub     8, 8, 0    				# numBytes-=(512-startByte); 
  add     9, 9, 0    				# dst+=(512-startByte); 
			
loopF1:
  srwi    7, 8, 9    				# r7 = numFullBlocks = numBytes/512; 
  li      10, 0         			# r10 = i = 0; 
  cmpwi   7, 0    
  beq     loopF4    				# no numFullBlocks, so skip the reading part 
  
loopF3:
  mr      3, 9        				# move dst 
  li      4, 0         				# 0 startByte 
  li      5, 512       				# 512 length 
  bl    rcvr_datablock 				# rcvr_datablock(dst, 0,512); 
  addi    9, 9, 512  				# dst+=512; 
  addi    8, 8, -512 				# numBytes-=512; 
  addi    10, 10 , 1     			# i++; 
  cmpw    5, 10, 7
  blt     5, loopF3    				# if i < numFullBlocks, loop again 
  		
loopF4:
  cmpwi   8, 0    
  beq     loopFend  				# if(!numBytes), finish up 
		  
  mr      3, 9        				# move dst 
  li      4, 0         				# 0 startByte 
  mr      5, 8        				# move numBytes 
  bl    rcvr_datablock 				# rcvr_datablock(dst,0, numBytes); 
  
loopFend:
  li      3, 0x4C     
  li      4, 0        
  bl      send_cmd     				# send_cmd(CMD12, 0); 
  lwz     0, CHN_SR(11)
  andi.   0, 0, 0x405 
  stw     0, CHN_SR(11)				# Deselect SDCARD from EXI bus 

  bl      rcvr_spi    				# one byte read req after deselect 
  
  # Reload regs
	lwz 		11, 	24(1)		# save r11
	lwz 		0,		0x44(1)		# load lr
	addi		1,		1, 0x40		# kill stack frame
	mtlr		0					# restore lr
 blr

 
 	.globl do_write   
do_write:
	# save registers
	mflr		0					# move lr
	stwu		1,	 -0x40(1) 		# grow stack frame
	stw 		0,	  0x44(1)		# save lr
	stw			3,		12(1)		# save r3 - dst
	stw			4,		16(1)		# save r4 - len
	stw			5,		20(1)		# save r5 - offset
	stw 		11, 	24(1)		# save r11


  # setup registers for our code
  lis     11, 0xCC00    			# r11 is constant 0xCC0068xx
  ori     11, 11, 0x6800			# slot A by default

  lis     6, VAR_AREA   
  lwz	  7, VAR_EXI_SLOT(6)		# setup our reg base for the slot
  cmpwi   5, 7, 0
  beq     5, skip_slot_b_adj_2
  addi    11, 11, 0x14				# make exi reg base 0xCC006814 for slot 2
skip_slot_b_adj_2:
  lwz     6, VAR_MEMCARD_LBA(6)		# load sector address from memory to r6
  
  srwi	  8, 4, 9					# r8 = numfullblocks
  mr      9, 3        				# r9 = dst 
  srwi    4, 5, 9      				# offset >> 9 
  add     4, 4, 6      				# r4 = startSector = base_sector+offset>>9 

  lis	  3, VAR_AREA
  lwz	  3, VAR_SD_TYPE(3)			# SD Card Type (SDHC=0, SD=1)
  cmpwi	  5, 3, 0
  beq	  5, _skip_shift_2
  slwi    4, 4,9       				# startSector<<=9; if normal SDcard use byteaddr
_skip_shift_2:
  mr	  7, 4
write_loop_start:
  mr	  4, 7
  li      3, 0x58      				# CMD24 - Write single block command 
  bl      send_cmd    				# send_cmd(CMD24, startSector); 

write_wait:
  bl      rcvr_spi    
  cmpwi   5, 3, 0xFF  
  bne     5, write_wait    # while (token != 0xFF); 
  
  # send start token 0xFE to start write of block
  li	  3, 0xFE
  bl	  xmit_spi
  
  # send 512 bytes out
  li	  4, 512
write_512:
  lbz	  3, 0(9)
  bl	  xmit_spi
  subi	  4, 4, 1
  addi	  9, 9, 1
  cmpwi	  5, 4, 0
  bne	  5, write_512
  
  li	  3, 0xFF
  bl	  xmit_spi		# dummy CRC
  li	  3, 0xFF
  bl	  xmit_spi		# dummy CRC
  bl	  rcvr_spi		# read status bit, discard.

  subi    8, 8, 1     # numFullBlocks--;
  addi	  7, 7, 1		# sector ++;
  cmpwi   5, 8, 0
  bne     5, write_loop_start    # if numFullBlocks == 0, end
  
  lwz     0, CHN_SR(11)
  andi.   0, 0, 0x405 
  stw     0, CHN_SR(11)# Deselect SDCARD from EXI bus 

  bl      rcvr_spi     # one byte read req after deselect 
  
# Reload regs
	lwz			3,		12(1)		# load r3
	lwz			4,		16(1)		# load r4
	lwz			5,		20(1)		# load r5
	lwz 		11, 	24(1)		# load r11
	lwz 		0,		0x44(1)		# load lr
	addi		1,		1, 0x40		# kill stack frame
	mtlr		0					# restore lr
 blr
 


# End of SD functions 
