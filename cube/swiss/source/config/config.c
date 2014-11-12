#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "config.h"
#include "settings.h"

// This is an example Swiss settings entry (sits at the top of swiss.ini)
//!!Swiss Settings Start!!
//Default Device=Yes
//SD/IDE Speed=32MHz
//Swiss Video Mode=Auto
//Enable Debug=No
//Force No DVD Drive Mode=No
//Hide Unknown file types=No
//Stop DVD Motor on startup=Yes
//Enable WiiRD debug=Yes
//Enable File Management=No
//SMBUserName=user
//SMBPassword=password
//SMBShareName=nas
//SMBHostIP=192.168.1.32
//!!Swiss Settings End!!

// This is an example game entry
//ID=GAFE
//Name=Animal Crossing (NTSC)
//Comment=Playable without issues
//Status=Working
//Force Video Mode=Progressive
//Mute Audio Streaming=Yes
//No Disc Mode=Yes
//Force Widescreen=Yes
//Emulate Memory Card via SDGecko=Yes

static ConfigEntry configEntries[2048]; // That's a lot of Games!
static int configEntriesCount = 0;
static SwissSettings configSwissSettings;

void strnscpy(char *s1, char *s2, int num) {
	strncpy(s1, s2, num);
	s1[num] = 0;
}

/** 
	Initialises the configuration file
	Returns 1 on successful file open, 0 otherwise
*/
int config_init() {
	sprintf(txtbuffer, "%sswiss.ini", deviceHandler_initial->name);
	FILE *fp = fopen(txtbuffer, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (size > 0) {
			configEntriesCount = 0;
			char *configData = (char*) memalign(32, size);
			if (configData) {
				fread(configData, 1, size, fp);
				config_parse(configData);
				free(configData);
			}
		}
		else {
			return 0;
		}
		fclose(fp);
		return 1;
	}
	else {
		return 0;
	}
	
	return 1;
}


/**
	Creates a configuration file on the root of the device "swiss.ini"
	Returns 1 on successful creation, 0 otherwise
*/
int config_create() {
	return config_update_file();
}

int config_update_file() {
	sprintf(txtbuffer, "%sswiss.ini", deviceHandler_initial->name);
	FILE *fp = fopen( txtbuffer, "wb" );
	if(fp) {
		// Write out header every time
		char *str = "# Swiss Configuration File!\r\n# Anything written in here will be lost!\r\n\r\n#!!Swiss Settings Start!!\r\n";
		fwrite(str, 1, strlen(str), fp);
		
		// Write out Swiss settings
		sprintf(txtbuffer, "Default Device=%s\r\n",(configSwissSettings.defaultDevice ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "SD/IDE Speed=%s\r\n",(configSwissSettings.exiSpeed ? "32MHz":"16MHz"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Swiss Video Mode=%s\r\n",(uiVModeStr[configSwissSettings.uiVMode]));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Enable Debug=%s\r\n",(configSwissSettings.debugUSB ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Force No DVD Drive Mode=%s\r\n",(configSwissSettings.hasDVDDrive ? "No":"Yes"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Hide Unknown file types=%s\r\n",(configSwissSettings.hideUnknownFileTypes ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Stop DVD Motor on startup=%s\r\n",(configSwissSettings.stopMotor ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Enable WiiRD debug=%s\r\n",(configSwissSettings.wiirdDebug ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "Enable File Management=%s\r\n",(configSwissSettings.enableFileManagement ? "Yes":"No"));
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "SMBUserName=%s\r\n",configSwissSettings.smbUser);
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "SMBPassword=%s\r\n",configSwissSettings.smbPassword);
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "SMBShareName=%s\r\n",configSwissSettings.smbShare);
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "SMBHostIP=%s\r\n",configSwissSettings.smbServerIp);
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		sprintf(txtbuffer, "#!!Swiss Settings End!!\r\n\r\n");
		fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		
		
		// Write out Game Configs
		int i;
		for(i = 0; i < configEntriesCount; i++) {
			char buffer[256];
			strnscpy(buffer, &configEntries[i].game_id[0], 4);
			sprintf(txtbuffer, "ID=%s\r\n",buffer);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			strnscpy(buffer, &configEntries[i].game_name[0], 32);
			sprintf(txtbuffer, "Name=%s\r\n",buffer);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			strnscpy(buffer, &configEntries[i].comment[0], 128);
			sprintf(txtbuffer, "Comment=%s\r\n",buffer);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			strnscpy(buffer, &configEntries[i].status[0], 32);
			sprintf(txtbuffer, "Status=%s\r\n",buffer);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Force Video Mode=%s\r\n",uiVModeStr[configEntries[i].gameVMode]);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Soft Progressive=%s\r\n",softProgressiveStr[configEntries[i].softProgressive]);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Mute Audio Streaming=%s\r\n",(configEntries[i].muteAudioStreaming ? "Yes":"No"));
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
					
			sprintf(txtbuffer, "Force Widescreen=%s\r\n",forceWidescreenStr[configEntries[i].forceWidescreen]);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Force Anisotropy=%s\r\n",(configEntries[i].forceAnisotropy ? "Yes":"No"));
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Emulate Memory Card via SDGecko=%s\r\n",(configEntries[i].emulatemc ? "Yes":"No"));
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
			
			sprintf(txtbuffer, "Force Encoding=%s\r\n\r\n\r\n",forceEncodingStr[configEntries[i].forceEncoding]);
			fwrite(txtbuffer, 1, strlen(txtbuffer), fp);
		}
		fclose(fp);
		return 1;
	}
	return 0;
}

