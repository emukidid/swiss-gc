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
#include <sys/types.h>
#include <network.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "fsplib.h"
#include "lock.h"

/* ************ Internal functions **************** */ 

/* builds filename in packet output buffer, appends password if needed */
static int buildfilename(const FSP_SESSION *s,FSP_PKT *out,const char *dirname)
{
    int len;
    
    len=strlen(dirname);
    if(len >= FSP_SPACE - 1)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    /* copy name + \0 */
    memcpy(out->buf,dirname,len+1);
    out->len=len;
    if(s->password)
    {
        out->buf[len]='\n';
        out->len++;
        
        len=strlen(s->password);
        if(out->len+ len >= FSP_SPACE -1 )
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        memcpy(out->buf+out->len,s->password,len+1);
        out->len+=len;
    }
    /* add terminating \0 */
    out->len++;
    return 0;
}

/* simple FSP command */
static int simplecommand(FSP_SESSION *s,const char *directory,unsigned char command)
{
   FSP_PKT in,out;
   
   if(buildfilename(s,&out,directory))
       return -1;
   out.cmd=command;
   out.xlen=0;
   out.pos=0;

   if(fsp_transaction(s,&out,&in))
       return -1;
   
   if(in.cmd == FSP_CC_ERR)
   {
       errno = EPERM;
       return -1;
   }

   errno = 0;
   return  0;
}
/* Get directory part of filename. You must free() the result */
static char * directoryfromfilename(const char *filename)
{
    char *result;
    char *tmp;
    int pos;

    result=strrchr(filename,'/');
    if (result == NULL)
        return strdup("");
    pos=result-filename;
    tmp=malloc(pos+1);
    if(!tmp)
        return NULL;
    memcpy(tmp,filename,pos);
    tmp[pos]='\0';
    return tmp;         
}

/* ************  Packet encoding / decoding *************** */

/* write binary representation of FSP packet p into *space. */
/* returns number of bytes used or zero on error            */
/* Space must be long enough to hold created packet.        */
/* Maximum created packet size is FSP_MAXPACKET             */

size_t fsp_pkt_write(const FSP_PKT *p,void *space)
{
    size_t used;
    unsigned char *ptr;
    int checksum;
    size_t i;

    if(p->xlen + p->len > FSP_SPACE )
    {
        /* not enough space */
        errno = EMSGSIZE;
        return 0;
    }
    ptr=space;
    /* pack header */
    ptr[FSP_OFFSET_CMD]=p->cmd;
    ptr[FSP_OFFSET_SUM]=0;
    *(uint16_t *)(ptr+FSP_OFFSET_KEY)=htons(p->key);
    *(uint16_t *)(ptr+FSP_OFFSET_SEQ)=htons(p->seq);
    *(uint16_t *)(ptr+FSP_OFFSET_LEN)=htons(p->len);
    *(uint32_t *)(ptr+FSP_OFFSET_POS)=htonl(p->pos);
    used=FSP_HSIZE;
    /* copy data block */
    memcpy(ptr+FSP_HSIZE,p->buf,p->len);
    used+=p->len;
    /* copy extra data block */
    memcpy(ptr+used,p->buf+p->len,p->xlen);
    used+=p->xlen;
    /* compute checksum */
    checksum = 0;
    for(i=0;i<used;i++)
    {
        checksum += ptr[i];
    }
    checksum +=used;
    ptr[FSP_OFFSET_SUM] =  checksum + (checksum >> 8);
    return used;
}

