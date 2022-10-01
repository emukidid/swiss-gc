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

#include "images_tpl.h"
#include "images.h"
#include "buttons_tpl.h"
#include "buttons.h"

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
	TEX_STAR,
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
uiDrawObj_t* DrawTitleBar();
void DrawUpdateProgressBar(uiDrawObj_t *evt, int percent);
void DrawUpdateProgressBarDetail(uiDrawObj_t *evt, int percent, int speed, int timestart, int timeremain);
void DrawUpdateMenuButtons(int selection);
void DrawUpdateFileBrowserButton(uiDrawObj_t *evt, int mode);
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
void DrawLoadBackdrop();
void DrawShutdown();
void DrawVideoMode(GXRModeObj *videoMode);

#endif
