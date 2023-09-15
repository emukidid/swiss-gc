#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include "exi.h"
#include "swiss.h"
#include "deviceHandler.h"

/* Network Globals */
int net_initialized = 0;
struct in_addr bba_localip;
struct in_addr bba_netmask;
struct in_addr bba_gateway;

// Init the GC net interface (bba)
void init_network(void *args) {

	int res = 0;

	if(!net_initialized) {
		inet_aton(swissSettings.bbaLocalIp, &bba_localip);
		bba_netmask.s_addr = INADDR_BROADCAST << (32 - swissSettings.bbaNetmask);
		inet_aton(swissSettings.bbaGateway, &bba_gateway);

		res = if_configex(&bba_localip, &bba_netmask, &bba_gateway, swissSettings.bbaUseDhcp);
		if(res >= 0) {
			strcpy(swissSettings.bbaLocalIp, inet_ntoa(bba_localip));
			swissSettings.bbaNetmask = (32 - __builtin_ctz(bba_netmask.s_addr));
			strcpy(swissSettings.bbaGateway, inet_ntoa(bba_gateway));
			net_initialized = 1;
		}
		else {
			net_initialized = 0;
		}

		deviceHandler_setDeviceAvailable(&__device_smb, deviceHandler_SMB_test());
		deviceHandler_setDeviceAvailable(&__device_ftp, deviceHandler_FTP_test());
		deviceHandler_setDeviceAvailable(&__device_fsp, deviceHandler_FSP_test());
	}
}

