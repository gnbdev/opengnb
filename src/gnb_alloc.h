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

#ifndef GNB_ALLOC_H
#define GNB_ALLOC_H

#include <stdio.h>
#include <stdint.h>

typedef struct _gnb_heap_fragment_t gnb_heap_fragment_t;

typedef struct _gnb_heap_t{

    uint32_t max_fragment;
    uint32_t fragment_nums;
    uint32_t alloc_byte;
    uint32_t ralloc_byte;
    gnb_heap_fragment_t *fragment_list[0];

}gnb_heap_t;

gnb_heap_t* gnb_heap_create(uint32_t max_fragment);

void* gnb_heap_alloc(gnb_heap_t *gnb_heap, uint32_t size);

void gnb_heap_free(gnb_heap_t *gnb_heap, void *p);

void gnb_heap_clean(gnb_heap_t *gnb_heap);

void gnb_heap_release(gnb_heap_t *gnb_heap);

#endif
