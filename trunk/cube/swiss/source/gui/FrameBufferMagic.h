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
	TEX_BTNEXIT
};

void init_textures();
void DrawImage(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2);
void DrawFrameStart();
void DrawFrameFinish();
void DrawProgressBar(int percent, char *message);
void DrawMessageBox(int type, char *message);
void DrawRawFont(int x, int y, char *message);
void DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode, u32 color);
void DrawEmptyBox(int x1, int y1, int x2, int y2, int color);
void DrawFileBrowserButton(int x1, int y1, int x2, int y2, char *message, file_handle *file, int mode, u32 color);
void DrawMenuButtons(int selection);

#endif