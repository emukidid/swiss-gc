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
#include "util.h"
#include "files.h"
#include "video.h"
#include <sdcard/card_cmn.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"

#define in_range(x, a, b) (((x) >= (a)) && ((x) < (b)))

#define RECENT_MAX 8
#define FILES_PER_PAGE 8
#define FILES_PER_PAGE_CAROUSEL 10
extern int current_view_start;
extern int current_view_end;
extern int curMenuSelection;	      //menu selection
extern int curSelection;		      //entry selection
extern int needsDeviceChange;
extern int needsRefresh;
extern int curMenuLocation;

extern char* _menu_array[];
extern file_handle curFile;
extern file_handle curDir;
extern char IPLInfo[256] __attribute__((aligned(32)));
extern u8 driveVersion[32];
extern DiskHeader GCMDisk;

extern void udelay(int s);
extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);
extern u32 sdgecko_getAddressingType(s32 drv_no);
extern u32 sdgecko_getSpeed(s32 drv_no);
extern void sdgecko_setSpeed(s32 drv_no, u32 freq);
extern u32 sdgecko_getPageSize(s32 drv_no);
extern s32 sdgecko_setPageSize(s32 drv_no, u32 size);
extern s32 sdgecko_readCID(s32 drv_no);
extern s32 sdgecko_readCSD(s32 drv_no);
extern s32 sdgecko_readStatus(s32 drv_no);
extern s32 sdgecko_setHS(s32 drv_no);

extern syssram* __SYS_LockSram();
extern u32 __SYS_UnlockSram(u32 write);
extern syssramex* __SYS_LockSramEx();
extern u32 __SYS_UnlockSramEx(u32 write);

extern uiDrawObj_t * renderFileBrowser(file_handle** directory, int num_files, uiDrawObj_t *container);

extern void menu_loop();
extern void boot_dol();
extern bool manage_file();
extern void load_file();
extern int check_game();
extern int cheats_game();
extern void install_game();
extern int info_game();
extern void settings();
extern void credits();
extern void drawFiles(file_handle** directory, int num_files, uiDrawObj_t *container);

extern void select_speed();
extern int select_slot();
extern void select_device(int type);
extern bool select_dest_dir(file_handle* directory, file_handle* selection);

typedef struct {
	int debugUSB; // Debug prints over USBGecko
	int hasDVDDrive;	// 0 if none, 1 if something, 2 if present and init'd from cold boot
	int exiSpeed;
	int uiVMode;	// What mode to force Swiss into
	int gameVMode;	// What mode to force a Game into
	int forceHScale;
	short forceVOffset;
	int forceVFilter;
	int disableDithering;
	int forceAnisotropy;
	int forceWidescreen;
	int fontEncode;
	int audioStreaming;
	int invertCStick;
	int triggerLevel;
	int wiirdDebug;	// Enable WiiRD debug
	int hideUnknownFileTypes;
	int sram60Hz;
	int sramProgressive;
	int sramStereo;
	int stopMotor;
	int enableFileManagement;
	int disableVideoPatches;
	int forceVideoActive;
	int forceDTVStatus;
	int emulateReadSpeed;
	int emulateMemoryCard;
	s8 sramHOffset;
	u8 sramLanguage;
	u8 sramVideo;
	char smbUser[32];		//20
	char smbPassword[32];	//16
	char smbShare[128];		//80
	char smbServerIp[32];	//80
	char ftpUserName[64];
	char ftpPassword[32];
	char ftpHostIp[32];
	u16 ftpPort;
	bool ftpUsePasv;
	char fspHostIp[32];
	u16 fspPort;
	char fspPassword[32];
	int autoCheats;
	int igrType;
	int initNetworkAtStart;
	int aveCompat;
	u8 configDeviceId;	// see deviceHandler.h
	int fileBrowserType;
	int bs2Boot;
	int showHiddenFiles;
	int recentListLevel;	// on, lazy, off
	char gcloaderTopVersion[32];
	char autoload[PATHNAME_MAX];
	char recent[RECENT_MAX][PATHNAME_MAX];
} SwissSettings;
extern SwissSettings swissSettings;

enum fileOptions
{
	COPY_OPTION=0,
	MOVE_OPTION,
	DELETE_OPTION,
	RENAME_OPTION
};

enum fileBrowserTypes
{
	BROWSER_STANDARD=0,
	BROWSER_CAROUSEL,
	BROWSER_MAX
};

#endif 
