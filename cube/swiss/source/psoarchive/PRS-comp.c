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

/******************************************************************************
    PRS Compression Library

    PRS Compression, as used by Sega in many games (including all of the PSO
    games), is basically just a normal implementation of the Lempel-Ziv '77
    (LZ77) sliding-window compression algorithm. LZ77 is the basis of many other
    compression algorithms, including the very popular DEFLATE algorithm (from
    gzip and zlib). The code in here actually takes a number of cues from
    DEFLATE and specifically from zlib.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "psoarchive-error.h"
#include "PRS.h"

#define MAX_WINDOW   0x2000
#define WINDOW_MASK  (MAX_WINDOW - 1)
#define HASH_SIZE    (1 << 8)
#define HASH_MASK    (HASH_SIZE - 1)
#define HASH(c1, c2) (c1 ^ c2)
#define ENT(h)       hash[h]
#define PREV(s)      h_prev[((uintptr_t)(s)) & WINDOW_MASK]

#define ADD_TO_HASH(htab, s, h) { \
    htab->PREV((s)) = htab->ENT(h); \
    htab->ENT(h) = s; \
}

#define INIT_ADD_HASH(htab, s, h) { \
    h = HASH(*(s), *(s + 1)); \
    ADD_TO_HASH(htab, s, h); \
}

#define HASH_STR(s) HASH(*(s), *((s) + 1))

struct prs_comp_cxt {
    uint8_t flags;

    int bits_left;
    const uint8_t *src;
    uint8_t *dst;
    uint8_t *flag_ptr;

    size_t src_len;
    size_t dst_len;
    size_t src_pos;
    size_t dst_pos;
};

struct prs_hash_cxt {
    const uint8_t *hash[HASH_SIZE];
    const uint8_t *h_prev[MAX_WINDOW];
};

/******************************************************************************
    Determine the max size of a "compressed" version of a piece of data.

    This function determines the maximum size that a file will be "compressed"
    to. Note that this value will *always* be greater than the original size of
    the file. This is used internally to allocate a buffer to start compression
    into, where the final buffer is resized accordingly.

    The formula for this size is as follows:
        len + ceil((len + 2) / 8) + 2

    This is derived from the fact that if a file is completely incompressible
    (no patterns repeat within the window), it will be spat out as a bunch of
    literals. For every literal spat out by the compressor, one bit of a flag
    byte is used. At the end of the file, a final two bit pattern is output into
    the flag byte, as well as two NUL bytes to symbolize the end of the data.
    Thus, the number of bits in the various flag bytes will be capped at a max
    of len + 2, leading to ceil((len + 2) / 8) bytes used for flags. The
    individual data bytes make up the len part of the formula and the two final
    NUL bytes make up the rest.

    Most of the time, the compressed output will be quite a bit smaller than the
    value returned here. We just use the maximum value so that we never have to
    worry about reallocating buffers as we go along.
 ******************************************************************************/
size_t pso_prs_max_compressed_size(size_t len) {
    len += 2;
    return len + (len >> 3) + ((len & 0x07) ? 1 : 0);
}

static int set_bit(struct prs_comp_cxt *cxt, int value) {
    if(!cxt->bits_left--) {
        if(cxt->dst_pos >= cxt->dst_len)
            return PSOARCHIVE_ENOSPC;

        /* Write out the flags to their position in the file, and set up the
           next flag position. */
        *cxt->flag_ptr = cxt->flags;
        cxt->flag_ptr = cxt->dst + cxt->dst_pos++;
        cxt->bits_left = 7;
    }

    cxt->flags >>= 1;
    if(value)
        cxt->flags |= 0x80;

    return PSOARCHIVE_OK;
}

static int write_final_flags(struct prs_comp_cxt *cxt) {
    cxt->flags >>= cxt->bits_left;
    *cxt->flag_ptr = cxt->flags;

    return PSOARCHIVE_OK;
}

static int copy_literal(struct prs_comp_cxt *cxt) {
    if(cxt->dst_pos >= cxt->dst_len)
        return PSOARCHIVE_ENOSPC;

    if(cxt->src_pos >= cxt->src_len)
        return PSOARCHIVE_ERANGE;

    *(cxt->dst + cxt->dst_pos++) = *(cxt->src + cxt->src_pos++);

    return PSOARCHIVE_OK;
}

