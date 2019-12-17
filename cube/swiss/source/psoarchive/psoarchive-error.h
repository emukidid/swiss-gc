/*
    This file is part of libpsoarchive.

    Copyright (C) 2015 Lawrence Sebald

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

#ifndef PSOARCHIVE__ERROR_H
#define PSOARCHIVE__ERROR_H

typedef enum {
    PSOARCHIVE_OK = 0,
    PSOARCHIVE_EFILE = -1,
    PSOARCHIVE_EMEM = -2,
    PSOARCHIVE_EFATAL = -3,
    PSOARCHIVE_NOARCHIVE = -4,
    PSOARCHIVE_EMPTY = -5,
    PSOARCHIVE_EIO = -6,
    PSOARCHIVE_ERANGE = -7,
    PSOARCHIVE_EFAULT = -8,
    PSOARCHIVE_EINVAL = -9,
    PSOARCHIVE_ENOSPC = -10,
    PSOARCHIVE_EBADMSG = -11,
    PSOARCHIVE_ENOTSUPP = -12
} pso_error_t;

#define PSOARCHIVE_HND_INVALID  0xFFFFFFFF

const char *pso_strerror(pso_error_t err);

#endif /* !PSOARCHIVE__ERROR_H */
