/****************************************************************************
 * TinyFTP
 * Nintendo Wii/GameCube FTP implementation
 *
 * FTP devoptab
 * Implemented by hax
 ****************************************************************************/

#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/stat.h>
#include <network.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/mutex.h>
#include <network.h>
#include "bba.h"

#include "ftp_devoptab.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//cache pages count
#define FTP_CACHE_PAGES				3
//cache page size
#define NET_READ_BUFFERSIZE			7300
#define NET_WRITE_BUFFERSIZE		4096

#define FTP_READ_BUFFERSIZE			(128*1024)

#define FTP_MAXPATH					1024
#define FTP_MAX_LINE				255

#define NET_TIMEOUT					10000  // network connection timeout, in ms

#define SOCKET s32

//#define FTP_DEBUG 1

//file structure
typedef struct
{
	off_t offset;					//current read position
	off_t size;						//length of file
	char filename[FTP_MAXPATH];		//full filename, "\doc\file.dat"
	int envIndex;					//connection this file belongs too
} FTPFILESTRUCT;

//cache page
typedef struct
{
	off_t offset;           //offset in file
	u64 last_used;			//last used, ms
	FTPFILESTRUCT *file;	//file this item read from, NULL if free page
	void *ptr;				//poiter to data, FTP_READ_BUFFERSIZE bytes
} ftp_cache_page;

//directory list entry
 typedef struct
{
  off_t size;				//file size
  bool isDirectory;			//item is directory
  char name[FTP_MAX_LINE];	//'file.dat', 'doc'
} FTPDIRENTRY;

//directory linked list item
typedef struct
{
	FTPDIRENTRY item;
	void* next;				//FTPDIRENTRYLISTITEM* next
} FTPDIRENTRYLISTITEM;

// FTP connection
typedef struct
{
	char* hostname;			//'ftp.host.com', can't be NULL
	char* user;				//'anonymous', can't be NULL
	char* password;			//'anonymous', can't be NULL
	char* share;			//share name without terminating  '/', with starting '/', can't be NULL
    unsigned short port;	//FTP port, standard 21

	bool ftp_passive;		//use PASV connection

	SOCKET ctrl_socket;
	SOCKET data_socket;

	char *name;             //'ftp1', NULL means free item
	int envIndex;           //position if this item in FTPEnv array

	char currentpath[FTP_MAXPATH];  //Always terminated by '/'. '/' for share root. Always contains'/', not '\'.

	devoptab_t *devoptab;

	char 					dir_cache[FTP_MAXPATH];  //the following to mombers store directory contents
	FTPDIRENTRYLISTITEM* 	dir_cache_list;				//dir_cache_list = NULL means cache not filled

} ftp_env;

//structure for enumerating directory
typedef struct
{
	int envIndex;							//connection this list belongs to
	FTPDIRENTRYLISTITEM* list;				//linked list of entries
	FTPDIRENTRYLISTITEM* next_enum_item;  	//next item to be returned by findNext
} FTPDIRSTATESTRUCT;


//======================
// Global variables
//======================

static bool FirstInit=true;
static mutex_t _FTP_mutex=LWP_MUTEX_NULL;

//holds opened ftp connections and free items
static ftp_env FTPEnv[MAX_FTP_MOUNTED];

//global readahead cache
//we maintain single cache for all connections
static ftp_cache_page *FTPReadAheadCache = NULL;
static u32 numFTP_RA_pages = 0;

//used to reuse data connections with RETR
static char last_cmd[FTP_MAX_LINE] = "";
static off_t last_off = 0;

static bool dir_changed = false;

//looks like net_bind can't allocate free port automatically.
//use random_port++ every time.
static u16 random_port = 1025;

#ifdef FTP_DEBUG
static u16 debug_msgid = 0;
static s32 debug_sock = INVALID_SOCKET;
#endif

static bool ReadFTPFromCache( void *buf, off_t len, FTPFILESTRUCT *file );
static int ftp_execute(ftp_env* env, char *cmd, int res, int reconnect);

static inline void _FTP_lock()
{
	if(_FTP_mutex!=LWP_MUTEX_NULL) LWP_MutexLock(_FTP_mutex);
}

static inline void _FTP_unlock()
{
	if(_FTP_mutex!=LWP_MUTEX_NULL) LWP_MutexUnlock(_FTP_mutex);
}

//find index of item in FTPEnv erray by name
//name = 'ftp1'
static ftp_env* FindFTPEnv(const char *name)
{
	int i;

	for(i=0;i<MAX_FTP_MOUNTED ;i++)
	{
		if(FTPEnv[i].name!=NULL && strcmp(name,FTPEnv[i].name)==0)
		{
			return &FTPEnv[i];
		}
	}
	return NULL;
}

static s32 set_blocking(s32 s, bool blocking)
{
	u32 set;

// Switch off Nagle, ON TCP_NODELAY ***/

//  review: do we need this? lack of documentation
//	nodelay = blocking?0:1;
	set = 1;
	net_setsockopt(s,IPPROTO_TCP,TCP_NODELAY,&set,sizeof(set));

	set = !blocking;
	return net_ioctl(s,FIONBIO,&set);
}

#if 0
//set to blocking before closing socket;
//allows to send all data before closing
static s32 net_close_blocking(s32 s)
{
    set_blocking(s, true);
    return net_close(s);
}
#endif

static void ReplaceForwardSlash(char* str)
{
	char* p = str;
	while (*p!=0)
	{
		if (*p=='\\') *p='/';
		p++;
	}
}

//fix ill-formed path with double '\' or '/'
static void FTP_FixPath( const char** path, char* fixed_path )
{
	char* p = fixed_path;

	if (**path == 0 )
	{
		*fixed_path = 0;
		*path = fixed_path;
		return;
	}


	bool prevslash = false;

	while ( **path != 0 )
	{
		if ( **path == '\\' || **path == '/' )
		{
			if ( prevslash )
			{
				(*path)++;
				continue;
			}

			prevslash = true;
		}
		else
		{
			prevslash = false;
		}

		*p = **path;
		p++;
		(*path)++;
	}

	*p = 0;
	*path = fixed_path;
}

//send count bytes
static bool SocketSend(SOCKET theSocket, const char* buf, size_t count)
{
	s32 ret;
	u64 t1,t2;

	t1=ticks_to_millisecs(gettime());
	while(count>0)
	{
		ret=net_send(theSocket,buf,(count>NET_WRITE_BUFFERSIZE)?NET_WRITE_BUFFERSIZE:count,0);
		if(ret==-EAGAIN)
		{
			t2=ticks_to_millisecs(gettime());
			if( (t2 - t1) > NET_TIMEOUT)
			{
				return false;	/* timeout */
			}
			usleep(100);	/* allow system to perform work. Stabilizes system */
			continue;
		}
		else if(ret<0)
		{
			return false;	/* some error happened */
		}
		else
		{
			buf+=ret;
			count-=ret;
			if(count==0)
			{
				return true;
			}
			t1=ticks_to_millisecs(gettime());
		}
	}

	return true;
}

//exitonempty = true: receive count bytes, fail if timeout
//exitonempty = false: return up to count bytes or return when socket is empty
//return number of bytes received or -1 (timeout)
static int SocketRecv(SOCKET theSocket, char* buf, size_t count, bool exitonempty)
{
	s32 ret;
	u64 t1,t2;
	u32 received = 0;

	t1=ticks_to_millisecs(gettime());
	while(count>0)
	{
		ret=net_recv(theSocket,buf,(count>NET_READ_BUFFERSIZE)?NET_READ_BUFFERSIZE:count,0);
		if(ret==-EAGAIN)
		{
			t2=ticks_to_millisecs(gettime());
			if( (t2 - t1) > NET_TIMEOUT)
			{
				return -1;	/* timeout */
			}
			usleep(4000);	/* allow system to perform work. Stabilizes system */
			continue;
		}
		else if(ret<0)
		{
			return -1;	/* some error happened */
		}
		else
		{
			buf+=ret;
			count-=ret;
			received+=ret;
			if(count==0 || (exitonempty == true && ret == 0))
			{
				return received;
			}
			t1=ticks_to_millisecs(gettime());
			usleep(4000);
		}
	}

	return received;
}

