/*
    This file is part of libpsoarchive.

    Copyright (C) 2014, 2015, 2017 Lawrence Sebald

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 2.1 or
    version 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "psoarchive-error.h"

struct prs_dec_cxt {
    uint8_t flags;

    int bit_pos;
    const uint8_t *src;
    uint8_t *dst;
    void *udata;

    size_t src_len;
    size_t dst_len;
    size_t src_pos;
    size_t dst_pos;

    int (*copy_byte)(struct prs_dec_cxt *cxt);
    int (*offset_copy)(struct prs_dec_cxt *cxt, int offset);
    int (*fetch_bit)(struct prs_dec_cxt *cxt);
    int (*fetch_byte)(struct prs_dec_cxt *cxt);
    int (*fetch_short)(struct prs_dec_cxt *cxt);
};

/******************************************************************************
    PRS Decompression Function

    This function does the real work of decompressing whatever you throw at it.
    It uses a bunch of callbacks in the context provided to read the compressed
    data and do whatever is needed with it.
 ******************************************************************************/
static int do_decompress(struct prs_dec_cxt *cxt) {
    int flag, size;
    int32_t offset;

    for(;;) {
        /* Read the flag bit for this pass. */
        if((flag = cxt->fetch_bit(cxt)) < 0)
            return flag;

        /* Flag bit = 1 -> Simple byte copy from src to dst. */
        if(flag) {
            if((flag = cxt->copy_byte(cxt)) < 0)
                return flag;

            continue;
        }

        /* The flag starts with a zero, so it isn't just a simple byte copy.
           Read the next bit to see what we have left to do. */
        if((flag = cxt->fetch_bit(cxt)) < 0)
            return flag;

        /* Flag bit = 1 -> Either long copy or end of file. */
        if(flag) {
            if((offset = cxt->fetch_short(cxt)) < 0)
                return offset;

            /* Two zero bytes implies that this is the end of the file. Return
               the length of the file. */
            if(!offset)
                return (int)cxt->dst_pos;

            /* Do we need to read a size byte, or is it encoded in what we
               already got? */
            size = offset & 0x0007;
            offset >>= 3;

            if(!size) {
                if((size = cxt->fetch_byte(cxt)) < 0)
                    return size;

                ++size;
            }
            else {
                size += 2;
            }

            offset |= 0xFFFFE000;
        }
        /* Flag bit = 0 -> short copy. */
        else {
            /* Fetch the two bits needed to determine the size. */
            if((flag = cxt->fetch_bit(cxt)) < 0)
                return flag;

            if((size = cxt->fetch_bit(cxt)) < 0)
                return size;

            size = (size | (flag << 1)) + 2;

            /* Fetch the offset byte. */
            if((offset = cxt->fetch_byte(cxt)) < 0)
                return offset;

            offset |= 0xFFFFFF00;
        }

        /* Copy the data. */
        while(size--) {
            if((flag = cxt->offset_copy(cxt, offset)) < 0)
                return flag;
        }
    }
}

/******************************************************************************
    Internal utility functions.

    Depending on how the compressed data is to be obtained, different sets of
    these functions will be used.
 ******************************************************************************/
static int fetch_bit(struct prs_dec_cxt *cxt) {
    int rv;

    /* Did we finish with a full byte last time we were in here? */
    if(!cxt->bit_pos) {
        /* Make sure we won't fall off the end of the file by reading the byte
           from it. */
        if(cxt->src_pos >= cxt->src_len)
            return PSOARCHIVE_EBADMSG;

        cxt->flags = *cxt->src++;
        ++cxt->src_pos;
        cxt->bit_pos = 8;
    }

    /* Fetch the bit and shift it off the end of the byte. */
    rv = cxt->flags & 1;
    cxt->flags >>= 1;
    --cxt->bit_pos;

    return rv;
}

