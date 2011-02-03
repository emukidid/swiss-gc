#***************************************************************************
# SDNGC
#
# Patchcode to be at 0x80001800 and to be called for Read and DVDLowReadDiskId
#**************************************************************************

# memory map
# we have 0x1800 bytes to play with at 0x80001800 (code+data)
# This code is placed at 0x80001800
# 0x80002F00 is the base file sector for disk 1
# 0x80002F10 is the base file sector for disk 2
# 0x80002F20 is the currently selected disk sector
# 0x80002F30 is the SD card speed
# 0x80002F40 is the Video mode to patch
# 0x80002F80 is the Disc ID of the first disk
# 0x80002FA0 is the Disc ID of the second disk
# 0x80002FC0 is a 32 byte area to redirect reads to

# register map
# r17 is sd card speed
# r18 is constant 0xFFFFFFFF
# r19 is constant 0xCC000000
# r20 is saved dst (r3)
# r21 is saved len (r4)
# r22 is saved ofs (r5)
# r23,r24,r25,r26 used in rcvr_datablock
# r27,r28,r29 used in do_read
# r30 is constant 49
# r31 is temp LR in rcvr_datablock

#SLOT A
.set CHN_SR,      0x6800
.set CHN_CR,      0x680C
.set CHN_DATA,    0x6810

#SLOT B
#.set CHN_SR,      0x6814
#.set CHN_CR,      0x6820
#.set CHN_DATA,    0x6824

  .section .text  
	.globl _start   
 _start:         

  b       _do_read_v1     # read 
  b       _do_readdiskid  # read a disk id
  b       _do_videomode   # set a different video mode to r3
  b       _do_seek        # make seek go to offset 0
  b       _do_read_v2     # patch read version 2

_do_read_v1:
  stw     0, 4 (1)      # this is what we patched over, so do it here. 
  b			do_read_generic
  
_do_read_v2:
  stwu		1, -0x0020 (1)	# these 4 are instructions we patched over, so do them here
  stw 		31, 28 (1)
  # this read stores dst and len using:
  # stw 		3, 8 (1)  
  # mr        31, 4    
  # so we'll set it later to ours.
  mflr		31
  stw		31, 8(1)
  bl		do_read_generic
  lwz		31, 8(1)
  mtlr		31
  stw 		3, 8 (1)  
  mr        31, 4  
  blr

_do_seek: 
  stw     0, 4 (1)      # this is what we patched over, so do it here.
  li      3, 0          # make it go to offset 0
  blr
  
_do_videomode:
  stw     0, 4 (1)      # this is what we patched over, so do it here.
  lis     3, 0x8000
  lwz     3, 0x2F40(3)  # load our video mode to r3 ;)
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
  
do_read_generic:
  cmpwi   3,  0         # no dst? return
  beqlr
  cmpwi   4,  0         # no len? return
  beqlr
	stwu    1,  -128(1)   # grow stack frame 
	mflr    0             # move lr  
	stw     0,    0(1)    # save lr  
	stw     6,   12(1)    # save r6
  stmw    17,  16(1)    # save r17->r31
  
  # setup registers for our code
  li      18, -1        # r18 is constant 0xFFFFFFFF
  lis     19, 0xCC00    # r19 is constant 0xCC000000
  li      30, 49        # r30 is constant 49
  mr      20, 3         # r20 is saved dst (r3)
  mr      21, 4         # r21 is saved len (r4)
  mr      22, 5         # r22 is saved ofs (r5)
	
	
	lis     6, 0x8000   
  lwz     17,0x2F30(6)  # load sd speed from memory to r6
	lwz     6, 0x2F20(6)  # load sector address from memory to r6
	
	bl      do_read       # do_read(dst,size, offset, start_sector); 

	mr      3, 20         # load dst from r20 
	mr      4, 21         # load len from r21  
  bl      dcache_flush_icache_inv

  lwz     6,    12(1)   # reload r6
  lmw     17,   16(1)   # load r17->r31
  li      5,  0         # offset 0
  li      4, 32         # len 32
  lis     3,    0x8000  # to this dest
  ori     3, 3, 0x2FC0  # in memory
	lwz     0,    0(1)    # load lr 
	mtlr    0             # restore lr 
	addi    1, 1, 128     # kill stack frame 
 blr

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

do_read:
  mflr    0
  stwu    1,-32(1)     # grow stack frame            
  stw     0,  0(1)     

  andi.   27, 5, 511   # r27 = startByte = (offset%512) 
  mr      28, 4        # r28 = numbytes = len 
  mr      29, 3        # r29 = dst 
  srwi    4, 5, 9      # offset >> 9 
  add     4, 4, 6      # r4 = startSector = base_sector+offset>>9 
  
#  slwi    4, 4,9       # startSector<<=9; if normal SDcard use byteaddr 

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