static int ftp_getIP(char *buf, unsigned *ip, unsigned short *port)
{
	char *b;
	int i;

	*ip = *port = 0;

	buf = strchr(buf,'(') + 1;
	if(!buf)
		return -1;
	for(i = 0; i<4; i++){
		b = buf;
		*ip = (*ip << 8) + (strtoul(b, &b, 0) );
		buf = strchr(buf, ',') + 1;
	}

	*ip = htonl(*ip);

	b = buf;
	*port = (unsigned short)strtoul(b, &b, 0) << 8;
	buf = strchr(buf, ',') + 1;
	b = buf;
	*port = htons(*port + (unsigned short)strtoul(b, &b, 0));

	return 0;
}

static int ftp_readline(SOCKET socket, char *buf, int len)
{
	int i = 0, out = 0;
	int l;

	do{
		if( (l = SocketRecv(socket, &(buf[i]), 1, true)) > 0 )
		{
			if(buf[i] == '\n') out = 1;
			i++;
		}
		else
		{
			buf[i] = 0;
//			NET_PRINTF(" ftp_readline() failed: '%s'\n", buf );

			return -1;
		}

	}while((i < (len-1))&&(out == 0));

	buf[i] = 0;
//	NET_PRINTF(" ftp_readline() ok: '%s'\n", buf );
	return i;
}

static int ftp_get_response(ftp_env* env)
{
	char buf[FTP_MAX_LINE], *b;
	int i, res = 0;
	int multiline = 0;

	i = ftp_readline(env->ctrl_socket, buf, FTP_MAX_LINE);
	if(i < 0)
	{
		NET_PRINTF("FTP response: NULL", 0 );
		return i;
	}

	NET_PRINTF("FTP response: %s\n", buf );

	if(buf[3] == '-')
	{
		buf[3] = 0;
		multiline = strtoul(buf, &b, 0);
		if(b != buf + 3)
		{
			NET_PRINTF("FTP response: bad response\n", 0 );
			return -1;
		}
	}

	if(multiline)
	{
		do {
			i = ftp_readline(env->ctrl_socket, buf, FTP_MAX_LINE);
			if(i < 0)
			{
				return i;
			}

			if(buf[3] == ' ')
			{
				buf[3] = 0;
				res = strtoul(buf, &b, 0);
			}
		} while(res != multiline);
	}
	else
	{
		buf[3] = 0;
		res = strtoul(buf, &b, 0);
		if(b != buf + 3)
		{
			NET_PRINTF("FTP response: bad response\n", 0 );
			return -1;
		}
	}

	return res;
}

static int ftp_close_data(ftp_env* env)
{
	if( env->data_socket != INVALID_SOCKET )
	{
		NET_PRINTF("ftp_close_data()\n", 0);

		net_close(env->data_socket);
		env->data_socket = INVALID_SOCKET;
		return ftp_get_response(env);
	}

	return 0;
}

static u32 ResolveHostAddress( const char* hostname )
{
	//struct hostent* hostEntry = net_gethostbyname(hostname);
    //
	//if ( hostEntry != NULL && hostEntry->h_length>0 && hostEntry->h_addr_list != NULL && hostEntry->h_addr_list[0]!=NULL )
	//{
	//	struct in_addr addr;
	//	addr.s_addr = *(u32*)(hostEntry->h_addr_list[0]);
	//	NET_PRINTF( "Resolve address: %s -> %s\n", hostname, inet_ntoa(addr));
	//	return addr.s_addr;
	//}
	//else
	//{
		NET_PRINTF( "Resolve address: %s -> %s\n", hostname, hostname );
		return inet_addr( hostname );
	//}

}

static bool ftp_doconnect( ftp_env* env )
{
	NET_PRINTF( "ftp_doconnect()\n", 0 );

	char buf[FTP_MAX_LINE];
	int response;
	struct sockaddr_in server_addr;

/*
	LPHOSTENT hostEntry = gethostbyname(env->hostname);
	if(hostEntry == NULL)
	{
		return false;
	}

	env->ctrl_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(env->ctrl_socket == SOCKET_ERROR)
	{
		NET_PRINTF("ftp_doconnect() - socket error\n");
		return false;
	}

	SOCKADDR_IN serverInfo;

	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr = *((LPIN_ADDR)*hostEntry->h_addr_list);
	serverInfo.sin_port = htons(21);

	int rVal = connect(env->ctrl_socket,(LPSOCKADDR)&serverInfo, sizeof(serverInfo));

	if( rVal == SOCKET_ERROR )
	{
		NET_PRINTF("ftp_doconnect() - socket error (2)\n");
		return false;
	}
*/

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = ResolveHostAddress( env->hostname );
	server_addr.sin_port = htons(env->port);

	env->ctrl_socket = net_socket( AF_INET, SOCK_STREAM, IPPROTO_IP );
	if( env->ctrl_socket == INVALID_SOCKET ) return false;

	set_blocking( env->ctrl_socket, false );

	// net_connect is always blocking in lwIP 1.1.1
	if( net_connect( env->ctrl_socket, (struct sockaddr*)&server_addr, sizeof(server_addr) ) < 0 )
	{
		NET_PRINTF( "ftp_doconnect() - cant connect to '%s'\n", env->hostname );
		return false;
	}

	if( ftp_get_response(env) != 220 )
	{
		NET_PRINTF( "ftp_doconnect() - no response from ftp server\n", 0 );
		return false;
	}

	sprintf(buf, "USER %s", env->user);
	if( ftp_execute(env, buf, 0, 0) < 0 )
	{
		NET_PRINTF( "ftp_doconnect() - USER failed\n", 0 );
		return false;
	}

	response = ftp_get_response(env);
	if((response  < 0)||((response != 331)&&(response != 230)))
	{
		NET_PRINTF( "ftp_doconnect() - invalid user\n", 0 );
		return false;
	}

	if(response == 331)
	{
		sprintf(buf, "PASS %s", env->password);

		if(ftp_execute(env, buf, 230, 0) < 0)
		{
			NET_PRINTF( "ftp_doconnect() - invalid login\n", 0 );
			return false;
		}
	}

	return true;
}

static int ftp_reconnect(ftp_env* env)
{
	int res;

	NET_PRINTF( "ftp_reconnect()\n",0 );

	if(env->data_socket != INVALID_SOCKET)
	{
		net_close(env->data_socket);
		env->data_socket = INVALID_SOCKET;
	}

	if((res = ftp_doconnect(env)) >= 0)
	{
		NET_PRINTF( "ftp_reconnect() - ok\n", 0 );
		return 0;
	}
	else
	{
		NET_PRINTF( "ftp_reconnect() - FAILED\n", 0 );
		if(env->ctrl_socket != INVALID_SOCKET)
		{
			net_close(env->ctrl_socket);
			env->ctrl_socket = INVALID_SOCKET;
		}
		return res;
	}
}

// if res!=0, server response is checked.
static int ftp_execute(ftp_env* env, char *cmd, int res, int reconnect)
{
	char buf[FTP_MAX_LINE];
	int r;

	NET_PRINTF("FTP_EXECUTE: '%s', expect res=%d\n", cmd, res);


	ftp_close_data(env);

	if(env->ctrl_socket == INVALID_SOCKET)
	{
		r = ftp_reconnect(env);
		if(r < 0)
			return r;
	}

	sprintf(buf, "%s\r\n", cmd);

	if( SocketSend(env->ctrl_socket, buf, strlen(buf)) == false )
	{
		NET_PRINTF( "FTP_EXECUTE: SEND FAILED\n", 0 );

		if(reconnect)
		{
			if((r = ftp_reconnect(env)) < 0)
			{
				return r;
			}
			else
			{
				return -EAGAIN;
			}
		}
		else
		{
			return -1;
		}
	}

	if( res!=0 )
	{
		r = ftp_get_response(env);
		NET_PRINTF(" -> got res = %d\n", r);
		if(r != res)
		{
			NET_PRINTF( "FTP_EXECUTE: COMMAND '%s' FAILED\n", buf );

			if((reconnect) && ((r < 0)||(r == 421)))
			{
				if(( r = ftp_reconnect(env)) < 0)
					return r;
				else
					return -EAGAIN;
			}
			else
			{
				return -1;
			}
		}

	}
	return 0;
}

static int ftp_get_fname(char* s, char* d)
{
	int i, cnt;
	int in_space = 1;

	for(i = 0, cnt = 9; (i < strlen(s))&&(cnt > 0); i++)
	{
		if(in_space)
		{
			if(s[i] != ' ')
			{
				in_space = 0;
				cnt--;
			}
		}
		else
		{
			if(s[i] == ' ') in_space = 1;
		}
	}
	if(i >= strlen(s))
	{
//		VERBOSE(" jumped the horse here!\n");
		d[0] = 0;
		return -1;
	}

	strcpy(d, &(s[i-1]));
	d[strlen(d) - 2] = 0;

	return 0;
}