void config_parse(char *configData) {
	// Parse each entry and put it into our array
	char *line, *linectx = NULL;
	int first = 1;
	line = strtok_r( configData, "\r\n", &linectx );
	while( line != NULL ) {
		//print_gecko("Line [%s]\r\n", line);
		if(line[0] != '#') {
			// Is this line a new game entry?
			char *name, *namectx = NULL;
			char *value = NULL;
			name = strtok_r(line, "=", &namectx);
			if(name != NULL)
				value = strtok_r(NULL, "=", &namectx);
			
			if(value != NULL) {
				//print_gecko("Name [%s] Value [%s]\r\n", name, value);

				if(!strcmp("ID", name)) {
					if(!first) {
						configEntriesCount++;
					}
					strncpy(&configEntries[configEntriesCount].game_id[0], value, 4);
					first = 0;
					// Fill this entry with defaults incase some values are missing..
					strcpy(&configEntries[configEntriesCount].comment[0],"No Comment");
					strcpy(&configEntries[configEntriesCount].status[0],"Unknown");
					configEntries[configEntriesCount].gameVMode = 0;
					configEntries[configEntriesCount].softProgressive = 0;
					configEntries[configEntriesCount].muteAudioStreaming = 1;
					configEntries[configEntriesCount].forceWidescreen = 0;
					configEntries[configEntriesCount].forceAnisotropy = 0;
					configEntries[configEntriesCount].forceEncoding = 0;
					configEntries[configEntriesCount].emulatemc = 0;
				}
				else if(!strcmp("Name", name)) {
					strncpy(&configEntries[configEntriesCount].game_name[0], value, 64);
				}
				else if(!strcmp("Comment", name)) {
					strncpy(&configEntries[configEntriesCount].comment[0], value, 128);
				}
				else if(!strcmp("Status", name)) {
					strncpy(&configEntries[configEntriesCount].status[0], value, 32);
				}
				else if(!strcmp("Force Video Mode", name)) {
					if(!strcmp(uiVModeStr[0], value))
						configEntries[configEntriesCount].gameVMode = 0;
					else if(!strcmp(uiVModeStr[1], value))
						configEntries[configEntriesCount].gameVMode = 1;
					else if(!strcmp(uiVModeStr[2], value))
						configEntries[configEntriesCount].gameVMode = 2;
					else if(!strcmp(uiVModeStr[3], value))
						configEntries[configEntriesCount].gameVMode = 3;
					else if(!strcmp(uiVModeStr[4], value))
						configEntries[configEntriesCount].gameVMode = 4;
					else if(!strcmp(uiVModeStr[5], value))
						configEntries[configEntriesCount].gameVMode = 5;
					else if(!strcmp(uiVModeStr[6], value))
						configEntries[configEntriesCount].gameVMode = 6;
				}
				else if(!strcmp("Soft Progressive", name)) {
					if(!strcmp(softProgressiveStr[0], value))
						configEntries[configEntriesCount].softProgressive = 0;
					else if(!strcmp(softProgressiveStr[1], value))
						configEntries[configEntriesCount].softProgressive = 1;
					else if(!strcmp(softProgressiveStr[2], value))
						configEntries[configEntriesCount].softProgressive = 2;
				}
				else if(!strcmp("Mute Audio Streaming", name)) {
					configEntries[configEntriesCount].muteAudioStreaming = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force Widescreen", name)) {
					if(!strcmp(forceWidescreenStr[0], value))
						configEntries[configEntriesCount].forceWidescreen = 0;
					else if(!strcmp(forceWidescreenStr[1], value))
						configEntries[configEntriesCount].forceWidescreen = 1;
					else if(!strcmp(forceWidescreenStr[2], value))
						configEntries[configEntriesCount].forceWidescreen = 2;
				}
				else if(!strcmp("Force Anisotropy", name)) {
					configEntries[configEntriesCount].forceAnisotropy = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Emulate Memory Card via SDGecko", name)) {
					configEntries[configEntriesCount].emulatemc = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force Encoding", name)) {
					if(!strcmp(forceEncodingStr[0], value))
						configEntries[configEntriesCount].forceEncoding = 0;
					else if(!strcmp(forceEncodingStr[1], value))
						configEntries[configEntriesCount].forceEncoding = 1;
					else if(!strcmp(forceEncodingStr[2], value))
						configEntries[configEntriesCount].forceEncoding = 2;
				}
				
				// Swiss settings
				else if(!strcmp("Default Device", name)) {
					configSwissSettings.defaultDevice = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("SD/IDE Speed", name)) {
					configSwissSettings.exiSpeed = !strcmp("32MHz", value) ? 1:0;
				}
				else if(!strcmp("Enable Debug", name)) {
					configSwissSettings.debugUSB = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force No DVD Drive Mode", name)) {
					configSwissSettings.hasDVDDrive = !strcmp("No", value) ? 1:0;
				}
				else if(!strcmp("Hide Unknown file types", name)) {
					configSwissSettings.hideUnknownFileTypes = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Stop DVD Motor on startup", name)) {
					configSwissSettings.stopMotor = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Enable WiiRD debug", name)) {
					configSwissSettings.wiirdDebug = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Enable File Management", name)) {
					configSwissSettings.enableFileManagement = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Swiss Video Mode", name)) {
					if(!strcmp(uiVModeStr[0], value))
						configSwissSettings.uiVMode = 0;
					else if(!strcmp(uiVModeStr[1], value))
						configSwissSettings.uiVMode = 1;
					else if(!strcmp(uiVModeStr[2], value))
						configSwissSettings.uiVMode = 2;
					else if(!strcmp(uiVModeStr[3], value))
						configSwissSettings.uiVMode = 3;
					else if(!strcmp(uiVModeStr[4], value))
						configSwissSettings.uiVMode = 4;
					else if(!strcmp(uiVModeStr[5], value))
						configSwissSettings.uiVMode = 5;
					else if(!strcmp(uiVModeStr[6], value))
						configSwissSettings.uiVMode = 6;
				}
				else if(!strcmp("SMBUserName", name)) {
					strncpy(configSwissSettings.smbUser, value, 20);
				}
				else if(!strcmp("SMBPassword", name)) {
					strncpy(configSwissSettings.smbPassword, value, 16);
				}
				else if(!strcmp("SMBShareName", name)) {
					strncpy(configSwissSettings.smbShare, value, 80);
				}
				else if(!strcmp("SMBHostIP", name)) {
					strncpy(configSwissSettings.smbServerIp, value, 80);
				}
			}
		}
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}

	if(configEntriesCount > 0 || !first)
		configEntriesCount++;
	
	 print_gecko("Found %i entries in the config file\r\n",configEntriesCount);
}