/* read binary representation of FSP packet received from network into p  */
/* return zero on success */
int fsp_pkt_read(FSP_PKT *p,const void *space,size_t recv_len)
{
    int mysum;
    size_t i;
    const unsigned char *ptr;
    
    if(recv_len<FSP_HSIZE)
    {
        /* too short */
        errno = ERANGE;
        return -1;
    }
    if(recv_len>FSP_MAXPACKET)
    {
        /* too long */
        errno = EMSGSIZE;
        return -1;
    }

    ptr=space;
    /* check sum */
    mysum=-ptr[FSP_OFFSET_SUM];
    for(i=0;i<recv_len;i++)
    {
        mysum+=ptr[i];
    }

   mysum = (mysum + (mysum >> 8)) & 0xff;

   if(mysum != ptr[FSP_OFFSET_SUM])
   {
       /* checksum failed */
       errno = EIO;
       return -1;
   }

   /* unpack header */
   p->cmd=ptr[FSP_OFFSET_CMD];
   p->sum=mysum;
   p->key=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_KEY) );
   p->seq=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_SEQ) );
   p->len=ntohs( *(const uint16_t *)(ptr+FSP_OFFSET_LEN) );
   p->pos=ntohl( *(const uint32_t *)(ptr+FSP_OFFSET_POS) );
   if(p->len > recv_len)
   {
       /* bad length field, should not never happen */
       errno = EMSGSIZE;
       return -1;
   }   
   p->xlen=recv_len - p->len - FSP_HSIZE;
   /* now copy data */
   memcpy(p->buf,ptr+FSP_HSIZE,recv_len - FSP_HSIZE);
   return 0;
}

/* ****************** packet sending functions ************** */

/* make one send + receive transaction with server */
/* outgoing packet is in p, incomming in rpkt */
int fsp_transaction(FSP_SESSION *s,FSP_PKT *p,FSP_PKT *rpkt)
{
    char buf[FSP_MAXPACKET];
    size_t l;
    ssize_t r;
    fd_set mask;
    struct timeval start[8],stop;
    int i;
    unsigned int retry,dupes;
    int w_delay; /* how long to wait on next packet */
    int f_delay; /* how long to wait after first send */
    int l_delay; /* last delay */
    unsigned int t_delay; /* time from first send */
    

    if(p == rpkt)
    {
        errno = EINVAL;
        return -2;
    }
    FD_ZERO(&mask);
    /* get the next key */
    p->key = client_get_key((FSP_LOCK *)s->lock);

    retry = random() & 0xfff8;
    if (s->seq == retry)
        s->seq ^= 0x1080;
    else
        s->seq = retry; 
    dupes = retry = 0;
    t_delay = 0;
    /* compute initial delay here */
    /* we are using hardcoded value for now */
    f_delay = 1340;
    l_delay = 0;
    for(;;retry++)
    {
        if(t_delay >= s->timeout)
        {
            client_set_key((FSP_LOCK *)s->lock,p->key);
            errno = ETIMEDOUT;
            return -1;
        }
        /* make a packet */
        p->seq = (s->seq) | (retry & 0x7);
        l=fsp_pkt_write(p,buf);
        
        /* We should compute next delay wait time here */
        gettimeofday(&start[retry & 0x7],NULL);
        if(retry == 0 )
            w_delay=f_delay;
        else
        {
            w_delay=l_delay*3/2; 
        }

        l_delay=w_delay;

        /* send packet */
        r=net_send(s->fd,buf,l,0);
        if(r < 0 )
        {
            if(r == -ENOTSOCK)
            {
                client_set_key((FSP_LOCK *)s->lock,p->key);
                errno = -r;
                return -1;
            }
            /* io terror */
            sleep(1);
            /* avoid wasting retry slot */
            retry--;
            t_delay += 1000;
            continue; 
        }

        /* keep delay value within sane limits */
        if (w_delay > (int) s->maxdelay) 
            w_delay=s->maxdelay;
        else
            if(w_delay < 1000 ) 
                w_delay = 1000;

        t_delay += w_delay;
        /* receive loop */
        while(1)
        {
            if(w_delay <= 0 ) break;
            /* convert w_delay to timeval */
            stop.tv_sec=w_delay/1000;
            stop.tv_usec=(w_delay % 1000)*1000;
            FD_SET(s->fd,&mask);
            i=net_select(s->fd+1,&mask,NULL,NULL,&stop);
            if(i==0)
                break; /* timed out */
            r=net_recv(s->fd,buf,FSP_MAXPACKET,0);
            if(r < 0 )
            {
                /* serious recv error */
                client_set_key((FSP_LOCK *)s->lock,p->key);
                errno = -r;
                return -1;
            }

            gettimeofday(&stop,NULL);
            w_delay-=1000*(stop.tv_sec -  start[retry & 0x7].tv_sec);
            w_delay-=     (stop.tv_usec -  start[retry & 0x7].tv_usec)/1000;

            /* process received packet */
            if ( fsp_pkt_read(rpkt,buf,r) < 0)
            {
                /* unpack failed */
                continue;
            }

            /* check sequence number */
            if( (rpkt->seq & 0xfff8) != s->seq )
            {
                /* duplicate */
                dupes++;
                continue;
            }

            /* check command code */
            if( (rpkt->cmd != p->cmd) && (rpkt->cmd != FSP_CC_ERR))
            {
                dupes++;
                continue;
            }

            /* check correct filepos */
            if( (rpkt->pos != p->pos) && ( p->cmd == FSP_CC_GET_DIR ||
                p->cmd == FSP_CC_GET_FILE || p->cmd == FSP_CC_UP_LOAD ||
                p->cmd == FSP_CC_GRAB_FILE || p->cmd == FSP_CC_INFO) )
            {
                dupes++;
                continue;
            }

            /* now we have a correct packet */

            /* compute rtt delay */
            w_delay=1000*(stop.tv_sec - start[retry & 0x7].tv_sec);
            w_delay+=(stop.tv_usec -  start[retry & 0x7].tv_usec)/1000;
            /* update last stats */
            s->last_rtt=w_delay;
            s->last_delay=f_delay;
            s->last_dupes=dupes;
            s->last_resends=retry;
            /* update cumul. stats */
            s->dupes+=dupes;
            s->resends+=retry;
            s->trips++;
            s->rtts+=w_delay;

            /* grab a next key */
            client_set_key((FSP_LOCK *)s->lock,rpkt->key);
            errno = 0;
            return 0;
        }
    }
}