static int ftp_get_substring(char *s, char *d, int n)
{
	int in_space = 1;
	int len = 0, i;

	for(i = 0; i < strlen(s); i++)
	{
		if(in_space)
		{
			if(s[i] != ' ')
			{
				in_space = 0;
				len = 0;
				d[len++] = s[i];
			}
		}
		else
		{
			if(s[i] != ' ')
			{
				d[len++] = s[i];
			}
			else
			{
				in_space = 1;
				d[len] = 0;
				if((--n) == 0) return len;
				len = 0;
			}
		}
	}

	if((len > 2)&&(n == 1))
	{
		d[len - 2] = 0;
		return len - 2;
	}

	return -1;
}

static int ftp_execute_open_actv(ftp_env* env, char *cmd, char *type, off_t offset, SOCKET *data_sock)
{

	struct sockaddr_in addr;
	SOCKET ssock;
	char buf[FTP_MAX_LINE];
	int res;
	u64 t1,t2;


	if( (env->data_socket == INVALID_SOCKET) || (offset != last_off) || (strcmp(last_cmd, cmd)) )
	{
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ftp_close_data(env);

		t1=ticks_to_millisecs(gettime());

execute_open_actv_retry:

		ssock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

		set_blocking( ssock, false );

		addr.sin_port = htons(random_port++);
		if ( random_port > 60000 ) random_port = 1025;


		if ( net_bind( ssock, (struct sockaddr *) &addr, sizeof(addr) ) < 0 )
		{
			net_close(ssock);
			return -1;
		}

		if ( net_listen( ssock, 2) < 0 )
		{
			net_close(ssock);
			return -1;
		}


		addr.sin_addr.s_addr = inet_addr(bba_ip);

		sprintf(buf, "PORT %u,%u,%u,%u,%u,%u",
			(ntohl(addr.sin_addr.s_addr) >> 24) & 0xff,
			(ntohl(addr.sin_addr.s_addr) >> 16) & 0xff,
			(ntohl(addr.sin_addr.s_addr) >> 8) & 0xff,
			ntohl(addr.sin_addr.s_addr) & 0xff,
			ntohs(addr.sin_port) >> 8,
			ntohs(addr.sin_port) & 0xff);

		if((res = ftp_execute(env, buf, 200, 1)) < 0)
		{
			net_close(ssock);

			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_actv_retry;
				}
			}


			return res;
		}

		sprintf(buf, "TYPE %s", type);

		if((res = ftp_execute(env, buf, 200, 1)) < 0)
		{
			net_close(ssock);

			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_actv_retry;
				}
			}

			return res;
		}

		if ( offset!=0 )
		{
			//2^32 = 4.294.967.296

			u32 low_part = offset % 1000000000;
			u32 high_part = offset / 1000000000;

			NET_PRINTF("offset=%u:%u\n", (u32)(offset >> 32), (u32)(offset & 0xffffFFFF));

			sprintf(buf, "REST %u%u", high_part, low_part );
			NET_PRINTF("REST=%s\n",buf);
			if((res = ftp_execute(env, buf, 350, 1)) < 0)
			{
				net_close(ssock);

				if(res == -EAGAIN)
				{
					t2 = ticks_to_millisecs(gettime());

					if ( t2 - t1 > NET_TIMEOUT)
					{
						return -1;
					}
					else
					{
						goto execute_open_actv_retry;
					}
				}

				return res;
			}
		}

		if((res = ftp_execute(env, cmd, 0, 1)) < 0)
		{
			net_close(ssock);

			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_actv_retry;
				}
			}

			return res;
		}

		res = ftp_get_response(env);

		if ( res != 150 && res != 125 )
		{
			net_close(ssock);

			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_actv_retry;
				}
			}

			return -1;
		}

/*
		*data_sock =  accept(ssock, NULL, NULL);
		if (*data_sock == INVALID_SOCKET)
		{
			return -1;
		}
*/

		NET_PRINTF("net_accept()\n",0);


        struct sockaddr_in data_peer_address;
        socklen_t addrlen = sizeof(data_peer_address);

		*data_sock = net_accept( ssock, (struct sockaddr *)&data_peer_address ,&addrlen );
		if (*data_sock == INVALID_SOCKET)
		{
			NET_PRINTF("net_accept() - failed\n",0);
			net_close(ssock);
			return -1;
		}

		NET_PRINTF("Accepted connection from %s\n", inet_ntoa(data_peer_address.sin_addr));

		env->data_socket = *data_sock;
		net_close(ssock);
		strcpy(last_cmd, cmd);
		last_off = offset;

	}
	else
	{
		*data_sock = env->data_socket;
	}

	return 0;
}

static int ftp_execute_open_pasv(ftp_env* env, char *cmd, char *type, off_t offset, SOCKET *data_sock)
{
	int l;
	char buf[FTP_MAX_LINE];
	char *b;
	unsigned ip;
	unsigned short port;
	int res;
	struct sockaddr_in server_addr;
	u64 t1,t2;

	if( (env->data_socket == INVALID_SOCKET) || (offset != last_off) || (strcmp(last_cmd, cmd)) )
	{
		ftp_close_data(env);
		t1=ticks_to_millisecs(gettime());

execute_open_retry:

		if( (res = ftp_execute(env, "PASV", 0, 1)) < 0)
		{
			//PASV command failed!
			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_retry;
				}
			}

			return res;
		}

		if((res = ftp_readline(env->ctrl_socket, buf, FTP_MAX_LINE)) < 0)
		{
			//No response!
			t2 = ticks_to_millisecs(gettime());

			if ( t2 - t1 > NET_TIMEOUT)
			{
				return -1;
			}
			else
			{
				goto execute_open_retry;
			}
		}

		b = buf;

		if((l = strtoul(b, &b, 0)) != 227)
		{
			//"PASV command failed!
			return -1;
		}

		ftp_getIP(buf, &ip, &port);

/*
		DEBUG(" ip,port: %u.%u.%u.%u:%u\n",
			ip & 0xff,
			(ip >> 8) & 0xff,
			(ip >> 16) & 0xff,
			(ip >> 24) & 0xff,
			ntohs(port));
*/

		sprintf(buf, "TYPE %s", type);

		if((res = ftp_execute(env, buf, 200, 1)) < 0)
		{
			//Could not set transmission type!

			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_retry;
				}
			}

			return -1;
		}

		if( offset !=0 )
		{
			u32 low_part = offset % 1000000000;
			u32 high_part = offset / 1000000000;

			NET_PRINTF("offset=%u:%u\n", (u32)(offset >> 32), (u32)(offset & 0xffffFFFF));

			sprintf(buf, "REST %u%u", high_part, low_part );
			NET_PRINTF("REST=%s\n",buf);
			if((res = ftp_execute(env, buf, 350, 1)) < 0)
			{
				//Could not set transmission offset!
				if(res == -EAGAIN)
				{
					t2 = ticks_to_millisecs(gettime());

					if ( t2 - t1 > NET_TIMEOUT)
					{
						return -1;
					}
					else
					{
						goto execute_open_retry;
					}
				}

				return -1;
			}
		}

		if((res = ftp_execute(env, cmd, 0, 1)) < 0)
		{
			//Could not execute cmd!
			if(res == -EAGAIN)
			{
				t2 = ticks_to_millisecs(gettime());

				if ( t2 - t1 > NET_TIMEOUT)
				{
					return -1;
				}
				else
				{
					goto execute_open_retry;
				}
			}

			return -1;
		}


		NET_PRINTF(" ip,port: %u.%u.%u.%u:%u\n",
			(ntohl(ip) >> 24) & 0xff,
			(ntohl(ip) >> 16) & 0xff,
			(ntohl(ip) >> 8) & 0xff,
			ntohl(ip) & 0xff,
			ntohs(port));

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = port;
		server_addr.sin_addr.s_addr = ip;

		*data_sock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if(*data_sock==INVALID_SOCKET)
		{
			return -1;
		}

		set_blocking( *data_sock, false );

		// net_connect is always blocking in lwIP 1.1.1
		if( net_connect( *data_sock, (struct sockaddr*)&server_addr, sizeof(server_addr) ) < 0 )
		{
			net_close(*data_sock);
			return -1;
		}

		res = ftp_get_response(env);
		if( res != 150 && res != 125)
		{
			//Bad server response!
			net_close(*data_sock);
			return -1;
		}

		env->data_socket = *data_sock;
		strcpy(last_cmd, cmd);
		last_off = offset;
	}
	else
	{
		*data_sock = env->data_socket;
	}
	return 0;
}

