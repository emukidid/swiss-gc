/***************************************************************************
# DVD patch code helper functions for Swiss
# emu_kidid 2013
#**************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

void patch_region()
{
	// Go through our reported areas which might 
	// contain DOL data and apply any patches the user had selected

}

// Perform a debug spinup/etc instead of a hard dvd reset
void handle_disc_swap()
{

	asm("mfmsr	5");
	asm("rlwinm	5,5,0,17,15");
	asm("mtmsr	5");
	
	volatile u32* dvd = (volatile u32*)0xCC006000;
	// Unlock
	// Part 1 with 'MATSHITA'
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF014D41;
	dvd[3] = 0x54534849;
	dvd[4] = 0x54410200;
	dvd[7] = 1;
	while ((dvd[0] & 0x14) == 0);
	// Part 2 with 'DVD-GAME'
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF004456;
	dvd[3] = 0x442D4741;
	dvd[4] = 0x4D450300;
	dvd[7] = 1;
	while ((dvd[0] & 0x14) == 0);

	// Enable drive extensions
	dvd[0] = 0x14;
	dvd[1] = 0;
	dvd[2] = 0x55010000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while ((dvd[0] & 0x14) == 0);
	
	// Enable motor/laser
	dvd[0] = 0x14;
	dvd[1] = 1;
	dvd[2] = 0xfe114100;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while ((dvd[0] & 0x14) == 0);
	
	// Set the status
	dvd[0] = 0x14;
	dvd[1] = 0;
	dvd[2] = 0xee060300;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while ((dvd[0] & 0x14) == 0);
	
	asm("mfmsr	5");
	asm("ori	5,5,0x8000");
	asm("mtmsr	5");
}

void dvd_read_patched_section() {
	// Check if this read offset+size lies in our patched area, if so, we write a 0xE000 cmd to the drive
	// and read from SDGecko.
	volatile u32* dvd = (volatile u32*)0xCC006000;
	u32 offset = dvd[3]<<2;
	u32 len = dvd[4];	// dvd[6] DMA len.
	u32 dst = dvd[5];
	
	
	// If it doesn't let the value about to be passed to the DVD drive be DVD_DMA | DVD_START
}

// Turn off the laser if the game has been sitting idle for a while
void check_drive_idle()
{
	// If it's been 10 seconds since the laser was last used, turn it off
	//if(diff_sec(curtime, oldtime) > 10) {
	//	stop_laser(0);
	//	laser = 0;
	//}
}

// Called to turn on the laser if we're about to perform a DVD read
void enable_drive()
{
	// Re-enable the laser if we'd previously turned it off
	//laser_control(1);
	//laser = 1;
}

void dvd_read_frag(void) {}
