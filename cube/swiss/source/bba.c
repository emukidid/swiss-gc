#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>

/* Network Globals */
int net_initialized = 0;
char bba_ip[16];

// net init thread
static lwp_t initnetthread = LWP_THREAD_NULL;
static int netInitHalted = 0;
static int netInitPending = 0;

void resume_netinit_thread() {
	if(initnetthread != LWP_THREAD_NULL) {
		netInitHalted = 0;
		LWP_ResumeThread(initnetthread);
	}
}

void pause_netinit_thread() {
	if(initnetthread != LWP_THREAD_NULL) {
		netInitHalted = 1;
		
		if(netInitPending) {
			return;
		}
	  
		// until it's completed for this iteration.
		while(!LWP_ThreadIsSuspended(initnetthread)) {
			usleep(100);
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

	while(1) {
		if(!net_initialized) {
			netInitPending = 1;
			res = if_config(bba_ip, NULL, NULL, true);
			if(res >= 0) {
				net_initialized = 1;
			}
			else {
				net_initialized = 0;
			}
			netInitPending = 0;
		}

		netsleep = 1000*1000; // 1 sec
		while(netsleep > 0) {
			if(netInitHalted) {
				LWP_SuspendThread(initnetthread);
			}
			usleep(100);
			netsleep -= 100;
		}
	}
	return NULL;
}

void init_network_thread() {
  LWP_CreateThread (&initnetthread, init_network, NULL, NULL, 0, 40);
}