static int ftp_execute_open(ftp_env* env, char *cmd, char *type, off_t offset, SOCKET *data_sock)
{
//	if( env->ftp_passive == false )
if (true)
		return ftp_execute_open_actv(env, cmd, type, offset, data_sock);
	else
		return ftp_execute_open_pasv(env, cmd, type, offset, data_sock);
}

static void freeDirList(FTPDIRENTRYLISTITEM* pItem)
{
	while (pItem != NULL )
	{
		FTPDIRENTRYLISTITEM* pItem1 = (FTPDIRENTRYLISTITEM*)pItem->next;
		free( pItem );
		pItem = pItem1;
	}
}

//close FTP connection
static void FTP_Close( ftp_env* env )
{
	NET_ASSERT( env != NULL );
	NET_ASSERT( env->name != NULL );

	NET_PRINTF( "FTP_Close()" , 0 );

	free(env->name);
	free(env->hostname);
	free(env->share);
	free(env->user);
	free(env->password);

	if ( env->dir_cache_list != NULL )
	{
		freeDirList( env->dir_cache_list );
		env->dir_cache_list = NULL;
	}

	if ( env->ctrl_socket != INVALID_SOCKET )
	{
		net_close(env->ctrl_socket);
		env->ctrl_socket = INVALID_SOCKET;
	}

	if ( env->data_socket != INVALID_SOCKET )
	{
		net_close(env->data_socket);
		env->data_socket = INVALID_SOCKET;
	}

	env->name = NULL;  //tag free item
}

//name = 'ftp1'
//open ftp connection
static bool FTP_Connect(ftp_env* env, const char* name, const char* user, const char* password, const char* share, const char* hostname, unsigned short port, bool ftp_passive)
{
	NET_ASSERT( env != NULL );
	NET_ASSERT( name != NULL );
	NET_ASSERT( user != NULL );
	NET_ASSERT( password != NULL );
	NET_ASSERT( share != NULL );
	NET_ASSERT( hostname != NULL );

	env->name = strdup(name);
	env->hostname = strdup(hostname);
	env->user = strdup(user);
	env->password = strdup(password);
	env->port = (unsigned short) port;
	env->ftp_passive = ftp_passive;

	env->dir_cache_list = NULL;

	//form share name:
	//- form starting '/'
	//- replace slashes  '\' ->'/'
	//- remove terminating slash

	if (share[0]!='\\' && share[0]!='/')
	{
		strcpy(env->currentpath, "/");
	}
	else
	{
		env->currentpath[0] = 0;
	}

	strcat(env->currentpath, share);

	ReplaceForwardSlash(env->currentpath);

	if (env->currentpath[strlen(env->currentpath)-1]=='/')
	{
		env->currentpath[strlen(env->currentpath)-1] = 0;
	}
	env->share = strdup(env->currentpath);

	strcpy(env->currentpath,"/");

	env->ctrl_socket = INVALID_SOCKET;
	env->data_socket = INVALID_SOCKET;

	if (ftp_doconnect(env) == false)
	{
		FTP_Close(env);
		return false;
	}

	return true;
}

static void AddDirEntry(FTPDIRSTATESTRUCT* state, FTPDIRENTRYLISTITEM*** ppLastItem, const char* name, off_t size, bool isDirectory)
{
	FTPDIRENTRYLISTITEM* pItem = ( FTPDIRENTRYLISTITEM* )malloc( sizeof( FTPDIRENTRYLISTITEM ) );

	strcpy( pItem->item.name, name);
	pItem->item.size = size;
	pItem->item.isDirectory = isDirectory;
	pItem->next = NULL;

	**ppLastItem = pItem;
	*ppLastItem = (FTPDIRENTRYLISTITEM**)&pItem->next;
}

static void FTP_FindClose(FTPDIRSTATESTRUCT* state)
{
	NET_ASSERT( state != NULL );

	NET_PRINTF( "FTP_FindClose()\n",0 );

	freeDirList( state->list );

	NET_PRINTF( "FTP_FindClose() ok\n",0 );

}

static void copyDirList(FTPDIRENTRYLISTITEM** ppDest, FTPDIRENTRYLISTITEM* pSrc)
{
	*ppDest = NULL;
	while (pSrc != NULL )
	{
		*ppDest = malloc( sizeof(FTPDIRENTRYLISTITEM) );
		memcpy( *ppDest, pSrc, sizeof(FTPDIRENTRYLISTITEM) );
		(*ppDest)->next = NULL;
		pSrc = (FTPDIRENTRYLISTITEM*)(pSrc->next);
		ppDest = (FTPDIRENTRYLISTITEM**)&((*ppDest)->next);
	}
}

static u64 mystrtoul64( const char* p )
{
	u64 res = 0;

	while ( ( *p!=0 ) && ( *p==' ' ) )
	{
		p++;
	}


	while ( ( *p!=0 ) && ( *p>='0' ) && ( *p<='9' ) )
	{
		res = res*10 + ((*p)-'0');
		p++;
	}


	//NET_PRINTF( "value=%s, strtoul64=%x:%x\n", p1, (u32)(res >> 32), (u32)( res & 0xffffFFFF) );

	return res;
}

static bool FTP_FindFirst(const char *path_abs, FTPDIRSTATESTRUCT* state, ftp_env* env)
{
	SOCKET data_sock;
	char buf[FTP_MAXPATH + 5], buf2[FTP_MAX_LINE];
	char fixed_path[FTP_MAXPATH];
	char filename[FTP_MAXPATH];
	char *b = buf2;
	FTPDIRENTRYLISTITEM** ppLastItem = &state->list;
	int res;
	off_t filesize;
	bool isdirectory;
	u64 t1,t2;

	NET_ASSERT( filename != NULL );
	NET_ASSERT( state != NULL );
	NET_ASSERT( env != NULL );

	NET_PRINTF( "FTP_FindFirst( path_abs = '%s' )\n", path_abs );

	FTP_FixPath( &path_abs, fixed_path );

	if(strlen(env->share) + strlen(path_abs) > FTP_MAXPATH)
	{
		//Path too long!
		return false;
	}

	state->envIndex = env->envIndex;

    //cache needs to be reloaded because of a new file might be created
	if ( env->dir_cache_list != NULL && strcmp( env->dir_cache, path_abs ) == 0 && !dir_changed)
	{
		copyDirList( &(state->list), env->dir_cache_list );

		state->next_enum_item = state->list;

		NET_PRINTF( "FTP_FindFirst() - Ok (Cache reused)\n", 0 );
		return true;
	}

	sprintf(buf, "CWD %s%s", env->share, path_abs);

	t1=ticks_to_millisecs(gettime());

loaddir_retry:

	if((res = ftp_execute(env, buf, 250, 1)) < 0)
	{
		//review: Golden FTP server drops connection on any error, including chaging to not existing directory
		//if app is trying to list not absent directory (\subtitles), we have long timeout here
		if(res == -EAGAIN)
		{
			t2=ticks_to_millisecs(gettime());
			if (t2 - t1 > NET_TIMEOUT)
			{
				return false;
			}
			else
			{
				goto loaddir_retry;
			}
		}
		return false;
	}

	res = ftp_execute_open(env, "LIST -al", "A", 0, &data_sock);
	if(res < 0)
	{
		return false;
	}

	AddDirEntry(state, &ppLastItem, ".", 0, true);

	//add '..' item, if not root dir
	if ( strlen(path_abs) != 1 )
	{
		AddDirEntry(state, &ppLastItem, "..", 0, true);
	}

	bool hasItems = false;

	while((res = ftp_readline(data_sock, buf, FTP_MAX_LINE)) > 0)
	{
//		NET_PRINTF( "ftp_readline() : %s\n", buf );

		if(ftp_get_fname(buf, buf2) >= 0)
		{
			if((strcmp(buf2, ".") == 0) || (strcmp(buf2, "..") == 0))
			{
				continue;
			}

			if(strstr(buf2, "->"))
			{
				continue;
			}

			strcpy(filename, buf2);

			ftp_get_substring(buf, buf2, 5);
			b = buf2;
			filesize = mystrtoul64( b );

			ftp_get_substring(buf, buf2, 1);

			if( buf2[0] == 'd' || buf2[0] == 'd' )
			{
				isdirectory = true;
			}
			else if	(buf2[0] == 'l' || buf2[0] == 'L')
			{
				continue;  //review!!! - S_IFLNK;
			}
			else
			{
				isdirectory = false;
			}

			AddDirEntry(state, &ppLastItem, filename, filesize, isdirectory);
			hasItems = true;
		}
	}

	res = ftp_close_data(env);
	if(res < 0)
	{
		FTP_FindClose(state);
		return false;
	}

	state->next_enum_item = state->list;

	//temp: do not cache contents of empty directory
	//this can be just connection error
	if ( hasItems )
	{
		if ( env->dir_cache_list != NULL)
		{
			freeDirList( env->dir_cache_list );
		}

		strcpy( env->dir_cache, path_abs);
		copyDirList( &(env->dir_cache_list), state->list );
	}

    dir_changed = false;

	NET_PRINTF( "FTP_FindFirst() - Ok\n", 0 );

	return true;
}

