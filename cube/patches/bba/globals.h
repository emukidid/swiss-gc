/* 
 * Copyright (c) 2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "../../reservedarea.h"

static uint16_t *const _port      = (uint16_t *)VAR_SERVER_PORT;
static uint8_t  *const _filelen   = (uint8_t  *)VAR_DISC_1_FNLEN;
static char     *const _file      = (char     *)VAR_DISC_1_FN;
static uint8_t  *const _file2len  = (uint8_t  *)VAR_DISC_2_FNLEN;
static char     *const _file2     = (char     *)VAR_DISC_2_FN;
static tb_t     *const _start     = (tb_t     *)VAR_TIMER_START;
static uint32_t *const _changing  = (uint32_t *)VAR_DISC_CHANGING;
static uint16_t *const _id        = (uint16_t *)VAR_IPV4_ID;
static uint16_t *const _key       = (uint16_t *)VAR_FSP_KEY;
static uint16_t *const _data_size = (uint16_t *)VAR_FSP_DATA_LENGTH;
static uint32_t *const _disc2     = (uint32_t *)VAR_CURRENT_DISC;
static uint32_t *const _position  = (uint32_t *)VAR_LAST_OFFSET;
static void    **const _data      = (void    **)VAR_TMP1;
static uint32_t *const _remainder = (uint32_t *)VAR_TMP2;
static uint32_t *const _received  = (uint32_t *)VAR_INTERRUPT_TIMES;

#endif /* GLOBALS_H */
