#ifndef __EXI_H
#define __EXI_H


extern int GCN_SD_SPEED;
#define exi_chan1sr *(volatile unsigned int*)0xCC006814
#define exi_chan1data	*(volatile unsigned int*) 0xCC006824
#define exi_chan1cr		*(volatile unsigned int*) 0xCC006820


void exi_select(int channel, int device, int freq);
void exi_deselect(int channel);
inline void exi_imm(int channel, void* data, int len, int mode, int zero);
void exi_sync(int channel);
void exi_imm_ex(int channel, void* data, int len, int mode);
void exi_read(int channel, void* data, int len);
void exi_write(int channel, void* data, int len);
void ipl_set_config(unsigned char c);
#endif