//filename - full path, "\doc\file.dat" or "\doc\"
static bool FTP_PathInfo(const char* path_absolute, FTPDIRENTRY* dentry, ftp_env* env)
{
	FTPDIRSTATESTRUCT dirlist;
	char dirname[FTP_MAXPATH];
	char filename[FTP_MAXPATH];
	char fixed_path[FTP_MAXPATH];
	FTPDIRENTRYLISTITEM* item;

	NET_ASSERT( path_absolute != NULL );
	NET_ASSERT( dentry != NULL );
	NET_ASSERT( env != NULL );

	NET_PRINTF("FTP_PathInfo( path='%s' )\n", path_absolute );

	FTP_FixPath( &path_absolute, fixed_path );

	//list parent directory and return corresponding entry

	strcpy(dirname, path_absolute);
	ReplaceForwardSlash(dirname);

	if (strcmp(dirname,"/") == 0)
	{
		//root directory

		dentry->size = 0;
		dentry->isDirectory = true;
		strcpy(dentry->name,"/");
		return true;
	}

	//delete trailing '/'

	if (dirname[strlen(dirname)-1] == '/' )
	{
		dirname[ strlen(dirname)-1 ] = 0;
	}

	//trim line to last '/'
	char* c = dirname;
	char* c1 = NULL;
	while (*c != 0)
	{
		if (*c == '/') c1 = c;
		c++;
	}

	if (c1 == NULL)
	{
		//path_absolute = "file.dat"
		strcpy(filename,dirname);
		strcpy(dirname,"/");
	}
	else
	{
		//path_absolute = "dir/file.dat"
		strcpy(filename,c1+1);
		*(c1+1)=0;
	}


	if (FTP_FindFirst(dirname, &dirlist, env) == false)
	{
		return false;
	}

	for (item = dirlist.list; item!=NULL; item=(FTPDIRENTRYLISTITEM*)item->next)
	{
		if (strcmp(filename, item->item.name) == 0)
		{
			*dentry = item->item;
			FTP_FindClose(&dirlist);
			return true;
		}
	}

	FTP_FindClose(&dirlist);

	return false;
}

//filename - full path, no share name, "/doc/file.dat"
static bool FTP_OpenFile( const char* filename, FTPFILESTRUCT* file, ftp_env* env, int flags)
{
	FTPDIRENTRY dentry;
	char fixed_path[FTP_MAXPATH];

	NET_ASSERT( filename != NULL );
	NET_ASSERT( file != NULL );
	NET_ASSERT( env != NULL );

	NET_PRINTF("FTP_OpenFile( fileName='%s' )\n", filename );

	FTP_FixPath( &filename, fixed_path );


	if ( FTP_PathInfo( filename, &dentry, env ) == false )
	{
		NET_PRINTF("FTP_OpenFile( fileName='%s' ) - file does not exist\n", filename );

		if( ((flags & 0x03) == O_WRONLY) || ((flags & 0x03) == O_RDWR) )
		{
            file->envIndex = env->envIndex;
            file->offset = 0;
            file->size = 0;
            strcpy(file->filename, filename);
            return true;
		}

		return false;
	}

	file->envIndex = env->envIndex;
	file->offset = 0;
	file->size = dentry.size;
	strcpy(file->filename, filename);

	return true;
}

//assumption: offset+length range is inside file
//this method does not advance file->offset member
//assumption: offset + length segment is inside file
static bool FTP_ReadFile(void* buf, off_t len, off_t offset, FTPFILESTRUCT* file)
{
	NET_ASSERT( file != NULL );
	NET_ASSERT( offset + len <= file->size );

	NET_PRINTF( "FTP_ReadFile( filename = '%s', offset = %u:%u, len= %u:%u )\n", file->filename, (u32)(offset >> 32), (u32)(offset & 0xffffFFFF), (u32)(len >> 32), (u32)(len  & 0xffffFFFF));

	char buf2[FTP_MAXPATH + 6];
	int res;
	SOCKET data_sock = INVALID_SOCKET;
	ftp_env* env = &FTPEnv[file->envIndex];

	if ( len == 0 ) return true;

read_retry:

	sprintf(buf2, "RETR %s%s", env->share, file->filename);

	if( (res = ftp_execute_open(env, buf2, "I", offset, &data_sock)) < 0)
	{
		//Couldn't open data connection
		NET_PRINTF( "FTP_ReadFile( filename = '%s', offset = %u:%u, len= %u:%u ) - FAILED\n", file->filename, (u32)(offset>>32), (u32)(offset & 0xffffFFFF), (u32)(len>32), (u32)(len & 0xffffFFFF) );
		return false;
	}

	if( SocketRecv( data_sock, (char*)buf, len, false) < 0)
	{
		goto read_retry;
	}

	last_off = offset + len;

	return true;
}

static bool FTP_WriteFile(void* buf, off_t len, off_t offset, FTPFILESTRUCT* file)
{
	NET_ASSERT( file != NULL );
	NET_ASSERT( offset + len <= file->size );

	NET_PRINTF( "FTP_WriteFile( filename = '%s', offset = %u:%u, len= %u:%u )\n", file->filename, (u32)(offset >> 32), (u32)(offset & 0xffffFFFF), (u32)(len >> 32), (u32)(len  & 0xffffFFFF));

	char buf2[FTP_MAXPATH + 6];
	int res;
	SOCKET data_sock = INVALID_SOCKET;
	ftp_env* env = &FTPEnv[file->envIndex];

    _FTP_lock();

read_retry:

	sprintf(buf2, "STOR %s%s", env->share, file->filename);

	if( (res = ftp_execute_open(env, buf2, "I", offset, &data_sock)) < 0)
	{
		//Couldn't open data connection
		NET_PRINTF( "FTP_WriteFile( filename = '%s', offset = %u:%u, len= %u:%u ) - FAILED\n", file->filename, (u32)(offset>>32), (u32)(offset & 0xffffFFFF), (u32)(len>32), (u32)(len & 0xffffFFFF) );
		_FTP_unlock();
		return false;
	}

	if( SocketSend( data_sock, (char*)buf, len) == false)
	{
		goto read_retry;
	}

	last_off = offset + len;
    _FTP_unlock();

	return true;
}

static bool FTP_MkDir(const char * name, ftp_env * env)
{
	char buf2[FTP_MAXPATH + 6];
	int res;

	sprintf(buf2, "MKD %s%s", env->share, name);

	if((res = ftp_execute(env, buf2, 257, 0)) < 0)
	{
		//Couldn't create directory
		return false;
	}

	return true;
}

static bool FTP_DeleteFile(const char * name, ftp_env * env)
{
	char buf2[FTP_MAXPATH + 6];
	int res;

	sprintf(buf2, "DELE %s%s", env->share, name);

	if((res = ftp_execute(env, buf2, 250, 0)) < 0)
	{
		//Couldn't delete file
		return false;
	}

	return true;
}

static bool FTP_DeleteDir(const char * name, ftp_env * env)
{
	char buf2[FTP_MAXPATH + 6];
	int res;

    sprintf(buf2, "CDUP");

	if((res = ftp_execute(env, buf2, 250, 0)) < 0)
    {
        //Couldn't delete directory
        return false;
    }

    if(name[strlen(name)-1] == '/')
        ((char *) name)[strlen(name)-1] = 0;

    char * relativename = strrchr(name, '/');
    if(!relativename) {
        return false;
	}

	sprintf(buf2, "RMD %s", relativename+1);

	if((res = ftp_execute(env, buf2, 250, 0)) < 0)
	{
		//Couldn't delete directory
		return false;
	}

    ftp_close_data(env);

	return true;
}

