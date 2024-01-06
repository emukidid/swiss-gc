/**
*
* Gecko OS/WiiRD cheat engine (kenobigc)
* 
* Adapted to Swiss by emu_kidid 2012-2015
*
*/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>
#include "swiss.h"
#include "main.h"
#include "cheats.h"
#include "patcher.h"
#include "deviceHandler.h"
#include "FrameBufferMagic.h"

static CheatEntries _cheats;

void printCheats(void) {
	int i = 0, j = 0;
	print_gecko("There are %i cheats\r\n", _cheats.num_cheats);
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		print_gecko("Cheat: (%i codes) %s\r\n", cheat->num_codes, cheat->name);
		for(j = 0; j < cheat->num_codes; j++) {
			print_gecko("%08X %08X\r\n", cheat->codes[j][0], cheat->codes[j][1]);
		}
	}
}

int getEnabledCheatsSize(void) {
	int i = 0, size = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		if(cheat->enabled) {
			size += ((cheat->num_codes*2)*4);
		}
	}
	//print_gecko("Size of cheats: %i\r\n", size);
	return size;
}

int getEnabledCheatsCount(void) {
	int i = 0, enabled = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		if(cheat->enabled) {
			enabled++;
		}
	}
	//print_gecko("Size of cheats: %i\r\n", size);
	return enabled;
}

// Checks that the line contains a valid code in the format of "01234567 89ABCDEF"
int isValidCode(char *line) {
	int len = 0;
	return sscanf(line, "%*8[0-9A-Fa-f] %*8[0-9A-Fa-f]%n", &len) == 0 && len > 16;
}

int isCheatCode(char *line) {
	int len = 0;
	return sscanf(line, "%*2[0-9A-Fa-f]%*6[0-9A-Za-z] %*8[0-9A-Za-z]%n", &len) == 0 && len > 16;
}

/** 
	Given a char array with the contents of a .txt, 
	this method will allocate and return a populated Parameters struct 
*/
void parseCheats(char *filecontents) {
	char *line = NULL, *prevLine = NULL, *linectx = NULL;
	int numCheats = 0;
	line = strtok_r( filecontents, "\r\n", &linectx );

	// Free previous
	if(_cheats.cheat) {
		int i;
		for(i = 0; i < _cheats.num_cheats; i++) {
			CheatEntry *cheat = &_cheats.cheat[i];
			free(cheat->name);
			free(cheat->codes);
		}
		free(_cheats.cheat);
		_cheats.cheat = NULL;
		_cheats.num_cheats = 0;
	}
	
	CheatEntry *curCheat = NULL;	// The current one we're parsing
	while( line != NULL ) {
		//print_gecko("Line [%s]\r\n", line);
		if(isCheatCode(line)) {		// The line looks like a valid code
			_cheats.cheat = reallocarray(_cheats.cheat, numCheats+1, sizeof(CheatEntry));
			curCheat = &_cheats.cheat[numCheats];
			memset(curCheat, 0, sizeof(CheatEntry));
			
			if(prevLine != NULL) {
				curCheat->name = strdup(prevLine);
				//print_gecko("Cheat Name: [%s]\r\n", prevLine);
			}
			int numCodes = 0, unsupported = 0;
			if(isValidCode(line)) {
				// Add this valid code as the first code for this cheat
				curCheat->codes = reallocarray(curCheat->codes, numCodes+1, sizeof(*curCheat->codes));
				sscanf(line, "%x %x", &curCheat->codes[numCodes][0], &curCheat->codes[numCodes][1]);
				numCodes++;
			}
			else {
				// If a code contains "XX" in it, it is unsupported, discard it entirely
				unsupported = 1;
				numCodes++;
			}
			
			line = strtok_r( NULL, "\r\n", &linectx);
			// Keep going until we're out of codes for this cheat
			while( line != NULL ) {
				if(isCheatCode(line)) {
					if(isValidCode(line)) {
						// Add this valid code
						curCheat->codes = reallocarray(curCheat->codes, numCodes+1, sizeof(*curCheat->codes));
						sscanf(line, "%x %x", &curCheat->codes[numCodes][0], &curCheat->codes[numCodes][1]);
						numCodes++;
					}
					else {
						// If a code contains "XX" in it, it is unsupported, discard it entirely
						unsupported = 1;
						numCodes++;
					}
				}
				else {
					break;
				}
				line = strtok_r( NULL, "\r\n", &linectx);
			}
			
			if(unsupported) {
				free(curCheat->name);
				free(curCheat->codes);
			}
			else {
				curCheat->num_codes = numCodes;
				numCheats++;
			}
		}
		prevLine = line;
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}
	_cheats.num_cheats = numCheats;
	//printCheats();
}

