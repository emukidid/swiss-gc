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
//AutoCheats=Yes
//InitNetwork=No
//IGRType=Disabled
//FTPUserName=user
//FTPPassword=password
//FTPHostIP=192.168.1.32
//FTPPort=21
//FTPUsePasv=No
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

static ConfigEntry configEntries[2048]; // That's a lot of Games!
static int configEntriesCount = 0;

/** Crappy dynamic string appender */
#define APPEND_BLOCKSIZE 256
typedef struct {
	char *mem;
	u32 memlen;
} appended_string;

appended_string *string_append(appended_string *appstr, char* str) {

	if(appstr == NULL) {
		appstr = calloc(1, sizeof(appended_string));
		appstr->memlen = (str != NULL) ? strlen(str) : APPEND_BLOCKSIZE;
		appstr->memlen += APPEND_BLOCKSIZE-(appstr->memlen % APPEND_BLOCKSIZE);	// 64b sized blocks
		appstr->mem = calloc(1, appstr->memlen);
		if(appstr->mem != NULL && str != NULL) {
			strcpy(appstr->mem, str);
		}
	}
	else {
		if(str != NULL) {
			u32 oldlen = strlen(appstr->mem);
			u32 newlenreq = strlen(str) + strlen(appstr->mem);
			if(newlenreq >= appstr->memlen) {
				newlenreq += APPEND_BLOCKSIZE-(newlenreq%APPEND_BLOCKSIZE);
				appstr->mem = realloc(appstr->mem, newlenreq);
				appstr->memlen = newlenreq;
				memset(appstr->mem + oldlen, 0, newlenreq-oldlen);
			}
			strcpy(appstr->mem + oldlen, str);
		}
	}
	return appstr;
}

// Tries to init the current config device
bool config_set_device() {
	// Set the current config device to whatever the current configDeviceId is
	DEVICEHANDLER_INTERFACE *configDevice = getDeviceByUniqueId(swissSettings.configDeviceId);
	devices[DEVICE_CONFIG] = NULL;
	if(configDevice != NULL) {
		if(configDevice->test()) {
			deviceHandler_setDeviceAvailable(configDevice, true);
			devices[DEVICE_CONFIG] = configDevice;
		}
	}
	
	// Not available or not a writable device? That's too bad.
	if(devices[DEVICE_CONFIG] == NULL) {
		return false;
	}
	print_gecko("Save device is %s\r\n", devices[DEVICE_CONFIG]->deviceName);
	deviceHandler_setStatEnabled(0);
	// If we're not using this device already, init it.
	if(devices[DEVICE_CONFIG] != devices[DEVICE_CUR]) {
		print_gecko("Save device is not current, current is (%s)\r\n", devices[DEVICE_CUR] == NULL ? "NULL":devices[DEVICE_CUR]->deviceName);
		if(!devices[DEVICE_CONFIG]->init(devices[DEVICE_CONFIG]->initial)) {
			print_gecko("Save device failed to init\r\n");
			deviceHandler_setStatEnabled(1);
			return false;
		}
	}
	deviceHandler_setStatEnabled(1);
	return true;
}

/** 
	Initialises the configuration file
	Returns 1 on successful file open, 0 otherwise
*/
int config_init() {
	if(!config_set_device()) return 0;
	file_handle *configFile = (file_handle*)calloc(1, sizeof(file_handle));
	snprintf(configFile->name, PATHNAME_MAX, "%sswiss.ini", devices[DEVICE_CONFIG]->initial->name);
	
	// Read config
	if(devices[DEVICE_CONFIG]->readFile(configFile, txtbuffer, 1) == 1) {
		devices[DEVICE_CONFIG]->seekFile(configFile, 0, DEVICE_HANDLER_SEEK_SET);
		char *configData = (char*) memalign(32, configFile->size);
		configEntriesCount = 0;
		if (configData) {
			print_gecko("Config Size %i\r\n", configFile->size);
			memset(configData, 0, configFile->size);
			devices[DEVICE_CONFIG]->readFile(configFile, configData, configFile->size);
			devices[DEVICE_CONFIG]->closeFile(configFile);
			if(devices[DEVICE_CONFIG] != devices[DEVICE_CUR]) {
				devices[DEVICE_CONFIG]->deinit(devices[DEVICE_CONFIG]->initial);
			}
			config_parse(configData);
			free(configData);
			return 1;
		}
	}
	return 0;
}

