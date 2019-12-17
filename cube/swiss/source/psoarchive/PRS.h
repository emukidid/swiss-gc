/*
    This file is part of libpsoarchive.

    Copyright (C) 2014, 2015 Lawrence Sebald

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

#ifndef PSOARCHIVE__PRS_H
#define PSOARCHIVE__PRS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "psoarchive-error.h"

/* Compress a buffer with PRS compression.

   This function compresses the data in the src buffer into a new buffer. This
   function will never produce output larger than that of the prs_archive
   function, and will usually beat that function rather significantly.

   In testing, the compressed output of this function actually beats Sega's own
   compression slightly (by 100 bytes or so on an uncompressed version of the
   ItemPMT.prs from PSO Episode I & II Plus).

   It is the caller's responsibility to free *dst when it is no longer in use.

   Returns a negative value on failure (specifically something from
   psoarchive-error.h). Returns the size of the compressed output on success.
*/
int pso_prs_compress(const uint8_t *src, uint8_t **dst, size_t src_len);

/* Archive a buffer in PRS format.

   This function archives the data in the src buffer into a new buffer. This
   function will always produce output that is larger in size than the input
   data (it does not actually compress the output. There's probably no good
   reason to ever use this, but it is here if you want it for some reason.

   All the notes about parameters and return values from prs_compress also apply
   to this function. The size of the output from this function will be equal to
   the return value of prs_max_compressed_size when called on the same length.
*/
int pso_prs_archive(const uint8_t *src, uint8_t **dst, size_t src_len);

/* Archive a buffer in PRS format into a preallocated buffer.

   This function archives the data in the src buffer into a preallocated buffer.
   This function will always produce output that is larger in size than the
   input data (it does not actually compress the output. There's probably no
   good reason to ever use this, but it is here if you want it for some reason.

   The buffer must be at least as large as what prs_max_compressed_size returns
   when given the same input length.

   All the notes about parameters and return values from prs_compress also apply
   to this function. The size of the output from this function will be equal to
   the return value of prs_max_compressed_size when called on the same length.

   While I said that pso_prs_archive probably has no good use, this function has
   even less potentially good uses. Basically, it's used internally by
   pso_prsd_archive, and that's about the only place it's probably applicable.
*/
int pso_prs_archive2(const uint8_t *src, uint8_t *dst, size_t src_len,
                     size_t dst_len);

/* Return the maximum size of archiving a buffer in PRS format.

   This function returns the size that prs_archive will spit out. This is used
   internally to allocate memory for prs_archive and prs_compress and probably
   has little utility outside of that.
*/
size_t pso_prs_max_compressed_size(size_t len);

/* Decompress a PRS archive from a file.

   This function opens the file specified and decompresses the data from the
   file into a newly allocated memory buffer.

   It is the caller's responsibility to free *dst when it is no longer in use.

   Returns a negative value on failure (specifically something from
   psoarchive-error.h). Returns the size of the decompressed output on success.
*/
int pso_prs_decompress_file(const char *fn, uint8_t **dst);

/* Decompress PRS-compressed data from a memory buffer.

   This function decompresses PRS-compressed data from the src buffer into a
   newly allocated memory buffer.

   It is the caller's responsibility to free *dst when it is no longer in use.

   Returns a negative value on failure (specifically something from
   psoarchive-error.h). Returns the size of the decompressed output on success.
*/
int pso_prs_decompress_buf(const uint8_t *src, uint8_t **dst, size_t src_len);

/* Decompress PRS-compressed data from a memory buffer into a previously
   allocated memory buffer.

   This function decompresses PRS-compressed data from the src buffer into the
   previously allocated allocated memory buffer dst. You must have already
   allocated the buffer at dst, and it should be at least the size returned by
   prs_decompress_size on the compressed input (otherwise, you will get an error
   back from the function).

   Returns a negative value on failure (specifically something from
   psoarchive-error.h). Returns the size of the decompressed output on success.
*/
int pso_prs_decompress_buf2(const uint8_t *src, uint8_t *dst, size_t src_len,
                            size_t dst_len);

/* Determine the size that the PRS-compressed data in a buffer will expand to.

   This function essentially decompresses the PRS-compressed data from the src
   buffer without writing any output. By doing this, it determines the actual
   size of the decompressed output and returns it.

   Returns a negative value on failure (specifically something from
   psoarchive-error.h). Returns the size of the decompressed output on success.
*/
int pso_prs_decompress_size(const uint8_t *src, size_t src_len);

#endif /* !PSOARCHIVE__PRS_H */
