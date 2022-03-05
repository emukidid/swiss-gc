#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include "exi.h"

/* Network Globals */
int net_initialized = 0;
int bba_exists = 0;
char bba_local_ip[16];
char bba_netmask[16];
char bba_gateway[16];

// Init the GC net interface (bba)
void init_network(void *args) {

	int res = 0;

	bba_exists = exi_bba_exists();
	if(bba_exists && !net_initialized) {
		res = if_config(bba_local_ip, bba_netmask, bba_gateway, true);
		if(res >= 0 && strcmp("255.255.255.255", bba_local_ip)) {
			net_initialized = 1;
		}
		else {
			net_initialized = 0;
		}
	}
}

