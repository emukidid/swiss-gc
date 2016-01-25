/***************************************************************************
* DVD Interface for GC (checks regs and simulates DVD drive functions)
* emu_kidid 2015
*
***************************************************************************/

#include "../../reservedarea.h"

#define DI_SR 		(0)
#define DI_CVR		(1)
#define DI_CMD		(2)
#define DI_LBA		(3)
#define DI_SRCLEN	(4)
#define DI_DMAADDR	(5)
#define DI_DMALEN	(6)
#define DI_CR		(7)
#define DI_IMM		(8)
#define DI_CFG		(9)

#define AGGRESSIVE_INT 1

typedef struct {
	unsigned long l, u;
} tb_t;

#define mftb(rval) ({unsigned long u; do { \
	 asm volatile ("mftbu %0" : "=r" (u)); \
	 asm volatile ("mftb %0" : "=r" ((rval)->l)); \
	 asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
	 } while(u != ((rval)->u)); })

//int usb_sendbuffer_safe(const void *buffer,int size);

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

void trigger_dvd_interrupt() {
	volatile u32* realDVD = (volatile u32*)0xCC006000;
	if(!(realDVD[7]&1))
	{
		realDVD[0] = 0x7E;
		realDVD[2] = 0xE0000000;
		realDVD[7] |= 1;
	}
#ifndef AGGRESSIVE_INT
	while(realDVD[7] & 1);
#endif
}

#define ALIGN_BACKWARD(x,align) \
	((typeof(x))(((u32)(x)) & (~(align-1))))

u32 branch(u32 dst, u32 src)
{
	u32 newval = dst - src;
	newval &= 0x03FFFFFC;
	return newval | 0x48000000;
}
void appldr_start();

