#include <gccore.h>
#include <stdlib.h>
#include <asndlib.h>
#include <mp3player.h>
#include "main.h"
#include "util.h"
#include "swiss.h"
#include "mp3.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

static int useShuffle = 0;
static int volume = 255;

s32 mp3Reader(void *cbdata, void *dst, s32 size) {
	file_handle *file = cbdata;
	devices[DEVICE_CUR]->seekFile(file,file->offset,DEVICE_HANDLER_SEEK_SET);
	int ret = devices[DEVICE_CUR]->readFile(file,dst,size);
	return ret;
}

uiDrawObj_t* updatescreen_mp3(file_handle *file, int state, int numFiles, int curMP3) {
	uiDrawObj_t* player = DrawEmptyBox(10,100, getVideoMode()->fbWidth-10, 400);
	sprintf(txtbuffer, "%s -  Volume (%i%%)", (state == PLAYER_PAUSE ? "Paused":"Playing"), (int)(((float)volume/(float)255)*100));
	DrawAddChild(player, DrawStyledLabel(640/2, 130, txtbuffer, 1.0f, true, defaultColor));
	sprintf(txtbuffer, "(%i/%i) %s",curMP3, numFiles,getRelativeName(file->name));
	float scale = GetTextScaleToFitInWidth(txtbuffer, getVideoMode()->fbWidth-10-10);
	DrawAddChild(player, DrawStyledLabel(640/2, 160, txtbuffer, scale, true, defaultColor));
	memset(txtbuffer, 0, 256);
	sprintf(txtbuffer, "------------------------------");
	float percentPlayed = (float)(((float)file->offset / (float)file->size) * 30);
	txtbuffer[(int)percentPlayed] = '*';
	DrawAddChild(player, DrawStyledLabel(640/2, 210, txtbuffer, 1.0f, true, defaultColor));
	DrawAddChild(player, DrawStyledLabel(640/2, 300, "(<-) Rewind (->) Forward (X) Vol+ (Y) Vol-", 1.0f, true, defaultColor));
	DrawAddChild(player, DrawStyledLabel(640/2, 330, "(B) Stop (L) Prev (R) Next (Start) Pause", 1.0f, true, defaultColor));
	sprintf(txtbuffer, "Shuffle is currently %s press (Z) to toggle", (useShuffle ? "on":"off"));
	DrawAddChild(player, DrawStyledLabel(640/2, 360, txtbuffer, 1.0f, true, defaultColor));
	return player;
}

int play_mp3(file_handle *file, int numFiles, int curMP3) {
	int ret = PLAYER_NEXT;
	file->offset = 0;
	MP3Player_PlayFile(file, &mp3Reader, NULL);
	uiDrawObj_t* player = NULL;
	while(MP3Player_IsPlaying() || ret == PLAYER_PAUSE ) {
	
		u32 buttons = padsButtonsHeld();
		if(buttons & PAD_BUTTON_B) {			// Stop
			while((padsButtonsHeld() & PAD_BUTTON_B)){VIDEO_WaitVSync();};
			MP3Player_Stop();
			ret = PLAYER_STOP;
		}
		else if(buttons & PAD_BUTTON_START) {	// Pause
			while((padsButtonsHeld() & PAD_BUTTON_START)){VIDEO_WaitVSync();};
			if(ret != PLAYER_PAUSE) {
				MP3Player_Stop();
				ret = PLAYER_PAUSE;
			}
			else {
				MP3Player_PlayFile(file, &mp3Reader, NULL);	// Unpause
				ret = PLAYER_NEXT;
			}
		}
		else if(buttons & PAD_BUTTON_X) {		// VOL+
			if(volume!=255) volume++;
			MP3Player_Volume(volume);
		}
		else if(buttons & PAD_BUTTON_Y) {		// VOL-
			if(volume!=0) volume--;
			MP3Player_Volume(volume);
		}
		else if(buttons & PAD_TRIGGER_R) {		// Next
			MP3Player_Stop();
			ret = PLAYER_NEXT;
		}
		else if(buttons & PAD_TRIGGER_L) {		// Previous
			MP3Player_Stop();
			ret = PLAYER_PREV;
		}
		else if(buttons & PAD_BUTTON_RIGHT) {		// Fwd
			MP3Player_Stop();
			if(file->offset+0x8000 < file->size) {
				file->offset += 0x8000;
				MP3Player_PlayFile(file, &mp3Reader, NULL);
			}
			else {
				ret = PLAYER_NEXT;
				break;
			}
		}
		else if(buttons & PAD_BUTTON_LEFT) {		// Rewind
			MP3Player_Stop();
			if(file->offset-0x10000 > 0) file->offset -= 0x10000;
			else file->offset = 0;
			MP3Player_PlayFile(file, &mp3Reader, NULL);
		}
		else if(buttons & PAD_TRIGGER_Z) {		// Toggle Shuffle
			while((padsButtonsHeld() & PAD_TRIGGER_Z)){VIDEO_WaitVSync();};
			useShuffle ^=1;
		}
		uiDrawObj_t* display = updatescreen_mp3(file, ret, numFiles, curMP3);
		if(player != NULL) {
			DrawDispose(player);
		}
		DrawPublish(display);
		player = display;
		usleep(5000);
	}
	if(player != NULL)
		DrawDispose(player);
	return ret;
}

/* Plays a MP3 file */
void mp3_player(file_handle* allFiles, int numFiles, file_handle* curFile) {
	// Initialise the audio subsystem
	ASND_Init();
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
		if((strlen(allFiles[i].name)>4) && endsWith(allFiles[i].name,".mp3")) {
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

