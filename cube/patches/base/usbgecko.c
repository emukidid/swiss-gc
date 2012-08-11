

/************* USB GECKO */ /*

#define EXI_CHAN1SR		*(volatile unsigned long*) 0xCC006814 // Channel 1 Status Register
#define EXI_CHAN1CR		*(volatile unsigned long*) 0xCC006820 // Channel 1 Control Register
#define EXI_CHAN1DATA	*(volatile unsigned long*) 0xCC006824 // Channel 1 Immediate Data

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

// return 1, it is ok to send data to PC
// return 0, FIFO full
static unsigned int gecko_checktx()
{
	unsigned int i  = 0;

	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xC0000000;
	EXI_CHAN1CR = 0x09;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0x0;

	return (i&0x04000000);
}

int usb_sendbuffer_safe(const void *buffer,int size)
{
	char *sendbyte = (char*) buffer;
	unsigned int ret = 0;

	while (size  > 0)
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

	return 0;
}

void print_int_hex(unsigned int num) {
	const char nybble_chars[] = "0123456789ABCDEF";
	int i = 0;
	for (i = 7; i >= 0; i--) {	
		usb_sendbuffer_safe(&nybble_chars[ ( num >> (i*4)) & 0x0F ],1);
	}
}
*/

/************************/