static int write_literal(struct prs_comp_cxt *cxt, uint8_t val) {
    if(cxt->dst_pos >= cxt->dst_len)
        return PSOARCHIVE_ENOSPC;

    *(cxt->dst + cxt->dst_pos++) = val;

    return PSOARCHIVE_OK;
}

static int write_eof(struct prs_comp_cxt *cxt) {
    int rv;

    /* Set the last two bits (01) and write the flags to the buffer. */
    if((rv = set_bit(cxt, 0)))
        return rv;

    if((rv = set_bit(cxt, 1)))
        return rv;

    if((rv = write_final_flags(cxt)))
        return rv;

    /* Write the two NUL bytes that the file must end with. */
    if((rv = write_literal(cxt, 0)))
        return rv;

    if((rv = write_literal(cxt, 0)))
        return rv;

    return 0;
}

static int match_length(struct prs_comp_cxt *cxt, const uint8_t *s2) {
    int len = 0;
    const uint8_t *s1 = cxt->src + cxt->src_pos, *end = cxt->src + cxt->src_len;

    while(s1 < end && *s1 == *s2) {
        ++len;
        ++s1;
        ++s2;
    }

    return len;
}

static int find_longest_match(struct prs_comp_cxt *cxt, struct prs_hash_cxt *hc,
                              int *pos, int lazy) {
    uint8_t hash;
    const uint8_t *ent, *ent2;
    int mlen;
    int longest = 0;
    const uint8_t *longest_match = NULL;
    uintptr_t diff;

    if(cxt->src_pos >= cxt->src_len)
        return 0;

    /* Figure out where we're looking. */
    hash = HASH_STR(cxt->src + cxt->src_pos);

    /* Is there anything in the table at that point? If not, we obviously don't
       have a match, so bail out now. */
    if(!(ent = hc->ENT(hash))) {
        if(!lazy)
            ADD_TO_HASH(hc, cxt->src + cxt->src_pos, hash);
        return 0;
    }

    /* Make sure our initial match isn't outside the window. */
    diff = (uintptr_t)ent - (uintptr_t)cxt->src;

    /* If we'd go outside the window, truncate the hash chain now. */
    if(cxt->src_pos - diff > (MAX_WINDOW - 1)) {
        hc->ENT(hash) = NULL;
        if(!lazy)
            ADD_TO_HASH(hc, cxt->src + cxt->src_pos, hash);
        return 0;
    }

    /* Ok, we have something in the hash table that matches the hash value. That
       doesn't necessarily mean we have a matching string though, of course.
       Follow the chain to see if we do, and find the longest match. */
    while(ent) {
        if((mlen = match_length(cxt, ent))) {
            if(mlen > longest || mlen >= 256) {
                longest = mlen;
                longest_match = ent;
            }
        }

        /* Follow the chain, making sure not to exceed a difference of 8KiB. */
        if((ent2 = hc->PREV(ent))) {
            diff = (uintptr_t)ent2 - (uintptr_t)cxt->src;

            /* If we'd go outside the window, truncate the hash chain now. */
            if(cxt->src_pos - diff > (MAX_WINDOW - 1)) {
                hc->PREV(ent) = NULL;
                ent2 = NULL;
            }
        }

        /* Follow the chain for the next pass. */
        ent = ent2;
    }

    /* Did we find a match? */
    if(longest) {
        diff = (uintptr_t)longest_match - (uintptr_t)cxt->src;
        *pos = ((int)diff - (int)cxt->src_pos);
    }

    /* Add our current string to the hash. */
    if(!lazy)
        ADD_TO_HASH(hc, cxt->src + cxt->src_pos, hash);

    return longest;
}

static void add_intermediates(struct prs_comp_cxt *cxt, struct prs_hash_cxt *hc,
                              int len) {
    int i;
    uint8_t hash;

    for(i = 1; i < len; ++i) {
        hash = HASH_STR(cxt->src + cxt->src_pos + i);
        ADD_TO_HASH(hc, cxt->src + cxt->src_pos + i, hash);
    }
}

