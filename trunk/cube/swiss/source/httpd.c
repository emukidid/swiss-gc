#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include "bba.h"
#include "dvd.h"
#include "main.h"
#include "devices/dvd/deviceHandler-DVD.h"

static	lwp_t httd_handle = (lwp_t)NULL;
const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "<html> \
                               <head><title>Swiss httpd page</title></head> \
                               <body> \
                               <a href=\"dvd.iso\">Dump DVD Disc</a><br> \
							   <a href=\"ipl.bin\">Dump IPL Mask ROM</a><br> \
                               </body> \
                               </html>";
							   
const static char nodisc[] = "<html> \
                               <head><title>Swiss httpd page</title></head> \
                               <body> \
                               No DVD Disc found for dumping. Error is: %s \
                               </body> \
                               </html>";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_data_hdr[] = "Content-type: application/octet-stream\r\n\r\n";
const static char http_len_hdr[] = "Content-Length: %d\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";
const static char http_get_dvd[] = "GET /dvd.iso HTTP/1.1\r\n";
const static char http_get_ipl[] = "GET /ipl.bin HTTP/1.1\r\n";

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
			print_gecko("Error %d binding socket!\r\n", ret);
		} else {
			if ( (ret = net_listen( sock, 5)) ) {
				print_gecko("Error %d listening!\r\n", ret);
			} else {
				while(1) {
					csock = net_accept (sock, (struct sockaddr *) &client, &clientlen);
					if ( csock < 0 ) {
						print_gecko("Error connecting socket %d!\r\n", csock);
						while(1);
					}

					print_gecko("Connecting port %d from %s\r\n", client.sin_port, inet_ntoa(client.sin_addr));
					memset (temp, 0, 1026);
					ret = net_recv (csock, temp, 1024, 0);

					//index page
					if ( !strncmp( temp, http_get_index, strlen(http_get_index) ) ) {
						net_send(csock, http_200, strlen(http_200), 0);
						net_send(csock, http_html_hdr, strlen(http_html_hdr), 0);
						memcpy(temp, indexdata, sizeof(indexdata));
						net_send(csock, temp, strlen(temp), 0);
					}
					// download a disc image
					else if ( !strncmp( temp, http_get_dvd, strlen(http_get_dvd) ) ) {
						net_send(csock, http_200, strlen(http_200), 0);
						// See if there's a valid disc in the drive
						if(initialize_disc(DISABLE_AUDIO) != DRV_ERROR) {
							sprintf(temp, http_len_hdr, DISC_SIZE);
							net_send(csock, temp, strlen(temp), 0);
							// Loop and pump DVD data out
							int ofs = 0;
							char *dvd_buffer = (char*)memalign(32,2048);
							for(ofs = 0; ofs < DISC_SIZE; ofs+=2048) {
								DVD_Read(dvd_buffer,ofs,2048);
								net_send(csock, dvd_buffer, 2048, 0);
							}
							free(dvd_buffer);
						}
						else {
							net_send(csock, http_html_hdr, strlen(http_html_hdr), 0);
							sprintf(temp, nodisc, dvd_error_str());
							net_send(csock, temp, strlen(temp), 0);
						}
					}
					// download the IPL
					else if ( !strncmp( temp, http_get_ipl, strlen(http_get_ipl) ) ) {
						net_send(csock, http_200, strlen(http_200), 0);
						sprintf(temp, http_len_hdr, 2*1024*1024);
						net_send(csock, temp, strlen(temp), 0);
						// Loop and pump IPL data out
						int i = 0;
						char *ipl_buffer = (char*)memalign(32,2048);
						for(i = 0; i < 2*1024*1024; i+=2048) {
							__SYS_ReadROM(ipl_buffer,2048,i);
							net_send(csock, ipl_buffer, 2048, 0);
						}
						free(ipl_buffer);
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