static bool FTP_Rename(const char * oldpath_absolute, const char * path_absolute, ftp_env * env)
{
	char buf2[FTP_MAXPATH + 6];
	int res;

    sprintf(buf2, "RNFR %s%s", env->share, oldpath_absolute);

	if((res = ftp_execute(env, buf2, 350, 0)) < 0)
    {
        //Couldn't find
        return false;
    }

	sprintf(buf2, "RNTO %s%s", env->share, path_absolute);

	if((res = ftp_execute(env, buf2, 250, 0)) < 0)
	{
        //Couldn't rename
		return false;
	}

	return true;
}

static void FTP_CloseFile( FTPFILESTRUCT* file )
{
	NET_ASSERT( file != NULL );

	NET_PRINTF("FTP_CloseFile( fileName='%s' )\n", file->filename );

    ftp_env* env = &FTPEnv[file->envIndex];
    ftp_close_data(env);
	//nothing to do
}

//destroy readaheadcache
static void DestroyFTPReadAheadCache()
{

	int i;
	if (FTPReadAheadCache == NULL) return;

	NET_PRINTF("DestroyFTPReadAheadCache()\n", 0 );


	for (i = 0; i < numFTP_RA_pages; i++)
	{
		free(FTPReadAheadCache[i].ptr);
	}

	free(FTPReadAheadCache);
	FTPReadAheadCache = NULL;
	numFTP_RA_pages = 0;

}

//create readahead cache
//pages - number of cache pages
static void CreateFTPReadAheadCache(u32 pages)
{
	int i;

	DestroyFTPReadAheadCache();

	NET_PRINTF("CreateFTPReadAheadCache()\n", 0 );

	if (pages == 0) return;

	numFTP_RA_pages = pages;
	FTPReadAheadCache = (ftp_cache_page *) malloc(sizeof(ftp_cache_page) * numFTP_RA_pages);

	for (i = 0; i < numFTP_RA_pages; i++)
	{
		FTPReadAheadCache[i].offset = 0;
		FTPReadAheadCache[i].last_used = 0;
		FTPReadAheadCache[i].file = NULL;
//		FTPReadAheadCache[i].ptr = memalign(32, FTP_READ_BUFFERSIZE);
		FTPReadAheadCache[i].ptr = malloc(FTP_READ_BUFFERSIZE);
	}

}

//invalidate readahead cache for file
//called when file is closed
static void ClearFTPFileCache(FTPFILESTRUCT *file)
{
	int i;

	if (FTPReadAheadCache == NULL) return;

	for (i = 0; i < numFTP_RA_pages; i++)
	{
		if (FTPReadAheadCache[i].file == file)
		{
			FTPReadAheadCache[i].file = NULL;
		}
	}
}

//read from file, from current position, use readaheadcache
//calls FTP_ReadFile() if necessary
//this method does not advance 'offset' member in file structure
//assumption: offset+length range is inside file
static bool ReadFTPFromCache(void *buf, off_t len, FTPFILESTRUCT *file)
{
	int i, leastUsed;

	off_t rest;
	off_t new_offset;

	_FTP_lock();

	if (FTPReadAheadCache == NULL)
	{
		if ( FTP_ReadFile(buf, len, file->offset, file) == false )
		{
			_FTP_unlock();
			return false;
		}
		_FTP_unlock();
		return true;
	}

	new_offset = file->offset;
	rest = len;

	continue_read:

	for (i = 0; i < numFTP_RA_pages; i++)
	{
		if ( FTPReadAheadCache[i].file == file )
		{
			if ((new_offset >= FTPReadAheadCache[i].offset) &&
				(new_offset < (FTPReadAheadCache[i].offset + FTP_READ_BUFFERSIZE)))
			{
				//we hit the page
				//copy as much as we can
				FTPReadAheadCache[i].last_used = gettime();

				off_t buffer_used = (FTPReadAheadCache[i].offset + FTP_READ_BUFFERSIZE) - new_offset;
				if (buffer_used > rest) buffer_used = rest;
				memcpy(buf, FTPReadAheadCache[i].ptr + (new_offset - FTPReadAheadCache[i].offset), buffer_used);
				buf += buffer_used;
				rest -= buffer_used;
				new_offset += buffer_used;

				if (rest == 0)
				{
					_FTP_unlock();
					return true;
				}

				goto continue_read;
			}
		}
	}

	//find least used page
	leastUsed = 0;
	for ( i=1; i < numFTP_RA_pages; i++)
	{
		if ((FTPReadAheadCache[i].last_used < FTPReadAheadCache[leastUsed].last_used))
		{
			leastUsed = i;
		}
	}

	//fill least used page with new data

	off_t cache_offset = new_offset;

	//do not interset with existing pages
	for (i = 0; i < numFTP_RA_pages; i++)
	{
		if ( i == leastUsed ) continue;
		if ( FTPReadAheadCache[i].file != file ) continue;

		if ( (cache_offset < FTPReadAheadCache[i].offset ) && (cache_offset + FTP_READ_BUFFERSIZE > FTPReadAheadCache[i].offset) )
		{
			//tail of new page intersects with some cache block
			if ( FTPReadAheadCache[i].offset >= FTP_READ_BUFFERSIZE )
			{
				cache_offset = FTPReadAheadCache[i].offset - FTP_READ_BUFFERSIZE;
			}
			else
			{
				cache_offset = 0 ;
			}
		}

		if ( (cache_offset >= FTPReadAheadCache[i].offset ) && (cache_offset < FTPReadAheadCache[i].offset + FTP_READ_BUFFERSIZE ) )
		{
			//head of new page intersects with some cache block

			cache_offset = FTPReadAheadCache[i].offset + FTP_READ_BUFFERSIZE;

			NET_ASSERT( cache_offset < file->size );
		}
	}

	off_t cache_to_read = file->size - cache_offset;
	if ( cache_to_read > FTP_READ_BUFFERSIZE )
	{
		cache_to_read = FTP_READ_BUFFERSIZE;
	}

	if ( FTP_ReadFile(FTPReadAheadCache[leastUsed].ptr, cache_to_read, cache_offset, file) == false )
	{
		FTPReadAheadCache[leastUsed].file = NULL;
		_FTP_unlock();
		return false;
	}

	FTPReadAheadCache[leastUsed].last_used = gettime();

	FTPReadAheadCache[leastUsed].offset = cache_offset;
	FTPReadAheadCache[leastUsed].file = file;

	goto continue_read;
}

//return absolute path with no device name
//if relative path, 'exchange/file.dat'   ->   currentpath+'exchange/file.dat'
//if absolute path, '/exchange/file.dat'   ->   '/exchange/file.dat'
//if '' ->  '/'
//if invalid path -> '/'
//all '\' replaced to '/'
static void ftp_absolute_path_no_device(const char *srcpath, char *destpath, int envIndex)
{
	//skip 'ftp1:'
	if (strchr(srcpath, ':') != NULL)
	{
		srcpath = strchr(srcpath, ':') + 1;
	}

	if (strchr(srcpath, ':') != NULL)
	{
		//invalid path, with two ':' in path
		strcpy(destpath, "/");
		return;
	}


	if (srcpath[0] != '\\' && srcpath[0] != '/')
	{
		//relative path, 'exchange/file.dat'   ->   currentpath+'exchange/file.dat'
		//append to current path and return
		strcpy(destpath, FTPEnv[envIndex].currentpath);
		strcat(destpath, srcpath);
	}
	else
	{
		//absolute path, '/exchange/file.dat'   ->   '/exchange/file.dat'
		strcpy(destpath, srcpath);
	}

	if (destpath[0] == 0 )
	{
		strcpy(destpath, "/" );
	}

	ReplaceForwardSlash( destpath );
}



// 'ftp1://exchange/list.dat'   ->   'ftp1'
static char *ExtractDevice(const char *path, char *device)
{
	int i,l;
	l=strlen(path);

	for(i=0;i<l && path[i]!='\0' && path[i]!=':' && i < 20;i++)
		device[i]=path[i];
	if(path[i]!=':')device[0]='\0';
	else device[i]='\0';
	return device;
}

