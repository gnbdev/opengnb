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


#include <stdlib.h>
#include <string.h>

#include "gnb_ring_buffer_fixed.h"

size_t gnb_ring_buffer_fixed_sum_size(size_t block_size, unsigned short block_num_mask){
    
    size_t memory_size;

    size_t block_num = 0xFF+1;
    
    switch (block_num_mask) {
        
        case 0xFFFF : //65535
        case 0x7FFF : //32767
        case 0x3FFF : //16383
        case 0x1FFF : //8191
        case 0xFFF  : //4095
        case 0x7FF  : //2047
        case 0x3FF  : //1023
        case 0x1FF  : //511
        case 0xFF   : //255
        case 0x7F   : //127
        case 0x3F   : //63
        case 0x1F   : //31
        case 0xF    : //15
        case 0x7    : //7
        case 0x3    : //3
            block_num = block_num_mask+1;
            break;

        default:
            block_num = 0xFF+1;
            break;

    }

    memory_size = sizeof(gnb_ring_buffer_fixed_t) + block_size * block_num;
    
    return memory_size;

}

gnb_ring_buffer_fixed_t* gnb_ring_buffer_fixed_init(void *memory, size_t block_size, unsigned short block_num_mask){

    gnb_ring_buffer_fixed_t *gnb_ring_buffer_fixed;

    size_t memory_size;

    gnb_ring_buffer_fixed = (gnb_ring_buffer_fixed_t*)memory;

    memory_size = gnb_ring_buffer_fixed_sum_size(block_size, block_num_mask);

    memset(memory, 0, memory_size);
    
    gnb_ring_buffer_fixed->block_num_mask = block_num_mask;
    
    gnb_ring_buffer_fixed->block_size = block_size;

    gnb_ring_buffer_fixed->memory_size = memory_size;

    return gnb_ring_buffer_fixed;
    
}

void* gnb_ring_buffer_fixed_push(gnb_ring_buffer_fixed_t *ring_buffer_fixed){

    int tail_next_idx = (ring_buffer_fixed->tail_idx + 1) & ring_buffer_fixed->block_num_mask;

    if ( tail_next_idx == ring_buffer_fixed->head_idx ) {
        return  NULL;
    }

    void *buffer_header = ring_buffer_fixed->blocks + ring_buffer_fixed->block_size * ring_buffer_fixed->tail_idx;

    return buffer_header;

}

void gnb_ring_buffer_fixed_push_submit(gnb_ring_buffer_fixed_t *ring_buffer_fixed){

    int tail_next_idx = (ring_buffer_fixed->tail_idx + 1) & ring_buffer_fixed->block_num_mask;

    ring_buffer_fixed->tail_idx = tail_next_idx;

}

void* gnb_ring_buffer_fixed_pop(gnb_ring_buffer_fixed_t *ring_buffer_fixed){

    if ( ring_buffer_fixed->head_idx == ring_buffer_fixed->tail_idx ) {
        return NULL;
    }

    void *buffer_header = ring_buffer_fixed->blocks + ring_buffer_fixed->block_size * ring_buffer_fixed->head_idx;

    return buffer_header;

}

void gnb_ring_buffer_fixed_pop_submit(gnb_ring_buffer_fixed_t *ring_buffer_fixed){

    int head_next_idx = (ring_buffer_fixed->head_idx + 1) & ring_buffer_fixed->block_num_mask;

    ring_buffer_fixed->head_idx = head_next_idx;

}