static int copy_byte(struct prs_dec_cxt *cxt) {
    /* Make sure we still have data left in the input buffer. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Make sure we have space left in the destination buffer. */
    if(cxt->dst_pos >= cxt->dst_len)
        return PSOARCHIVE_ENOSPC;

    /* Copy the byte and increment all the counters/pointers. */
    *(cxt->dst + cxt->dst_pos) = *cxt->src++;
    ++cxt->src_pos;
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int fetch_byte(struct prs_dec_cxt *cxt) {
    uint8_t rv;

    /* Make sure we still have data left in the input buffer. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Read the byte from the buffer. */
    rv = *cxt->src++;
    ++cxt->src_pos;

    return (int)rv;
}

static int fetch_short(struct prs_dec_cxt *cxt) {
    uint16_t rv;

    /* Make sure we still have data left in the input buffer. */
    if(cxt->src_pos + 1 >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Read the two bytes from the buffer. */
    rv = *cxt->src++;
    ++cxt->src_pos;
    rv |= *cxt->src++ << 8;
    ++cxt->src_pos;

    return (int)rv;
}

static int offset_copy(struct prs_dec_cxt *cxt, int offset) {
    int tmp = (int)cxt->dst_pos + offset;

    /* Make sure the offset is valid. */
    if(tmp < 0)
        return PSOARCHIVE_EBADMSG;

    /* Make sure we have space left in the destination buffer. */
    if(cxt->dst_pos >= cxt->dst_len)
        return PSOARCHIVE_ENOSPC;

    /* Copy the byte and increment all the counters/pointers. */
    *(cxt->dst + cxt->dst_pos) = *(cxt->dst + offset);
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int nocopy_byte(struct prs_dec_cxt *cxt) {
    /* Make sure we still have data left in the input buffer. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Increment the counters/pointers. */
    ++cxt->src;
    ++cxt->src_pos;
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int offset_nocopy(struct prs_dec_cxt *cxt, int offset) {
    int tmp = (int)cxt->dst_pos + offset;

    /* Make sure the offset is valid. */
    if(tmp < 0)
        return PSOARCHIVE_EBADMSG;

    /* Increment the counter... */
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int file_bit(struct prs_dec_cxt *cxt) {
    int rv;

    /* Did we finish with a full byte last time we were in here? */
    if(!cxt->bit_pos) {
        /* Make sure we won't fall off the end of the file by reading the byte
           from it. */
        if(cxt->src_pos >= cxt->src_len)
            return PSOARCHIVE_EBADMSG;

        /* Read the next byte from the file. */
        if((rv = fgetc((FILE *)cxt->udata)) == EOF) {
            if(ferror((FILE *)cxt->udata))
                return PSOARCHIVE_EIO;
            return PSOARCHIVE_EBADMSG;
        }

        cxt->flags = (uint8_t)rv;
        ++cxt->src_pos;
        cxt->bit_pos = 8;
    }

    /* Fetch the bit and shift it off the end of the byte. */
    rv = cxt->flags & 1;
    cxt->flags >>= 1;
    --cxt->bit_pos;

    return rv;
}

static int copy_fbyte(struct prs_dec_cxt *cxt) {
    int b;
    void *tmp;

    /* Make sure we still have data left in the input file. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Make sure we have space left in the destination buffer. */
    if(cxt->dst_pos >= cxt->dst_len) {
        if(!(tmp = realloc(cxt->dst, cxt->dst_len * 2)))
            return PSOARCHIVE_EMEM;

        cxt->dst = (uint8_t *)tmp;
        cxt->dst_len *= 2;
    }

    /* Read the next byte from the file. */
    if((b = fgetc((FILE *)cxt->udata)) == EOF) {
        if(ferror((FILE *)cxt->udata))
            return PSOARCHIVE_EIO;
        return PSOARCHIVE_EBADMSG;
    }

    /* Copy the byte and increment all the counters/pointers. */
    *(cxt->dst + cxt->dst_pos) = (uint8_t)b;
    ++cxt->src_pos;
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int file_byte(struct prs_dec_cxt *cxt) {
    int rv;

    /* Make sure we still have data left in the input file. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Read the next byte from the file. */
    if((rv = fgetc((FILE *)cxt->udata)) == EOF) {
        if(ferror((FILE *)cxt->udata))
            return PSOARCHIVE_EIO;
        return PSOARCHIVE_EBADMSG;
    }

    ++cxt->src_pos;

    return (int)rv;
}

static int file_short(struct prs_dec_cxt *cxt) {
    uint16_t rv;
    uint8_t b[2];

    /* Make sure we still have data left in the input file. */
    if(cxt->src_pos + 1 >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Read the next two bytes from the file. */
    if(fread(b, 1, 2, (FILE *)cxt->udata) != 2)
        return PSOARCHIVE_EIO;

    /* Combine the bytes into the 16-bit value we're looking for. */
    rv = b[0] | (b[1] << 8);
    cxt->src_pos += 2;

    return (int)rv;
}

static int offset_copy_alloc(struct prs_dec_cxt *cxt, int offset) {
    int tmp = (int)cxt->dst_pos + offset;
    void *tmp2;

    /* Make sure the offset is valid. */
    if(tmp < 0)
        return PSOARCHIVE_EBADMSG;

    /* Make sure we have space left in the destination buffer. */
    if(cxt->dst_pos >= cxt->dst_len) {
        if(!(tmp2 = realloc(cxt->dst, cxt->dst_len * 2)))
            return PSOARCHIVE_EMEM;

        cxt->dst = (uint8_t *)tmp2;
        cxt->dst_len *= 2;
    }

    /* Copy the byte and increment all the counters/pointers. */
    *(cxt->dst + cxt->dst_pos) = *(cxt->dst + cxt->dst_pos + offset);
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

static int copy_abyte(struct prs_dec_cxt *cxt) {
    void *tmp;

    /* Make sure we still have data left in the input file. */
    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_EBADMSG;

    /* Make sure we have space left in the destination buffer. */
    if(cxt->dst_pos >= cxt->dst_len) {
        if(!(tmp = realloc(cxt->dst, cxt->dst_len * 2)))
            return PSOARCHIVE_EMEM;

        cxt->dst = (uint8_t *)tmp;
        cxt->dst_len *= 2;
    }

    /* Copy the byte and increment all the counters/pointers. */
    *(cxt->dst + cxt->dst_pos) = *cxt->src++;
    ++cxt->src_pos;
    ++cxt->dst_pos;

    return PSOARCHIVE_OK;
}

/******************************************************************************
    Public interface functions

    These functions are the public functions used to decompress PRS-compressed
    data. There are a variety of functions provided here for different purposes.

    prs_decompress_buf:
        Decompress data from a memory buffer into another memory buffer,
        allocating space as needed for the destination buffer. It is the
        caller's responsibility to free the decompressed memory buffer when it
        is no longer needed.

    prs_decompress_buf2:
        Decompress data from a memory buffer into another (pre-allocated) memory
        buffer. If the buffer is not large enough, an error (-ENOSPC) will be
        returned.

    prs_decompress_size:
        Determine the decompressed size of a block of memory containing PRS-
        compressed data.

    prs_decompress_file:
        Open the specified PRS-compressed file and decompress it into a new
        memory buffer. It is the caller's responsibility to free the
        decompressed memory buffer when it is no longer needed.

    All of these functions will return the size of the decompressed data on
    success, or a error code (from psoarchive-error) on error. Common error
    codes include the following:
        PSOARCHIVE_EBADMSG: Invalid compressed data encountered while decoding.
        PSOARCHIVE_EINVAL: Invalid source length (0) given.
        PSOARCHIVE_EFAULT: NULL pointer passed in.

    In addition, prs_decompress_file may return many other error codes related
    to reading from a file. prs_decompress_file and prs_decompress_buf may also
    return errors related to memory allocation.
 ******************************************************************************/
int pso_prs_decompress_buf(const uint8_t *src, uint8_t **dst, size_t src_len) {
    struct prs_dec_cxt cxt =
        { 0, 0, src, NULL, NULL, src_len, src_len * 2, 0, 0, &copy_abyte,
          &offset_copy_alloc, &fetch_bit, &fetch_byte, &fetch_short };
    int rv;

    if(!src || !dst)
        return PSOARCHIVE_EFAULT;

    if(!src_len)
        return PSOARCHIVE_EINVAL;

    /* The minimum length of a PRS compressed file (if you were to "compress" a
       zero-byte file) is 3 bytes. If we don't have that, then bail out now. */
    if(cxt.src_len < 3)
        return PSOARCHIVE_EBADMSG;

    /* Allocate some space for the output. Start with two times the length of
       the input (we will resize this later, as needed). */
    if(!(cxt.dst = (uint8_t *)malloc(cxt.dst_len)))
        return PSOARCHIVE_EMEM;

    /* Do the decompression. */
    if((rv = do_decompress(&cxt)) < 0) {
        free(cxt.dst);
        return rv;
    }

    /* Resize the output (if realloc fails to resize it, then just use the
       unshortened buffer). */
    if(!(*dst = realloc(cxt.dst, rv)))
        *dst = cxt.dst;

    return rv;
}

int pso_prs_decompress_buf2(const uint8_t *src, uint8_t *dst, size_t src_len,
                            size_t dst_len) {
    struct prs_dec_cxt cxt =
        { 0, 0, src, dst, NULL, src_len, dst_len, 0, 0, &copy_byte,
          &offset_copy, &fetch_bit, &fetch_byte, &fetch_short };

    if(!src || !dst)
        return PSOARCHIVE_EFAULT;

    if(!src_len || !dst_len)
        return PSOARCHIVE_EINVAL;

    /* The minimum length of a PRS compressed file (if you were to "compress" a
       zero-byte file) is 3 bytes. If we don't have that, then bail out now. */
    if(cxt.src_len < 3)
        return PSOARCHIVE_EBADMSG;

    return do_decompress(&cxt);
}

int pso_prs_decompress_size(const uint8_t *src, size_t src_len) {
    struct prs_dec_cxt cxt =
        { 0, 0, src, NULL, NULL, src_len, SIZE_MAX, 0, 0, &nocopy_byte,
          &offset_nocopy, &fetch_bit, &fetch_byte, &fetch_short };

    if(!src)
        return PSOARCHIVE_EFAULT;

    if(!src_len)
        return PSOARCHIVE_EINVAL;

    /* The minimum length of a PRS compressed file (if you were to "compress" a
       zero-byte file) is 3 bytes. If we don't have that, then bail out now. */
    if(cxt.src_len < 3)
        return PSOARCHIVE_EBADMSG;

    return do_decompress(&cxt);
}

int pso_prs_decompress_file(const char *fn, uint8_t **dst) {
    struct prs_dec_cxt cxt =
        { 0, 0, NULL, NULL, NULL, 0, 0, 0, 0,
          &copy_fbyte, &offset_copy_alloc, &file_bit, &file_byte, &file_short };
    long len;
    int rv;
    FILE *fp;

    if(!fn || !dst)
        return PSOARCHIVE_EFAULT;

    if(!(fp = fopen(fn, "rb")))
        return PSOARCHIVE_EFILE;

    cxt.udata = fp;

    /* Figure out the length of the file. */
    if(fseek(fp, 0, SEEK_END)) {
        fclose(fp);
        return PSOARCHIVE_EIO;
    }

    if((len = ftell(fp)) < 0) {
        fclose(fp);
        return PSOARCHIVE_EIO;
    }

    if(fseek(fp, 0, SEEK_SET)) {
        fclose(fp);
        return PSOARCHIVE_EIO;
    }

    cxt.src_len = (size_t)len;
    cxt.dst_len = cxt.src_len * 2;

    /* The minimum length of a PRS compressed file (if you were to "compress" a
       zero-byte file) is 3 bytes. If we don't have that, then bail out now. */
    if(cxt.src_len < 3) {
        fclose(fp);
        return PSOARCHIVE_EBADMSG;
    }

    /* Allocate some space for the output. Start with two times the length of
       the input (we will resize this later, as needed). */
    if(!(cxt.dst = (uint8_t *)malloc(cxt.dst_len))) {
        fclose(fp);
        return PSOARCHIVE_EMEM;
    }

    /* Do the decompression. */
    if((rv = do_decompress(&cxt)) < 0) {
        free(cxt.dst);
        fclose(fp);
        return rv;
    }

    fclose(fp);

    /* Resize the output (if realloc fails to resize it, then just use the
       unshortened buffer). */
    if(!(*dst = realloc(cxt.dst, rv)))
        *dst = cxt.dst;

    return rv;
}
