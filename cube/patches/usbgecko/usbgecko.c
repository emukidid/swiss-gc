/***************************************************************************
* USB Gecko In-Game Read code for GC/Wii
* emu_kidid 2012
***************************************************************************/

#include "../../reservedarea.h"
#include "../base/common.h"

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} __attribute((packed, scalar_storage_order("little-endian"))) usb_data_req;

#define EXI_CHAN1SR		*(volatile unsigned int*) 0xCC006814 // Channel 1 Status Register
#define EXI_CHAN1CR		*(volatile unsigned int*) 0xCC006820 // Channel 1 Control Register
#define EXI_CHAN1DATA	*(volatile unsigned int*) 0xCC006824 // Channel 1 Immediate Data

#define EXI_TSTART			1


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

static void gecko_receiveword(unsigned long* data)
{
	unsigned long receiveword = 0;
	unsigned int i = 0;
	
	while (i < 4)
	{
		i += gecko_receivebyte((char*)&receiveword + i);
	}
	
	*data = receiveword;
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

void exi_handler() {}

void trigger_dvd_interrupt()
{
	(*DI)[2] = 0xE0000000;
	(*DI)[3] = 0;
	(*DI)[4] = 0;
	(*DI)[5] = 0;
	(*DI)[6] = 0;
	(*DI)[8] = 0;
	(*DI)[7] = 1;
}

void perform_read()
{
	u32 off = (*DI)[3] << 2;
	u32 len = (*DI)[4];
	u32 dst = (*DI)[5] | 0xC0000000;
	
	*(u32 *)VAR_TMP1 = dst;
	*(u32 *)VAR_TMP2 = len;
	
	usb_data_req req = {off, len};
	gecko_send(&req, sizeof(usb_data_req));
}

void *tickle_read()
{
	u32 *dest = *(u32 **)VAR_TMP1;
	u32  size = *(u32  *)VAR_TMP2;
	
	while (size && gecko_checkrx()) {
		gecko_receiveword(dest);
		
		dest++;
		size -= sizeof(u32);
		
		*(u32 **)VAR_TMP1 = dest;
		*(u32  *)VAR_TMP2 = size;
		
		if (!size) trigger_dvd_interrupt();
	}
	
	return 0;
}

void tickle_read_hook(u32 enable)
{
	tickle_read();
	restore_interrupts(enable);
}

void tickle_read_idle()
{
	disable_interrupts();
	tickle_read();
	enable_interrupts();
}
