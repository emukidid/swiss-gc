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
#include "input.h"
#include <sdcard/card_cmn.h>
#include "config.h"
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"

#define in_range(x, a, b) (((x) >= (a)) && ((x) <= (b)))

#define RECENT_MAX 8
#define FILES_PER_PAGE 8
#define FILES_PER_PAGE_FULLWIDTH 7
#define FILES_PER_PAGE_CAROUSEL 9
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
extern dvdcmdblk DVDCommandBlock;
extern dvdcmdblk DVDInquiryBlock;
extern dvdcmdblk DVDStopMotorBlock;
extern dvddrvinfo DVDDriveInfo[2];
extern u16* const DVDDeviceCode;
extern dvddiskid* DVDDiskID;
extern DiskHeader GCMDisk;

extern u32 sdgecko_getAddressingType(s32 drv_no);
extern u32 sdgecko_getTransferMode(s32 drv_no);
extern u32 sdgecko_getDevice(s32 drv_no);
extern void sdgecko_setDevice(s32 drv_no, u32 dev);
extern u32 sdgecko_getSpeed(s32 drv_no);
extern void sdgecko_setSpeed(s32 drv_no, u32 freq);
extern u32 sdgecko_getPageSize(s32 drv_no);
extern s32 sdgecko_setPageSize(s32 drv_no, u32 size);
extern s32 sdgecko_enableCRC(s32 drv_no, bool enable);
extern s32 sdgecko_readCID(s32 drv_no);
extern s32 sdgecko_readCSD(s32 drv_no);
extern s32 sdgecko_readStatus(s32 drv_no);

extern syssram* __SYS_LockSram(void);
extern syssramex* __SYS_LockSramEx(void);
extern u32 __SYS_UnlockSram(u32 write);
extern u32 __SYS_UnlockSramEx(u32 write);
extern u32 __SYS_SyncSram(void);
extern u32 __SYS_CheckSram(void);
extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);

extern uiDrawObj_t* renderFileBrowser(file_handle** directory, int num_files, uiDrawObj_t* filePanel);

extern void menu_loop();
extern void boot_dol(file_handle* file, int argc, char *argv[]);
extern bool manage_file();
extern void load_file();
extern int check_game(file_handle *file, file_handle *file2, ExecutableFile *filesToPatch);
extern int cheats_game();
extern void install_game();
extern int info_game(ConfigEntry *config);
extern void settings();
extern void credits();
extern void drawFiles(file_handle** directory, int num_files, uiDrawObj_t *containerPanel);

extern void select_speed();
extern int select_slot();
extern void select_device(int type);
extern bool select_dest_dir(file_handle* initial, file_handle* selection);

typedef struct {
	int cubebootInvoked;
	int enableUSBGecko; // Debug prints over USBGecko
	int waitForUSBGecko;
	int hasDVDDrive;	// 0 if none, 1 if something
	int hasFlippyDrive;
	int exiSpeed;
	int uiVMode;	// What mode to force Swiss into
	int gameVMode;	// What mode to force a Game into
	int forceHScale;
	short forceVOffset;
	int forceVFilter;
	int forceVJitter;
	int disableDithering;
	int forceAnisotropy;
	int forceWidescreen;
	int fontEncode;
	int audioStreaming;
	int forcePollRate;
	int invertCStick;
	int swapCStick;
	int triggerLevel;
	int wiirdDebug;	// Enable WiiRD debug
	int wiirdEngine;
	int hideUnknownFileTypes;
	int sram60Hz;
	int sramProgressive;
	int sramStereo;
	int initDVDDriveAtStart;
	int stopMotor;
	int enableFileManagement;
	int disableMCPGameID;
	int disableVideoPatches;
	int forceVideoActive;
	int forceDTVStatus;
	int lastDTVStatus;
	int pauseAVOutput;
	int emulateAudioStream;
	int emulateReadSpeed;
	int emulateEthernet;
	int emulateMemoryCard;
	int disableMemoryCard;
	int disableHypervisor;
	int preferCleanBoot;
	s8 sramHOffset;
	u8 sramBoot;
	u8 sramLanguage;
	u8 sramVideo;
	char rt4kHostIp[16];
	u16 rt4kPort;
	bool rt4kOptim;
	char morph4kHostIp[16];
	char smbUser[32];		//20
	char smbPassword[32];	//16
	char smbShare[128];		//80
	char smbServerIp[16];	//80
	char ftpUserName[64];
	char ftpPassword[32];
	char ftpHostIp[16];
	u16 ftpPort;
	bool ftpUsePasv;
	char fspHostIp[16];
	u16 fspPort;
	char fspPassword[32];
	u16 fspPathMtu;
	char bbaLocalIp[16];
	u16 bbaNetmask;
	char bbaGateway[16];
	bool bbaUseDhcp;
	int autoBoot;
	int autoCheats;
	int igrType;
	int initNetworkAtStart;
	int aveCompat;
	int rt4kProfile;
	char morph4kPreset[128];
	u8 configDeviceId;	// see deviceHandler.h
	int fileBrowserType;
	int bs2Boot;
	int showHiddenFiles;
	int recentListLevel;	// off, lazy, on
	int gcloaderHwVersion;
	char gcloaderTopVersion[32];
	char autoload[PATHNAME_MAX];
	char flattenDir[PATHNAME_MAX];
	char recent[RECENT_MAX][PATHNAME_MAX];
} SwissSettings;
extern SwissSettings swissSettings;

enum enableUSBGecko
{
	USBGECKO_OFF=0,
	USBGECKO_MEMCARD_SLOT_A,
	USBGECKO_MEMCARD_SLOT_B,
	USBGECKO_SERIAL_PORT_2,
	USBGECKO_MAX
};

enum aveCompat
{
	AVE_N_DOL_COMPAT=0,
	AVE_P_DOL_COMPAT,
	CMPV_DOL_COMPAT,
	GCDIGITAL_COMPAT,
	GCVIDEO_COMPAT,
	AVE_RVL_COMPAT,
	AVE_COMPAT_MAX
};

enum fileOptions
{
	COPY_OPTION=0,
	MOVE_OPTION,
	DELETE_OPTION,
	RENAME_OPTION,
	HIDE_OPTION
};

enum fileBrowserTypes
{
	BROWSER_STANDARD=0,
	BROWSER_FULLWIDTH,
	BROWSER_CAROUSEL,
	BROWSER_MAX
};

#endif 
