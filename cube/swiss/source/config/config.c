#include <argz.h>
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
#include "bba.h"

// This is an example Swiss settings entry (sits at the top of global.ini)
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

#define SWISS_SETTINGS_FILENAME_LEGACY "swiss.ini"
#define SWISS_SETTINGS_FILENAME "global.ini"
#define SWISS_RECENTLIST_FILENAME "recent.ini"
#define SWISS_BASE_DIR "swiss"
#define SWISS_SETTINGS_DIR "swiss/settings"
#define SWISS_GAME_SETTINGS_DIR "swiss/settings/game"

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
	//print_debug("Save device is %s\n", devices[DEVICE_CONFIG]->deviceName);
	deviceHandler_setStatEnabled(0);
	// If we're not using this device already, init it.
	if(devices[DEVICE_CONFIG] != devices[DEVICE_CUR]) {
		print_debug("Save device is not current, current is (%s)\n", devices[DEVICE_CUR] == NULL ? "NULL":devices[DEVICE_CUR]->deviceName);
		if(devices[DEVICE_CONFIG]->init(devices[DEVICE_CONFIG]->initial)) {
			print_debug("Save device failed to init\n");
			deviceHandler_setStatEnabled(1);
			return false;
		}
	}
	deviceHandler_setStatEnabled(1);
	return true;
}

void config_unset_device() {
	if(devices[DEVICE_CONFIG] != devices[DEVICE_CUR]) {
		devices[DEVICE_CONFIG]->deinit(devices[DEVICE_CONFIG]->initial);
	}
}

// Reads from a file and returns a populated buffer, NULL if anything goes wrong.
char* config_file_read(char* filename) {
	char* readBuffer = NULL;
	file_handle *configFile = (file_handle*)calloc(1, sizeof(file_handle));
	concat_path(configFile->name, devices[DEVICE_CONFIG]->initial->name, filename);
	print_debug("config_file_read: looking for %s\n", configFile->name);
	if(!devices[DEVICE_CONFIG]->statFile(configFile)) {
		readBuffer = (char*)calloc(1, configFile->size + 1);
		if (readBuffer) {
			print_debug("config_file_read: reading %i byte file\n", configFile->size);
			devices[DEVICE_CONFIG]->readFile(configFile, readBuffer, configFile->size);
			devices[DEVICE_CONFIG]->closeFile(configFile);
		}
	}
	free(configFile);
	return readBuffer;
}

int config_file_write(char* filename, char* contents) {
	file_handle *configFile = (file_handle*)calloc(1, sizeof(file_handle));
	concat_path(configFile->name, devices[DEVICE_CONFIG]->initial->name, filename);

	u32 len = strlen(contents);
	print_debug("config_file_write: writing %i bytes to %s\n", len, configFile->name);
	devices[DEVICE_CONFIG]->deleteFile(configFile);
	if(devices[DEVICE_CONFIG]->writeFile(configFile, contents, len) == len &&
		!devices[DEVICE_CONFIG]->closeFile(configFile)) {
		free(configFile);
		return 1;
	}
	devices[DEVICE_CONFIG]->closeFile(configFile);
	free(configFile);
	return 0;
}

void config_file_delete(char* filename) {
	file_handle *configFile = (file_handle*)calloc(1, sizeof(file_handle));
	concat_path(configFile->name, devices[DEVICE_CONFIG]->initial->name, filename);
	print_debug("config_file_delete: deleting %s\n", configFile->name);
	devices[DEVICE_CONFIG]->deleteFile(configFile);
	free(configFile);
}

