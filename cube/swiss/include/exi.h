#ifndef __EXI_H
#define __EXI_H

// EXI Device ID's
#define EXI_BBA_ID 		0x04020200
#define EXI_IDEEXIV2_ID 0x49444532

extern int GCN_SD_SPEED;
#define exi_chan1sr *(volatile unsigned int*)0xCC006814
#define exi_chan1data	*(volatile unsigned int*) 0xCC006824
#define exi_chan1cr		*(volatile unsigned int*) 0xCC006820


void exi_select(int channel, int device, int freq);
void exi_deselect(int channel);
void exi_imm(int channel, void* data, int len, int mode, int zero);
void exi_sync(int channel);
void exi_imm_ex(int channel, void* data, int len, int mode);
void exi_read(int channel, void* data, int len);
void exi_write(int channel, void* data, int len);
void ipl_set_config(unsigned char c);
int exi_bba_exists();
unsigned int exi_get_id(int chn, int dev);

#endif
