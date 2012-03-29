#***************************************************************************
# SD Read code for GC/Wii via SDGecko
# emu_kidid 2007-2012
#**************************************************************************

# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
# memory map for our variables that sit at the top 0x100 of memory
.set VAR_AREA, 			0x8180	# Base location of our variables
.set VAR_DISC_1_LBA, 	-0x100	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 	-0xF0	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 	-0xE0	# is the currently selected disk sector
.set VAR_EXI_BUS_SPD, 	-0xD0	# is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
.set VAR_SD_TYPE, 		-0xCC	# is the Card Type (SDHC=0, SD=1)
.set VAR_EXI_FREQ, 		-0xC8	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 		-0xC4	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_DISC_1_ID, 	-0xC0	# is the Disc ID of the first disk
.set VAR_DISC_2_ID,		-0xA0	# is the Disc ID of the second disk
.set VAR_32B_BUF,		-0x80	# is a 32 byte area to redirect reads to
.set VAR_TMP1,  		-0x60	# space for a variable if required
.set VAR_TMP2,  		-0x5C	# space for a variable if required
.set VAR_TMP3,  		-0x58	# space for a variable if required
.set VAR_TMP4,  		-0x54	# space for a variable if required
.set VAR_CB_ADDR,		-0x50	# high level read callback addr
.set VAR_CB_ARG1,		-0x4C	# high level read callback r3
.set VAR_CB_ARG2,		-0x48	# high level read callback r4

# register map
# r17 is sd card speed
# r18 is constant 0xFFFFFFFF
# r19 is constant reg base 0xCC0068xx (either 00 or 14 depending on slot)
# r20 is saved dst (r3)
# r21 is saved len (r4)
# r22 is saved ofs (r5)
# r23,r24,r25,r26 used in rcvr_datablock
# r27,r28,r29 used in do_read
# r30 is constant 49
# r31 is temp LR in rcvr_datablock

#SLOT A
.set CHN_SR,      0
.set CHN_CR,      0xC
.set CHN_DATA,    0x10

  .section .text  

# SD Functions 

rcvr_spi:  
  li      3,  1       
  stw     18, CHN_DATA(19)  #write out 0xFFFFFFFF to read data reg
  stw     3, CHN_CR(19)     #set to 1 to start xfer of 1 byte

loopA1:                     
  lwz     0, CHN_CR(19)     #load the word if word&1, loop until xfer bit is gone
  andi.   0, 0, 1           
  bne     0, loopA1         
  lwz     0, CHN_DATA(19)	  	
  rlwinm  3, 0, 8, 24, 31   #return word >>24
 blr

rcvr_spi_word:  
  stw     18, CHN_DATA(19)  #write out 0xFFFFFFFF to read data reg
  stw     30, CHN_CR(19)    #set to 49 to start xfer of 4 bytes

loopA2:                     
  lwz     0, CHN_CR(19)     #load the word if word&1, loop until xfer bit is gone
  andi.   0, 0, 1           
  bne     0, loopA2         
  lwz     3, CHN_DATA(19)	  #return word
 blr

send_cmd:
  mflr    31                # r31 = old LR
  
  lwz     0, CHN_SR(19)     # deselect SDCard
  andi.   0, 0, 0x405 
  stw     0, CHN_SR(19)
  
  lwz     0, CHN_SR(19)     # select SDCard 
  andi.   0, 0, 0x405 
  or      0, 0, 17          # speed set by sd-boot (192 or 208)
  stw     0, CHN_SR(19)
  
  mr       9, 3             # wait until ready 
  mr      10, 4          
  bl      rcvr_spi          # waste one read via rcvr_spi 
loopC1:                     
  bl      rcvr_spi          # while (res != 0xFF); 
  cmpwi   5, 3, 0xFF  
  bne     5, loopC1   
  mr      3,  9     
  mr      4, 10     
  
  slwi    3, 3, 24          # send cmd 
  stw     3, CHN_DATA(19)   # exi[SD_CHANNEL * 5 + 4] = ((snd[d]) << 24); 

  li      0, 5        
  stw     0, CHN_CR(19)     # exi[SD_CHANNEL * 5 + 3] = ((0) << 4) | (1 << 2) | 1; 
