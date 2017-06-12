/***************************************************************************
* USB Gecko In-Game Read code for GC/Wii
* emu_kidid 2012
***************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} usb_data_req;

#define EXI_CHAN1SR		*(volatile unsigned int*) 0xCC006814 // Channel 1 Status Register
#define EXI_CHAN1CR		*(volatile unsigned int*) 0xCC006820 // Channel 1 Control Register
#define EXI_CHAN1DATA	*(volatile unsigned int*) 0xCC006824 // Channel 1 Immediate Data

#define EXI_TSTART			1

#define _CPU_ISR_Enable() \
	{ register u32 _val, _enable_mask; \
	  __asm__ __volatile__ ( \
		"mfmsr %0\n" \
		"andi. %1,%0,0x2\n" \
		"beq 1f\n" \
		"ori %0,%0,0x8000\n" \
		"mtmsr %0\n" \
		"1:" \
		: "=r" (_val), "=r" (_enable_mask) \
	  ); \
	}

#define _CPU_ISR_Disable() \
  { register u32 _val; \
    __asm__ __volatile__ ( \
	  "mfmsr %0\n" \
	  "rlwinm %0,%0,0,17,15\n" \
	  "mtmsr %0\n" \
	  : "=r" (_val) \
	); \
  }


static unsigned int gecko_sendbyte(char data)
{
	unsigned int i = 0;

	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xB0000000 | (data << 20);
	EXI_CHAN1CR = 0x19;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;
	
	if (i&0x04000000)
	{
		return 1;
	}

	return 0;
}


static unsigned int gecko_receivebyte(char* data)
{
	unsigned int i = 0;
	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xA0000000;
	EXI_CHAN1CR = 0x19;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;

	if (i&0x08000000)
	{
		*data = (i>>16)&0xff;
		return 1;
	} 
	
	return 0;
}


// return 1, it is ok to send data to PC
// return 0, FIFO full
static unsigned int gecko_checktx()
{
	unsigned int i = 0;

	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xC0000000;
	EXI_CHAN1CR = 0x09;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;

	return (i&0x04000000);
}


// return 1, there is data in the FIFO to recieve
// return 0, FIFO is empty
static unsigned int gecko_checkrx()
{
	unsigned int i = 0;
	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xD0000000;
	EXI_CHAN1CR = 0x09;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;

	return (i&0x04000000);
}

static void gecko_send(const void *buffer,unsigned int size)
{
	char *sendbyte = (char*)buffer;
	unsigned int ret = 0;

	while (size > 0)
	{
		if(gecko_checktx())
		{
			ret = gecko_sendbyte(*sendbyte);
			if(ret == 1)
			{
				sendbyte++;
				size--;
			}
		}
	}
}

static void gecko_receive(void *buffer,unsigned int size)
{
	char *receivebyte = (char*)buffer;
	unsigned int ret = 0;

	while (size > 0)
	{
		if(gecko_checkrx())
		{
			ret = gecko_receivebyte(receivebyte);
			if(ret == 1)
			{
				receivebyte++;
				size--;
			}
		}
	}
}


void perform_read() {
	// Check if this read offset+size lies in our patched area, if so, we write a 0xE000 cmd to the drive and read from SDGecko.
	volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
	u32 dst = dvd[5];
	u32 len = dvd[4];
	u32 offset = (dvd[3]<<2);
	
	usb_data_req req;
	req.offset = __builtin_bswap32(offset);
	req.size = __builtin_bswap32(len);
	//_CPU_ISR_Enable();
	gecko_send(&req, sizeof(usb_data_req));
	gecko_receive((void*)(dst | 0x80000000), len);
	//_CPU_ISR_Disable();
	dcache_flush_icache_inv((void*)(dst | 0x80000000), len);
	dvd[2] = 0xE0000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[8] = 0;
	dvd[7] = 1;
}