int config_update_file() {
	if(!config_set_device()) return 0;

	// Write out header every time
	char *str = "# Swiss Configuration File!\r\n# Anything written in here will be lost!\r\n\r\n#!!Swiss Settings Start!!\r\n";
	appended_string *configString = string_append(NULL, str);
	// Write out Swiss settings
	sprintf(txtbuffer, "SD/IDE Speed=%s\r\n",(swissSettings.exiSpeed ? "32MHz":"16MHz"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Swiss Video Mode=%s\r\n",(uiVModeStr[swissSettings.uiVMode]));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Enable Debug=%s\r\n",(swissSettings.debugUSB ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Hide Unknown file types=%s\r\n",(swissSettings.hideUnknownFileTypes ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Stop DVD Motor on startup=%s\r\n",(swissSettings.stopMotor ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Enable WiiRD debug=%s\r\n",(swissSettings.wiirdDebug ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Enable File Management=%s\r\n",(swissSettings.enableFileManagement ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Disable Video Patches=%s\r\n",(swissSettings.disableVideoPatches ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Force Video Active=%s\r\n",(swissSettings.forceVideoActive ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Force DTV Status=%s\r\n",(swissSettings.forceDTVStatus ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "SMBUserName=%s\r\n",swissSettings.smbUser);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "SMBPassword=%s\r\n",swissSettings.smbPassword);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "SMBShareName=%s\r\n",swissSettings.smbShare);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "SMBHostIP=%s\r\n",swissSettings.smbServerIp);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "AutoCheats=%s\r\n", (swissSettings.autoCheats ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "InitNetwork=%s\r\n", (swissSettings.initNetworkAtStart ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "IGRType=%s\r\n", (igrTypeStr[swissSettings.igrType]));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "AVECompat=%s\r\n", (aveCompatStr[swissSettings.aveCompat]));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FileBrowserType=%s\r\n", (fileBrowserStr[swissSettings.fileBrowserType]));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "BS2Boot=%s\r\n", (bs2BootStr[swissSettings.bs2Boot]));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FTPUserName=%s\r\n",swissSettings.ftpUserName);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FTPPassword=%s\r\n",swissSettings.ftpPassword);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FTPHostIP=%s\r\n",swissSettings.ftpHostIp);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FTPPort=%hu\r\n",swissSettings.ftpPort);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FTPUsePasv=%s\r\n",swissSettings.ftpUsePasv ? "Yes":"No");
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FSPHostIP=%s\r\n",swissSettings.fspHostIp);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FSPPort=%hu\r\n",swissSettings.fspPort);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "FSPPassword=%s\r\n",swissSettings.fspPassword);
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "Autoload=%s\r\n",swissSettings.autoload);
	string_append(configString, txtbuffer);
	// Write out the default game config portion too
	sprintf(txtbuffer, "Force Video Mode=%s\r\n",gameVModeStr[swissSettings.gameVMode]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Force Horizontal Scale=%s\r\n",forceHScaleStr[swissSettings.forceHScale]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Force Vertical Offset=%+hi\r\n",swissSettings.forceVOffset);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Force Vertical Filter=%s\r\n",forceVFilterStr[swissSettings.forceVFilter]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Disable Alpha Dithering=%s\r\n",(swissSettings.disableDithering ? "Yes":"No"));
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Force Anisotropic Filter=%s\r\n",(swissSettings.forceAnisotropy ? "Yes":"No"));
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Force Widescreen=%s\r\n",forceWidescreenStr[swissSettings.forceWidescreen]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Invert Camera Stick=%s\r\n",invertCStickStr[swissSettings.invertCStick]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Emulate Read Speed=%s\r\n",emulateReadSpeedStr[swissSettings.emulateReadSpeed]);
	string_append(configString, txtbuffer);
	
	sprintf(txtbuffer, "Emulate Memory Card=%s\r\n",(swissSettings.emulateMemoryCard ? "Yes":"No"));
	string_append(configString, txtbuffer);
	sprintf(txtbuffer, "#!!Swiss Settings End!!\r\n\r\n");
	string_append(configString, txtbuffer);
	
	// Write out Game Configs
	int i;
	for(i = 0; i < configEntriesCount; i++) {
		sprintf(txtbuffer, "ID=%.4s\r\n",configEntries[i].game_id);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Name=%.64s\r\n",configEntries[i].game_name);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Comment=%.128s\r\n",configEntries[i].comment);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Status=%.32s\r\n",configEntries[i].status);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Video Mode=%s\r\n",gameVModeStr[configEntries[i].gameVMode]);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Horizontal Scale=%s\r\n",forceHScaleStr[configEntries[i].forceHScale]);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Vertical Offset=%+hi\r\n",configEntries[i].forceVOffset);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Vertical Filter=%s\r\n",forceVFilterStr[configEntries[i].forceVFilter]);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Disable Alpha Dithering=%s\r\n",(configEntries[i].disableDithering ? "Yes":"No"));
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Anisotropic Filter=%s\r\n",(configEntries[i].forceAnisotropy ? "Yes":"No"));
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Force Widescreen=%s\r\n",forceWidescreenStr[configEntries[i].forceWidescreen]);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Invert Camera Stick=%s\r\n",invertCStickStr[configEntries[i].invertCStick]);
		string_append(configString, txtbuffer);
		
		sprintf(txtbuffer, "Emulate Read Speed=%s\r\n",emulateReadSpeedStr[configEntries[i].emulateReadSpeed]);
		string_append(configString, txtbuffer);
	}

	file_handle *configFile = (file_handle*)calloc(1, sizeof(file_handle));
	snprintf(configFile->name, PATHNAME_MAX, "%sswiss.ini", devices[DEVICE_CONFIG]->initial->name);

	u32 len = strlen(configString->mem);
	// TODO ask overwrite?
	// TODO rename old file then delete if write successful
	devices[DEVICE_CONFIG]->deleteFile(configFile);
	if(devices[DEVICE_CONFIG]->writeFile(configFile, configString->mem, len) == len) {
		devices[DEVICE_CONFIG]->closeFile(configFile);
		if(devices[DEVICE_CONFIG] != devices[DEVICE_CUR]) {
			devices[DEVICE_CONFIG]->deinit(devices[DEVICE_CONFIG]->initial);
		}
		return 1;
	}
	else {
		return 0;
	}

	return 1;
}