loopD1:
  lwz     0, CHN_CR(19)
  andi.   0, 0, 1       
  bne     0, loopD1         # while (exi[SD_CHANNEL * 5 + 3] & 1); 

  stw     4, CHN_DATA(19)   # send data (sector number) 
  li      0, 53       
  stw     0, CHN_CR(19)     # exi[SD_CHANNEL * 5 + 3] = ((3) << 4) | (1 << 2) | 1; 
loopD21:
  lwz     0, CHN_CR(19)
  andi.   0, 0, 1     
  bne     0, loopD21   # while (exi[SD_CHANNEL * 5 + 3] & 1); 

  li      0, 1         # send 0x01 
  slwi    0, 0, 24    
  stw     0, CHN_DATA(19) # exi[SD_CHANNEL * 5 + 4] = ((snd[d]) << 24); 
  li      0, 5        
  stw     0, CHN_CR(19) # exi[SD_CHANNEL * 5 + 3] = ((0) << 4) | (1 << 2) | 1; 
loopD3:
  lwz     0, CHN_CR(19)
  andi.   0, 0, 1       
  bne     0, loopD3    # while (exi[SD_CHANNEL * 5 + 3] & 1); 
  
  srwi    3, 3, 24    
  cmpwi   5, 3, 0x4C   # if (cmd == CMD12) rcvr_spi(); 
  bne     5, loopD4   
  bl      rcvr_spi    
loopD4:

  li      5, 10        # timeout=10; 
  
loopD5:
  cmpwi   5, 5, 0      
  beq     5, loopD6    # timeout == 0, bail out 
  bl      rcvr_spi     # res = rcvr_spi(); 
  addi    5, 5, -1     # timeout-- 
  andi.   0, 3, 0x80   # (res & 0x80) 
  bne     0, loopD5   
  
loopD6:             
  mtlr    31           # restore LR from r31       
  blr

rcvr_datablock:
  mflr    31           # r31 = old LR        
  
  mr      23, 3        # r23 = *buff 
  mr      24, 4        # r24 = start_byte  
  mr      25, 5        # r25 = bytes to read 
  add     26, 4, 5     # r26 = total_read = start_byte+btr 
  
loopE1:
  bl      rcvr_spi    
  cmpwi   5, 3, 0xFF  
  beq     5, loopE1    # while (token == 0xFF); 
  
loopE2:
  cmpwi   5, 24, 0     # if(start_byte) 
  beq     5, loopE3    # get out if no start_byte 
  bl      rcvr_spi    
  addi    24, 24, -1   # start_byte-- 
  b       loopE2      
  
loopE3:
  cmpwi    5, 25, 4     # if(btr<4) 
  blt      5, loopE4    # go read the rest 
  bl       rcvr_spi_word
  stw      3, 0(23)     # else, write one word 
  addi     23, 23, 4    # buff+=4 
  addi     25, 25, -4   # btr-=4 
  b        loopE3      
  
loopE4:
  cmpwi    5, 25, 0     # anything left? 
  beq      5, loopE5    # no, continue 
  bl       rcvr_spi     # else, yes, read bytes til done 
  stb      3, 0(23)    
  addi     23, 23, 1   
  addi     25, 25, -1  
  b        loopE4      
  
loopE5:
  cmpwi    5, 26, 512   # if(total_read<512) 
  blt      5, loopE6   
  b        loopE8       # finish 
loopE6:
  li       6, 512       # r6 = 512 
  sub      6, 6, 26     # r6 = remainder = 512-total_read 
  loopE7:
  cmpwi    5, 6, 0      # if(!remainder) 
  beq      5, loopE8    # bail out 
  bl       rcvr_spi     # read out and discard 
  addi     6, 6, -1     # remainder-- 
  b        loopE7      
  
loopE8:
  bl      rcvr_spi     # discard CRC 
  bl      rcvr_spi     # discard CRC  
  mtlr    31           # restore LR
 blr

 	.globl do_read   
