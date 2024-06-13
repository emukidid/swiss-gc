#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include "bba.h"
#include "exi.h"
#include "httpd.h"
#include "swiss.h"
#include "deviceHandler.h"

/* Network Globals */
int net_initialized = 0;
struct in_addr bba_localip;
struct in_addr bba_netmask;
struct in_addr bba_gateway;
u32 bba_location = LOC_UNK;

static lwp_t net_thread = LWP_THREAD_NULL;

static void *net_thread_func(void *arg)
{
	inet_aton(swissSettings.bbaLocalIp, &bba_localip);
	bba_netmask.s_addr = INADDR_BROADCAST << (32 - swissSettings.bbaNetmask);
	inet_aton(swissSettings.bbaGateway, &bba_gateway);

	if (if_configex(&bba_localip, &bba_netmask, &bba_gateway, swissSettings.bbaUseDhcp) == -1) {
		net_initialized = 0;
		return NULL;
	}

	net_initialized = 1;
	strcpy(swissSettings.bbaLocalIp, inet_ntoa(bba_localip));
	swissSettings.bbaNetmask = (32 - __builtin_ctz(bba_netmask.s_addr));
	strcpy(swissSettings.bbaGateway, inet_ntoa(bba_gateway));

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

	init_httpd_thread();
	init_wiiload_thread();
	return NULL;
}

// Init the GC net interface (bba)
void init_network_async(void)
{
	if (!net_initialized && net_thread == LWP_THREAD_NULL)
		LWP_CreateThread(&net_thread, net_thread_func, NULL, NULL, 0, LWP_PRIO_NORMAL);
}

bool init_network(void)
{
	init_network_async();
	wait_network();
	return net_initialized && (bba_localip.s_addr = net_gethostip()) != INADDR_ANY;
}

void wait_network(void)
{
	lwp_t thread = net_thread;
	net_thread = LWP_THREAD_NULL;
	if (thread != LWP_THREAD_NULL) {
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Initialising Network"));
		LWP_JoinThread(thread, NULL);
		DrawDispose(msgBox);
	}
}

u32 bba_exists(u32 location)
{
	u32 id;

	if (location & bba_location)
		return bba_location;

	if ((location & LOC_SERIAL_PORT_1) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_2, &id)) {
		switch (id) {
			case EXI_MX98730EC_ID:
			case EXI_ENC28J60_ID:
				return LOC_SERIAL_PORT_1;
		}
	}
	if ((location & LOC_SERIAL_PORT_2) && EXI_GetID(EXI_CHANNEL_2, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return LOC_SERIAL_PORT_2;
		}
	}
	if ((location & LOC_MEMCARD_SLOT_A) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return LOC_MEMCARD_SLOT_A;
		}
	}
	if ((location & LOC_MEMCARD_SLOT_B) && EXI_GetID(EXI_CHANNEL_1, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_ENC28J60_ID:
				return LOC_MEMCARD_SLOT_B;
		}
	}
	return LOC_UNK;
}

const char *bba_address_str(void)
{
	static char string[18];

	if ((bba_localip.s_addr = net_gethostip()) == INADDR_ANY) {
		u8 mac_addr[6];

		if (net_get_mac_address(mac_addr) < 0)
			return "Not initialised";

		sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		return string;
	}

	sprintf(string, "%u.%u.%u.%u", ip4_addr1(&bba_localip), ip4_addr2(&bba_localip), ip4_addr3(&bba_localip), ip4_addr4(&bba_localip));
	return string;
}