static char emulateReadSpeedEntries[][4] = {"GQSD", "GQSE", "GQSF", "GQSI", "GQSP", "GQSS", "GTOJ"};

void config_defaults(ConfigEntry *entry) {
	strcpy(entry->comment, "No Comment");
	strcpy(entry->status, "Unknown");
	entry->gameVMode = swissSettings.gameVMode;
	entry->forceHScale = swissSettings.forceHScale;
	entry->forceVOffset = swissSettings.forceVOffset;
	entry->forceVOffset = swissSettings.aveCompat == 1 ? -3:0;
	entry->forceVFilter = swissSettings.forceVFilter;
	entry->disableDithering = swissSettings.disableDithering;
	entry->forceAnisotropy = swissSettings.forceAnisotropy;
	entry->forceWidescreen = swissSettings.forceWidescreen;
	entry->invertCStick = swissSettings.invertCStick;
	entry->emulateReadSpeed = swissSettings.emulateReadSpeed;

	for(int i = 0; i < sizeof(emulateReadSpeedEntries) / sizeof(*emulateReadSpeedEntries); i++) {
		if(!strncmp(entry->game_id, emulateReadSpeedEntries[i], 4)) {
			entry->emulateReadSpeed = 1;
			break;
		}
	}
}

void config_parse(char *configData) {
	// Parse each entry and put it into our array
	char *line, *linectx = NULL;
	int first = 1;
	bool defaultPassed = false;
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
					defaultPassed = true;
					if(!first) {
						configEntriesCount++;
					}
					strncpy(&configEntries[configEntriesCount].game_id[0], value, 4);
					first = 0;
					// Fill this entry with defaults incase some values are missing..
					config_defaults(&configEntries[configEntriesCount]);
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
					int *ptr = !defaultPassed ? &swissSettings.gameVMode : &configEntries[configEntriesCount].gameVMode;
					for(int i = 0; i < 15; i++) {
						if(!strcmp(gameVModeStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Horizontal Scale", name)) {
					int *ptr = !defaultPassed ? &swissSettings.forceHScale : &configEntries[configEntriesCount].forceHScale;
					for(int i = 0; i < 7; i++) {
						if(!strcmp(forceHScaleStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Vertical Offset", name)) {
					if(defaultPassed)
						configEntries[configEntriesCount].forceVOffset = atoi(value);
					else
						swissSettings.forceVOffset = atoi(value);
				}
				else if(!strcmp("Force Vertical Filter", name)) {
					int *ptr = !defaultPassed ? &swissSettings.forceVFilter : &configEntries[configEntriesCount].forceVFilter;
					for(int i = 0; i < 4; i++) {
						if(!strcmp(forceVFilterStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Alpha Dithering", name)) {
					if(defaultPassed)
						configEntries[configEntriesCount].disableDithering = !strcmp("Yes", value) ? 1:0;
					else
						swissSettings.disableDithering = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force Anisotropic Filter", name)) {
					if(defaultPassed)
						configEntries[configEntriesCount].forceAnisotropy = !strcmp("Yes", value) ? 1:0;
					else
						swissSettings.forceAnisotropy = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force Widescreen", name)) {
					int *ptr = !defaultPassed ? &swissSettings.forceWidescreen : &configEntries[configEntriesCount].forceWidescreen;
					for(int i = 0; i < 3; i++) {
						if(!strcmp(forceWidescreenStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Invert Camera Stick", name)) {
					int *ptr = !defaultPassed ? &swissSettings.invertCStick : &configEntries[configEntriesCount].invertCStick;
					for(int i = 0; i < 4; i++) {
						if(!strcmp(invertCStickStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Read Speed", name)) {
					int *ptr = !defaultPassed ? &swissSettings.emulateReadSpeed : &configEntries[configEntriesCount].emulateReadSpeed;
					for(int i = 0; i < 3; i++) {
						if(!strcmp(emulateReadSpeedStr[i], value)) {
							*ptr = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Memory Card", name)) {
					swissSettings.emulateMemoryCard = !strcmp("Yes", value) ? 1:0;
				}
				
				// Swiss settings
				else if(!strcmp("SD/IDE Speed", name)) {
					swissSettings.exiSpeed = !strcmp("32MHz", value) ? 1:0;
				}
				else if(!strcmp("Enable Debug", name)) {
					swissSettings.debugUSB = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Hide Unknown file types", name)) {
					swissSettings.hideUnknownFileTypes = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Stop DVD Motor on startup", name)) {
					swissSettings.stopMotor = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Enable WiiRD debug", name)) {
					swissSettings.wiirdDebug = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Enable File Management", name)) {
					swissSettings.enableFileManagement = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Disable Video Patches", name)) {
					swissSettings.disableVideoPatches = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force Video Active", name)) {
					swissSettings.forceVideoActive = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Force DTV Status", name)) {
					swissSettings.forceDTVStatus = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("Swiss Video Mode", name)) {
					if(!strcmp(uiVModeStr[0], value))
						swissSettings.uiVMode = 0;
					else if(!strcmp(uiVModeStr[1], value))
						swissSettings.uiVMode = 1;
					else if(!strcmp(uiVModeStr[2], value))
						swissSettings.uiVMode = 2;
					else if(!strcmp(uiVModeStr[3], value))
						swissSettings.uiVMode = 3;
					else if(!strcmp(uiVModeStr[4], value))
						swissSettings.uiVMode = 4;
				}
				else if(!strcmp("SMBUserName", name)) {
					strlcpy(&swissSettings.smbUser[0], value, sizeof(swissSettings.smbUser));
				}
				else if(!strcmp("SMBPassword", name)) {
					strlcpy(&swissSettings.smbPassword[0], value, sizeof(swissSettings.smbPassword));
				}
				else if(!strcmp("SMBShareName", name)) {
					strlcpy(&swissSettings.smbShare[0], value, sizeof(swissSettings.smbShare));
				}
				else if(!strcmp("SMBHostIP", name)) {
					strlcpy(&swissSettings.smbServerIp[0], value, sizeof(swissSettings.smbServerIp));
				}
				else if(!strcmp("AutoCheats", name)) {
					swissSettings.autoCheats = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("InitNetwork", name)) {
					swissSettings.initNetworkAtStart = !strcmp("Yes", value) ? 1:0;
				}
				else if(!strcmp("IGRType", name)) {
					if(!strcmp(igrTypeStr[0], value))
						swissSettings.igrType = 0;
					else if(!strcmp(igrTypeStr[1], value))
						swissSettings.igrType = 1;
					else if(!strcmp(igrTypeStr[2], value))
						swissSettings.igrType = 2;
				}
				else if(!strcmp("AVECompat", name)) {
					if(!strcmp(aveCompatStr[0], value))
						swissSettings.aveCompat = 0;
					else if(!strcmp(aveCompatStr[1], value))
						swissSettings.aveCompat = 1;
					else if(!strcmp(aveCompatStr[2], value))
						swissSettings.aveCompat = 2;
					else if(!strcmp(aveCompatStr[3], value))
						swissSettings.aveCompat = 3;
				}
				else if(!strcmp("FileBrowserType", name)) {
					if(!strcmp(fileBrowserStr[0], value))
						swissSettings.fileBrowserType = 0;
					else if(!strcmp(fileBrowserStr[1], value))
						swissSettings.fileBrowserType = 1;
				}
				else if(!strcmp("BS2Boot", name)) {
					if(!strcmp(bs2BootStr[0], value))
						swissSettings.bs2Boot = 0;
					else if(!strcmp(bs2BootStr[1], value))
						swissSettings.bs2Boot = 1;
					else if(!strcmp(bs2BootStr[2], value))
						swissSettings.bs2Boot = 2;
					else if(!strcmp(bs2BootStr[3], value))
						swissSettings.bs2Boot = 3;
				}
				else if(!strcmp("FTPUserName", name)) {
					strlcpy(&swissSettings.ftpUserName[0], value, sizeof(swissSettings.ftpUserName));
				}
				else if(!strcmp("FTPPassword", name)) {
					strlcpy(&swissSettings.ftpPassword[0], value, sizeof(swissSettings.ftpPassword));
				}
				else if(!strcmp("FTPHostIP", name)) {
					strlcpy(&swissSettings.ftpHostIp[0], value, sizeof(swissSettings.ftpHostIp));
				}
				else if(!strcmp("FTPPort", name)) {
					swissSettings.ftpPort = atoi(value);
				}
				else if(!strcmp("FTPUsePasv", name)) {
					swissSettings.ftpUsePasv = !strcmp("Yes", value);
				}
				else if(!strcmp("FSPHostIP", name)) {
					strlcpy(&swissSettings.fspHostIp[0], value, sizeof(swissSettings.fspHostIp));
				}
				else if(!strcmp("FSPPort", name)) {
					swissSettings.fspPort = atoi(value);
				}
				else if(!strcmp("FSPPassword", name)) {
					strlcpy(&swissSettings.fspPassword[0], value, sizeof(swissSettings.fspPassword));
				}
				else if(!strcmp("Autoload", name)) {
					strlcpy(&swissSettings.autoload[0], value, sizeof(swissSettings.autoload));
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
	config_defaults(entry);
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

SwissSettings backup;

void config_load_current(ConfigEntry *config) {
	// load settings for this game to current settings
	memcpy(&backup, &swissSettings, sizeof(SwissSettings));
	swissSettings.gameVMode = config->gameVMode;
	swissSettings.forceHScale = config->forceHScale;
	swissSettings.forceVOffset = config->forceVOffset;
	swissSettings.forceVFilter = config->forceVFilter;
	swissSettings.disableDithering = config->disableDithering;
	swissSettings.forceAnisotropy = config->forceAnisotropy;
	swissSettings.forceWidescreen = config->forceWidescreen;
	swissSettings.invertCStick = config->invertCStick;
	swissSettings.emulateReadSpeed = config->emulateReadSpeed;
}

void config_unload_current() {
	swissSettings.gameVMode = backup.gameVMode;
	swissSettings.forceHScale = backup.forceHScale;
	swissSettings.forceVOffset = backup.forceVOffset;
	swissSettings.forceVFilter = backup.forceVFilter;
	swissSettings.disableDithering = backup.disableDithering;
	swissSettings.forceAnisotropy = backup.forceAnisotropy;
	swissSettings.forceWidescreen = backup.forceWidescreen;
	swissSettings.invertCStick = backup.invertCStick;
	swissSettings.emulateReadSpeed = backup.emulateReadSpeed;
}