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
u32 bba_location = LOC_UNK;

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

		char ifname[4];
		if (if_indextoname(1, ifname)) {
			if (ifname[0] == 'E') {
				switch (ifname[1]) {
					case 'A': bba_location = LOC_MEMCARD_SLOT_A; break;
					case 'B': bba_location = LOC_MEMCARD_SLOT_B; break;
					case '1': bba_location = LOC_SERIAL_PORT_1;  break;
					case '2': bba_location = LOC_SERIAL_PORT_2;  break;
				}
			} else if (ifname[0] == 'e')
				bba_location = LOC_SERIAL_PORT_1;

			if (ifname[0] == 'E' && strchr("2AB", ifname[1])) {
				if (__device_gcloader.features & FEAT_PATCHES)
					__device_gcloader.emulable |= EMU_ETHERNET;
				__device_ata_c.emulable |= EMU_ETHERNET;
				__device_sd_c.emulable |= EMU_ETHERNET;
				__device_sd_a.emulable |= EMU_ETHERNET;
				__device_sd_b.emulable |= EMU_ETHERNET;
				__device_ata_a.emulable |= EMU_ETHERNET;
				__device_ata_b.emulable |= EMU_ETHERNET;
			}
		}

		deviceHandler_setDeviceAvailable(&__device_smb, deviceHandler_SMB_test());
		deviceHandler_setDeviceAvailable(&__device_ftp, deviceHandler_FTP_test());
		deviceHandler_setDeviceAvailable(&__device_fsp, deviceHandler_FSP_test());
	}
}

bool bba_exists(u32 location)
{
	u32 id;

	if ((location & LOC_MEMCARD_SLOT_A) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return true;
		}
	}
	if ((location & LOC_MEMCARD_SLOT_B) && EXI_GetID(EXI_CHANNEL_1, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return true;
		}
	}
	if ((location & LOC_SERIAL_PORT_1) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_2, &id)) {
		switch (id) {
			case EXI_MX98730EC_ID:
			case EXI_ENC28J60_ID:
				return true;
		}
	}
	if ((location & LOC_SERIAL_PORT_2) && EXI_GetID(EXI_CHANNEL_2, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return true;
		}
	}
	return false;
}