/* ******************* Session management functions ************ */

/* initializes a session */
FSP_SESSION * fsp_open_session(const char *host,unsigned short port,const char *password)
{
    FSP_SESSION *s;
    int fd;
    struct sockaddr_in addrin;
    FSP_LOCK *lock;

    memset (&addrin, 0, sizeof (addrin));
    /* fspd do not supports inet6 */
    addrin.sin_family = AF_INET;

    if (port == 0)
        addrin.sin_port = 21;
    else
        addrin.sin_port = port;

    if ( !inet_aton(host,&addrin.sin_addr))
    {
        errno = EINVAL;
        return NULL;
    }

    /* create socket */
    fd=net_socket(AF_INET,SOCK_DGRAM,IPPROTO_IP);
    if ( fd < 0)
    {
        errno = ENFILE;
        return NULL;
    }
    
    /* connect socket */
    net_connect(fd,(struct sockaddr *)&addrin,sizeof(addrin));

    /* allocate memory */
    s=calloc(1,sizeof(FSP_SESSION));
    if ( !s )
    {
        net_close(fd);
        errno = ENOMEM;
        return NULL;
    }

    lock=malloc(sizeof(FSP_LOCK));

    if ( !lock )
    {
        net_close(fd);
        free(s);
        errno = ENOMEM;
        return NULL;
    }

    s->lock=lock;

    /* init locking subsystem */
    if ( client_init_key( (FSP_LOCK *)s->lock,addrin.sin_addr.s_addr,ntohs(addrin.sin_port)))
    {
        free(s);
        net_close(fd);
        free(lock);
        return NULL;
    }

    s->fd=fd;
    s->timeout=300000; /* 5 minutes */
    s->maxdelay=60000; /* 1 minute  */
    s->seq=random() & 0xfff8;
    if ( password ) 
        s->password = strdup(password);
    return s;
}

/* closes a session */
void fsp_close_session(FSP_SESSION *s)
{
    FSP_PKT bye,in;
    
    if( s == NULL)
        return;
    if ( s->fd == -1)
        return; 
    /* Send bye packet */
    bye.cmd=FSP_CC_BYE;
    bye.len=bye.xlen=0;
    bye.pos=0;
    s->timeout=7000;
    fsp_transaction(s,&bye,&in);

    net_close(s->fd);
    if (s->password) free(s->password);
    client_destroy_key((FSP_LOCK *)s->lock);
    free(s->lock);
    memset(s,0,sizeof(FSP_SESSION));
    s->fd=-1;
    free(s);
}

