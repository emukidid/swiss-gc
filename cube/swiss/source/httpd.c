#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include "bba.h"

static	lwp_t httd_handle = (lwp_t)NULL;
const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "<html> \
                               <head><title>A test page</title></head> \
                               <body> \
                               This small test page has had %d hits. \
                               </body> \
                               </html>";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";

//---------------------------------------------------------------------------------
void *httpd (void *arg) {
//---------------------------------------------------------------------------------
	
	print_gecko("httpd Waiting\r\n");
	while(!net_initialized) {
		sleep(1);
	}
	print_gecko("httpd Alive\r\n");
	s32 sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];
	static int hits=0;
	
	clientlen = sizeof(client);

	sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock == INVALID_SOCKET) {
      print_gecko ("Cannot create a socket!\r\n");
    } else {

		memset (&server, 0, sizeof (server));
		memset (&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (80);
		server.sin_addr.s_addr = INADDR_ANY;
		ret = net_bind (sock, (struct sockaddr *) &server, sizeof (server));
		
		if ( ret ) {
			sprintf(temp,"Error %d binding socket!\r\n", ret);
			print_gecko(temp);
		} else {
			if ( (ret = net_listen( sock, 5)) ) {
				sprintf(temp,"Error %d listening!\r\n", ret);
				print_gecko(temp);
			} else {
				while(1) {
					csock = net_accept (sock, (struct sockaddr *) &client, &clientlen);
					if ( csock < 0 ) {
						sprintf(temp, "Error connecting socket %d!\r\n", csock);
						print_gecko(temp);
						while(1);
					}

					sprintf(temp,"Connecting port %d from %s\r\n", client.sin_port, inet_ntoa(client.sin_addr));
					print_gecko(temp);
					memset (temp, 0, 1026);
					ret = net_recv (csock, temp, 1024, 0);

					if ( !strncmp( temp, http_get_index, strlen(http_get_index) ) ) {
						hits++;
						net_send(csock, http_200, strlen(http_200), 0);
						net_send(csock, http_html_hdr, strlen(http_html_hdr), 0);
						sprintf(temp, indexdata, hits);
						net_send(csock, temp, strlen(temp), 0);
					}

					net_close (csock);

				}
			}
		}
	}
	return NULL;
}

void init_httpd_thread() {
	if(bba_exists) {
		LWP_CreateThread(	&httd_handle,	/* thread handle */ 
							httpd,			/* code */ 
							bba_ip,		/* arg pointer for thread */
							NULL,			/* stack base */ 
							16*1024,		/* stack size */
							40				/* thread priority */ );
	}
}
