/* main.h 
	- defines and externs
	by emu_kidid
 */

#ifndef MAIN_H
#define MAIN_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <ogcsys.h>
#include <ogc/exi.h>
#include <sdcard/card_cmn.h>
#include "deviceHandler.h"

/*** 2D Video Globals ***/
extern GXRModeObj *vmode;	/*** Graphics Mode Object ***/
extern u32 *xfb[2];			/*** Framebuffers ***/
extern int whichfb;			/*** Frame buffer toggle ***/


//Cheats
enum useCheats
{
	NO_CHEATS=0,
	CHEATS
};

//File type
enum fileTypes
{
  IS_FILE=0,
  IS_DIR,
  IS_SPECIAL
};

//Device Type
enum deviceTypes
{
	DVD_DISC=0,
	SD_CARD,
	IDEEXI,
	QOOB_FLASH,
	WODE,
	MEMCARD
};

//Destination Device Type
enum destDeviceTypes
{
	DEST_SD_CARD=0,
	DEST_IDEEXI,
	DEST_MEMCARD
};

//Where on the screen are we?
enum guiPos
{
  ON_OPTIONS=0,
  ON_FILLIST
};

//Video Modes (strings)
#define ProgStr     "NTSC (480p)"
#define NTSCStr     "NTSC (480i)"
#define PALStr      "PAL (576i)"

#define IDE_EXI      0x51

//DVD Disc Type (enum)
enum dvdDiscTypes
{
	COBRA_MULTIGAME_DISC=0, //Cobra Multigame Disc, games can be every 32Kb interval
	GCOSD5_MULTIGAME_DISC,  //GCOS Single Layer Disc, games can be every 128Kb interval
	GCOSD9_MULTIGAME_DISC,  //GCOS Single Dual Disc, games can be every 128Kb interval
	MULTIDISC_DISC,         //Multi Disc Gamecube Game
	GAMECUBE_DISC,          //Single Disc Gamecube Game
	ISO9660_DISC,           //ISO9660 Filesystem Disc
	UNKNOWN_DISC	          //Unknown
};

//DVD Drive Status (enum)
enum dvdDrvStatus
{
  DRV_ERROR=0,  //Something screwed up, no disc most likely
  NORMAL_MODE,  //Normal, no debug patchcode uploaded to drive
  DEBUG_MODE    //Patched Drive firmware (Gamecube only)
};

//Audio Streaming setup (enum)
enum setupStream
{
  DISABLE_AUDIO=0,  //Don't enable audio streaming on the drive
  ENABLE_AUDIO,     //Enable it
  ENABLE_BYDISK     //Check from disc header whether to enable it
};

// Mediums we can have swiss search on boot
enum bootDevices
{
	SD_SLOTA=0,
	SD_SLOTB,
	IDE_SLOTA,
	IDE_SLOTB
};

#define DVD_MAGIC   0xC2339F3D

//Disc Types
#define CobraStr    "Cobra MultiGame"
#define GCOSD5Str   "GCOS MultiGame"
#define GCOSD9Str   "GCOS MultiGame"
#define MDStr       "2-Disc Gamecube"
#define GCDStr      "Gamecube"
#define ISO9660Str  "ISO9660"

#define UnkStr      "Unknown"
#define NotInitStr  "Un-Initialized"

#define ROOT_DIR    ""

#define MAX_SETTING_POS 0 //max menu number-1
#define MAX_VIDEO_MODES 4	//video modes

//DVD read thread vars
#define THREAD_SLEEP 100
#define NUM_BUFFERS 2
#define DVD_STACK_SIZE 1024

//Console Version Type Helpers
#define GC_CPU_VERSION01 0x00083214
#define GC_CPU_VERSION02 0x00083410
#ifndef mfpvr
#define mfpvr()  ({unsigned int rval; asm volatile("mfpvr %0" : "=r" (rval)); rval;})
#endif

#define is_gamecube() (((mfpvr() == GC_CPU_VERSION01)||((mfpvr() == GC_CPU_VERSION02))))

#define MAX_MULTIGAME 128
#define MULTIGAME_TABLE_OFFSET 64

/* Externs */
extern void GCARSStartGame(u32* codelist);
extern u32 *getCodeBasePtr();
extern u32 getCodeBaseSize();

extern char *dvdDiscTypeStr;
extern int dvdDiscTypeInt;
extern int curDevice;  //SD_CARD or DVD_DISC
extern int drive_status;

extern int files;                  //number of files in a directory
extern int curMenuSelection;	      //menu selection
extern int curSelection;		        //game selection
extern int needsDeviceChange;
extern int needsRefresh;
extern int curMenuLocation;

typedef struct {
  u8   mid;         //Manufacturer ID
  char oid[2];      //OEM/Application ID
  char pnm[5];      //Product Name
  u8   prv;         //product version 0001 0001 = 1.1
  u32  psn;         //product serial number
  u16  mdt;         //bottom 4 bits are month, 8 bits above is year since 2000
  u8   unk;
} CIDdata __attribute__((aligned(32)));

void print_gecko(char *string);

#endif

