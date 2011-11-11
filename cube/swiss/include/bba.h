#ifndef __BBA_H
#define __BBA_H

extern char bba_ip[16];
extern int net_initialized;
extern int bba_exists;

void resume_netinit_thread();
void pause_netinit_thread();
void init_network_thread();

#endif