/* *************** Directory listing functions *************** */

/* get a directory listing from a server */
FSP_DIR * fsp_opendir(FSP_SESSION *s,const char *dirname)
{
    FSP_PKT in,out;
    int pos;
    unsigned short blocksize;
    FSP_DIR *dir;
    unsigned char *tmp;

    if (s == NULL) return NULL;
    if (dirname == NULL) return NULL;

    if(buildfilename(s,&out,dirname))
    {
        return NULL;
    }
    pos=0;
    blocksize=0;
    dir=NULL;
    out.cmd = FSP_CC_GET_DIR;
    out.xlen=0;
    
    /* load directory listing from the server */
    while(1)
    {
        out.pos=pos;
        if ( fsp_transaction(s,&out,&in) )
        {
            pos = -1;
            break;
        }
        if ( in.cmd == FSP_CC_ERR )
        {
            /* bad reply from the server */
            pos = -1;
            break;
        }
        /* End of directory? */
        if ( in.len == 0)
            break;
        /* set blocksize */
        if (blocksize == 0 )
            blocksize = in.len;
        /* alloc directory */
        if (dir == NULL)
        {
            dir = calloc(1,sizeof(FSP_DIR));
            if (dir == NULL)
            {
                pos = -1;
                break;
            }
        }
        /* append data */
        tmp=realloc(dir->data,pos+in.len);
        if(tmp == NULL)
        {
            pos = -1;
            break;
        }
        dir->data=tmp;
        memcpy(dir->data + pos, in.buf,in.len);
        pos += in.len;
        if (in.len < blocksize)
            /* last block is smaller */
            break;
    }
    if (pos == -1)
    {
        /* failure */
        if (dir)
        {
            if(dir->data)
                free(dir->data);
            free(dir);
        }
        errno = EPERM;
        return NULL;
    }

    dir->inuse=1;
    dir->blocksize=blocksize;
    dir->dirname=strdup(dirname);
    dir->datasize=pos;
    
    errno = 0;
    return dir;
}

int fsp_readdir_r(FSP_DIR *dir,struct dirent *entry, struct dirent **result)
{
    FSP_RDENTRY fentry,*fresult;
    int rc;
    char *c;

    if (dir == NULL || entry == NULL || *result == NULL)
        return -EINVAL;
    if (dir->dirpos<0 || dir->dirpos % 4)
        return -ESPIPE;

    rc=fsp_readdir_native(dir,&fentry,&fresult);

    if (rc != 0)
        return rc;

    /* convert FSP dirent to OS dirent */

    if (fentry.type == FSP_RDTYPE_DIR )
        entry->d_type=DT_DIR;
    else
        entry->d_type=DT_REG;

    /* remove symlink destination */
    c=strchr(fentry.name,'\n');
    if (c)
    {
        *c='\0';
        rc=fentry.namlen-strlen(fentry.name);
        fentry.reclen-=rc;
        fentry.namlen-=rc;
    }

    strncpy(entry->d_name,fentry.name,NAME_MAX);

    if (fentry.namlen >= NAME_MAX)
    {
        entry->d_name[NAME_MAX] = '\0';
    }

    if (fresult == &fentry )
    {
        *result = entry;
    }
    else
        *result = NULL; 

    return 0; 
}

