#ifndef VIDEO_H
#define VIDEO_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

extern u32 *xfb[2];
extern int whichfb;
void setVideoMode(GXRModeObj *m);
char* getVideoModeString();
GXRModeObj* getVideoMode();
GXRModeObj* getVideoModeFromSwissSetting(int uiVMode);
int getDTVStatus();
int getFontEncode();

#endif 
