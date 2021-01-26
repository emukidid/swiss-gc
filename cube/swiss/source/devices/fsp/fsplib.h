#ifndef _FSPLIB_H
#define _FSPLIB_H 1
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stddef.h>

/* The FSP v2 protocol support library - public interface */

/*
This file is part of fsplib - FSP protocol stack implemented in C
language. See http://fsp.sourceforge.net for more information.

Copyright (c) 2003-2005 by Radim HSN Kolar (hsn@sendmail.cz)

You may copy or modify this file in any manner you wish, provided
that this notice is always included, and that you hold the author
harmless for any loss or damage resulting from the installation or
use of this software.

                     This is a free software.  Be creative.
                    Let me know of any bugs and suggestions.
*/

/* definition of FSP protocol v2 commands */

#define FSP_CC_VERSION      0x10    /* return server's version string.      */
#define FSP_CC_INFO         0x11    /* return server's extended info block  */
#define FSP_CC_ERR          0x40    /* error response from server.          */
#define FSP_CC_GET_DIR      0x41    /* get a directory listing.             */
#define FSP_CC_GET_FILE     0x42    /* get a file.                          */
#define FSP_CC_UP_LOAD      0x43    /* open a file for writing.             */
#define FSP_CC_INSTALL      0x44    /* close a file opened for writing.     */
#define FSP_CC_DEL_FILE     0x45    /* delete a file.                       */
#define FSP_CC_DEL_DIR      0x46    /* delete a directory.                  */
#define FSP_CC_GET_PRO      0x47    /* get directory protection.            */
#define FSP_CC_SET_PRO      0x48    /* set directory protection.            */
#define FSP_CC_MAKE_DIR     0x49    /* create a directory.                  */
#define FSP_CC_BYE          0x4A    /* finish a session.                    */
#define FSP_CC_GRAB_FILE    0x4B    /* atomic get+delete a file.            */
#define FSP_CC_GRAB_DONE    0x4C    /* atomic get+delete a file done.       */
#define FSP_CC_STAT         0x4D    /* get information about file.          */
#define FSP_CC_RENAME       0x4E    /* rename file or directory.            */
#define FSP_CC_CH_PASSWD    0x4F    /* change password                      */
#define FSP_CC_LIMIT        0x80    /* # > 0x7f for future cntrl blk ext.   */
#define FSP_CC_TEST         0x81    /* reserved for testing                 */

/* FSP v2 packet size */
#define FSP_HSIZE 12                           /* 12 bytes for v2 header */
#define FSP_SPACE 1024                         /* maximum payload.       */
#define FSP_MAXPACKET   FSP_HSIZE+FSP_SPACE    /* maximum packet size.   */

/* byte offsets of fields in the FSP v2 header */
#define FSP_OFFSET_CMD          0
#define FSP_OFFSET_SUM          1
#define FSP_OFFSET_KEY          2
#define FSP_OFFSET_SEQ          4
#define FSP_OFFSET_LEN          6
#define FSP_OFFSET_POS          8

/* types of directory entry */ 
#define FSP_RDTYPE_END      0x00
#define FSP_RDTYPE_FILE     0x01
#define FSP_RDTYPE_DIR      0x02
#define FSP_RDTYPE_LINK	    0x03
#define FSP_RDTYPE_SKIP     0x2A

/* definition of directory bitfield for directory information */
/* directory information is just going to be a bitfield encoding
 * of which protection bits are set/unset
 */

#define FSP_PRO_BYTES	1	/* currently only 8 bits or less of info  */
#define FSP_DIR_OWNER	0x01	/* does caller own directory              */
#define FSP_DIR_DEL	0x02	/* can files be deleted from this dir     */
#define FSP_DIR_ADD	0x04	/* can files be added to this dir         */
#define FSP_DIR_MKDIR	0x08	/* can new subdirectories be created      */
#define FSP_DIR_GET	0x10	/* are files readable by non-owners?      */
#define FSP_DIR_README	0x20	/* does this dir contain an readme file?  */
#define FSP_DIR_LIST    0x40    /* public can list directory              */
#define FSP_DIR_RENAME  0x80    /* can files be renamed in this dir       */
 
/* decoded FSP packet */
typedef struct FSP_PKT {
                        unsigned char       cmd; /* message code.             */
                        unsigned char       sum; /* message checksum.         */
                        unsigned short      key; /* message key.              */
                        unsigned short      seq; /* message sequence number.  */
                        unsigned short      len; /* number of bytes in buf 1. */
                        unsigned int        pos; /* location in the file.     */                        unsigned short     xlen; /* number of bytes in buf 2  */

                        unsigned char   buf[FSP_SPACE];   /* packet payload */
              } FSP_PKT;