CheatEntries* getCheats() {
	return &_cheats;
}

// Installs the GeckoOS (kenobiGC) cheats engine and sets up variables/copies cheats
void kenobi_install_engine() {
	int isDebug = swissSettings.wiirdDebug;
	// If high memory is in use, we'll use low, otherwise high.
	const u8 *ptr = isDebug ? kenobigc_dbg_bin : kenobigc_bin;
	u32 size = isDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size;
	
	print_gecko("Copying kenobi%s to %08X\r\n", (isDebug?"_dbg":""),(u32)CHEATS_ENGINE);
	memcpy(CHEATS_ENGINE, ptr, size);
	memcpy(CHEATS_GAMEID, VAR_AREA, CHEATS_GAMEID_LEN);
	if(!isDebug) {
		CHEATS_ENABLE_CHEATS = CHEATS_TRUE;
	}
	CHEATS_START_PAUSED = isDebug ? CHEATS_TRUE : CHEATS_FALSE;
	memset(CHEATS_LOCATION(size), 0, kenobi_get_maxsize());
	print_gecko("Copying %i bytes of cheats to %08X\r\n", getEnabledCheatsSize(),(u32)CHEATS_LOCATION(size));
	u32 *cheatsLocation = (u32*)CHEATS_LOCATION(size);
	cheatsLocation[0] = 0x00D0C0DE;
	cheatsLocation[1] = 0x00D0C0DE;
	cheatsLocation+=2;
	
	int i = 0, j = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		if(cheat->enabled) {
			for(j = 0; j < cheat->num_codes; j++) {
				// Copy & fix cheats that want to jump to the old cheat engine location 0x800018A8 -> CHEATS_ENGINE+0xA8
				cheatsLocation[0] = cheat->codes[j][0];
				cheatsLocation[1] = cheat->codes[j][1] == 0x800018A8 ? (u32)(CHEATS_ENGINE+0xA8) : cheat->codes[j][1];
				print_gecko("Copied cheat [%08X %08X] to [%08X %08X]\r\n", cheatsLocation[0], cheatsLocation[1], (u32)cheatsLocation, (u32)cheatsLocation+4);
				cheatsLocation+=2;
			}
		}
	}
	cheatsLocation[0] = 0xFF000000;
	DCFlushRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
	ICInvalidateRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
}

int kenobi_get_maxsize() {
	return CHEATS_MAX_SIZE((swissSettings.wiirdDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size));
}

