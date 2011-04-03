/* -----------------------------------------------------------
      FrameBufferMagic.c - Crappy framebuffer routines
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
#include <ogc/exi.h>
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "bnr2yuv.h"
#include "backdrop.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"

char topStr[256];

void _DrawBackdrop() {
  drawBitmap(backdrop_Bitmap, 0, 0, 640,480);
  WriteFont(55,40, "Swiss v0.1 for Gamecube");
  if(mfpvr() == GC_CPU_VERSION) {
	  if(!IPLInfo[0x55]) {
		sprintf(topStr, "IPL: NTSC Revision 1.0   DVD: %08X",driveVersion);
	  }
	  else {
	  	sprintf(topStr, "IPL: %s   DVD: %08X", &IPLInfo[0x55],driveVersion);
	  }
	  WriteFont(55, 66, topStr);
  }
  if(curDevice == SD_CARD) {
    sprintf(topStr, "%s - %s:%s @ %s",videoStr,!GC_SD_CHANNEL?"Slot A":"Slot B",!SDHCCard?"SDHC":"SD",GC_SD_SPEED==EXI_SPEED16MHZ?"16Mhz":"32Mhz");
  }
  else if(curDevice == DVD_DISC) {
    sprintf(topStr, "%s - %s DVD Disc",videoStr,dvdDiscTypeStr);
  }
  else if(curDevice == IDEEXI) {
	sprintf(topStr, "%s - IDE-EXI Device (%d GB)",videoStr, ataDriveInfo.sizeInGigaBytes);
  }
  else if(curDevice == QOOB_FLASH) {
	  sprintf(topStr, "%s - IPL Replacement",videoStr);
  }
  WriteFont(55,420, topStr);
}

void _DrawHLine (int x1, int x2, int y, int color)
{
  int i;
  y = (vmode->fbWidth/2) * y;
  x1 >>= 1;
  x2 >>= 1;
  for (i = x1; i <= x2; i++) xfb[whichfb][y + i] = color;
}

void _DrawVLine (int x, int y1, int y2, int color)
{
  int i;
  x >>= 1;
  for (i = y1; i <= y2; i++) xfb[whichfb][x + (vmode->fbWidth * i) / 2] = color;
}

void _DrawBox (int x1, int y1, int x2, int y2, int color)
{
  _DrawHLine (x1, x2, y1, color);
  _DrawHLine (x1, x2, y2, color);
  _DrawVLine (x1, y1, y2, color);
  _DrawVLine (x2, y1, y2, color);
}

void _DrawBoxFilled (int x1, int y1, int x2, int y2, int color)
{
  int h;
  for (h = y1; h <= y2; h++) _DrawHLine (x1, x2, h, color);
}

// Externally accessible functions

// Call this when starting a screen
void DrawFrameStart() {
  whichfb ^= 1;
  _DrawBackdrop();
}

// Call this at the end of a screen
void DrawFrameFinish() {
  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();
  if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}

void DrawProgressBar(int percent, char *message) {
  int x1 = ((back_framewidth/2) - (PROGRESS_BOX_WIDTH/2));
  int x2 = ((back_framewidth/2) + (PROGRESS_BOX_WIDTH/2));
  int y1 = ((back_frameheight/2) - (PROGRESS_BOX_HEIGHT/2));
  int y2 = ((back_frameheight/2) + (PROGRESS_BOX_HEIGHT/2));
  int i = 0, middleY, borderSize = 10;

  middleY = (((y2-y1)/2)-12)+y1;
    
  if(middleY+24 > y2) {
    middleY = y1+3;
  }
  //Draw Text and backfill
  for(i = (y1+borderSize); i < (y2-borderSize); i++) {
    _DrawHLine (x1+borderSize, x2-borderSize, i, BUTTON_COLOUR_INNER); //inner
  }
  WriteCentre(middleY, message);
  sprintf(txtbuffer,"%d%% percent complete",percent);
  WriteCentre(middleY+30, txtbuffer);
  //Draw Borders
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y1+borderSize)-i, BUTTON_COLOUR_OUTER); //top
  }
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y2-borderSize)+i, BUTTON_COLOUR_OUTER); //bottom
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x1+borderSize)-(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //left
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x2-borderSize)+(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //right
  }
  int multiplier = (PROGRESS_BOX_WIDTH-20)/100;
  int progressBarWidth = multiplier*100;
  DrawEmptyBox((back_framewidth/2 - progressBarWidth/2), y1+25,
                 ((back_framewidth/2 - progressBarWidth/2) + ((multiplier*(100)))), y1+45, 
                 PROGRESS_BOX_BARALL);  //Progress Bar backing
  DrawEmptyBox((back_framewidth/2 - progressBarWidth/2), y1+25,
                 ((back_framewidth/2 - progressBarWidth/2) + ((multiplier*(percent)))), y1+45, 
                 PROGRESS_BOX_BAR);  //Progress Bar
}

void DrawMessageBox(int type, char *message) {
  int x1 = ((back_framewidth/2) - (PROGRESS_BOX_WIDTH/2));
  int x2 = ((back_framewidth/2) + (PROGRESS_BOX_WIDTH/2));
  int y1 = ((back_frameheight/2) - (PROGRESS_BOX_HEIGHT/2));
  int y2 = ((back_frameheight/2) + (PROGRESS_BOX_HEIGHT/2));
  int i = 0, middleY, borderSize = 10;
  
  middleY = (((y2-y1)/2)-12)+y1;
    
  if(middleY+24 > y2) {
    middleY = y1+3;
  }
  //Draw Text and backfill
  for(i = (y1+borderSize); i < (y2-borderSize); i++) {
    _DrawHLine (x1+borderSize, x2-borderSize, i, BUTTON_COLOUR_INNER); //inner
  }
  WriteCentre(middleY, message);
  //Draw Borders
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y1+borderSize)-i, BUTTON_COLOUR_OUTER); //top
  }
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y2-borderSize)+i, BUTTON_COLOUR_OUTER); //bottom
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x1+borderSize)-(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //left
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x2-borderSize)+(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //right
  }
}

void DrawRawFont(int x, int y, char *message) {
  WriteFont(x, y, message);
}

void DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode, u32 color) {
  int i = 0, middleY, borderSize;
  color = (color == -1) ? BUTTON_COLOUR_INNER : color;
  
  borderSize = (mode==B_SELECTED) ? 6 : 4;
  middleY = (((y2-y1)/2)-12)+y1;
  
  //determine length of the text ourselves if x2 == -1
  x1 = (x2 == -1) ? x1+2:x1;
  x2 = (x2 == -1) ? GetTextSizeInPixels(message)+x1+(borderSize*2)+6 : x2;
  
  if(middleY+24 > y2) {
    middleY = y1+3;
  }
  //Draw Text and backfill (if selected)
  if(mode==B_SELECTED) {
    for(i = (y1+borderSize); i < (y2-borderSize); i++) {
      _DrawHLine (x1+borderSize, x2-borderSize, i, BUTTON_COLOUR_INNER); //inner
    }
    WriteFontHL(x1 + borderSize+3, middleY, x2-borderSize-8, middleY+24 , message, blit_lookup);
  }
  else {
    WriteFontHL(x1 + borderSize+3, middleY,x2-borderSize-8,middleY+24, message,blit_lookup_norm);
  }
  //Draw Borders
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y1+borderSize)-i, BUTTON_COLOUR_OUTER); //top
  }
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y2-borderSize)+i, BUTTON_COLOUR_OUTER); //bottom
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x1+borderSize)-(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //left
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x2-borderSize)+(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //right
  }
}

void DrawEmptyBox(int x1, int y1, int x2, int y2, int color) {
  int i = 0, middleY, borderSize;
  borderSize = (y2-y1) <= 30 ? 3 : 10;
  x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;
  middleY = (((y2-y1)/2)-12)+y1;
    
  if(middleY+24 > y2) {
    middleY = y1+3;
  }
  //Draw Text and backfill
  for(i = (y1+borderSize); i < (y2-borderSize); i++) {
    _DrawHLine (x1+borderSize, x2-borderSize, i, color); //inner
  }
  //Draw Borders
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y1+borderSize)-i, BUTTON_COLOUR_OUTER); //top
  }
  for(i = 0; i < borderSize; i++) {
    _DrawHLine (x1+(i*1), x2-(i*1), (y2-borderSize)+i, BUTTON_COLOUR_OUTER); //bottom
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x1+borderSize)-(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //left
  }
  for(i = 0; i < borderSize; i++) {
    _DrawVLine ((x2-borderSize)+(i*1), y1+(i*1), y2-(i*1), BUTTON_COLOUR_OUTER); //right
  }
}