/******************************************************************************
    Archive a buffer of data into PRS format.

    This function archives a buffer of data into a format that can be
    "decompressed" by a program that handles PRS files. By archive, I mean that
    the file is not actually compressed -- in fact, the output will be larger
    than the input data (see prs_max_compressed_size for how big it'll actually
    be).

    There's really very little reason to ever use this, but it is here for you
    if you really want it.
 ******************************************************************************/
int pso_prs_archive(const uint8_t *src, uint8_t **dst, size_t src_len) {
    size_t dl = pso_prs_max_compressed_size(src_len);
    int rv;
    uint8_t *db;

    /* Allocate our "compressed" buffer. */
    if(!(db = (uint8_t *)malloc(dl)))
        return PSOARCHIVE_EMEM;

    /* Call on the function for archiving into a preallocated buffer to do the
       real work. */
    if((rv = pso_prs_archive2(src, db, src_len, dl)) < 0) {
        free(db);
        return rv;
    }

    *dst = db;
    return rv;
}

int pso_prs_archive2(const uint8_t *src, uint8_t *dst, size_t src_len,
                     size_t dst_len) {
    struct prs_comp_cxt cxt;
    int rv;

    /* Check the input to make sure we've got valid source/destination pointers
       and something to do. */
    if(!src || !dst)
        return PSOARCHIVE_EFAULT;

    if(!src_len)
        return PSOARCHIVE_EINVAL;

    if(dst_len < pso_prs_max_compressed_size(src_len))
        return PSOARCHIVE_ENOSPC;

    /* Clear the context and fill in what we need to do our job. */
    memset(&cxt, 0, sizeof(cxt));
    cxt.src = src;
    cxt.src_len = src_len;
    cxt.dst_len = dst_len;
    cxt.dst = dst;
    cxt.flag_ptr = cxt.dst;

    /* Copy each byte, filling in the flags as we go along. */
    while(src_len--) {
        /* Set the bit in the flag since we're just putting a literal in the
           output. */
        if((rv = set_bit(&cxt, 1)))
            return rv;

        /* Copy the byte over. */
        if((rv = copy_literal(&cxt)))
            return rv;
    }

    if((rv = write_eof(&cxt)))
        return rv;

    return (int)cxt.dst_pos;
}

/******************************************************************************
    Compress a buffer of data into PRS format.

    This function compresses a buffer of data with PRS compression. This
    function will never produce output larger than that of the prs_archive
    function, and will usually produce output that is significantly smaller.
 ******************************************************************************/
