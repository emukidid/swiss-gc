/* -----------------------------------------------------------
      FrameBufferMagic.h - Framebuffer routines with GX
	      - by emu_kidid & sepp256

      Version 1.0 11/11/2009
        - Initial Code
   ----------------------------------------------------------- */
   
#ifndef FRAMEBUFFERMAGIC_H
#define FRAMEBUFFERMAGIC_H

#include <gccore.h>
#include "deviceHandler.h"

extern GXRModeObj *vmode;
extern u32 *xfb[2];
extern int whichfb;

#define D_WARN  0
#define D_INFO  1
#define D_FAIL  2
#define D_PASS  3
#define B_NOSELECT 0
#define B_SELECTED 1

#define BUTTON_COLOUR_INNER 0x2088207C
#define BUTTON_COLOUR_OUTER COLOR_SILVER

#define PROGRESS_BOX_WIDTH  600
#define PROGRESS_BOX_HEIGHT 125
#define PROGRESS_BOX_BAR    COLOR_GREEN
#define PROGRESS_BOX_BARALL COLOR_RED
#define PROGRESS_BOX_BACK   COLOR_BLUE

#define MENU_MAX 5
#define MENU_NOSELECT -1
#define MENU_DEVICE 0
#define MENU_SETTINGS 1
#define MENU_INFO 2
#define MENU_REFRESH 3
#define MENU_EXIT 4

#include "backdrop_tpl.h"
#include "backdrop.h"
#include "gcdvdsmall_tpl.h"
#include "gcdvdsmall.h"
#include "sdsmall_tpl.h"
#include "sdsmall.h"
#include "hdd_tpl.h"
#include "hdd.h"
#include "qoob_tpl.h"
#include "qoob.h"
#include "wodeimg_tpl.h"
#include "wodeimg.h"
#include "wiikeyimg_tpl.h"
#include "wiikeyimg.h"
#include "systemimg_tpl.h"
#include "systemimg.h"
#include "usbgeckoimg_tpl.h"
#include "usbgeckoimg.h"
#include "memcardimg_tpl.h"
#include "memcardimg.h"
#include "sambaimg_tpl.h"
#include "sambaimg.h"
#include "btnnohilight_tpl.h"
#include "btnnohilight.h"
#include "btnhilight_tpl.h"
#include "btnhilight.h"
#include "btndevice_tpl.h"
#include "btndevice.h"
#include "btnsettings_tpl.h"
#include "btnsettings.h"
#include "btninfo_tpl.h"
#include "btninfo.h"
#include "btnrefresh_tpl.h"
#include "btnrefresh.h"
#include "btnexit_tpl.h"
#include "btnexit.h"
#include "boxinner_tpl.h"
#include "boxinner.h"
#include "boxouter_tpl.h"
#include "boxouter.h"
#include "ntscj_tpl.h"
#include "ntscj.h"
#include "ntscu_tpl.h"
#include "ntscu.h"
#include "pal_tpl.h"
#include "pal.h"
#include "mp3img_tpl.h"
#include "dolimg_tpl.h"
#include "dirimg_tpl.h"
#include "fileimg_tpl.h"
#include "checked_32_tpl.h"
#include "checked_32.h"
#include "unchecked_32_tpl.h"
#include "unchecked_32.h"

enum TextureId
{
	TEX_BACKDROP=0,
	TEX_BANNER,
	TEX_GCDVDSMALL,
	TEX_SDSMALL,
	TEX_HDD,
	TEX_QOOB,
	TEX_WODEIMG,
	TEX_BTNNOHILIGHT,
	TEX_BTNHILIGHT,
	TEX_BTNDEVICE,
	TEX_BTNSETTINGS,
	TEX_BTNINFO,
	TEX_BTNREFRESH,
	TEX_BTNEXIT,
	TEX_MEMCARD,
	TEX_WIIKEY,
	TEX_SYSTEM,
	TEX_USBGECKO,
	TEX_SAMBA,
	TEX_NTSCJ,
	TEX_NTSCU,
	TEX_PAL,
	TEX_CHECKED,
	TEX_UNCHECKED
};

void init_textures();
void DrawImage(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered);
void DrawTexObj(GXTexObj *texObj, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered);
void DrawFrameStart();
void DrawFrameFinish();
void DrawProgressBar(int percent, char *message);
void DrawMessageBox(int type, char *message);
void DrawRawFont(int x, int y, char *message);
void DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode, u32 color);
void DrawEmptyBox(int x1, int y1, int x2, int y2, int color);
void DrawTransparentBox(int x1, int y1, int x2, int y2);
void DrawFileBrowserButton(int x1, int y1, int x2, int y2, char *message, file_handle *file, int mode, u32 color);
void DrawMenuButtons(int selection);
int DrawYesNoDialog(char *message);
void DrawVertScrollBar(int x, int y, int width, int height, float scrollPercent, int scrollHeight);
void DrawArgsSelector(char *fileName);
void DrawCheatsSelector(char *fileName);

#endif