//FILE IO
static int __ftp_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	NET_PRINTF("__ftp_open( path='%s' )\n", path );
	FTPFILESTRUCT *file = (FTPFILESTRUCT*) fileStruct;

	char fixedpath[FTP_MAXPATH];

	ftp_env *env;

	ExtractDevice(path,fixedpath);

	if(fixedpath[0]=='\0')
	{
		getcwd(fixedpath,FTP_MAXPATH);		//review: getcwd, or env->currentdirectory?
		ExtractDevice(fixedpath,fixedpath);
	}

	env=FindFTPEnv(fixedpath);
	if (env==NULL)
	{
		r->_errno = ENODEV;
		return -1;
	}


	ftp_absolute_path_no_device(path, fixedpath, file->envIndex);

	NET_PRINTF("absolutepath='%s'\n", fixedpath );

	_FTP_lock();
	bool res = FTP_OpenFile(fixedpath, file, env, flags);
	_FTP_unlock();


	if ( res == false )
	{
		r->_errno = ENOENT;
		return -1;
	}
	return 0;
}

static off_t __ftp_seek(struct _reent *r, void *fd, off_t pos, int dir)
{
	off_t position;
	FTPFILESTRUCT *file = (FTPFILESTRUCT*) fd;

	if (file == NULL)
	{
		NET_PRINTF( "__ftp_seek( file=NULL )\n",0 );
		r->_errno = EBADF;
		return -1;
	}

	NET_PRINTF("__ftp_seek( file='%s', pos=%u:%u, dir=%d )\n", file->filename, (u32)(pos>>32), (u32)(pos & 0xffffFFFF),  dir );

	switch (dir)
	{
	case SEEK_SET:
		position = pos;
		break;
	case SEEK_CUR:
		position = file->offset + pos;
		break;
	case SEEK_END:
		position = file->size + pos;
		break;
	default:
		r->_errno = EINVAL;
		return -1;
	}

	if (((pos > 0) && (position < 0)) || (position > file->size))
	{
		r->_errno = EOVERFLOW;
		return -1;
	}
	if (position < 0)
	{
		r->_errno = EINVAL;
		return -1;
	}

	// Save position
	file->offset = position;
	return position;
}

static ssize_t __ftp_read(struct _reent *r, void *fd, char *ptr, size_t len)
{
	FTPFILESTRUCT *file = (FTPFILESTRUCT*) fd;

	if (file == NULL)
	{
		NET_PRINTF( "__ftp_read( file=NULL )\n",0 );
		r->_errno = EBADF;
		return -1;
	}

	//	NET_PRINTF("__ftp_read( filename='%s',offset=%u:%u, len=%u:%u )\n", file->filename, (u32)(file->offset >> 32), (u32)(file->offset & 0xffffFFFF), (u32)(0), (u32)(len  & 0xffffFFFF) );

	// Don't try to read if the read pointer is past the end of file
	if (file->offset >= file->size)
	{
		r->_errno = EOVERFLOW;
		return -1;
	}

	// Don't read past end of file
	if (len + file->offset > file->size)
	{
		len = file->size - file->offset;
	}

	// Short circuit cases where len is 0 (or less)
	if (len == 0)
	{
		return 0;
	}

	if (ReadFTPFromCache(ptr, len, file) == false)
	{
		r->_errno = EIO;
		return -1;
	}

	file->offset += len;

	return len;
}

static ssize_t __ftp_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
	FTPFILESTRUCT *file = (FTPFILESTRUCT*) fd;
	if (file == NULL)
	{
		NET_PRINTF( "__ftp_write( file=NULL )\n",0 );
		r->_errno = EBADF;
		return -1;
	}

	// Short circuit cases where len is 0 (or less)
	if (len == 0)
	{
		return 0;
	}

	if (FTP_WriteFile((char *) ptr, len, file->offset, file) == false)
	{
		r->_errno = EIO;
		return -1;
	}

	file->offset += len;

	dir_changed = true;

	return len;
}

static int __ftp_close(struct _reent *r, void *fd)
{
	NET_PRINTF("__ftp_close()\n", 0 );

	FTPFILESTRUCT *file = (FTPFILESTRUCT*) fd;
	_FTP_lock();
	ClearFTPFileCache(file);
	FTP_CloseFile(file);
	_FTP_unlock();

	return 0;
}

