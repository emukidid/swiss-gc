/*----------------------------------------------------------------------------*\
 *  CRC-32 version 2.0.0 by Craig Bruce, 2006-04-29.
 *
 *  This program generates the CRC-32 values for the files named in the
 *  command-line arguments.  These are the same CRC-32 values used by GZIP,
 *  PKZIP, and ZMODEM.  The Crc32_ComputeBuf() can also be detached and
 *  used independently.
 *
 *  THIS PROGRAM IS PUBLIC-DOMAIN SOFTWARE.
 *
 *  Based on the byte-oriented implementation "File Verification Using CRC"
 *  by Mark R. Nelson in Dr. Dobb's Journal, May 1992, pp. 64-67.
 *
 *  v1.0.0: original release.
 *  v1.0.1: fixed printf formats.
 *  v1.0.2: fixed something else.
 *  v1.0.3: replaced CRC constant table by generator function.
 *  v1.0.4: reformatted code, made ANSI C.  1994-12-05.
 *  v2.0.0: rewrote to use memory buffer & static table, 2006-04-29.
\*----------------------------------------------------------------------------*/

#ifndef CRC32_H_
#define CRC32_H_

unsigned long Crc32_ComputeBuf( unsigned long inCrc32, const void *buf,
                                       size_t bufLen );

#endif