int pso_prs_compress(const uint8_t *src, uint8_t **dst, size_t src_len) {
    struct prs_comp_cxt cxt;
    struct prs_hash_cxt *hcxt;
    int rv, mlen, mlen2;
    uint8_t tmp;
    int offset, offset2;

    /* Check the input to make sure we've got valid source/destination pointers
       and something to do. */
    if(!src || !dst)
        return PSOARCHIVE_EFAULT;

    if(!src_len)
        return PSOARCHIVE_EINVAL;

    /* Meh. Don't feel like dealing with it here, since it's not compressible
       at all anyway. */
    if(src_len <= 3)
        return pso_prs_archive(src, dst, src_len);

    /* Allocate the hash context. */
    if(!(hcxt = (struct prs_hash_cxt *)malloc(sizeof(struct prs_hash_cxt))))
        return PSOARCHIVE_EMEM;

    /* Clear the contexts and fill in what we need to do our job. */
    memset(&cxt, 0, sizeof(cxt));
    memset(hcxt, 0, sizeof(struct prs_hash_cxt));
    cxt.src = src;
    cxt.src_len = src_len;
    cxt.dst_len = pso_prs_max_compressed_size(src_len);

    /* Allocate our "compressed" buffer. */
    if(!(cxt.dst = (uint8_t *)malloc(cxt.dst_len))) {
        free(hcxt);
        return PSOARCHIVE_EMEM;
    }

    cxt.flag_ptr = cxt.dst;

    /* Add the first two "strings" to the hash table. */
    INIT_ADD_HASH(hcxt, src, tmp);
    INIT_ADD_HASH(hcxt, src + 1, tmp);

    /* Copy the first two bytes as literals... */
    if((rv = set_bit(&cxt, 1)))
        goto out;

    if((rv = copy_literal(&cxt)))
        goto out;

    if((rv = set_bit(&cxt, 1)))
        goto out;

    if((rv = copy_literal(&cxt)))
        goto out;

    /* Process each byte. */
    while(cxt.src_pos < cxt.src_len - 1) {
        /* Is there a match? */
        if((mlen = find_longest_match(&cxt, hcxt, &offset, 0))) {
            cxt.src_pos++;
            mlen2 = find_longest_match(&cxt, hcxt, &offset2, 1);
            cxt.src_pos--;

            /* Did the "lazy match" produce something more compressed? */
            if(mlen2 > mlen) {
                /* Check if it is a good idea to switch from a short match to a
                   long one, if we would do that. */
                if(mlen >= 2 && mlen <= 5 && offset2 < offset) {
                    if(offset >= -256 && offset2 < -256) {
                        if(mlen2 - mlen < 3) {
                            goto blergh;
                        }
                    }
                }

                if((rv = set_bit(&cxt, 1)))
                    goto out;

                if((rv = copy_literal(&cxt)))
                    goto out;

                continue;
            }

blergh:
            /* What kind of match did we find? */
            if(mlen >= 2 && mlen <= 5 && offset >= -256) {
                /* Short match. */
                if((rv = set_bit(&cxt, 0)))
                    goto out;

                if((rv = set_bit(&cxt, 0)))
                    goto out;

                if((rv = set_bit(&cxt, (mlen - 2) & 0x02)))
                    goto out;

                if((rv = set_bit(&cxt, (mlen - 2) & 0x01)))
                    goto out;

                if((rv = write_literal(&cxt, offset & 0xFF)))
                    goto out;

                add_intermediates(&cxt, hcxt, mlen);
                cxt.src_pos += mlen;
                continue;
            }
            else if(mlen >= 3 && mlen <= 9) {
                /* Long match, short length. */
                if((rv = set_bit(&cxt, 0)))
                    goto out;

                if((rv = set_bit(&cxt, 1)))
                    goto out;

                tmp = ((offset & 0x1f) << 3) | ((mlen - 2) & 0x07);
                if((rv = write_literal(&cxt, tmp)))
                    goto out;

                tmp = offset >> 5;
                if((rv = write_literal(&cxt, tmp)))
                    goto out;

                add_intermediates(&cxt, hcxt, mlen);
                cxt.src_pos += mlen;
                continue;
            }
            else if(mlen > 9) {
                /* Long match, long length. */
                if(mlen > 256)
                    mlen = 256;

                if((rv = set_bit(&cxt, 0)))
                    goto out;

                if((rv = set_bit(&cxt, 1)))
                    goto out;

                tmp = ((offset & 0x1f) << 3);
                if((rv = write_literal(&cxt, tmp)))
                    goto out;

                tmp = offset >> 5;
                if((rv = write_literal(&cxt, tmp)))
                    goto out;

                if((rv = write_literal(&cxt, mlen - 1)))
                    goto out;

                add_intermediates(&cxt, hcxt, mlen);
                cxt.src_pos += mlen;
                continue;
            }
        }

        /* If we get here, we didn't find a suitable match, so just write the
           byte as a literal in the output. */
        if((rv = set_bit(&cxt, 1)))
            goto out;

        /* Copy the byte over. */
        if((rv = copy_literal(&cxt)))
            goto out;
    }

    /* If we still have a left over byte at the end, put it in as a literal. */
    if(cxt.src_pos < cxt.src_len) {
        /* Set the bit in the flag since we're just putting a literal in the
           output. */
        if((rv = set_bit(&cxt, 1)))
            goto out;

        /* Copy the byte over. */
        if((rv = copy_literal(&cxt)))
            goto out;
    }

    if((rv = write_eof(&cxt)))
        goto out;

    free(hcxt);

    /* Resize the output (if realloc fails to resize it, then just use the
       unshortened buffer). */
    if(!(*dst = realloc(cxt.dst, cxt.dst_pos)))
        *dst = cxt.dst;

    return (int)cxt.dst_pos;

out:
    free(cxt.dst);
    free(hcxt);
    return rv;
}