static int __ftp_mkdir(struct _reent *r, const char *name, int mode)
{
    NET_PRINTF("__ftp_mkdir()\n",0);

	char path_absolute[FTP_MAXPATH];

	ExtractDevice(name,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	ftp_env* env;
	env=FindFTPEnv(path_absolute);
	if (env == NULL)
	{
		r->_errno = EINVAL;
		return -1;
	}

	ftp_absolute_path_no_device(name, path_absolute, env->envIndex);

	_FTP_lock();
	if ( FTP_MkDir(path_absolute, env) == false)
	{
		_FTP_unlock();
		r->_errno = ENOTDIR;
		return -1;
	}
	_FTP_unlock();

	dir_changed = true;

	return 0;
}

static int __ftp_unlink(struct _reent *r, const char *name)
{
	FTPDIRENTRY dentry;
	char path_absolute[FTP_MAXPATH];

	ExtractDevice(name,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	ftp_env* env;
	env=FindFTPEnv(path_absolute);
	if (env == NULL)
	{
		r->_errno = EINVAL;
		return -1;
	}

	ftp_absolute_path_no_device(name, path_absolute, env->envIndex);

	if ( FTP_PathInfo(path_absolute, &dentry, env ) == false )
	{
		r->_errno = EBADF;
	    return -1;
	}

	_FTP_lock();
	if (dentry.isDirectory)
	{
	    if(FTP_DeleteDir(path_absolute, env) == false)
        {
            _FTP_unlock();
            r->_errno = ENOTDIR;
            return -1;
        }
	}
    else
    {
	    if(FTP_DeleteFile(path_absolute, env) == false)
        {
            _FTP_unlock();
            r->_errno = ENOTDIR;
            return -1;
        }
	}
	_FTP_unlock();

	dir_changed = true;

	return 0;
}

static int __ftp_rename(struct _reent *r, const char *oldName, const char *newName)
{
	char oldpath_absolute[FTP_MAXPATH];

	ExtractDevice(oldName,oldpath_absolute);
	if(oldpath_absolute[0]=='\0')
	{
		getcwd(oldpath_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(oldpath_absolute,oldpath_absolute);
	}

	char path_absolute[FTP_MAXPATH];

	ExtractDevice(newName,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	ftp_env* env;
	env=FindFTPEnv(oldpath_absolute);
	if (env == NULL)
	{
		r->_errno = EINVAL;
		return -1;
	}

	ftp_absolute_path_no_device(newName, path_absolute, env->envIndex);

	ftp_absolute_path_no_device(oldName, oldpath_absolute, env->envIndex);

	_FTP_lock();

    if(FTP_Rename(oldpath_absolute, path_absolute, env) == false)
    {
        _FTP_unlock();
        r->_errno = ENOTDIR;
        return -1;
    }

    _FTP_unlock();

	dir_changed = true;

	return 0;
}

static int __ftp_chdir(struct _reent *r, const char *path)
{
	NET_PRINTF("__ftp_chdir( path='%s' )\n", path );

	char path_absolute[FTP_MAXPATH];

	ExtractDevice(path,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	ftp_env* env;
	env=FindFTPEnv(path_absolute);  //path_absolute holds device name here
	if (env == NULL)
	{
		r->_errno = EINVAL;
		return -1;
	}

	ftp_absolute_path_no_device(path, path_absolute,env->envIndex);
	//path_absolute holds absolute path here

	//if directory has been specified as 'exchange/name', change to 'exchange/name/'
	//ftp_absolute_path_no_device() does not do this
	strcpy(env->currentpath, path_absolute);
	if (env->currentpath[strlen(env->currentpath) - 1] != '/')
			strcat(env->currentpath, "/");

	return 0;
}

static int __ftp_dirreset(struct _reent *r, DIR_ITER *dirState)
{
	char path_abs[FTP_MAXPATH];
	FTPDIRSTATESTRUCT* state = (FTPDIRSTATESTRUCT*) (dirState->dirStruct);
	FTPDIRENTRY dentry;

	memset(&dentry, 0, sizeof(FTPDIRENTRY));

	_FTP_lock();
	FTP_FindClose(state);

	strcpy(path_abs,FTPEnv[state->envIndex].currentpath);

	NET_PRINTF("__ftp_dirreset( path='%s' )\n", path_abs );

	FTP_FindFirst(path_abs, state, &FTPEnv[state->envIndex]);
	_FTP_unlock();

	return 0;
}

static DIR_ITER* __ftp_diropen(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	NET_PRINTF("__ftp_diropen()\n",0);

	char path_absolute[FTP_MAXPATH];
	FTPDIRSTATESTRUCT* state = (FTPDIRSTATESTRUCT*) (dirState->dirStruct);

	ExtractDevice(path,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);			//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	ftp_env* env;
	env=FindFTPEnv(path_absolute);
	if (env == NULL)
	{
		r->_errno = EINVAL;
		return NULL;
	}

	ftp_absolute_path_no_device(path, path_absolute, env->envIndex);

	if (path_absolute[strlen(path_absolute) - 1] != '/')
	{
		strcat(path_absolute, "/");
	}

	NET_PRINTF("__ftp_diropen( path='%s' )\n", path_absolute );

	_FTP_lock();
	if ( FTP_FindFirst(path_absolute, state, env) == false)
	{
		_FTP_unlock();
		r->_errno = ENOTDIR;
		return NULL;
	}

	_FTP_unlock();

	return dirState;
}

static int dentry_to_stat(FTPDIRENTRY *dentry, struct stat *st)
{
	if (!st)
		return -1;
	if (!dentry)
		return -1;

	st->st_dev = 0;
	st->st_ino = 0;

	st->st_mode = ((dentry->isDirectory) ? S_IFDIR : S_IFREG);
	st->st_nlink = 1;
	st->st_uid = 1; // Faked
	st->st_rdev = st->st_dev;
	st->st_gid = 2; // Faked
	st->st_size = dentry->size;
	st->st_atime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	st->st_blksize = 1024;
	st->st_blocks = (st->st_size + st->st_blksize - 1) / st->st_blksize; // File size in blocks

	return 0;
}

static int __ftp_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	NET_PRINTF("__ftp_dirnext( )\n",0 );

	FTPDIRSTATESTRUCT* state = (FTPDIRSTATESTRUCT*) (dirState->dirStruct);

	if (filestat == NULL || state->next_enum_item == NULL)
	{
		r->_errno = ENOENT;
		NET_PRINTF("__ftp_dirnext( ) - NULL or next NULL\n",0 );
		return -1;
	}

	strcpy(filename, state->next_enum_item->item.name);

	NET_PRINTF("__ftp_dirnext( ): %s\n", filename );

	dentry_to_stat(&state->next_enum_item->item, filestat);

	state->next_enum_item = state->next_enum_item->next;

	return 0;
}

static int __ftp_dirclose(struct _reent *r, DIR_ITER *dirState)
{
	NET_PRINTF("__ftp_dirclose(  )\n",0 );

	FTPDIRSTATESTRUCT* state = (FTPDIRSTATESTRUCT*) (dirState->dirStruct);

	FTP_FindClose(state);

	memset(state, 0, sizeof(FTPDIRSTATESTRUCT));

	return 0;
}

static int __ftp_stat(struct _reent *r, const char *path, struct stat *st)
{
	NET_PRINTF("__ftp_stat( path='%s' )\n", path );

	char path_absolute[FTP_MAXPATH];
	FTPDIRENTRY dentry;

	ExtractDevice(path,path_absolute);
	if(path_absolute[0]=='\0')
	{
		getcwd(path_absolute,FTP_MAXPATH);				//review: getcwd, or env->currentdirectory?
		ExtractDevice(path_absolute,path_absolute);
	}

	NET_PRINTF("absolutpath='%s'\n", path_absolute );

	ftp_env* env;
	env=FindFTPEnv(path_absolute);

	ftp_absolute_path_no_device(path, path_absolute, env->envIndex);

	_FTP_lock();

	memset(st, 0, sizeof(stat));

	if ( FTP_PathInfo(path_absolute, &dentry, env) == false )
	{
		_FTP_unlock();
		r->_errno = ENOENT;
		return -1;
	}
	_FTP_unlock();

	if (dentry.name[0] == '\0')
	{
		r->_errno = ENOENT;
		return -1;
	}

	dentry_to_stat(&dentry, st);

	return 0;
}

static int __ftp_fstat(struct _reent *r, void *fd, struct stat *st)
{
	FTPFILESTRUCT *filestate = (FTPFILESTRUCT *) fd;

	if (!filestate)
	{
		r->_errno = EBADF;
		return -1;
	}

	memset(st, 0, sizeof(stat));

	st->st_size = filestate->size;

	return 0;
}

static void MountDevice(const char *name, int envIndex)
{
	NET_PRINTF("MountDevice( path='%s' )\n", name );

	devoptab_t *dotab_ftp;

	dotab_ftp=(devoptab_t*)malloc(sizeof(devoptab_t));

	dotab_ftp->name=strdup(name);
	dotab_ftp->structSize=sizeof(FTPFILESTRUCT); // size of file structure
	dotab_ftp->open_r=__ftp_open; // device open
	dotab_ftp->close_r=__ftp_close; // device close
	dotab_ftp->write_r=__ftp_write; // device write
	dotab_ftp->read_r=__ftp_read; // device read
	dotab_ftp->seek_r=__ftp_seek; // device seek
	dotab_ftp->fstat_r=__ftp_fstat; // device fstat
	dotab_ftp->stat_r=__ftp_stat; // device stat
	dotab_ftp->link_r=NULL; // device link
	dotab_ftp->unlink_r=__ftp_unlink; // device unlink
	dotab_ftp->chdir_r=__ftp_chdir; // device chdir
	dotab_ftp->rename_r=__ftp_rename; // device rename
	dotab_ftp->mkdir_r=__ftp_mkdir; // device mkdir

	dotab_ftp->dirStateSize=sizeof(FTPDIRSTATESTRUCT); // dirStateSize
	dotab_ftp->diropen_r=__ftp_diropen; // device diropen_r
	dotab_ftp->dirreset_r=__ftp_dirreset; // device dirreset_r
	dotab_ftp->dirnext_r=__ftp_dirnext; // device dirnext_r
	dotab_ftp->dirclose_r=__ftp_dirclose; // device dirclose_r
	dotab_ftp->statvfs_r=NULL;			// device statvfs_r
	dotab_ftp->ftruncate_r=NULL;               // device ftruncate_r
	dotab_ftp->fsync_r=NULL;           // device fsync_r
	dotab_ftp->deviceData=NULL;       	/* Device data */

	AddDevice(dotab_ftp);

	FTPEnv[envIndex].devoptab = dotab_ftp;
}

bool ftpInitDevice(const char* name, const char *user, const char *password, const char *share, const char *hostname, unsigned short port, bool ftp_passive)
{
	NET_ASSERT( name != NULL );
	NET_ASSERT( user != NULL );
	NET_ASSERT( password != NULL );
	NET_ASSERT( share != NULL );
	NET_ASSERT( hostname != NULL );

	NET_PRINTF("ftpInitDevice( name='%s', user='%s', password='%s', share='%s', hostname='%s', port='%i' )\n", name, user, password, share, hostname, port);

	int i;

	if(FirstInit)
	{
		for(i=0;i<MAX_FTP_MOUNTED;i++)
		{
			FTPEnv[i].name= NULL;
			FTPEnv[i].currentpath[0]='\\';
			FTPEnv[i].currentpath[1]='\0';
			FTPEnv[i].envIndex=i;
		}

		LWP_MutexInit(&_FTP_mutex, false);

		CreateFTPReadAheadCache( FTP_CACHE_PAGES );

		FirstInit=false;

	}

	for(i=0;i<MAX_FTP_MOUNTED && FTPEnv[i].name!=NULL;i++);
	if(i==MAX_FTP_MOUNTED) return false; //all allowed ftp connections reached

	if (if_config(bba_ip, NULL, NULL, true) < 0)
		return false;

	_FTP_lock();

	if (FTP_Connect(&FTPEnv[i], name, user, password, share, hostname, port, ftp_passive) == false)
	{
		_FTP_unlock();
		return false;
	}

	_FTP_unlock();

	MountDevice(name,i);

	return true;
}

void ftpClose(const char* name)
{
	NET_PRINTF("ftpClose( name='%s' )\n", name );

	ftp_env *env;
	env=FindFTPEnv(name);
	if(env==NULL) return;

	RemoveDevice(env->name);

	_FTP_lock();
	FTP_Close(env);
	_FTP_unlock();
}

bool CheckFTPConnection(const char* name)
{
	NET_PRINTF("CheckFTPConnection( name='%s' )\n", name );

	//reconnection not implemented
	//if initial connection is not established, we do not reconnect here
	//but run-time reconnection does work without any special requests

	return ( FindFTPEnv( name ) != NULL );
}