int config_update_global(bool checkConfigDevice) {
	if(checkConfigDevice && !config_set_device()) return 0;

	char *configString = NULL;
	size_t len = 0;
	FILE *fp = open_memstream(&configString, &len);
	if(!fp) return 0;

	// Write out Swiss settings
	fprintf(fp, "# Swiss Configuration File!\r\n# Anything written in here will be lost!\r\n\r\n#!!Swiss Settings Start!!\r\n");
	fprintf(fp, "SD/IDE Speed=%s\r\n", swissSettings.exiSpeed ? "32MHz":"16MHz");
	fprintf(fp, "System Boot Mode=%s\r\n", swissSettings.sramBoot ? "Production":"Default");
	fprintf(fp, "System Sound=%s\r\n", swissSettings.sramStereo ? "Stereo":"Mono");
	fprintf(fp, "Screen Position=%+hi\r\n", swissSettings.sramHOffset);
	fprintf(fp, "System Language=%s\r\n", sramLang[swissSettings.sramLanguage]);
	fprintf(fp, "Swiss Video Mode=%s\r\n", uiVModeStr[swissSettings.uiVMode]);
	fprintf(fp, "Enable USB Gecko=%s\r\n", enableUSBGeckoStr[swissSettings.enableUSBGecko]);
	fprintf(fp, "Wait for USB Gecko=%s\r\n", swissSettings.waitForUSBGecko ? "Yes":"No");
	fprintf(fp, "Hide Unknown file types=%s\r\n", swissSettings.hideUnknownFileTypes ? "Yes":"No");
	fprintf(fp, "Init DVD Drive at startup=%s\r\n", swissSettings.initDVDDriveAtStart ? "Yes":"No");
	fprintf(fp, "Stop DVD Drive motor=%s\r\n", swissSettings.stopMotor ? "Yes":"No");
	fprintf(fp, "Configure Audio Buffer=%s\r\n", configAudioBufferStr[swissSettings.configAudioBuffer]);
	fprintf(fp, "Enable WiiRD debug=%s\r\n", swissSettings.wiirdDebug ? "Yes":"No");
	fprintf(fp, "Enable File Management=%s\r\n", swissSettings.enableFileManagement ? "Yes":"No");
	fprintf(fp, "Disable MemCard PRO GameID=%s\r\n", disableMCPGameIDStr[swissSettings.disableMCPGameID]);
	fprintf(fp, "Disable Video Patches=%s\r\n", disableVideoPatchesStr[swissSettings.disableVideoPatches]);
	fprintf(fp, "Force Video Active=%s\r\n", swissSettings.forceVideoActive ? "Yes":"No");
	fprintf(fp, "Force DTV Status=%s\r\n", swissSettings.forceDTVStatus ? "Yes":"No");
	fprintf(fp, "Last DTV Status=%s\r\n", getRawDTVStatus() ? "Yes":"No");
	fprintf(fp, "Pause for resolution change=%s\r\n", swissSettings.pauseAVOutput ? "Yes":"No");
	fprintf(fp, "AutoBoot=%s\r\n", swissSettings.autoBoot ? "Yes":"No");
	fprintf(fp, "AutoCheats=%s\r\n", swissSettings.autoCheats ? "Yes":"No");
	fprintf(fp, "InitNetwork=%s\r\n", swissSettings.initNetworkAtStart ? "Yes":"No");
	fprintf(fp, "IGRType=%s\r\n", igrTypeStr[swissSettings.igrType]);
	fprintf(fp, "AVECompat=%s\r\n", aveCompatStr[swissSettings.aveCompat]);
	fprintf(fp, "FileBrowserType=%s\r\n", fileBrowserStr[swissSettings.fileBrowserType]);
	fprintf(fp, "BS2Boot=%s\r\n", bs2BootStr[swissSettings.bs2Boot]);
	fprintf(fp, "RT4KHostIP=%s\r\n", swissSettings.rt4kHostIp);
	fprintf(fp, "RT4KPort=%hu\r\n", swissSettings.rt4kPort);
	fprintf(fp, "RT4KOptim=%s\r\n", swissSettings.rt4kOptim ? "Yes":"No");
	fprintf(fp, "SMBUserName=%s\r\n", swissSettings.smbUser);
	fprintf(fp, "SMBPassword=%s\r\n", swissSettings.smbPassword);
	fprintf(fp, "SMBShareName=%s\r\n", swissSettings.smbShare);
	fprintf(fp, "SMBHostIP=%s\r\n", swissSettings.smbServerIp);
	fprintf(fp, "FTPUserName=%s\r\n", swissSettings.ftpUserName);
	fprintf(fp, "FTPPassword=%s\r\n", swissSettings.ftpPassword);
	fprintf(fp, "FTPHostIP=%s\r\n", swissSettings.ftpHostIp);
	fprintf(fp, "FTPPort=%hu\r\n", swissSettings.ftpPort);
	fprintf(fp, "FTPUsePasv=%s\r\n", swissSettings.ftpUsePasv ? "Yes":"No");
	fprintf(fp, "FSPHostIP=%s\r\n", swissSettings.fspHostIp);
	fprintf(fp, "FSPPort=%hu\r\n", swissSettings.fspPort);
	fprintf(fp, "FSPPassword=%s\r\n", swissSettings.fspPassword);
	fprintf(fp, "FSPPathMTU=%hu\r\n", swissSettings.fspPathMtu);
	fprintf(fp, "BBALocalIP=%s\r\n", swissSettings.bbaLocalIp);
	fprintf(fp, "BBANetmask=%hu\r\n", swissSettings.bbaNetmask);
	fprintf(fp, "BBAGateway=%s\r\n", swissSettings.bbaGateway);
	fprintf(fp, "BBAUseDHCP=%s\r\n", swissSettings.bbaUseDhcp ? "Yes":"No");
	fprintf(fp, "ShowHiddenFiles=%s\r\n", swissSettings.showHiddenFiles ? "Yes":"No");
	fprintf(fp, "RecentListLevel=%s\r\n", recentListLevelStr[swissSettings.recentListLevel]);
	fprintf(fp, "GCLoaderHWVersion=%i\r\n", swissSettings.gcloaderHwVersion);
	fprintf(fp, "GCLoaderTopVersion=%s\r\n", swissSettings.gcloaderTopVersion);
	fprintf(fp, "Autoload=%s\r\n", swissSettings.autoload);
	fprintf(fp, "FlattenDir=%s\r\n", swissSettings.flattenDir);

	// Write out the default game config portion too
	fprintf(fp, "Force NTSC Video Mode=%s\r\n", gameVModeStr[swissSettings.gameVModeNtsc]);
	fprintf(fp, "Force PAL Video Mode=%s\r\n", gameVModeStr[swissSettings.gameVModePal]);
	fprintf(fp, "Force Horizontal Scale=%s\r\n", forceHScaleStr[swissSettings.forceHScale]);
	fprintf(fp, "Force Vertical Offset=%+hi\r\n", swissSettings.forceVOffset);
	fprintf(fp, "Force Vertical Filter=%s\r\n", forceVFilterStr[swissSettings.forceVFilter]);
	fprintf(fp, "Force Field Rendering=%s\r\n", forceVJitterStr[swissSettings.forceVJitter]);
	fprintf(fp, "Fix Pixel Center=%s\r\n", fixPixelCenterStr[swissSettings.fixPixelCenter]);
	fprintf(fp, "Disable Alpha Dithering=%s\r\n", swissSettings.disableDithering ? "Yes":"No");
	fprintf(fp, "Force Anisotropic Filter=%s\r\n", swissSettings.forceAnisotropy ? "Yes":"No");
	fprintf(fp, "Force Widescreen=%s\r\n", forceWidescreenStr[swissSettings.forceWidescreen]);
	fprintf(fp, "Force Polling Rate=%s\r\n", forcePollRateStr[swissSettings.forcePollRate]);
	fprintf(fp, "Invert Camera Stick=%s\r\n", invertCStickStr[swissSettings.invertCStick]);
	fprintf(fp, "Swap Camera Stick=%s\r\n", swapCStickStr[swissSettings.swapCStick]);
	fprintf(fp, "Digital Trigger Level=%hhu\r\n", swissSettings.triggerLevel);
	fprintf(fp, "Emulate Audio Streaming=%s\r\n", emulateAudioStreamStr[swissSettings.emulateAudioStream]);
	fprintf(fp, "Emulate Read Speed=%s\r\n", emulateReadSpeedStr[swissSettings.emulateReadSpeed]);
	fprintf(fp, "Emulate Broadband Adapter=%s\r\n", swissSettings.emulateEthernet ? "Yes":"No");
	fprintf(fp, "Emulate Memory Card=%s\r\n", swissSettings.emulateMemoryCard ? "Yes":"No");
	fprintf(fp, "Disable Memory Card=%s\r\n", disableMemoryCardStr[swissSettings.disableMemoryCard]);
	fprintf(fp, "Disable Hypervisor=%s\r\n", swissSettings.disableHypervisor ? "Yes":"No");
	fprintf(fp, "Prefer Clean Boot=%s\r\n", swissSettings.preferCleanBoot ? "Yes":"No");
	fprintf(fp, "RetroTINK-4K Profile=%i\r\n", swissSettings.rt4kProfile);
	fprintf(fp, "#!!Swiss Settings End!!\r\n\r\n");
	fclose(fp);

	ensure_path(DEVICE_CONFIG, SWISS_BASE_DIR, NULL, true);
	ensure_path(DEVICE_CONFIG, SWISS_SETTINGS_DIR, NULL, false);
	
	concat_path(txtbuffer, SWISS_SETTINGS_DIR, SWISS_SETTINGS_FILENAME);
	int res = config_file_write(txtbuffer, configString);
	free(configString);
	if(checkConfigDevice) {
		config_unset_device();
	}
	return res;
}

int config_update_recent(bool checkConfigDevice) {
	if(checkConfigDevice && !config_set_device()) return 0;

	char *configString = NULL;
	size_t len = 0;
	FILE *fp = open_memstream(&configString, &len);
	if(!fp) return 0;

	fprintf(fp, "# Recent list. Created by Swiss\r\n");
	int i;
	for(i = 0; i < RECENT_MAX; i++) {
		fprintf(fp, "Recent_%i=%s\r\n", i, swissSettings.recent[i]);
		//print_debug("added recent num %i [%s]\n", i, swissSettings.recent[i]);
	}
	fclose(fp);

	ensure_path(DEVICE_CONFIG, SWISS_BASE_DIR, NULL, true);
	ensure_path(DEVICE_CONFIG, SWISS_SETTINGS_DIR, NULL, false);
	
	concat_path(txtbuffer, SWISS_SETTINGS_DIR, SWISS_RECENTLIST_FILENAME);
	int res = config_file_write(txtbuffer, configString);
	free(configString);
	if(checkConfigDevice) {
		config_unset_device();
	}
	return res;
}


