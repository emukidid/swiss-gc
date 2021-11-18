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

static char *validCodeValues = "0123456789AaBbCcDdEeFf";
// Checks that the line contains a valid code in the format of "01234567 89ABCDEF"
int isValidCode(char *code) {
	int i, j;
	
	if(strlen(code) < 17) return 0; // Not in format
	for(i = 0; i < 16; i++) {
		if(i == 8 && code[i] != ' ') 
			return 0; // No space separating the two values
		if(i == 8)
			continue;
		int found = 0;
		for(j = 0; j < strlen(validCodeValues); j++) {
			if(code[i] == validCodeValues[j]) {
				found = 1;
				break;
			}
		}
		if(!found) return 0; 	// Wasn't a valid hexadecimal value
	}

	return 1;
}

int containsXX(char *line) {
	return strlen(line)>=16 
			&& (
				(tolower((int)line[6]) > 0x66 || tolower((int)line[7]) > 0x66 || tolower((int)line[15]) > 0x66 || tolower((int)line[16]) > 0x66)
				)
			;
}

/** 
	Given a char array with the contents of a .txt, 
	this method will allocate and return a populated Parameters struct 
*/
void parseCheats(char *filecontents) {
	char *line = NULL, *prevLine = NULL, *linectx = NULL;
	int numCheats = 0;
	line = strtok_r( filecontents, "\n", &linectx );

	// Free previous
	if(_cheats.num_cheats > 0) {
		int i;
		for(i = 0; i < _cheats.num_cheats; i++) {
			if(_cheats.cheat[i].codes) {
				free(_cheats.cheat[i].codes);
			}
		}
	}
	memset(&_cheats, 0, sizeof(CheatEntries));
	
	CheatEntry *curCheat = NULL;	// The current one we're parsing
	while( line != NULL && numCheats < CHEATS_MAX_FOR_GAME) {
		//print_gecko("Line [%s]\r\n", line);
		
		if(isValidCode(line)) {		// The line looks like a valid code
			if(prevLine != NULL) {
				curCheat = &_cheats.cheat[numCheats];
				strncpy(curCheat->name, prevLine, strlen(prevLine) > CHEATS_NAME_LEN ? CHEATS_NAME_LEN-1:strlen(prevLine));
				if(curCheat->name[strlen(curCheat->name)-1] == '\r')
					curCheat->name[strlen(curCheat->name)-1] = 0;
				//print_gecko("Cheat Name: [%s]\r\n", prevLine);
			}
			int unsupported = 0;
			u32 *codesBuf = (u32*)calloc(1, sizeof(u32) * 2 * SINGLE_CHEAT_MAX_LINES);
			
			// Add this valid code as the first code for this cheat
			codesBuf[(curCheat->num_codes << 1)+0] = (u32)strtoul(line, NULL, 16);
			codesBuf[(curCheat->num_codes << 1)+1] = (u32)strtoul(line+8, NULL, 16);
			curCheat->num_codes++;
			
			line = strtok_r( NULL, "\n", &linectx);
			// Keep going until we're out of codes for this cheat
			while( line != NULL) {
				if(curCheat->num_codes == SINGLE_CHEAT_MAX_LINES) {
					unsupported = 1;
					break;
				}
				// If a code contains "XX" in it, it is unsupported, discard it entirely
				if(containsXX(line)) {
					unsupported = 1;
					break;
				}
				if(isValidCode(line)) {
					// Add this valid code
					codesBuf[(curCheat->num_codes << 1)+0] = (u32)strtoul(line, NULL, 16);
					codesBuf[(curCheat->num_codes << 1)+1] = (u32)strtoul(line+8, NULL, 16);
					curCheat->num_codes++;
				}
				else {
					break;
				}
				line = strtok_r( NULL, "\n", &linectx);
			}
			
			if(unsupported) {
				memset(curCheat, 0, sizeof(CheatEntry));
				while(line != NULL && strlen(line) >=2) {
					line = strtok_r( NULL, "\n", &linectx);	// finish this unsupported cheat.
				}
			}
			else {
				numCheats++;
				// Alloc and store it.
				print_gecko("Allocating %i bytes for %i codes\r\n", sizeof *curCheat->codes * curCheat->num_codes, curCheat->num_codes);
				curCheat->codes = calloc(1, sizeof *curCheat->codes * curCheat->num_codes );
				memcpy(curCheat->codes, codesBuf, sizeof *curCheat->codes * curCheat->num_codes);
			}
			free(codesBuf);
		}
		prevLine = line;
		// And round we go again
		line = strtok_r( NULL, "\n", &linectx);
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
	u8 *ptr = isDebug ? kenobigc_dbg_bin : kenobigc_bin;
	u32 size = isDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size;
	
	print_gecko("Copying kenobi%s to %08X\r\n", (isDebug?"_dbg":""),(u32)CHEATS_ENGINE);
	memcpy(CHEATS_ENGINE, ptr, size);
	memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);
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
	file_handle *cheatsFile = memalign(32,sizeof(file_handle));
	memset(cheatsFile, 0, sizeof(file_handle));
	snprintf(cheatsFile->name, PATHNAME_MAX, "%s/cheats/%.6s.txt", devices[DEVICE_CUR]->initial->name, trimmedGameId);
	print_gecko("Looking for cheats file @ [%s]\r\n", cheatsFile->name);

	devices[DEVICE_TEMP] = devices[DEVICE_CUR];

	// Check SD in all slots if we're not already running from SD, or if we fail from current device
	if(devices[DEVICE_TEMP]->readFile(cheatsFile, &testBuffer, 8) != 8) {
		// Try SD slots now
		if(devices[DEVICE_CUR] != &__device_sd_a) {
			devices[DEVICE_TEMP] = &__device_sd_a;
		}
		else {
			devices[DEVICE_TEMP] = &__device_sd_b;
		}
		file_handle *slotFile = devices[DEVICE_TEMP]->initial;
		memset(cheatsFile, 0, sizeof(file_handle));
		snprintf(cheatsFile->name, PATHNAME_MAX, "%s/cheats/%.6s.txt", slotFile->name, trimmedGameId);
		print_gecko("Looking for cheats file @ [%s]\r\n", cheatsFile->name);

		deviceHandler_setStatEnabled(0);
		devices[DEVICE_TEMP]->init(cheatsFile);
		if(devices[DEVICE_TEMP]->readFile(cheatsFile, &testBuffer, 8) != 8) {
			if(devices[DEVICE_TEMP] == &__device_sd_b) {
				devices[DEVICE_TEMP] = &__device_sd_c;	// We already tried A & failed, so last thing to try is SP2 slot.
			}
			else if (devices[DEVICE_CUR] != &__device_sd_b) {
				devices[DEVICE_TEMP] = &__device_sd_b;
				slotFile = devices[DEVICE_TEMP]->initial;
				memset(cheatsFile, 0, sizeof(file_handle));
				snprintf(cheatsFile->name, PATHNAME_MAX, "%s/cheats/%.6s.txt", slotFile->name, trimmedGameId);
				print_gecko("Looking for cheats file @[%s]\r\n", cheatsFile->name);

				devices[DEVICE_TEMP]->init(cheatsFile);
				if(devices[DEVICE_TEMP]->readFile(cheatsFile, &testBuffer, 8) != 8) {
					devices[DEVICE_TEMP] = &__device_sd_c; // Last thing to try is SP2 slot.
				}
			}
			if (devices[DEVICE_TEMP] == &__device_sd_c && devices[DEVICE_CUR] != &__device_sd_c) {
				slotFile = devices[DEVICE_TEMP]->initial;
				memset(cheatsFile, 0, sizeof(file_handle));
				snprintf(cheatsFile->name, PATHNAME_MAX, "%s/cheats/%.6s.txt", slotFile->name, trimmedGameId);
				print_gecko("Looking for cheats file @[%s]\r\n", cheatsFile->name);

				devices[DEVICE_TEMP]->init(cheatsFile);
				if(devices[DEVICE_TEMP]->readFile(cheatsFile, &testBuffer, 8) != 8) {
					devices[DEVICE_TEMP] = NULL; // All three have failed.
				}
			}
		}
		deviceHandler_setStatEnabled(1);
	}
	// Still fail?
	if(devices[DEVICE_TEMP] == NULL || devices[DEVICE_TEMP]->readFile(cheatsFile, &testBuffer, 8) != 8) {
		if(!silent) {
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y);
			uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"No cheats file found.\nPress A to continue.");
			DrawPublish(msgBox);
			while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
			DrawDispose(msgBox);
		}
		free(cheatsFile);
		return 0;
	}
	print_gecko("Cheats file found with size %i\r\n", cheatsFile->size);
	char *cheats_buffer = memalign(32, cheatsFile->size);
	if(cheats_buffer) {
		devices[DEVICE_TEMP]->seekFile(cheatsFile, 0, DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_TEMP]->readFile(cheatsFile, cheats_buffer, cheatsFile->size);
		parseCheats(cheats_buffer);
		free(cheats_buffer);
		free(cheatsFile);
	}
	if(!silent && _cheats.num_cheats == 0) {
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y);
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"Empty or unreadable cheats file found.\nPress A to continue.");
		DrawPublish(msgBox);
		while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
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
