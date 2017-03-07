#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include "exi.h"

/* Network Globals */
int net_initialized = 0;
int bba_exists = 0;
int netInitPending = 1;
char bba_ip[16];

// net init thread
static lwp_t initnetthread = LWP_THREAD_NULL;
static int netInitHalted = 0;

void resume_netinit_thread() {
	if(initnetthread != LWP_THREAD_NULL) {
		netInitHalted = 0;
		LWP_ResumeThread(initnetthread);
	}
}

void pause_netinit_thread() {
	if(initnetthread != LWP_THREAD_NULL) {
		netInitHalted = 1;
		
		if(!netInitPending) {
			return;
		}
	  
		// until it's completed for this iteration.
		while(!LWP_ThreadIsSuspended(initnetthread)) {
			usleep(1000);
		}
	}
}

	
// Init the GC/Wii net interface (wifi/bba/etc)
static void* init_network(void *args) {
	int res = 0, netsleep = 1*1000*1000;
  
	while(netsleep > 0) {
		if(netInitHalted) {
			LWP_SuspendThread(initnetthread);
		}
        usleep(100);
        netsleep -= 100;
	}

	if(!net_initialized) {
		res = if_config(bba_ip, NULL, NULL, true, 20);
		if(res >= 0 && strcmp("255.255.255.255", bba_ip)) {
			net_initialized = 1;
		}
		else {
			memset(bba_ip, 0, sizeof(bba_ip));
			net_initialized = 0;
		}
		netInitPending = 0;
	}

	return NULL;
}

void init_network_thread() {
	bba_exists = exi_bba_exists();
	if(bba_exists) {
		LWP_CreateThread (&initnetthread, init_network, NULL, NULL, 0, 40);
	}
}