void config_find(ConfigEntry *entry) {
	//print_gecko("config_find: Looking for game with ID %s\r\n",entry->game_id);
	// Try to lookup this game based on game_id
	int i;
	for(i = 0; i < configEntriesCount; i++) {
		if(!strncmp(entry->game_id, configEntries[i].game_id, 4)) {
			memcpy(entry, &configEntries[i], sizeof(ConfigEntry));
			//print_gecko("config_find: Found %s\r\n",entry->game_id);
			return;
		}
	}
	// Didn't find it, setup defaults and add this entry
	strcpy(entry->comment,"No Comment");
	strcpy(entry->status,"Unknown");
	entry->gameVMode = 0;
	entry->softProgressive = 0;
	entry->muteAudioStreaming = 0;
	entry->forceWidescreen = 0;
	entry->forceAnisotropy = 0;
	entry->forceEncoding = 0;
	entry->emulatemc = 0;
	// Add this new entry to our collection
	memcpy(&configEntries[configEntriesCount], entry, sizeof(ConfigEntry));
	configEntriesCount++;
	//print_gecko("config_find: Couldn't find, creating %s\r\n",entry->game_id);
}

int config_update(ConfigEntry *entry) {
	//print_gecko("config_update: Looking for game with ID %s\r\n",entry->game_id);
	int i;
	for(i = 0; i < configEntriesCount; i++) {
		if(!strncmp(entry->game_id, configEntries[i].game_id, 4)) {
			//print_gecko("config_update: Found %s\r\n",entry->game_id);
			memcpy(&configEntries[i], entry, sizeof(ConfigEntry));
			return config_update_file();	// Write out the file now
		}
	}
	return 0; // should never happen since we add in the game
}

int config_get_count() {
	return configEntriesCount;
}

void config_copy_swiss_settings(SwissSettings *settings) {
	memcpy(&configSwissSettings, settings, sizeof(SwissSettings));
}

SwissSettings *config_get_swiss_settings() {
	return &configSwissSettings;
}
