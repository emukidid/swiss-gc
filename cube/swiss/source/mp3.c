#include <gccore.h>
#include <asndlib.h>
#include <mp3player.h>
#include "swiss.h"
#include "mp3.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

static int useShuffle = 0;

int mp3Reader(void *cbdata, void *dst, int size) {
	file_handle *file = cbdata;
	deviceHandler_seekFile(file,file->offset,DEVICE_HANDLER_SEEK_SET);
	int ret = deviceHandler_readFile(file,dst,size);
	return ret;
}

void updatescreen_mp3(file_handle *file, int state, int numFiles, int curMP3) {
	DrawFrameStart();
	DrawEmptyBox(10,150, vmode->fbWidth-10, 350, COLOR_BLACK);
	WriteFontStyled(640/2, 160, (state == PLAYER_PAUSE ? "Paused":"Playing"), 1.0f, true, defaultColor);
	sprintf(txtbuffer, "(%i/%i) %s",curMP3, numFiles,getRelativeName(file->name));
	float scale = GetTextScaleToFitInWidth(txtbuffer, vmode->fbWidth-10-10);
	WriteFontStyled(640/2, 200, txtbuffer, scale, true, defaultColor);
	memset(txtbuffer, 0, 256);
	sprintf(txtbuffer, "------------------------------");
	float percentPlayed = (float)(((float)file->offset / (float)file->size) * 30);
	txtbuffer[(int)percentPlayed] = '*';
	WriteFontStyled(640/2, 240, txtbuffer, 1.0f, true, defaultColor);
	WriteFontStyled(640/2, 300, "(B) Stop (L) Prev (R) Next (Start) Pause", 1.0f, true, defaultColor);
	sprintf(txtbuffer, "Shuffle is currently %s press (Z) to toggle", (useShuffle ? "on":"off"));
	WriteFontStyled(640/2, 330, txtbuffer, 1.0f, true, defaultColor);
	DrawFrameFinish();	
	VIDEO_WaitVSync();
}

int play_mp3(file_handle *file, int numFiles, int curMP3) {
	int ret = PLAYER_NEXT;
	file->offset = 0;
	MP3Player_PlayFile(file, &mp3Reader, NULL);
	while(MP3Player_IsPlaying() || ret == PLAYER_PAUSE ) {
	
		u32 buttons = PAD_ButtonsHeld(0);
		if(buttons & PAD_BUTTON_B) {			// Stop
			while((PAD_ButtonsHeld(0) & PAD_BUTTON_B)){VIDEO_WaitVSync();};
			MP3Player_Stop();
			ret = PLAYER_STOP;
		}
		else if(buttons & PAD_BUTTON_START) {	// Pause
			while((PAD_ButtonsHeld(0) & PAD_BUTTON_START)){VIDEO_WaitVSync();};
			if(ret != PLAYER_PAUSE) {
				MP3Player_Stop();
				ret = PLAYER_PAUSE;
			}
			else {
				MP3Player_PlayFile(file, &mp3Reader, NULL);	// Unpause
				ret = PLAYER_NEXT;
			}
		}
		else if(buttons & PAD_TRIGGER_R) {		// Next
			MP3Player_Stop();
			ret = PLAYER_NEXT;
		}
		else if(buttons & PAD_TRIGGER_L) {		// Previous
			MP3Player_Stop();
			ret = PLAYER_PREV;
		}
		else if(buttons & PAD_TRIGGER_Z) {		// Toggle Shuffle
			while((PAD_ButtonsHeld(0) & PAD_TRIGGER_Z)){VIDEO_WaitVSync();};
			useShuffle ^=1;
		}
		updatescreen_mp3(file, ret, numFiles, curMP3);
	}
	return ret;
}

/* Plays a MP3 file */
void mp3_player(file_handle* allFiles, int numFiles, file_handle* curFile) {
	// Initialise the audio subsystem
	ASND_Init(NULL);
	MP3Player_Init();
	
	// Find all the .mp3 files in the set of allFiles, and also where our file is.
	int curMP3 = 0, i = 0;

	for(i = 0; i < numFiles; i++) {
		if(!strcmp(allFiles[i].name,curFile->name)) {
			curMP3 = i;
			break;
		}
	}
	int first = 1;
	
	for(i = curMP3; i < numFiles; i++) {
		int ret = PLAYER_NEXT;
		if(!first && useShuffle) {
			i = (rand() % numFiles);
		}
		// if it's .mp3
		if((strlen(allFiles[i].name)>4) && ((strstr(allFiles[i].name,".MP3")!=NULL) || (strstr(allFiles[i].name,".mp3")!=NULL))) {
			ret = play_mp3(&allFiles[i], numFiles, i);
		}
		if(ret == PLAYER_STOP) {
			break;
		}
		if(ret == PLAYER_PREV) {
			i-=2;
		}
		first = 0;
	}
	
	MP3Player_Stop();
}

