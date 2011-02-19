#ifndef PATCHER_H
#define PATCHER_H

#include "devices/deviceHandler.h"

extern u8 _Read_original[46];
extern u8 _Read_original_2[38];
extern u32 _DVDLowReadDiskID_original[8];
extern u32 _DVDLowSeek_original[9];
extern u32 _AIResetStreamSampleCount_original[9];
extern u32 _getTiming_v2[28];
extern u32 _getTiming_v1[25];
extern u32 sig_fwrite[8];
extern u32 sig_fwrite_alt[8];
void dvd_patchDVDRead(void *addr, u32 len);
void dvd_patchDVDReadID(void *addr, u32 len);
void dvd_patchAISCount(void *addr, u32 len);
void dvd_patchDVDLowSeek(void *addr, u32 len);
void install_code();
void patch_VIInit(void *addr, u32 len);
void dvd_patchVideoMode(void *addr, u32 len, char mode);
void dvd_patchfwrite(void *addr, u32 len);
void dvd_patchreset(void *addr,u32 len);
int check_dol(file_handle *disc, unsigned int *sig, int size);
void set_base_addr(int useHi);
u32 get_base_addr();
void patchZeldaWW(void *addr, u32 len,int mode);

#endif