do_read:           
  mflr    0
  stwu    1,-32(1)     # grow stack frame            
  stw     0,  0(1)     

  # setup registers for our code
  li      18, -1        # r18 is constant 0xFFFFFFFF
  lis     19, 0xCC00    # r19 is constant 0xCC0068xx
  ori     19, 19, 0x6800# slot A by default
  li      30, 49        # r30 is constant 49
  mr      20, 3         # r20 is saved dst (r3)
  mr      21, 4         # r21 is saved len (r4)
  mr      22, 5         # r22 is saved ofs (r5)

  lis     6, VAR_AREA   
  lwz     17,VAR_EXI_BUS_SPD(6)		# load sd speed from memory to r17
  lwz	  27, VAR_EXI_SLOT(6)		# setup our reg base for the slot
  cmpwi   5, 27, 0
  beq     5, skip_slot_b_adj
  addi    19, 19, 0x14				# make exi reg base 0xCC006814 for slot 2
skip_slot_b_adj:
  lwz     6, VAR_CUR_DISC_LBA(6)	# load sector address from memory to r6
  
  andi.   27, 5, 511   # r27 = startByte = (offset%512) 
  mr      28, 4        # r28 = numbytes = len 
  mr      29, 3        # r29 = dst 
  srwi    4, 5, 9      # offset >> 9 
  add     4, 4, 6      # r4 = startSector = base_sector+offset>>9 

  lis	  3, VAR_AREA
  lwz	  3, VAR_SD_TYPE(3)	# SD Card Type (SDHC=0, SD=1)
  cmpwi	  5, 3, 0
  beq	  5, _skip_shift
  slwi    4, 4,9       # startSector<<=9; if normal SDcard use byteaddr 
_skip_shift:
  li      3, 0x52      # CMD18 - Read multiple block command 
  bl      send_cmd     # send_cmd(CMD18, startSector); 
  
  cmpwi   5, 27, 0    
  beq     5, loopF1    # if(!startByte), go to main loop read 
  
  li      0, 512      
  sub     0, 0, 27     # (512-startByte) 
  cmpw    5, 28, 0    
  bgt     5, loopF2    # if(len > (512-startByte)) we need to read from mis-aligned multiple sectors 
  
  mr      3, 29        # move dst 
  mr      4, 27        # move startByte 
  mr      5, 28        # move len 
  bl    rcvr_datablock # rcvr_datablock(dst,startByte, len); 
  b       loopFend     # return 

loopF2:
  mr      3, 29        # move dst 
  mr      4, 27        # move startByte 
  li      0, 512      
  sub     5, 0, 27     # move 512-startByte (which in this read is the length) 
  bl    rcvr_datablock # rcvr_datablock(dst,startByte, 512-startByte); 
  li      0, 512      
  sub     0, 0, 27     # 512-startByte 
  sub     28, 28, 0    # numBytes-=(512-startByte); 
	add     29, 29, 0    # dst+=(512-startByte); 
			
loopF1:
  srwi    27, 28, 9    # r27 = numFullBlocks = numBytes/512; 
  li      8, 0         # r8 = i = 0; 
  cmpwi   5, 27, 0    
  beq     5, loopF4    # no numFullBlocks, so skip the reading part 
  
loopF3:
  mr      3, 29        # move dst 
  li      4, 0         # 0 startByte 
  li      5, 512       # 512 length 
  bl    rcvr_datablock # rcvr_datablock(dst, 0,512); 
  addi    29, 29, 512  # dst+=512; 
  addi    28, 28, -512 # numBytes-=512; 
  addi    8, 8 , 1     # i++; 
  cmpw    5, 8, 27    
  blt     5, loopF3    # if i < numFullBlocks, loop again 
  		
loopF4:
  cmpwi   5, 28, 0    
  beq     5, loopFend  # if(!numBytes), finish up 
		  
  mr      3, 29        # move dst 
  li      4, 0         # 0 startByte 
  mr      5, 28        # move numBytes 
  bl    rcvr_datablock # rcvr_datablock(dst,0, numBytes); 
  
loopFend:
  li      3, 0x4C     
  li      4, 0        
  bl      send_cmd     # send_cmd(CMD12, 0); 
  lwz     0, CHN_SR(19)
  andi.   0, 0, 0x405 
  stw     0, CHN_SR(19)# Deselect SDCARD from EXI bus 

  bl      rcvr_spi     # one byte read req after deselect 
  
  lwz     0, 0(1)              
  addi    1, 1, 32     # kill stack frame 
  mtlr    0 
 blr


# End of SD functions 
