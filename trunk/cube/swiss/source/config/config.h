#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct {
	char game_id[4];
	char game_name[64];
	char comment[128];
	char status[32];
	int useHiLevelPatch;
	int useHiMemArea;
	int disableInterrupts;
	int gameVMode;
	int muteAudioStreaming;
	int muteAudioStutter;
	int noDiscMode;
} ConfigEntry __attribute__((aligned(32)));

void config_parse(char *configData);
void config_find(ConfigEntry *entry);
int config_update(ConfigEntry *entry);
int config_create();
int config_init();
int config_get_count();

#endif

