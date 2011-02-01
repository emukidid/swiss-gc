/*****************************************************************************
 * IPL FONT Engine
 *
 * Based on Qoob MP3 Player Font
 * Added IPL font extraction
 *****************************************************************************/
#ifndef FONT_H
#define FONT_H

#include <gccore.h>
#include <stdio.h>
#include <ogcsys.h>		/*** Needed for console support ***/

extern void doBackdrop();
extern char txtbuffer[2048];
extern int whichfb;
extern GXRModeObj *vmode;

void init_font(void);
void WriteCentre_HL( int y, const char *string);
void WriteCentre (int y, const char *text);
void write_font (int x, int y, const char *text);
void WaitPrompt (char *msg);
void ShowAction (char *msg);
void WaitButtonA ();
void unpackBackdrop ();
void ClearScreen ();
void SetScreen ();
void fntDrawBoxFilled (int x1, int y1, int x2, int y2, int color);
int fheight;
int font_size[256];
u16 back_framewidth;
int GetTextSizeInPixels(const char *string);
void animateBoxNoText(int x1,int y1, int x2, int y2, int color);
void animateBoxNoTextOffscreen(int x1,int y1, int x2, int y2, int color);
extern u8 norm_blit;
extern unsigned int blit_lookup_inv[4];
extern unsigned int blit_lookup[4];
extern unsigned int blit_lookup_norm[4];
void writex(int x, int y, int sx, int sy, const char *string, unsigned int *lookup);
#endif
