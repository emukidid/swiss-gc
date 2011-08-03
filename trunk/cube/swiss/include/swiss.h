/* swiss.h 
	- defines and externs
 */

#ifndef SWISS_H
#define SWISS_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <ogcsys.h>
#include <ogc/exi.h>
#include <sdcard/card_cmn.h>
#include "deviceHandler.h"

#define MENU_MAX 7
extern char* _menu_array[];
extern char *videoStr;
extern file_handle curFile;
extern int GC_SD_CHANNEL;
extern u32 GC_SD_SPEED;
extern int SDHCCard;
extern u8 g_CID[MAX_DRIVE][16];
extern u8 g_CSD[MAX_DRIVE][16];
extern u8 g_CardStatus[MAX_DRIVE][64];
extern char IPLInfo[256] __attribute__((aligned(32)));
extern int iplModchip;
extern u32 driveVersion;

extern void udelay(int s);
extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);
extern u32 sdgecko_getAddressingType(s32 drv_no);
extern void sdgecko_setSpeed(u32 freq);
extern u32 sdgecko_getPageSize(s32 drv_no);
extern u32 sdgecko_setPageSize(s32 drv_no, int size);
extern s32 sdgecko_readCID(s32 drv_no);
extern s32 sdgecko_readCSD(s32 drv_no);
extern s32 sdgecko_readStatus(s32 drv_no);
extern s32 sdgecko_setHS(s32 drv_no);

extern syssram* __SYS_LockSram();
extern u32 __SYS_UnlockSram(u32 write);

extern char *getVideoString();
extern void print_gecko(char *string);
extern void* Initialise (void);
extern void doBackdrop();
extern char *getRelativeName(char *str);
extern void textFileBrowser(file_handle** directory, int num_files);

extern void boot_dol();
extern void boot_file();
extern int check_game();
extern int cheats_game();
extern void install_game();
extern int info_game();
extern void settings();
extern void credits();

extern void select_speed();
extern void select_slot();
extern void select_device();

typedef struct {
	int useHiLevelPatch; // Use Hi Level (DVDRead patch) or low level (Read patch)
	int useHiMemArea; // Use Low mem or High mem for the patch to sit at
	int disableInterrupts; // On the Hi Level patch, disable interrupts or not
	int debugUSB; // Debug prints over USBGecko
	int curVideoSelection; //video forcing selection (default == auto)
} SwissSettings __attribute__((aligned(32)));
extern SwissSettings swissSettings;

#endif 
