/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GNB_FIXED_POOL_H
#define GNB_FIXED_POOL_H

#include <stdint.h>

#include "gnb_alloc.h"

typedef struct _gnb_fixed_pool_t gnb_fixed_pool_t;

gnb_fixed_pool_t* gnb_fixed_pool_create(gnb_heap_t *heap, uint32_t array_len, uint32_t bsize);

void* gnb_fixed_pool_pop(gnb_fixed_pool_t *fixed_pool);

uint32_t gnb_fixed_pool_push(gnb_fixed_pool_t *fixed_pool, void *block);

void gnb_fixed_pool_release(gnb_heap_t *heap,gnb_fixed_pool_t *fixed_pool);

#endif