/* FSP host:port */
typedef struct FSP_SESSION {
			void *   lock;            /* key locking         */
                        unsigned int   timeout;   /* send timeout 1/1000s*/
			unsigned int   maxdelay;  /* maximum recv. delay */
                        unsigned short seq;       /* sequence number     */
                        unsigned int dupes;       /* total pkt. dupes    */
                        unsigned int resends;     /* total pkt. sends    */
                        unsigned int trips;       /* total pkt trips     */
                        unsigned long rtts;       /* cumul. rtt          */
                        unsigned int last_rtt;    /* last rtt            */
                        unsigned int last_delay;  /* last delay time     */
                        unsigned int last_dupes;  /* last dupes          */
                        unsigned int last_resends;/* last resends        */
                        int fd;                   /* i/o descriptor      */
                        char *password;           /* host acccess password */
                } FSP_SESSION;

/* fsp directory handle */
typedef struct FSP_DIR {
                        char   *dirname;          /* directory name */
                        short   inuse;            /* in use counter */
                        int     dirpos;           /* current directory pos. */
                        unsigned short blocksize; /* size of directory block */
                        unsigned char  *data;     /* raw directory data */
                        unsigned int   datasize;  /* size of raw dir. data */
} FSP_DIR;

/* fsp directory entry */
typedef struct FSP_RDENTRY  {
                       char name[255 + 1];        /* entry name */
		       unsigned short namlen;     /* length     */
		       unsigned char type;        /* field type */
		       unsigned short reclen;     /* directory record length */
		       unsigned int  size;
		       unsigned int  lastmod;
} FSP_RDENTRY;

/* fsp file handle */
typedef struct FSP_FILE {
    		      FSP_PKT in,out;            /* io packets */
		      FSP_SESSION *s;            /* master session */
		      char *name;                /* filename for upload */
		      unsigned char writing;     /* opened for writing */
		      unsigned char eof;         /* EOF reached? */
		      unsigned char err;         /* i/o error? */
		      int bufpos;                /* position in buffer */
		      unsigned int pos;          /* position of next packet */
} FSP_FILE;


typedef union dirent_workaround {
      struct dirent dirent;
      char fill[offsetof (struct dirent, d_name) + NAME_MAX + 1];
} dirent_workaround;
 
/* function prototypes */

/* session management */
FSP_SESSION * fsp_open_session(const char *host,unsigned short port, const char *password);
void fsp_close_session(FSP_SESSION *s);

/* packet encoding/decoding */
size_t fsp_pkt_write(const FSP_PKT *p,void *space);
int fsp_pkt_read(FSP_PKT *p,const void *space,size_t recv_len);

/* send/receive round-trip */
int fsp_transaction(FSP_SESSION *s,FSP_PKT *p,FSP_PKT *rpkt);

/* directory listing commands */
FSP_DIR * fsp_opendir(FSP_SESSION *s,const char *dirname);
int fsp_readdir_r(FSP_DIR *dir,struct dirent *entry, struct dirent **result);
long fsp_telldir(FSP_DIR *dirp);
void fsp_seekdir(FSP_DIR *dirp, long loc);
void fsp_rewinddir(FSP_DIR *dirp);
struct dirent * fsp_readdir(FSP_DIR *dirp);
int fsp_readdir_native(FSP_DIR *dir,FSP_RDENTRY *entry, FSP_RDENTRY **result);
int fsp_closedir(FSP_DIR *dirp);
/* high level  file i/o */
FSP_FILE * fsp_fopen(FSP_SESSION *session, const char *path,const char *modeflags);
size_t fsp_fread(void *ptr,size_t size,size_t nmemb,FSP_FILE *file);
size_t fsp_fwrite(const void * source, size_t size, size_t count, FSP_FILE * file);
int fsp_fclose(FSP_FILE *file);
int fsp_fpurge(FSP_FILE *file);
int fsp_fflush(FSP_FILE *file);
int fsp_fseek(FSP_FILE *stream, long offset, int whence);
long fsp_ftell(FSP_FILE *f);
void fsp_rewind(FSP_FILE *f);
/* misc. functions */
int fsp_stat(FSP_SESSION *s,const char *path,struct stat *sb);
int fsp_mkdir(FSP_SESSION *s,const char *directory);
int fsp_rmdir(FSP_SESSION *s,const char *directory);
int fsp_unlink(FSP_SESSION *s,const char *directory);
int fsp_rename(FSP_SESSION *s,const char *from, const char *to);
int fsp_access(FSP_SESSION *s,const char *path, int mode);
/* fsp protocol specific functions */
int fsp_getpro(FSP_SESSION *s,const char *directory,unsigned char *result);
int fsp_install(FSP_SESSION *s,const char *fname,time_t timestamp);
int fsp_canupload(FSP_SESSION *s,const char *fname);
#endif
