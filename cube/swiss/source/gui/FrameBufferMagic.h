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
#define PROGRESS_BOX_BOTTOMLEFT 0
#define PROGRESS_BOX_TOPRIGHT 1

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
#include "dolcliimg_tpl.h"
#include "elfimg_tpl.h"
#include "dirimg_tpl.h"
#include "fileimg_tpl.h"
#include "checked_32_tpl.h"
#include "checked_32.h"
#include "unchecked_32_tpl.h"
#include "unchecked_32.h"
#include "loading_16_tpl.h"
#include "loading_16.h"
#include "gcloaderimg_tpl.h"
#include "gcloaderimg.h"
#include "m2loaderimg_tpl.h"

typedef struct uiDrawObj {
    int type;
	void *data;
	struct uiDrawObj *child;
	bool disposed;
} uiDrawObj_t;

enum TextureId
{
	TEX_BACKDROP=0,
	TEX_BANNER,
	TEX_GCDVDSMALL,
	TEX_SDSMALL,
	TEX_HDD,
	TEX_QOOB,
	TEX_WODEIMG,
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
	TEX_CHECKED,
	TEX_UNCHECKED,
	TEX_GCLOADER,
	TEX_M2LOADER
};

extern GXTexObj ntscjTexObj;
extern GXTexObj ntscuTexObj;
extern GXTexObj palTexObj;
extern GXTexObj mp3imgTexObj;
extern GXTexObj dolimgTexObj;
extern GXTexObj dolcliimgTexObj;
extern GXTexObj elfimgTexObj;
extern GXTexObj fileimgTexObj;
extern GXTexObj dirimgTexObj;

typedef struct kbBtn_ {
    int supportedEntryMode;
	char *val;
} kbBtn;

#define ENTRYMODE_ALPHA 	(1)
#define ENTRYMODE_NUMERIC 	(1<<1)
#define ENTRYMODE_IP	 	(1<<2)
#define ENTRYMODE_MASKED 	(1<<3)
#define ENTRYMODE_FILE	 	(1<<4)

uiDrawObj_t* DrawImage(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered);
uiDrawObj_t* DrawTexObj(GXTexObj *texObj, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered);
uiDrawObj_t* DrawProgressBar(bool indeterminate, int percent, const char *message);
uiDrawObj_t* DrawProgressLoading(int miniModePos);
uiDrawObj_t* DrawContainer();
uiDrawObj_t* DrawMessageBox(int type, const char *message);
uiDrawObj_t* DrawSelectableButton(int x1, int y1, int x2, int y2, const char *message, int mode);
uiDrawObj_t* DrawEmptyBox(int x1, int y1, int x2, int y2);
uiDrawObj_t* DrawEmptyColouredBox(int x1, int y1, int x2, int y2, GXColor colour);
uiDrawObj_t* DrawTransparentBox(int x1, int y1, int x2, int y2);
uiDrawObj_t* DrawStyledLabel(int x, int y, const char *string, float size, bool centered, GXColor color);
uiDrawObj_t* DrawStyledLabelWithCaret(int x, int y, const char *string, float size, bool centered, GXColor color, int caretPosition);
uiDrawObj_t* DrawLabel(int x, int y, const char *string);
uiDrawObj_t* DrawFadingLabel(int x, int y, const char *string, float size);
uiDrawObj_t* DrawMenuButtons(int selection);
uiDrawObj_t* DrawTooltip(const char *tooltip);
void DrawUpdateProgressBar(uiDrawObj_t *evt, int percent);
void DrawUpdateProgressBarDetail(uiDrawObj_t *evt, int percent, int speed, int timestart, int timeremain);
void DrawUpdateMenuButtons(u32 selection);
void DrawAddChild(uiDrawObj_t *parent, uiDrawObj_t *child);
uiDrawObj_t* DrawPublish(uiDrawObj_t *evt);
void DrawDispose(uiDrawObj_t *evt);
uiDrawObj_t* DrawFileBrowserButton(int x1, int y1, int x2, int y2, const char *message, file_handle *file, int mode);
uiDrawObj_t* DrawFileCarouselEntry(int x1, int y1, int x2, int y2, const char *message, file_handle *file, int distFromMiddle);
uiDrawObj_t* DrawVertScrollBar(int x, int y, int width, int height, float scrollPercent, int scrollHeight);
void DrawArgsSelector(const char *fileName);
void DrawCheatsSelector(const char *fileName);
void DrawGetTextEntry(int entryMode, const char *label, void *src, int size);
void DrawInit();
void DrawShutdown();
void DrawVideoMode(GXRModeObj *videoMode);

#endif