int config_update_game(ConfigEntry *entry, ConfigEntry *defaults, bool checkConfigDevice) {
	if(checkConfigDevice && !config_set_device()) return 0;

	char *configString = NULL;
	size_t len = 0;
	FILE *fp = open_memstream(&configString, &len);
	if(!fp) return 0;

	fprintf(fp, "# Game specific configuration file. Created by Swiss\r\n");
	fprintf(fp, "ID=%.4s\r\n", entry->game_id);
	fprintf(fp, "Name=%.64s\r\n", entry->game_name);
	fprintf(fp, "Comment=%.128s\r\n", entry->comment);
	fprintf(fp, "Status=%.32s\r\n", entry->status);
	if(entry->gameVMode != defaults->gameVMode) fprintf(fp, "Force Video Mode=%s\r\n", gameVModeStr[entry->gameVMode]);
	if(entry->forceHScale != defaults->forceHScale) fprintf(fp, "Force Horizontal Scale=%s\r\n", forceHScaleStr[entry->forceHScale]);
	if(entry->forceVOffset != defaults->forceVOffset) fprintf(fp, "Force Vertical Offset=%+hi\r\n", entry->forceVOffset);
	if(entry->forceVFilter != defaults->forceVFilter) fprintf(fp, "Force Vertical Filter=%s\r\n", forceVFilterStr[entry->forceVFilter]);
	if(entry->forceVJitter != defaults->forceVJitter) fprintf(fp, "Force Field Rendering=%s\r\n", forceVJitterStr[entry->forceVJitter]);
	if(entry->fixPixelCenter != defaults->fixPixelCenter) fprintf(fp, "Fix Pixel Center=%s\r\n", fixPixelCenterStr[entry->fixPixelCenter]);
	if(entry->disableDithering != defaults->disableDithering) fprintf(fp, "Disable Alpha Dithering=%s\r\n", entry->disableDithering ? "Yes":"No");
	if(entry->forceAnisotropy != defaults->forceAnisotropy) fprintf(fp, "Force Anisotropic Filter=%s\r\n", entry->forceAnisotropy ? "Yes":"No");
	if(entry->forceWidescreen != defaults->forceWidescreen) fprintf(fp, "Force Widescreen=%s\r\n", forceWidescreenStr[entry->forceWidescreen]);
	if(entry->forcePollRate != defaults->forcePollRate) fprintf(fp, "Force Polling Rate=%s\r\n", forcePollRateStr[entry->forcePollRate]);
	if(entry->invertCStick != defaults->invertCStick) fprintf(fp, "Invert Camera Stick=%s\r\n", invertCStickStr[entry->invertCStick]);
	if(entry->swapCStick != defaults->swapCStick) fprintf(fp, "Swap Camera Stick=%s\r\n", swapCStickStr[entry->swapCStick]);
	if(entry->triggerLevel != defaults->triggerLevel) fprintf(fp, "Digital Trigger Level=%hhu\r\n", entry->triggerLevel);
	if(entry->emulateAudioStream != defaults->emulateAudioStream) fprintf(fp, "Emulate Audio Streaming=%s\r\n", emulateAudioStreamStr[entry->emulateAudioStream]);
	if(entry->emulateReadSpeed != defaults->emulateReadSpeed) fprintf(fp, "Emulate Read Speed=%s\r\n", emulateReadSpeedStr[entry->emulateReadSpeed]);
	if(entry->emulateEthernet != defaults->emulateEthernet) fprintf(fp, "Emulate Broadband Adapter=%s\r\n", entry->emulateEthernet ? "Yes":"No");
	if(entry->disableMemoryCard != defaults->disableMemoryCard) fprintf(fp, "Disable Memory Card=%s\r\n", disableMemoryCardStr[entry->disableMemoryCard]);
	if(entry->disableHypervisor != defaults->disableHypervisor) fprintf(fp, "Disable Hypervisor=%s\r\n", entry->disableHypervisor ? "Yes":"No");
	if(entry->preferCleanBoot != defaults->preferCleanBoot) fprintf(fp, "Prefer Clean Boot=%s\r\n", entry->preferCleanBoot ? "Yes":"No");
	if(entry->rt4kProfile != defaults->rt4kProfile) fprintf(fp, "RetroTINK-4K Profile=%i\r\n", entry->rt4kProfile);
	fclose(fp);

	ensure_path(DEVICE_CONFIG, SWISS_BASE_DIR, NULL, true);
	ensure_path(DEVICE_CONFIG, SWISS_SETTINGS_DIR, NULL, false);
	ensure_path(DEVICE_CONFIG, SWISS_GAME_SETTINGS_DIR, NULL, false);
	
	concatf_path(txtbuffer, SWISS_GAME_SETTINGS_DIR, "%.4s.ini", entry->game_id);
	int res = config_file_write(txtbuffer, configString);
	free(configString);
	if(checkConfigDevice) {
		config_unset_device();
	}
	return res;
}

static char fixPixelCenterEntries[][4] = {"00\0E", "DNDD", "G2BE", "G2BP", "GD7E", "GD7P", "GEME", "GEMJ", "GEMP", "GNBE", "GNBJ", "GNBP", "GZBJ"};
static char triggerLevelEntries[][4] = {"GKGE", "GKGJ", "GKGP", "GY2E", "GY2J", "GY2P", "GY3E", "GY3J", "GYBE", "GYBJ", "GYBP"};
static char emulateAudioStreamEntries[][4] = {"UFZE", "UFZJ", "UFZP"};
static char emulateReadSpeedEntries[][4] = {"DRSE", "GQSD", "GQSE", "GQSF", "GQSI", "GQSP", "GQSS", "GRSE", "GRSJ", "GRSP", "GTOJ"};
static char emulateEthernetEntries[][4] = {"DPSJ", "GHEE", "GHEJ", "GKYE", "GKYJ", "GKYP", "GM4E", "GM4J", "GM4P", "GPJJ", "GPOE", "GPOJ", "GPOP", "GPSE", "GPSJ", "GPSP", "GTEE", "GTEJ", "GTEP", "GTEW", "PHEJ"};

void config_defaults(ConfigEntry *entry) {
	strcpy(entry->comment, "No Comment");
	strcpy(entry->status, "Unknown");
	entry->gameVMode = entry->region == 'P' ? swissSettings.gameVModePal : swissSettings.gameVModeNtsc;
	entry->forceHScale = swissSettings.forceHScale;
	entry->forceVOffset = swissSettings.forceVOffset;
	entry->forceVOffset = in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) ? -3:0;
	entry->forceVFilter = swissSettings.forceVFilter;
	entry->forceVJitter = swissSettings.forceVJitter;
	entry->fixPixelCenter = swissSettings.fixPixelCenter;
	entry->disableDithering = swissSettings.disableDithering;
	entry->forceAnisotropy = swissSettings.forceAnisotropy;
	entry->forceWidescreen = swissSettings.forceWidescreen;
	entry->forcePollRate = swissSettings.forcePollRate;
	entry->invertCStick = swissSettings.invertCStick;
	entry->swapCStick = swissSettings.swapCStick;
	entry->triggerLevel = swissSettings.triggerLevel;
	entry->emulateAudioStream = swissSettings.emulateAudioStream;
	entry->emulateReadSpeed = swissSettings.emulateReadSpeed;
	entry->emulateEthernet = swissSettings.emulateEthernet;
	entry->disableMemoryCard = swissSettings.disableMemoryCard;
	entry->disableHypervisor = swissSettings.disableHypervisor;
	entry->preferCleanBoot = swissSettings.preferCleanBoot;
	entry->rt4kProfile = swissSettings.rt4kProfile;

	for(int i = 0; i < sizeof(fixPixelCenterEntries) / sizeof(*fixPixelCenterEntries); i++) {
		if(!strncmp(entry->game_id, fixPixelCenterEntries[i], 4)) {
			entry->fixPixelCenter = 1;
			break;
		}
	}
	for(int i = 0; i < sizeof(triggerLevelEntries) / sizeof(*triggerLevelEntries); i++) {
		if(!strncmp(entry->game_id, triggerLevelEntries[i], 4)) {
			entry->triggerLevel = 0;
			break;
		}
	}
	for(int i = 0; i < sizeof(emulateAudioStreamEntries) / sizeof(*emulateAudioStreamEntries); i++) {
		if(!strncmp(entry->game_id, emulateAudioStreamEntries[i], 4)) {
			entry->emulateAudioStream = 0;
			break;
		}
	}
	for(int i = 0; i < sizeof(emulateReadSpeedEntries) / sizeof(*emulateReadSpeedEntries); i++) {
		if(!strncmp(entry->game_id, emulateReadSpeedEntries[i], 4)) {
			entry->emulateReadSpeed = 1;
			break;
		}
	}
	for(int i = 0; i < sizeof(emulateEthernetEntries) / sizeof(*emulateEthernetEntries); i++) {
		if(!strncmp(entry->game_id, emulateEthernetEntries[i], 4)) {
			entry->emulateEthernet = 1;
			break;
		}
	}
}

