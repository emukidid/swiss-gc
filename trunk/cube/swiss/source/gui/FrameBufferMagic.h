/* -----------------------------------------------------------
      FrameBufferMagic.h - Crappy framebuffer routines
	      - by emu_kidid
	   
      Version 1.0 11/11/2009
        - Initial Code
   ----------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>

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

void DrawFrameStart();
void DrawFrameFinish();
void DrawProgressBar(int percent, char *message);
void DrawMessageBox(int type, char *message);
void DrawRawFont(int x, int y, char *message);
void DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode);
void DrawEmptyBox(int x1, int y1, int x2, int y2, int color);