// DVD Drive command wrapper
void DIUpdateRegisters() {

	volatile u32 *dvd = (volatile u32*)VAR_DI_REGS;
	
	if(*(u32*)VAR_FAKE_IRQ_SET) {
		return;
	}
	u32 diOpCompleted = 0;
	int discChanging = *(u32*)VAR_DISC_CHANGING;
	
	if(discChanging == 1) {
		*(u32*)VAR_DISC_CHANGING = 2;
		
		if(*(u32*)VAR_CUR_DISC_LBA == *(u32*)VAR_DISC_1_LBA)
			*(u32*)VAR_CUR_DISC_LBA = *(u32*)VAR_DISC_2_LBA;
		else
			*(u32*)VAR_CUR_DISC_LBA = *(u32*)VAR_DISC_1_LBA;
		mftb((tb_t*)VAR_TIMER_START);
	}
	
	// If we have something, IMM or DMA command
	if((dvd[DI_CR] & 1))
	{

		u32 DIcommand = dvd[DI_CMD]>>24;
		switch( DIcommand )
		{
			case 0xAB:	// Seek
				diOpCompleted = 1;
				break;
			case 0xE1:	// play Audio Stream
				if(*(u32*)VAR_AS_ENABLED) {
					switch( (dvd[DI_CMD] >> 16) & 0xFF )
					{
						case 0x00:
							StreamStartStream(dvd[DI_LBA] << 2, dvd[DI_SRCLEN]);
							*(u8*)VAR_STREAM_DI = 1;
							break;
						case 0x01:
							StreamEndStream();
							*(u8*)VAR_STREAM_DI = 0;
							break;
						default:
							break;
					}
				}
				diOpCompleted = 1;
				break;
			case 0xE2:	// request Audio Status
				if(*(u32*)VAR_AS_ENABLED) {
					switch( (dvd[DI_CMD] >> 16) & 0xFF )
					{
						case 0x00:	// Streaming?
							*(u8*)VAR_STREAM_DI = !!(*(u32*)VAR_STREAM_CUR);
							dvd[DI_IMM] = *(u8*)VAR_STREAM_DI;
							break;
						case 0x01:	// What is the current address?
							if(*(u8*)VAR_STREAM_DI)
							{
								if((*(u32*)VAR_STREAM_CUR))
									dvd[DI_IMM] = ALIGN_BACKWARD((*(u32*)VAR_STREAM_CUR), 0x800) >> 2;
								else
									dvd[DI_IMM] = (*(u32*)VAR_STREAM_END) >> 2;
							}
							else
								dvd[DI_IMM] = 0;
							break;
						case 0x02:	// disc offset of file
							dvd[DI_IMM] = (*(u32*)VAR_STREAM_START) >> 2;
							break;
						case 0x03:	// Size of file
							dvd[DI_IMM] = *(u32*)VAR_STREAM_SIZE;
							break;
						default:
							break;
					}
				}
				diOpCompleted = 1;
				break;
			case 0xE4:	// Disable Audio
				diOpCompleted = 1;
				break;
			case 0x12:
				*(u32*)(dvd[DI_DMAADDR]+4) = 0x20010608;
				*(u32*)(dvd[DI_DMAADDR]+8) = 0x61000000;
				dvd[DI_DMAADDR] += 0x20;
				dvd[DI_DMALEN] = 0;
				diOpCompleted = 1;
				break;
			case 0xE0:	// Get error status
				dvd[DI_IMM] = 0;	// All OK
				diOpCompleted = 1;
				break;
			case 0xE3:	// Stop motor
				if(!discChanging) {
					*(u32*)VAR_DISC_CHANGING = 1;	// Lid is open
				}
				diOpCompleted = 1;
				break;
			case 0xA8:	// Read!
			{
				// If audio streaming is on, we give it a chance to read twice before we read data once
				if(!*(u8*)VAR_STREAM_DI || *(u32*)VAR_READS_IN_AS ==2) {
					*(u32*)VAR_READS_IN_AS = 0;
				
					u32 dst	= dvd[DI_DMAADDR];
					u32 len	= dvd[DI_DMALEN];
					u32 offset	= dvd[DI_LBA] << 2;

					// The only time we readComplete=1 is when reading the arguments for 
					// execD, since interrupts are off after this point and our handler isn't called again.
					int readComplete = *(u32*)VAR_LAST_OFFSET == *(u32*)VAR_EXECD_OFFSET;
					*(u32*)VAR_LAST_OFFSET = offset;

					if(offset && (offset == *(u32*)VAR_EXECD_OFFSET))
					{
						//execD, jump to our own handler
						*(u32*)dst = branch((u32)appldr_start, dst);
						dcache_flush_icache_inv(dst, 0x20);
						dvd[DI_DMALEN] = 0;
						diOpCompleted = 1;
					}
					else
					{
						// Read. We might not do the full read, depends how busy the game is
						u32 amountRead = process_queue(dst, len, offset, readComplete);
						if(amountRead) {
							dcache_flush_icache_inv(dst, amountRead);
							dvd[DI_DMALEN] -= amountRead;
							dvd[DI_DMAADDR] += amountRead;
							dvd[DI_LBA] += amountRead>>2;
						}

						if(dvd[DI_DMALEN] == 0) {
							diOpCompleted = 1;
						}
					}
				}
				*(u32*)VAR_READS_IN_AS += 1;
			} 
				break;
		}
	}
	// We're faking lid open, end it here.
	if(discChanging == 2) {
		tb_t* start = (tb_t*)VAR_TIMER_START;
		tb_t end;
		mftb(&end);
		u32 diff = tb_diff_usec(&end, start);
		if(diff > 2000000) {	//~2 sec
			*(u32*)VAR_DISC_CHANGING = 0;
			diOpCompleted = 1;
		}
	}
	
	if(diOpCompleted == 1) {
		// Keep mask on SR but also set our TC INT to indicate operation complete
		dvd[DI_SR] |= 0x10;
		dvd[DI_CR] &= 2;
		*(u32*)VAR_FAKE_IRQ_SET = 1;
		trigger_dvd_interrupt();
	}
}
