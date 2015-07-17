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
#include "gcm.h"
#include <sdcard/card_cmn.h>
#include "deviceHandler.h"

#define FILES_PER_PAGE 8
extern int current_view_start;
extern int current_view_end;
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
extern u8 driveVersion[8];
extern DiskHeader GCMDisk;

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
void print_gecko(const char* fmt, ...);
extern void doBackdrop();
extern char *getRelativeName(char *str);
extern void renderFileBrowser(file_handle** directory, int num_files);

extern void boot_dol();
extern void manage_file();
extern void load_file();
extern int check_game();
extern int cheats_game();
extern void install_game();
extern int info_game();
extern void settings();
extern void credits();
extern void drawFiles(file_handle** directory, int num_files);

extern void select_speed();
extern int select_slot();
extern void select_device();
extern void select_copy_device();
extern void select_dest_dir(file_handle* directory, file_handle* selection);

typedef struct {
	int useHiLevelPatch; // Use Hi Level (DVDRead patch) or low level (Read patch)
	int useHiMemArea; // Use Low mem or High mem for the patch to sit at
	int debugUSB; // Debug prints over USBGecko
	int hasDVDDrive;
	int defaultDevice;
	int exiSpeed;
	int uiVMode;	// What mode to force Swiss into
	int gameVMode;	// What mode to force a Game into
	int softProgressive;
	int forceWidescreen;
	int forceAnisotropy;
	int forceEncoding;
	int emulatemc;	// Emulate memcard via SDGecko
	int wiirdDebug;	// Enable WiiRD debug
	int muteAudioStreaming;
	int hideUnknownFileTypes;
	int sramStereo;
	int stopMotor;
	int noDiscMode;
	int enableFileManagement;
	u8 sramLanguage;
	char smbUser[32];		//20
	char smbPassword[32];	//16
	char smbShare[128];		//80
	char smbServerIp[128];	//80
} SwissSettings __attribute__((aligned(32)));
extern SwissSettings swissSettings;

enum fileOptions
{
	COPY_OPTION=0,
	MOVE_OPTION,
	DELETE_OPTION
};

#endif 
