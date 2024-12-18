#ifndef CONFIG_H_
#define CONFIG_H_

#include "swiss.h"
typedef struct {
	char game_id[4 + 1];
	char game_name[64 + 1];
	char comment[128 + 1];
	char status[32 + 1];
	char region;
	int gameVMode;
	int forceHScale;
	short forceVOffset;
	int forceVFilter;
	int forceVJitter;
	int disableDithering;
	int forceAnisotropy;
	int forceWidescreen;
	int forcePollRate;
	int invertCStick;
	int swapCStick;
	int triggerLevel;
	int emulateAudioStream;
	int emulateReadSpeed;
	int emulateEthernet;
	int disableMemoryCard;
	int forceCleanBoot;
	int preferCleanBoot;
	int rt4kProfile;
} ConfigEntry;

void config_find(ConfigEntry *entry);
void config_defaults(ConfigEntry *entry);
int config_update_game(ConfigEntry *entry, bool checkConfigDevice);
int config_update_global(bool checkConfigDevice);
int config_update_recent(bool checkConfigDevice);
int config_init(void (*progress_indicator)(char*, int, int));
void config_parse_args(int argc, char *argv[]);
void config_load_current(ConfigEntry *entry);
void config_unload_current();

#endif

