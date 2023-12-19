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

#ifndef gnb_ring_buffer_fixed_h
#define gnb_ring_buffer_fixed_h

#include <stdint.h>

typedef struct _gnb_ring_buffer_fixed_t{

    unsigned short block_num_mask;
    
    size_t block_size;

    size_t memory_size;
    
    unsigned int head_idx;
    unsigned int tail_idx;

    unsigned char blocks[0];
    
} __attribute__ ((aligned (4))) gnb_ring_buffer_fixed_t;


/*
 block_num_mask must be:

 0xFFFF  65535
 0x7FFF  32767
 0x3FFF  16383
 0x1FFF   8191
 0xFFF    4095
 0x7FF    2047
 0x3FF    1023
 0x1FF     511
 0xFF      255
 0x7F      127
 0x3F       63
 0x1F       31
 0xF        15
 0x7         7
 0x3         3
*/
size_t gnb_ring_buffer_fixed_sum_size(size_t block_size, unsigned short block_num_mask);

gnb_ring_buffer_fixed_t* gnb_ring_buffer_fixed_init(void *memory, size_t block_size, unsigned short block_num_mask);

void* gnb_ring_buffer_fixed_push(gnb_ring_buffer_fixed_t *ring_buffer_fixed);

void gnb_ring_buffer_fixed_push_submit(gnb_ring_buffer_fixed_t *ring_buffer_fixed);

void* gnb_ring_buffer_fixed_pop(gnb_ring_buffer_fixed_t *ring_buffer_fixed);

void gnb_ring_buffer_fixed_pop_submit(gnb_ring_buffer_fixed_t *ring_buffer_fixed);

#endif