/* native FSP directory reader */
int fsp_readdir_native(FSP_DIR *dir,FSP_RDENTRY *entry, FSP_RDENTRY **result)
{
    unsigned char ftype;
    int namelen;

    if (dir == NULL || entry == NULL || result == NULL)
        return -EINVAL;
    if (dir->dirpos<0 || dir->dirpos % 4)
        return -ESPIPE;

    while(1)
    {
       if ( dir->dirpos >= (int)dir->datasize )
       {
            /* end of the directory */
            *result = NULL;
            return 0;
       }
       if (dir->blocksize - (dir->dirpos % dir->blocksize) < 9)
           ftype= FSP_RDTYPE_SKIP;
       else
           /* get the file type */
           ftype=dir->data[dir->dirpos+8];

       if (ftype == FSP_RDTYPE_END )
       {
           dir->dirpos=dir->datasize;
           continue;
       }
       if (ftype == FSP_RDTYPE_SKIP )
       {
           /* skip to next directory block */
           dir->dirpos = ( dir->dirpos / dir->blocksize + 1 ) * dir->blocksize;
           continue;
       }
       /* extract binary data */
       entry->lastmod=ntohl( *(const uint32_t *)( dir->data+ dir->dirpos ));
       entry->size=ntohl( *(const uint32_t *)(dir->data+ dir->dirpos +4 ));
       entry->type=ftype;

       /* skip file date and file size */
       dir->dirpos += 9;
       /* read file name */
       entry->name[255] = '\0';
       strncpy(entry->name,(char *)( dir->data + dir->dirpos ),255);
       /* check for ASCIIZ encoded filename */
       if (memchr(dir->data + dir->dirpos,0,dir->datasize - dir->dirpos) != NULL)
       {
            namelen = strlen( (char *) dir->data+dir->dirpos);
       }
       else
       {
            /* \0 terminator not found at end of filename */
            *result = NULL;
            return 0;
       }
       /* skip over file name */
       dir->dirpos += namelen +1;

       /* set entry namelen field */
       if (namelen > 255)
           entry->namlen = 255;
       else
           entry->namlen = namelen;
       /* set record length */     
       entry->reclen = 10+namelen;

       /* pad to 4 byte boundary */
       while( dir->dirpos & 0x3 )
       {
         dir->dirpos++;
         entry->reclen++;
       }

       /* and return it */
       *result=entry;
       return 0;  
    }
}

struct dirent * fsp_readdir(FSP_DIR *dirp)
{
    static dirent_workaround entry;
    struct dirent *result;
    
    
    if (dirp == NULL) return NULL;
    if ( fsp_readdir_r(dirp,&entry.dirent,&result) )
        return NULL;
    else
        return result;
}

long fsp_telldir(FSP_DIR *dirp)
{
    return dirp->dirpos;
}

void fsp_seekdir(FSP_DIR *dirp, long loc)
{
    dirp->dirpos=loc;
}

void fsp_rewinddir(FSP_DIR *dirp)
{
    dirp->dirpos=0;
}

int fsp_closedir(FSP_DIR *dirp)
{
    if (dirp == NULL) 
        return -1;
    if(dirp->dirname) free(dirp->dirname);
    free(dirp->data);
    free(dirp);
    return 0;
}