// TODO kill this off in one major version from now. Don't add new settings to it.
void config_parse_legacy(char *configData, void (*progress_indicator)(char*, int, int)) {
	ConfigEntry configEntries[2048]; // That's a lot of Games!
	int configEntriesCount = 0;
	// Parse each entry and put it into our array
	char *line, *linectx = NULL;
	int first = 1;
	bool defaultPassed = false;
	line = strtok_r( configData, "\r\n", &linectx );
	while( line != NULL ) {
		//print_debug("Line [%s]\n", line);
		if(line[0] != '#') {
			// Is this line a new game entry?
			char *name, *value = NULL;
			name = strtok_r(line, "=", &value);
			
			if(value != NULL) {
				//print_debug("Name [%s] Value [%s]\n", name, value);

				if(!strcmp("ID", name)) {
					defaultPassed = true;
					if(!first) {
						configEntriesCount++;
					}
					strncpy(configEntries[configEntriesCount].game_id, value, 4);
					first = 0;
					// Fill this entry with defaults incase some values are missing..
					config_defaults(&configEntries[configEntriesCount]);
				}
				else if(!strcmp("Name", name)) {
					strncpy(configEntries[configEntriesCount].game_name, value, 64);
				}
				else if(!strcmp("Comment", name)) {
					strncpy(configEntries[configEntriesCount].comment, value, 128);
				}
				else if(!strcmp("Status", name)) {
					strncpy(configEntries[configEntriesCount].status, value, 32);
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
					for(int i = 0; i < 9; i++) {
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
						configEntries[configEntriesCount].disableDithering = !strcmp("Yes", value);
					else
						swissSettings.disableDithering = !strcmp("Yes", value);
				}
				else if(!strcmp("Force Anisotropic Filter", name)) {
					if(defaultPassed)
						configEntries[configEntriesCount].forceAnisotropy = !strcmp("Yes", value);
					else
						swissSettings.forceAnisotropy = !strcmp("Yes", value);
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
					swissSettings.emulateMemoryCard = !strcmp("Yes", value);
				}
				
				// Swiss settings
				else if(!strcmp("SD/IDE Speed", name)) {
					swissSettings.exiSpeed = !strcmp("32MHz", value);
				}
				else if(!strcmp("Swiss Video Mode", name)) {
					for(int i = 0; i < 7; i++) {
						if(!strcmp(uiVModeStr[i], value)) {
							swissSettings.uiVMode = i;
							break;
						}
					}
				}
				else if(!strcmp("Enable Debug", name)) {
					swissSettings.enableUSBGecko = !strcmp("Yes", value) ? USBGECKO_MEMCARD_SLOT_B : USBGECKO_OFF;
				}
				else if(!strcmp("Hide Unknown file types", name)) {
					swissSettings.hideUnknownFileTypes = !strcmp("Yes", value);
				}
				else if(!strcmp("Stop DVD Motor on startup", name)) {
					swissSettings.stopMotor = !strcmp("Yes", value);
				}
				else if(!strcmp("Enable WiiRD debug", name)) {
					swissSettings.wiirdDebug = !strcmp("Yes", value);
				}
				else if(!strcmp("Enable File Management", name)) {
					swissSettings.enableFileManagement = !strcmp("Yes", value);
				}
				else if(!strcmp("Disable Video Patches", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(disableVideoPatchesStr[i], value)) {
							swissSettings.disableVideoPatches = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Video Active", name)) {
					swissSettings.forceVideoActive = !strcmp("Yes", value);
				}
				else if(!strcmp("Force DTV Status", name)) {
					swissSettings.forceDTVStatus = !strcmp("Yes", value);
				}
				else if(!strcmp("SMBUserName", name)) {
					strlcpy(swissSettings.smbUser, value, sizeof(swissSettings.smbUser));
				}
				else if(!strcmp("SMBPassword", name)) {
					strlcpy(swissSettings.smbPassword, value, sizeof(swissSettings.smbPassword));
				}
				else if(!strcmp("SMBShareName", name)) {
					strlcpy(swissSettings.smbShare, value, sizeof(swissSettings.smbShare));
				}
				else if(!strcmp("SMBHostIP", name)) {
					strlcpy(swissSettings.smbServerIp, value, sizeof(swissSettings.smbServerIp));
				}
				else if(!strcmp("AutoCheats", name)) {
					swissSettings.autoCheats = !strcmp("Yes", value);
				}
				else if(!strcmp("InitNetwork", name)) {
					swissSettings.initNetworkAtStart = !strcmp("Yes", value);
				}
				else if(!strcmp("IGRType", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(igrTypeStr[i], value)) {
							swissSettings.igrType = i;
							break;
						}
					}
				}
				else if(!strcmp("AVECompat", name)) {
					for(int i = 0; i < AVE_COMPAT_MAX; i++) {
						if(!strcmp(aveCompatStr[i], value)) {
							setenv("AVE", aveCompatStr[i], 1);
							swissSettings.aveCompat = i;
							break;
						}
					}
				}
				else if(!strcmp("FileBrowserType", name)) {
					for(int i = 0; i < BROWSER_MAX; i++) {
						if(!strcmp(fileBrowserStr[i], value)) {
							swissSettings.fileBrowserType = i;
							break;
						}
					}
				}
				else if(!strcmp("BS2Boot", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(bs2BootStr[i], value)) {
							swissSettings.bs2Boot = i;
							break;
						}
					}
				}
				else if(!strcmp("FTPUserName", name)) {
					strlcpy(swissSettings.ftpUserName, value, sizeof(swissSettings.ftpUserName));
				}
				else if(!strcmp("FTPPassword", name)) {
					strlcpy(swissSettings.ftpPassword, value, sizeof(swissSettings.ftpPassword));
				}
				else if(!strcmp("FTPHostIP", name)) {
					strlcpy(swissSettings.ftpHostIp, value, sizeof(swissSettings.ftpHostIp));
				}
				else if(!strcmp("FTPPort", name)) {
					swissSettings.ftpPort = atoi(value);
				}
				else if(!strcmp("FTPUsePasv", name)) {
					swissSettings.ftpUsePasv = !strcmp("Yes", value);
				}
				else if(!strcmp("FSPHostIP", name)) {
					strlcpy(swissSettings.fspHostIp, value, sizeof(swissSettings.fspHostIp));
				}
				else if(!strcmp("FSPPort", name)) {
					swissSettings.fspPort = atoi(value);
				}
				else if(!strcmp("FSPPassword", name)) {
					strlcpy(swissSettings.fspPassword, value, sizeof(swissSettings.fspPassword));
				}
				else if(!strcmp("ShowHiddenFiles", name)) {
					swissSettings.showHiddenFiles = !strcmp("Yes", value);
				}
				else if(!strcmp("RecentListLevel", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(recentListLevelStr[i], value)) {
							swissSettings.recentListLevel = i;
							break;
						}
					}
				}
				else if(!strcmp("GCLoaderTopVersion", name)) {
					strlcpy(swissSettings.gcloaderTopVersion, value, sizeof(swissSettings.gcloaderTopVersion));
				}
				else if(!strcmp("Autoload", name)) {
					strlcpy(swissSettings.autoload, value, sizeof(swissSettings.autoload));
				}
				else if(!strncmp("Recent_", name, strlen("Recent_"))) {
					int recent_slot = atoi(name+strlen("Recent_"));
					if(recent_slot >= 0 && recent_slot < RECENT_MAX) {
						//print_debug("found recent num %i [%s]\n", recent_slot, value);
						strlcpy(swissSettings.recent[recent_slot], value, PATHNAME_MAX);
					}
				}
			}
		}
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}

	if(configEntriesCount > 0 || !first) {
		configEntriesCount++;
		config_defaults(&configEntries[configEntriesCount]);
	}
	 print_debug("Found %i entries in the (legacy) config file\n",configEntriesCount);
	 
	 // Write out to individual files.
	 int i;
	 for(i = 0; i < configEntriesCount; i++) {
		 progress_indicator("Migrating settings to new format.", 1, (int)(((float)i / (float)configEntriesCount) * 100));
		 config_update_game(&configEntries[i], &configEntries[configEntriesCount], false);
	 }
	 // Write out a new swiss.ini
	 config_update_global(false);
	 // Write out new recent.ini
	 config_update_recent(false);
	 // Kill off the old swiss.ini
	 config_file_delete(SWISS_SETTINGS_FILENAME_LEGACY);
}