int findCheats(bool silent) {
	char trimmedGameId[8], testBuffer[8];
	memset(trimmedGameId, 0, 8);
	memcpy(trimmedGameId, (char*)&GCMDisk, 6);
	file_handle *cheatsFile = calloc(1, sizeof(file_handle));
	concatf_path(cheatsFile->name, devices[DEVICE_CUR]->initial->name, "swiss/cheats/%.6s.txt", trimmedGameId);
	print_gecko("Looking for cheats file @ [%s]\r\n", cheatsFile->name);

	devices[DEVICE_CHEATS] = devices[DEVICE_CUR];

	// Check SD in all slots if we're not already running from SD, or if we fail from current device
	ensure_path(DEVICE_CHEATS, "swiss", NULL);
	ensure_path(DEVICE_CHEATS, "swiss/cheats", "cheats");	// TODO kill this off in our next major release.
	if(devices[DEVICE_CHEATS]->readFile(cheatsFile, &testBuffer, 8) != 8) {
		// Try SD slots now
		if(devices[DEVICE_CUR] != &__device_sd_a) {
			devices[DEVICE_CHEATS] = &__device_sd_a;
		}
		else {
			devices[DEVICE_CHEATS] = &__device_sd_b;
		}
		memset(cheatsFile, 0, sizeof(file_handle));
		concatf_path(cheatsFile->name, devices[DEVICE_CHEATS]->initial->name, "swiss/cheats/%.6s.txt", trimmedGameId);
		print_gecko("Looking for cheats file @ [%s]\r\n", cheatsFile->name);

		deviceHandler_setStatEnabled(0);
		devices[DEVICE_CHEATS]->init(cheatsFile);
		ensure_path(DEVICE_CHEATS, "swiss", NULL);
		ensure_path(DEVICE_CHEATS, "swiss/cheats", "cheats");	// TODO kill this off in our next major release.
		if(devices[DEVICE_CHEATS]->readFile(cheatsFile, &testBuffer, 8) != 8) {
			if(devices[DEVICE_CHEATS] == &__device_sd_b) {
				devices[DEVICE_CHEATS] = &__device_sd_c;	// We already tried A & failed, so last thing to try is SP2 slot.
			}
			else if (devices[DEVICE_CUR] != &__device_sd_b) {
				devices[DEVICE_CHEATS] = &__device_sd_b;
				memset(cheatsFile, 0, sizeof(file_handle));
				concatf_path(cheatsFile->name, devices[DEVICE_CHEATS]->initial->name, "swiss/cheats/%.6s.txt", trimmedGameId);
				print_gecko("Looking for cheats file @[%s]\r\n", cheatsFile->name);

				devices[DEVICE_CHEATS]->init(cheatsFile);
				ensure_path(DEVICE_CHEATS, "swiss", NULL);
				ensure_path(DEVICE_CHEATS, "swiss/cheats", "cheats");	// TODO kill this off in our next major release.
				if(devices[DEVICE_CHEATS]->readFile(cheatsFile, &testBuffer, 8) != 8) {
					devices[DEVICE_CHEATS] = &__device_sd_c; // Last thing to try is SP2 slot.
				}
			}
			if (devices[DEVICE_CHEATS] == &__device_sd_c && devices[DEVICE_CUR] != &__device_sd_c) {
				memset(cheatsFile, 0, sizeof(file_handle));
				concatf_path(cheatsFile->name, devices[DEVICE_CHEATS]->initial->name, "swiss/cheats/%.6s.txt", trimmedGameId);
				print_gecko("Looking for cheats file @[%s]\r\n", cheatsFile->name);

				devices[DEVICE_CHEATS]->init(cheatsFile);
				ensure_path(DEVICE_CHEATS, "swiss", NULL);
				ensure_path(DEVICE_CHEATS, "swiss/cheats", "cheats");	// TODO kill this off in our next major release.
				if(devices[DEVICE_CHEATS]->readFile(cheatsFile, &testBuffer, 8) != 8) {
					devices[DEVICE_CHEATS] = NULL; // All three have failed.
				}
			}
		}
		deviceHandler_setStatEnabled(1);
	}
	// Still fail?
	if(devices[DEVICE_CHEATS] == NULL || cheatsFile->size == 0) {
		if(!silent) {
			while(padsButtonsHeld() & PAD_BUTTON_Y);
			uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"No cheats file found.\nPress A to continue.");
			DrawPublish(msgBox);
			while(!(padsButtonsHeld() & PAD_BUTTON_A));
			while(padsButtonsHeld() & PAD_BUTTON_A);
			DrawDispose(msgBox);
		}
		free(cheatsFile);
		return 0;
	}
	print_gecko("Cheats file found with size %i\r\n", cheatsFile->size);
	char *cheats_buffer = calloc(1, cheatsFile->size + 1);
	if(cheats_buffer) {
		devices[DEVICE_CHEATS]->seekFile(cheatsFile, 0, DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_CHEATS]->readFile(cheatsFile, cheats_buffer, cheatsFile->size);
		parseCheats(cheats_buffer);
		free(cheats_buffer);
	}
	devices[DEVICE_CHEATS]->closeFile(cheatsFile);
	free(cheatsFile);

	if(!silent && _cheats.num_cheats == 0) {
		while(padsButtonsHeld() & PAD_BUTTON_Y);
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"Empty or unreadable cheats file found.\nPress A to continue.");
		DrawPublish(msgBox);
		while(!(padsButtonsHeld() & PAD_BUTTON_A));
		while(padsButtonsHeld() & PAD_BUTTON_A);
		DrawDispose(msgBox);
	}
	return _cheats.num_cheats;
}

int applyAllCheats() {
	int i = 0, j = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		cheat->enabled = 1;
		if(getEnabledCheatsSize() > kenobi_get_maxsize())
			cheat->enabled = 0;
		j++;
	}
	return j;
}
