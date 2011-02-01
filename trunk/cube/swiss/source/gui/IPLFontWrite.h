/* -----------------------------------------------------------
      IPLFontWrite.h - Font blitter for IPL ROM fonts
	      - by emu_kidid
	   
      Version 1.0 11/11/2009
        - Initial Code
   ----------------------------------------------------------- */

#ifndef IPLFontWrite_H
#define IPLFontWrite_H

#include "FrameBufferMagic.h"

#define back_framewidth vmode->fbWidth
#define back_frameheight vmode->xfbHeight

#define wait_press_A() ({while((PAD_ButtonsHeld(0) & PAD_BUTTON_A)); while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A));})

void init_font(void);
void WriteFont(int x, int y, const char *string);
void WriteFontHL(int x, int y, int sx, int sy, const char *string, unsigned int *lookup);
int GetTextSizeInPixels(const char *string);
void WriteCentre( int y, const char *string);
void WriteCentreHL( int y, const char *string);

extern char txtbuffer[2048];
extern unsigned int blit_lookup_inv[4];
extern unsigned int blit_lookup[4];
extern unsigned int blit_lookup_norm[4];

#endif
