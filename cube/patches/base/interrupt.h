/* 
 * Copyright (c) 2021-2023, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>
#include "dolphin/os.h"

OSInterruptHandler set_interrupt_handler(OSInterrupt interrupt, OSInterruptHandler handler);
OSInterruptHandler get_interrupt_handler(OSInterrupt interrupt);
OSInterruptMask mask_interrupts(OSInterruptMask mask);
OSInterruptMask unmask_interrupts(OSInterruptMask mask);
OSInterruptMask mask_user_interrupts(OSInterruptMask mask);
OSInterruptMask unmask_user_interrupts(OSInterruptMask mask);

uint32_t exi_get_interrupt_mask(unsigned chan);

#endif /* INTERRUPT_H */