void config_parse_global(char *configData) {
	char *line, *linectx = NULL;
	line = strtok_r( configData, "\r\n", &linectx );
	while( line != NULL ) {
		//print_debug("Line [%s]\n", line);
		if(line[0] != '#') {
			char *name, *value = NULL;
			name = strtok_r(line, "=", &value);
			
			if(value != NULL) {
				//print_debug("Name [%s] Value [%s]\n", name, value);

				if(!strcmp("Force NTSC Video Mode", name)) {
					for(int i = 0; i < 8; i++) {
						if(!strcmp(gameVModeStr[i], value)) {
							swissSettings.gameVModeNtsc = i;
							break;
						}
					}
				}
				else if(!strcmp("Force PAL Video Mode", name)) {
					for(int i = 0; i < 15; i++) {
						if(!strcmp(gameVModeStr[i], value)) {
							swissSettings.gameVModePal = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Horizontal Scale", name)) {
					for(int i = 0; i < 9; i++) {
						if(!strcmp(forceHScaleStr[i], value)) {
							swissSettings.forceHScale = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Vertical Offset", name)) {
					swissSettings.forceVOffset = atoi(value);
				}
				else if(!strcmp("Force Vertical Filter", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(forceVFilterStr[i], value)) {
							swissSettings.forceVFilter = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Field Rendering", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(forceVJitterStr[i], value)) {
							swissSettings.forceVJitter = i;
							break;
						}
					}
				}
				else if(!strcmp("Fix Pixel Center", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(fixPixelCenterStr[i], value)) {
							swissSettings.fixPixelCenter = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Alpha Dithering", name)) {
					swissSettings.disableDithering = !strcmp("Yes", value);
				}
				else if(!strcmp("Force Anisotropic Filter", name)) {
					swissSettings.forceAnisotropy = !strcmp("Yes", value);
				}
				else if(!strcmp("Force Widescreen", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(forceWidescreenStr[i], value)) {
							swissSettings.forceWidescreen = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Polling Rate", name)) {
					for(int i = 0; i < 13; i++) {
						if(!strcmp(forcePollRateStr[i], value)) {
							swissSettings.forcePollRate = i;
							break;
						}
					}
				}
				else if(!strcmp("Invert Camera Stick", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(invertCStickStr[i], value)) {
							swissSettings.invertCStick = i;
							break;
						}
					}
				}
				else if(!strcmp("Swap Camera Stick", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(swapCStickStr[i], value)) {
							swissSettings.swapCStick = i;
							break;
						}
					}
				}
				else if(!strcmp("Digital Trigger Level", name)) {
					swissSettings.triggerLevel = atoi(value);
				}
				else if(!strcmp("Emulate Audio Streaming", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(emulateAudioStreamStr[i], value)) {
							swissSettings.emulateAudioStream = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Read Speed", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(emulateReadSpeedStr[i], value)) {
							swissSettings.emulateReadSpeed = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Broadband Adapter", name)) {
					swissSettings.emulateEthernet = !strcmp("Yes", value);
				}
				else if(!strcmp("Emulate Memory Card", name)) {
					swissSettings.emulateMemoryCard = !strcmp("Yes", value);
				}
				else if(!strcmp("Disable Memory Card", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(disableMemoryCardStr[i], value)) {
							swissSettings.disableMemoryCard = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Hypervisor", name)) {
					swissSettings.disableHypervisor = !strcmp("Yes", value);
				}
				else if(!strcmp("Prefer Clean Boot", name)) {
					swissSettings.preferCleanBoot = !strcmp("Yes", value);
				}
				else if(!strcmp("RetroTINK-4K Profile", name)) {
					swissSettings.rt4kProfile = atoi(value);
				}
				
				// Swiss settings
				else if(!strcmp("SD/IDE Speed", name)) {
					swissSettings.exiSpeed = !strcmp("32MHz", value);
				}
				else if(!strcmp("System Boot Mode", name)) {
					swissSettings.sramBoot = !strcmp("Production", value) ? SYS_BOOT_PRODUCTION : SYS_BOOT_DEVELOPMENT;
				}
				else if(!strcmp("System Sound", name)) {
					swissSettings.sramStereo = !strcmp("Stereo", value) ? SYS_SOUND_STEREO : SYS_SOUND_MONO;
				}
				else if(!strcmp("Screen Position", name)) {
					swissSettings.sramHOffset = atoi(value);
				}
				else if(!strcmp("System Language", name)) {
					for(int i = 0; i < SRAM_LANG_MAX; i++) {
						if(!strcmp(sramLang[i], value)) {
							swissSettings.sramLanguage = i;
							break;
						}
					}
				}
				else if(!strcmp("Swiss Video Mode", name)) {
					for(int i = 0; i < 7; i++) {
						if(!strcmp(uiVModeStr[i], value)) {
							swissSettings.uiVMode = i;
							break;
						}
					}
				}
				else if(!strcmp("Enable Debug", name)) {
					swissSettings.enableUSBGecko = !strcmp("Yes", value) ? USBGECKO_MEMCARD_SLOT_B : USBGECKO_OFF;
				}
				else if(!strcmp("Enable USB Gecko", name) || !strcmp("USB Gecko debug output", name)) {
					for(int i = 0; i < USBGECKO_MAX; i++) {
						if(!strcmp(enableUSBGeckoStr[i], value)) {
							swissSettings.enableUSBGecko = i;
							break;
						}
					}
				}
				else if(!strcmp("Wait for USB Gecko", name)) {
					swissSettings.waitForUSBGecko = !strcmp("Yes", value);
				}
				else if(!strcmp("Hide Unknown file types", name)) {
					swissSettings.hideUnknownFileTypes = !strcmp("Yes", value);
				}
				else if(!strcmp("Init DVD Drive at startup", name)) {
					swissSettings.initDVDDriveAtStart = !strcmp("Yes", value);
				}
				else if(!strcmp("Stop DVD Motor on startup", name) || !strcmp("Stop DVD Drive motor", name)) {
					swissSettings.stopMotor = !strcmp("Yes", value);
				}
				else if(!strcmp("Configure Audio Buffer", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(configAudioBufferStr[i], value)) {
							swissSettings.configAudioBuffer = i;
							break;
						}
					}
				}
				else if(!strcmp("Enable WiiRD debug", name)) {
					swissSettings.wiirdDebug = !strcmp("Yes", value);
				}
				else if(!strcmp("Enable File Management", name)) {
					swissSettings.enableFileManagement = !strcmp("Yes", value);
				}
				else if(!strcmp("Disable MemCard PRO GameID", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(disableMCPGameIDStr[i], value)) {
							swissSettings.disableMCPGameID = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Video Patches", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(disableVideoPatchesStr[i], value)) {
							swissSettings.disableVideoPatches = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Video Active", name)) {
					swissSettings.forceVideoActive = !strcmp("Yes", value);
				}
				else if(!strcmp("Force DTV Status", name)) {
					swissSettings.forceDTVStatus = !strcmp("Yes", value);
				}
				else if(!strcmp("Last DTV Status", name)) {
					swissSettings.lastDTVStatus = !strcmp("Yes", value);
				}
				else if(!strcmp("Pause for resolution change", name)) {
					swissSettings.pauseAVOutput = !strcmp("Yes", value);
				}
				else if(!strcmp("AutoBoot", name)) {
					swissSettings.autoBoot = !strcmp("Yes", value);
				}
				else if(!strcmp("AutoCheats", name)) {
					swissSettings.autoCheats = !strcmp("Yes", value);
				}
				else if(!strcmp("InitNetwork", name)) {
					swissSettings.initNetworkAtStart = !strcmp("Yes", value);
				}
				else if(!strcmp("IGRType", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(igrTypeStr[i], value)) {
							swissSettings.igrType = i;
							break;
						}
					}
				}
				else if(!strcmp("AVECompat", name)) {
					for(int i = 0; i < AVE_COMPAT_MAX; i++) {
						if(!strcmp(aveCompatStr[i], value)) {
							setenv("AVE", aveCompatStr[i], 1);
							swissSettings.aveCompat = i;
							break;
						}
					}
				}
				else if(!strcmp("FileBrowserType", name)) {
					for(int i = 0; i < BROWSER_MAX; i++) {
						if(!strcmp(fileBrowserStr[i], value)) {
							swissSettings.fileBrowserType = i;
							break;
						}
					}
				}
				else if(!strcmp("BS2Boot", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(bs2BootStr[i], value)) {
							swissSettings.bs2Boot = i;
							break;
						}
					}
				}
				else if(!strcmp("RT4KHostIP", name)) {
					strlcpy(swissSettings.rt4kHostIp, value, sizeof(swissSettings.rt4kHostIp));
				}
				else if(!strcmp("RT4KPort", name)) {
					swissSettings.rt4kPort = atoi(value);
				}
				else if(!strcmp("RT4KOptim", name)) {
					swissSettings.rt4kOptim = !strcmp("Yes", value);
				}
				else if(!strcmp("SMBUserName", name)) {
					strlcpy(swissSettings.smbUser, value, sizeof(swissSettings.smbUser));
				}
				else if(!strcmp("SMBPassword", name)) {
					strlcpy(swissSettings.smbPassword, value, sizeof(swissSettings.smbPassword));
				}
				else if(!strcmp("SMBShareName", name)) {
					strlcpy(swissSettings.smbShare, value, sizeof(swissSettings.smbShare));
				}
				else if(!strcmp("SMBHostIP", name)) {
					strlcpy(swissSettings.smbServerIp, value, sizeof(swissSettings.smbServerIp));
				}
				else if(!strcmp("FTPUserName", name)) {
					strlcpy(swissSettings.ftpUserName, value, sizeof(swissSettings.ftpUserName));
				}
				else if(!strcmp("FTPPassword", name)) {
					strlcpy(swissSettings.ftpPassword, value, sizeof(swissSettings.ftpPassword));
				}
				else if(!strcmp("FTPHostIP", name)) {
					strlcpy(swissSettings.ftpHostIp, value, sizeof(swissSettings.ftpHostIp));
				}
				else if(!strcmp("FTPPort", name)) {
					swissSettings.ftpPort = atoi(value);
				}
				else if(!strcmp("FTPUsePasv", name)) {
					swissSettings.ftpUsePasv = !strcmp("Yes", value);
				}
				else if(!strcmp("FSPHostIP", name)) {
					strlcpy(swissSettings.fspHostIp, value, sizeof(swissSettings.fspHostIp));
				}
				else if(!strcmp("FSPPort", name)) {
					swissSettings.fspPort = atoi(value);
				}
				else if(!strcmp("FSPPassword", name)) {
					strlcpy(swissSettings.fspPassword, value, sizeof(swissSettings.fspPassword));
				}
				else if(!strcmp("FSPPathMTU", name)) {
					swissSettings.fspPathMtu = atoi(value);
				}
				else if(!strcmp("BBALocalIP", name)) {
					strlcpy(swissSettings.bbaLocalIp, value, sizeof(swissSettings.bbaLocalIp));
				}
				else if(!strcmp("BBANetmask", name)) {
					swissSettings.bbaNetmask = atoi(value);
				}
				else if(!strcmp("BBAGateway", name)) {
					strlcpy(swissSettings.bbaGateway, value, sizeof(swissSettings.bbaGateway));
				}
				else if(!strcmp("BBAUseDHCP", name)) {
					swissSettings.bbaUseDhcp = !strcmp("Yes", value);
				}
				else if(!strcmp("ShowHiddenFiles", name)) {
					swissSettings.showHiddenFiles = !strcmp("Yes", value);
				}
				else if(!strcmp("RecentListLevel", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(recentListLevelStr[i], value)) {
							swissSettings.recentListLevel = i;
							break;
						}
					}
				}
				else if(!strcmp("GCLoaderHWVersion", name)) {
					swissSettings.gcloaderHwVersion = atoi(value);
				}
				else if(!strcmp("GCLoaderTopVersion", name)) {
					strlcpy(swissSettings.gcloaderTopVersion, value, sizeof(swissSettings.gcloaderTopVersion));
				}
				else if(!strcmp("Autoload", name)) {
					strlcpy(swissSettings.autoload, value, sizeof(swissSettings.autoload));
				}
				else if(!strcmp("FlattenDir", name)) {
					strlcpy(swissSettings.flattenDir, value, sizeof(swissSettings.flattenDir));
				}
			}
		}
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}
}

void config_parse_args(int argc, char *argv[]) {
	if(argc == 0 || argv == NULL) {
		return;
	}
	char *argz;
	size_t argz_len;
	argz_create(&argv[1], &argz, &argz_len);
	argz_stringify(argz, argz_len, '\n');
	if(argz != NULL) {
		config_parse_global(argz);
		free(argz);
	}
}

void config_parse_recent(char *configData) {
	char *line, *linectx = NULL;
	line = strtok_r( configData, "\r\n", &linectx );
	while( line != NULL ) {
		//print_debug("Line [%s]\n", line);
		if(line[0] != '#') {
			char *name, *value = NULL;
			name = strtok_r(line, "=", &value);
			
			if(value != NULL) {
				//print_debug("Name [%s] Value [%s]\n", name, value);
				if(!strncmp("Recent_", name, strlen("Recent_"))) {
					int recent_slot = atoi(name+strlen("Recent_"));
					if(recent_slot >= 0 && recent_slot < RECENT_MAX) {
						//print_debug("found recent num %i [%s]\n", recent_slot, value);
						strlcpy(swissSettings.recent[recent_slot], value, PATHNAME_MAX);
					}
				}
			}
		}
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}
}

void config_parse_game(char *configData, ConfigEntry *entry) {
	// Parse each entry and put it into our array
	char *line, *linectx = NULL;
	line = strtok_r( configData, "\r\n", &linectx );
	while( line != NULL ) {
		//print_debug("Line [%s]\n", line);
		if(line[0] != '#') {
			char *name, *value = NULL;
			name = strtok_r(line, "=", &value);
			
			if(value != NULL) {
				//print_debug("Name [%s] Value [%s]\n", name, value);
				if(!strcmp("Name", name)) {
					strncpy(entry->game_name, value, 64);
				}
				else if(!strcmp("Comment", name)) {
					strncpy(entry->comment, value, 128);
				}
				else if(!strcmp("Status", name)) {
					strncpy(entry->status, value, 32);
				}
				else if(!strcmp("Force Video Mode", name)) {
					for(int i = 0; i < 15; i++) {
						if(!strcmp(gameVModeStr[i], value)) {
							entry->gameVMode = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Horizontal Scale", name)) {
					for(int i = 0; i < 9; i++) {
						if(!strcmp(forceHScaleStr[i], value)) {
							entry->forceHScale = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Vertical Offset", name)) {
					entry->forceVOffset = atoi(value);
				}
				else if(!strcmp("Force Vertical Filter", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(forceVFilterStr[i], value)) {
							entry->forceVFilter = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Field Rendering", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(forceVJitterStr[i], value)) {
							entry->forceVJitter = i;
							break;
						}
					}
				}
				else if(!strcmp("Fix Pixel Center", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(fixPixelCenterStr[i], value)) {
							entry->fixPixelCenter = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Alpha Dithering", name)) {
					entry->disableDithering = !strcmp("Yes", value);
				}
				else if(!strcmp("Force Anisotropic Filter", name)) {
					entry->forceAnisotropy = !strcmp("Yes", value);
				}
				else if(!strcmp("Force Widescreen", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(forceWidescreenStr[i], value)) {
							entry->forceWidescreen = i;
							break;
						}
					}
				}
				else if(!strcmp("Force Polling Rate", name)) {
					for(int i = 0; i < 13; i++) {
						if(!strcmp(forcePollRateStr[i], value)) {
							entry->forcePollRate = i;
							break;
						}
					}
				}
				else if(!strcmp("Invert Camera Stick", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(invertCStickStr[i], value)) {
							entry->invertCStick = i;
							break;
						}
					}
				}
				else if(!strcmp("Swap Camera Stick", name)) {
					for(int i = 0; i < 4; i++) {
						if(!strcmp(swapCStickStr[i], value)) {
							entry->swapCStick = i;
							break;
						}
					}
				}
				else if(!strcmp("Digital Trigger Level", name)) {
					entry->triggerLevel = atoi(value);
				}
				else if(!strcmp("Emulate Audio Streaming", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(emulateAudioStreamStr[i], value)) {
							entry->emulateAudioStream = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Read Speed", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(emulateReadSpeedStr[i], value)) {
							entry->emulateReadSpeed = i;
							break;
						}
					}
				}
				else if(!strcmp("Emulate Broadband Adapter", name)) {
					entry->emulateEthernet = !strcmp("Yes", value);
				}
				else if(!strcmp("Disable Memory Card", name)) {
					for(int i = 0; i < 3; i++) {
						if(!strcmp(disableMemoryCardStr[i], value)) {
							entry->disableMemoryCard = i;
							break;
						}
					}
				}
				else if(!strcmp("Disable Hypervisor", name)) {
					entry->disableHypervisor = !strcmp("Yes", value);
				}
				else if(!strcmp("Prefer Clean Boot", name)) {
					entry->preferCleanBoot = !strcmp("Yes", value);
				}
				else if(!strcmp("RetroTINK-4K Profile", name)) {
					entry->rt4kProfile = atoi(value);
				}
			}
		}
		// And round we go again
		line = strtok_r( NULL, "\r\n", &linectx);
	}
}

void config_find(ConfigEntry *entry) {
	// Fill out defaults
	config_defaults(entry);
	
	if(!config_set_device()) {
		return;
	}
	print_debug("config_find: Looking for config file with ID %s\n",entry->game_id);
	// See if we have an actual config file for this game
	concatf_path(txtbuffer, SWISS_GAME_SETTINGS_DIR, "%.4s.ini", entry->game_id);
	char* configEntry = config_file_read(txtbuffer);
	if(configEntry) {
		config_parse_game(configEntry, entry);
		free(configEntry);
	}
	config_unset_device();
}

/** 
	Initialises the configuration file
	Returns 1 on successful file open, 0 otherwise
*/
int config_init(void (*progress_indicator)(char*, int, int)) {
	int res = 0;
	progress_indicator("Loading settings", 1, -2);
	if(!config_set_device()) {
		progress_indicator(NULL, 0, 0);
		return res;
	}
	
	// Make the new settings base dir(s) if we don't have them already
	ensure_path(DEVICE_CONFIG, SWISS_BASE_DIR, NULL, true);
	ensure_path(DEVICE_CONFIG, SWISS_SETTINGS_DIR, NULL, false);
	ensure_path(DEVICE_CONFIG, SWISS_GAME_SETTINGS_DIR, NULL, false);
	
	// Read config (legacy /swiss.ini format)
	char* configData = config_file_read(SWISS_SETTINGS_FILENAME_LEGACY);
	if(configData != NULL) {
		progress_indicator(NULL, 0, 0);
		progress_indicator("Migrating settings to new format.", 1, -1);
		config_parse_legacy(configData, progress_indicator);
		free(configData);
	}
	
	// Read config (new format)
	concat_path(txtbuffer, SWISS_SETTINGS_DIR, SWISS_SETTINGS_FILENAME);
	configData = config_file_read(txtbuffer);
	if(configData != NULL) {
		config_parse_global(configData);
		free(configData);
		res = 1;
	}
	
	// Read the recent list if enabled
	if(swissSettings.recentListLevel > 0) {
		concat_path(txtbuffer, SWISS_SETTINGS_DIR, SWISS_RECENTLIST_FILENAME);
		configData = config_file_read(txtbuffer);
		if(configData != NULL) {
			config_parse_recent(configData);
			free(configData);
		}
	}
	progress_indicator(NULL, 0, 0);
	config_unset_device();
	return res;
}

void config_init_environ() {
	char *value;
	value = getenv("AVE");
	if(value == NULL || *value == '\0') {
		setenv("AVE", aveCompatStr[swissSettings.aveCompat], 1);
	} else {
		for(int i = 0; i < AVE_COMPAT_MAX; i++) {
			if(!strcmp(aveCompatStr[i], value)) {
				swissSettings.aveCompat = i;
				break;
			}
		}
	}
	
	value = getenv("CUBEBOOT");
	if(value != NULL) {
		swissSettings.cubebootInvoked = !!atoi(value);
	}
	value = getenv("FLIPPYDRIVE");
	if(value != NULL) {
		swissSettings.hasFlippyDrive = !!atoi(value);
	}
}

SwissSettings backup;
static char gameVModePalEntries[][4] = {"DLSP", "G3FD", "G3FF", "G3FP", "G3FS", "GLRD", "GLRF", "GLRP", "GM8P", "GSWD", "GSWF", "GSWI", "GSWP", "GSWS"};

void config_load_current(ConfigEntry *entry) {
	wait_network();
	// load settings for this game to current settings
	memcpy(&backup, &swissSettings, sizeof(SwissSettings));
	swissSettings.gameVMode = entry->gameVMode;
	swissSettings.forceHScale = entry->forceHScale;
	swissSettings.forceVOffset = entry->forceVOffset;
	swissSettings.forceVFilter = entry->forceVFilter;
	swissSettings.forceVJitter = entry->forceVJitter;
	swissSettings.fixPixelCenter = entry->fixPixelCenter;
	swissSettings.disableDithering = entry->disableDithering;
	swissSettings.forceAnisotropy = entry->forceAnisotropy;
	swissSettings.forceWidescreen = entry->forceWidescreen;
	swissSettings.fontEncode = entry->region == 'J' ? SYS_FONTENC_SJIS : SYS_FONTENC_ANSI;
	swissSettings.forcePollRate = entry->forcePollRate;
	swissSettings.invertCStick = entry->invertCStick;
	swissSettings.swapCStick = entry->swapCStick;
	swissSettings.triggerLevel = entry->triggerLevel;
	swissSettings.emulateAudioStream = entry->emulateAudioStream;
	swissSettings.emulateReadSpeed = entry->emulateReadSpeed;
	swissSettings.emulateEthernet = entry->emulateEthernet;
	swissSettings.disableMemoryCard = entry->disableMemoryCard;
	swissSettings.disableHypervisor = entry->disableHypervisor;
	
	if(entry->region == 'P')
		swissSettings.sram60Hz = getTVFormat() != VI_PAL;
	else if(!strchr("A?", entry->region))
		swissSettings.sram60Hz = 0;
	
	if(!strchr("PA?", entry->region))
		swissSettings.sramLanguage = SYS_LANG_ENGLISH;
	
	if(entry->region == 'P')
		swissSettings.sramVideo = SYS_VIDEO_PAL;
	else if((swissSettings.sramVideo == SYS_VIDEO_PAL && !strchr("A?", entry->region)) ||
			(swissSettings.sramVideo == SYS_VIDEO_MPAL && getDTVStatus()) ||
			swissSettings.fontEncode == SYS_FONTENC_SJIS)
		swissSettings.sramVideo = SYS_VIDEO_NTSC;
	
	if(swissSettings.gameVMode > 0 && swissSettings.disableVideoPatches < 2) {
		swissSettings.sram60Hz = in_range(swissSettings.gameVMode, 1, 7);
		swissSettings.sramProgressive = in_range(swissSettings.gameVMode, 4, 7) || in_range(swissSettings.gameVMode, 11, 14);
		
		if(swissSettings.sram60Hz) {
			for(int i = 0; i < sizeof(gameVModePalEntries) / sizeof(*gameVModePalEntries); i++) {
				if(!strncmp(entry->game_id, gameVModePalEntries[i], 4)) {
					swissSettings.gameVMode += 7;
					break;
				}
			}
		}
		
		if(swissSettings.sramProgressive && !getDTVStatus())
			swissSettings.gameVMode = 0;
		
		if(swissSettings.sramVideo == SYS_VIDEO_PAL)
			swissSettings.sramProgressive &= swissSettings.sram60Hz;
		else
			swissSettings.sram60Hz = 0;
	} else if(swissSettings.sramProgressive) {
		if(swissSettings.sramVideo == SYS_VIDEO_PAL) {
			swissSettings.sramProgressive = 0;
			swissSettings.gameVMode = -2;
		} else
			swissSettings.gameVMode = -1;
	} else
		swissSettings.gameVMode = 0;
	
	if(!strncmp(entry->game_id, "GB3E", 4))
		swissSettings.sramProgressive = 0;
	
	switch(swissSettings.gameVMode) {
		case 2: case 9: case 5: case 12: case -1: case -2:
			if(!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
			break;
		case 3: case 10:
			swissSettings.forceVOffset &= ~1;
			if(!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
			if(!swissSettings.forceVJitter)
				swissSettings.forceVJitter = 2;
			break;
		case 4: case 11:
			swissSettings.forceVOffset &= ~1;
			if(!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
			if(!swissSettings.forceVJitter)
				swissSettings.forceVJitter = 1;
			break;
		case 6: case 13:
			if(!swissSettings.forceHScale)
				swissSettings.forceHScale = 1;
			swissSettings.forceVOffset &= ~1;
			if(!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
			if(!swissSettings.forceVJitter)
				swissSettings.forceVJitter = 1;
			break;
		case 7: case 14:
			if(!swissSettings.forceHScale)
				swissSettings.forceHScale = 1;
			swissSettings.forceVOffset &= ~1;
			if(!swissSettings.forceVFilter)
				swissSettings.forceVFilter = 1;
			break;
	}
}

void config_unload_current() {
	swissSettings.gameVMode = backup.gameVMode;
	swissSettings.forceHScale = backup.forceHScale;
	swissSettings.forceVOffset = backup.forceVOffset;
	swissSettings.forceVFilter = backup.forceVFilter;
	swissSettings.forceVJitter = backup.forceVJitter;
	swissSettings.fixPixelCenter = backup.fixPixelCenter;
	swissSettings.disableDithering = backup.disableDithering;
	swissSettings.forceAnisotropy = backup.forceAnisotropy;
	swissSettings.forceWidescreen = backup.forceWidescreen;
	swissSettings.fontEncode = backup.fontEncode;
	swissSettings.forcePollRate = backup.forcePollRate;
	swissSettings.invertCStick = backup.invertCStick;
	swissSettings.swapCStick = backup.swapCStick;
	swissSettings.triggerLevel = backup.triggerLevel;
	swissSettings.wiirdDebug = backup.wiirdDebug;
	swissSettings.sram60Hz = backup.sram60Hz;
	swissSettings.sramProgressive = backup.sramProgressive;
	swissSettings.emulateAudioStream = backup.emulateAudioStream;
	swissSettings.emulateReadSpeed = backup.emulateReadSpeed;
	swissSettings.emulateEthernet = backup.emulateEthernet;
	swissSettings.disableMemoryCard = backup.disableMemoryCard;
	swissSettings.disableHypervisor = backup.disableHypervisor;
	swissSettings.sramLanguage = backup.sramLanguage;
	swissSettings.sramVideo = backup.sramVideo;
}
