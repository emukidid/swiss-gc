#ifndef __BBA_H
#define __BBA_H

extern int net_initialized;
extern int bba_exists;
extern char bba_local_ip[16];
extern char bba_netmask[16];
extern char bba_gateway[16];

void init_network();
void init_wiiload_thread();

#endif
