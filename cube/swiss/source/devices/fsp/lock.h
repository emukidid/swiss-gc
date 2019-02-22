#ifndef _FSPLIB_H_LOCK
#define _FSPLIB_H_LOCK 1

typedef struct FSP_LOCK {
               unsigned short share_key;
} FSP_LOCK;

/* prototypes */

unsigned short client_get_key (FSP_LOCK *lock);
void client_set_key (FSP_LOCK *lock,unsigned short key);
int client_init_key (FSP_LOCK *lock,
                            unsigned long server_addr,
			    unsigned short server_port);
void client_destroy_key(FSP_LOCK *lock);
#endif