/*  ***************** File input/output functions *********  */
FSP_FILE * fsp_fopen(FSP_SESSION *session, const char *path,const char *modeflags)
{
    FSP_FILE   *f;

    if(session == NULL || path == NULL || modeflags == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    f=calloc(1,sizeof(FSP_FILE));
    if (f == NULL)
    {
        return NULL;
    }

    /* check and parse flags */
    switch (*modeflags++)
    {
        case 'r':
                  break;
        case 'w':
                  f->writing=1;
                  break;
        case 'a':
                  /* not supported */
                  free(f);
                  errno = ENOTSUP;
                  return NULL;
        default:
                  free(f);
                  errno = EINVAL;
                  return NULL;
    }

    if (*modeflags == '+' || ( *modeflags=='b' && modeflags[1]=='+'))
    {
        free(f);
        errno = ENOTSUP;
        return NULL;
    }

    /* build request packet */
    if(f->writing)
    {
        f->out.cmd=FSP_CC_UP_LOAD;
    }
    else
    {
        if(buildfilename(session,&f->out,path))
        {
            free(f);
            return NULL;
        }
        f->bufpos=FSP_SPACE;
        f->out.cmd=FSP_CC_GET_FILE;
    }
    f->out.xlen=0;

    /* setup control variables */
    f->s=session;
    f->name=strdup(path);
    if(f->name == NULL)
    {
        free(f);
        errno = ENOMEM;
        return NULL;
    }

    return f;
}

size_t fsp_fread(void *dest,size_t size,size_t count,FSP_FILE *file)
{
    size_t total,done,havebytes;
    char *ptr;

    total=count*size;
    done=0;
    ptr=dest;
    
    if(file->eof) return 0;

    while(1)
    {
        /* need more data? */
        if(file->bufpos>=FSP_SPACE)
        {
            /* fill the buffer */
            file->out.pos=file->pos;
            if(fsp_transaction(file->s,&file->out,&file->in))
            {
                 file->err=1;
                 return done/size;
            }
            if(file->in.cmd == FSP_CC_ERR)
            {
                errno = EIO;
                file->err=1;
                return done/size;
            }
            file->bufpos=FSP_SPACE-file->in.len;
            if(file->bufpos > 0)
            {
               memmove(file->in.buf+file->bufpos,file->in.buf,file->in.len);
            }
            file->pos+=file->in.len;
        }
        havebytes=FSP_SPACE - file->bufpos;
        if (havebytes == 0 )
        {
            /* end of file! */
            file->eof=1;
            errno = 0;
            return done/size;
        }
        /* copy ready data to output buffer */
        if(havebytes <= total )
        {
            /* copy all we have */
            memcpy(ptr,file->in.buf+file->bufpos,havebytes);
            ptr+=havebytes;
            file->bufpos=FSP_SPACE;
            done+=havebytes;
            total-=havebytes;
        } else
        {
            /* copy bytes left */
            memcpy(ptr,file->in.buf+file->bufpos,total);
            file->bufpos+=total;
            errno = 0;
            return count;
        }
    }
}

size_t fsp_fwrite(const void * source, size_t size, size_t count, FSP_FILE * file)
{
    size_t total,done,freebytes;
    const char *ptr;

    if(file->eof || file->err)
        return 0;

    file->out.len=FSP_SPACE;
    total=count*size;
    done=0;
    ptr=source;

    while(1)
    {
        /* need to write some data? */
        if(file->bufpos>=FSP_SPACE)
        {
            /* fill the buffer */
            file->out.pos=file->pos;
            if(fsp_transaction(file->s,&file->out,&file->in))
            {
                 file->err=1;
                 return done/size;
            }
            if(file->in.cmd == FSP_CC_ERR)
            {
                errno = EIO;
                file->err=1;
                return done/size;
            }
            file->bufpos=0;
            file->pos+=file->out.len;
            done+=file->out.len;
        }
        freebytes=FSP_SPACE - file->bufpos;
        /* copy input data to output buffer */
        if(freebytes <= total )
        {
            /* copy all we have */
            memcpy(file->out.buf+file->bufpos,ptr,freebytes);
            ptr+=freebytes;
            file->bufpos=FSP_SPACE;
            total-=freebytes;
        } else
        {
            /* copy bytes left */
            memcpy(file->out.buf+file->bufpos,ptr,total);
            file->bufpos+=total;
            errno = 0;
            return count;
        }
    }
}

int fsp_fpurge(FSP_FILE *file)
{
    if(file->writing)
    {
        file->bufpos=0;
    }
    else
    {
        file->bufpos=FSP_SPACE;
    }
    errno = 0;
    return 0;
}

int fsp_fflush(FSP_FILE *file)
{
    if(file == NULL)
    {
        errno = ENOTSUP;
        return -1;
    }
    if(!file->writing)
    {
        errno = EBADF;
        return -1;
    }
    if(file->eof || file->bufpos==0)
    {
        errno = 0;
        return 0;
    }

    file->out.pos=file->pos;
    file->out.len=file->bufpos;
    if(fsp_transaction(file->s,&file->out,&file->in))
    {
         file->err=1;
         return -1;
    }
    if(file->in.cmd == FSP_CC_ERR)
    {
        errno = EIO;
        file->err=1;
        return -1;
    }
    file->bufpos=0;
    file->pos+=file->out.len;
    
    errno = 0;
    return 0;
}



int fsp_fclose(FSP_FILE *file)
{
    int rc;

    rc=0;
    errno = 0;
    if(file->writing)
    {
        if(fsp_fflush(file))
        {  
            rc=-1;
        }
        else if(fsp_install(file->s,file->name,0))
        {
            rc=-1;
        }
    }
    free(file->name);
    free(file);
    return rc;
}

int fsp_fseek(FSP_FILE *stream, long offset, int whence)
{
    long newoffset;

    switch(whence)
    {
        case SEEK_SET:
                      newoffset = offset;
                      break;
        case SEEK_CUR:
                      newoffset = stream->pos + offset;
                      break;
        case SEEK_END:
                      errno = ENOTSUP;
                      return -1;
        default:
                      errno = EINVAL;
                      return -1;
    }
    if(stream->writing)
    {
        if(fsp_fflush(stream))
        {
            return -1;
        }
    }
    stream->pos=newoffset;
    stream->eof=0;
    fsp_fpurge(stream);
    return 0;
}

long fsp_ftell(FSP_FILE *f)
{
    return f->pos + f->bufpos;
}

void fsp_rewind(FSP_FILE *f)
{
    if(f->writing)
        fsp_fflush(f);
    f->pos=0;
    f->err=0;
    f->eof=0;
    fsp_fpurge(f);
}

/*  **************** Utility functions ****************** */

/* return 0 if user has enough privs for uploading the file */
int fsp_canupload(FSP_SESSION *s,const char *fname)
{
  char *dir;
  unsigned char dirpro;
  int rc;
  struct stat sb;

  dir=directoryfromfilename(fname);
  if(dir == NULL)
  {
      errno = ENOMEM;
      return -1;
  }
  
  rc=fsp_getpro(s,dir,&dirpro);
  free(dir);

  if(rc)
  {
      return -1;
  }
  
  if(dirpro & FSP_DIR_OWNER) 
      return 0;
  
  if( ! (dirpro & FSP_DIR_ADD))
      return -1;
      
  if (dirpro & FSP_DIR_DEL)
     return 0;
     
  /* we need to check file existence, because we can not overwrite files */
  
  rc = fsp_stat(s,fname,&sb);
  
  if (rc == 0)
      return -1;
  else
      return 0;
}

/* install file opened for writing */
int fsp_install(FSP_SESSION *s,const char *fname,time_t timestamp)
{
    int rc;
    FSP_PKT in,out;

    /* and install a new file */
    out.cmd=FSP_CC_INSTALL;
    out.xlen=0;
    out.pos=0;
    rc=0;
    if( buildfilename(s,&out,fname) )
        rc=-1;
    else
        {
            if (timestamp != 0)
            {
                /* add timestamp extra data */
                *(uint32_t *)(out.buf+out.len)=htonl(timestamp);
                out.xlen=4;
                out.pos=4;
            }
            if(fsp_transaction(s,&out,&in))
            {
                rc=-1;
            } else
            {
                if(in.cmd == FSP_CC_ERR)
                {
                    rc=-1;
                    errno = EPERM;
                }
            }
        }

    return rc;
}
/* Get protection byte from the directory */
int fsp_getpro(FSP_SESSION *s,const char *directory,unsigned char *result)
{
   FSP_PKT in,out;
   
   if(buildfilename(s,&out,directory))
       return -1;
   out.cmd=FSP_CC_GET_PRO;
   out.xlen=0;
   out.pos=0;

   if(fsp_transaction(s,&out,&in))
       return -1;

   if(in.cmd == FSP_CC_ERR)
   {
       errno = ENOENT;
       return -1;
   }
   if(in.pos != FSP_PRO_BYTES)
   {
       errno = ENOMSG;
       return -1;
   }

   if(result)
      *result=in.buf[in.len];
   errno = 0;
   return  0;
}

int fsp_stat(FSP_SESSION *s,const char *path,struct stat *sb)
{
   FSP_PKT in,out;
   unsigned char ftype;
   
   if(buildfilename(s,&out,path))
       return -1;
   out.cmd=FSP_CC_STAT;
   out.xlen=0;
   out.pos=0;

   if(fsp_transaction(s,&out,&in))
       return -1;

   if(in.cmd == FSP_CC_ERR)
   {
       errno = ENOTSUP;
       return -1;
   }
   /* parse results */
   ftype=in.buf[8];
   if(ftype == 0)
   {
       errno = ENOENT;
       return -1;
   }
   sb->st_uid=sb->st_gid=0;
   sb->st_mtime=sb->st_ctime=sb->st_atime=ntohl( *(const uint32_t *)( in.buf ));
   sb->st_size=ntohl( *(const uint32_t *)(in.buf + 4 ));
   sb->st_blocks=(sb->st_size+511)/512;
   if (ftype==FSP_RDTYPE_DIR)
   {
       sb->st_mode=S_IFDIR | 0755;
       sb->st_nlink=2;
   }
   else
   {
       sb->st_mode=S_IFREG | 0644;
       sb->st_nlink=1;
   }

   errno = 0;
   return  0;
}

int fsp_mkdir(FSP_SESSION *s,const char *directory)
{
   return simplecommand(s,directory,FSP_CC_MAKE_DIR);
}

int fsp_rmdir(FSP_SESSION *s,const char *directory)
{
   return simplecommand(s,directory,FSP_CC_DEL_DIR);
}

int fsp_unlink(FSP_SESSION *s,const char *directory)
{
   return simplecommand(s,directory,FSP_CC_DEL_FILE);
}

int fsp_rename(FSP_SESSION *s,const char *from, const char *to)
{
   FSP_PKT in,out;
   int l;
   
   if(buildfilename(s,&out,from))
       return -1;
   /* append target name */
   l=strlen(to)+1;
   if( l + out.len > FSP_SPACE )
   {
       errno = ENAMETOOLONG;
       return -1;
   }
   memcpy(out.buf+out.len,to,l);
   out.xlen = l;

   if(s->password)
   {
       l=strlen(s->password)+1;
       if(out.len + out.xlen + l > FSP_SPACE)
       {
           errno = ENAMETOOLONG;
           return -1;
       }
       out.buf[out.len+out.xlen-1] = '\n';
       memcpy(out.buf+out.len+out.xlen,s->password,l);
       out.xlen += l;
   }

   out.cmd=FSP_CC_RENAME;
   out.pos=out.xlen;
    
   if(fsp_transaction(s,&out,&in))
       return -1;

   if(in.cmd == FSP_CC_ERR)
   {
       errno = EPERM;
       return -1;
   }

   errno = 0; 
   return 0;
}

int fsp_access(FSP_SESSION *s,const char *path, int mode)
{
    struct stat sb;
    int rc;
    unsigned char dirpro;
    char *dir;

    rc=fsp_stat(s,path,&sb);
    if(rc == -1)
    {
        /* not found */
        /* errno is set by fsp_stat */
        return -1;
    }

    /* just test file existence */
    if(mode == F_OK)
    {
        errno = 0;
        return  0;
    }

    /* deny execute access to file */
    if (mode & X_OK)
    {
        if(S_ISREG(sb.st_mode))
        {
            errno = EACCES;
            return -1;
        }
    }
    
    /* Need to get ACL of directory */
    if(S_ISDIR(sb.st_mode))
        dir=NULL;
    else
        dir=directoryfromfilename(path);        
    
    rc=fsp_getpro(s,dir==NULL?path:dir,&dirpro);
    /* get pro failure */
    if(rc)
    {
        if(dir) free(dir);
        errno = EACCES;
        return -1;
    }
    /* owner shortcut */
    if(dirpro & FSP_DIR_OWNER)
    {
        if(dir) free(dir);
        errno = 0;
        return 0;
    }
    /* check read access */
    if(mode & R_OK)
    {
        if(dir)
        {
            if(! (dirpro & FSP_DIR_GET))
            {
                free(dir);
                errno = EACCES;
                return -1;
            }
        } else
        {
            if(! (dirpro & FSP_DIR_LIST))
            {
                errno = EACCES;
                return -1;
            }
        }
    }
    /* check write access */
    if(mode & W_OK)
    {
        if(dir)
        {
            if( !(dirpro & FSP_DIR_DEL) || !(dirpro & FSP_DIR_ADD))
            {
                free(dir);
                errno = EACCES;
                return -1;
            }
        } else
        {
            /* when checking directory for write access we are cheating
               by allowing ADD or DEL right */
            if( !(dirpro & FSP_DIR_DEL) && !(dirpro & FSP_DIR_ADD))
            {
                errno = EACCES;
                return -1;
            }
        }
    }

    if(dir) free(dir);
    errno = 0;
    return 0;
}
