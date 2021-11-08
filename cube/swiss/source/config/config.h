#ifndef CONFIG_H_
#define CONFIG_H_

#include "swiss.h"
typedef struct {
	char game_id[4];
	char padding_1[1];
	char game_name[64];
	char padding_2[1];
	char comment[128];
	char padding_3[1];
	char status[32];
	char padding_4[1];
	int gameVMode;
	int forceHScale;
	short forceVOffset;
	int forceVFilter;
	int disableDithering;
	int forceAnisotropy;
	int forceWidescreen;
	int invertCStick;
	int emulateReadSpeed;
} ConfigEntry;

void config_find(ConfigEntry *entry);
int config_update_game(ConfigEntry *entry);
int config_update_global();
int config_update_recent();
int config_init(void (*progress_indicator)(char*, int));
void config_load_current(ConfigEntry *config);
void config_unload_current();

#endif

