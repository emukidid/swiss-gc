#ifndef _MP3_H_
#define _MP3_H_

#include <gccore.h>
#include <asndlib.h>
#include <mp3player.h>
#include "devices/deviceHandler.h"

#define PLAYER_PAUSE		0
#define PLAYER_STOP			1
#define PLAYER_NEXT			2
#define PLAYER_PREV			3

extern void mp3_player(file_handle* allFiles, int numFiles, file_handle* curFile);

#endif
