#ifndef _LIBFTF_H
#define _LIBFTF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FTP_MOUNTED 5

#ifdef FTP_DEBUG
#define NET_PRINTF(fmt, args...)  print_gecko(fmt, args)
#define NET_ASSERT(x) assert(x)
#else
#define NET_PRINTF(fmt, args...)  do{}while(0)
#define NET_ASSERT(x) do{}while(0)
#endif


//devoptab
bool ftpInitDevice(const char* name, const char *user, const char *password,
					const char *share, const char *hostname, unsigned short port,
                    bool ftp_passive);
void ftpClose(const char* name);
bool CheckFTPConnection(const char* name);


#ifdef __cplusplus
}
#endif

#endif /* _LIBFTF_H */
