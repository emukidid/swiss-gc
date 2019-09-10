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
static tb_t     *const _start     = (tb_t     *)VAR_TIMER_START;
static uint16_t *const _id        = (uint16_t *)VAR_IPV4_ID;
static uint16_t *const _key       = (uint16_t *)VAR_FSP_KEY;
static uint16_t *const _data_size = (uint16_t *)VAR_FSP_DATA_LENGTH;
static uint32_t *const _position  = (uint32_t *)VAR_FSP_POSITION;
static void    **const _data      = (void    **)VAR_TMP1;
static uint32_t *const _remainder = (uint32_t *)VAR_TMP2;
static uint32_t *const _received  = (uint32_t *)VAR_INTERRUPT_TIMES;

#endif /* GLOBALS_H */
