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

#ifndef GNB_RING_BUFFER_H
#define GNB_RING_BUFFER_H

#include <stdint.h>

typedef struct _gnb_ring_node_t gnb_ring_node_t;

typedef struct _gnb_ring_node_t{

    void *data;

    size_t size;

}gnb_ring_node_t;


typedef struct _gnb_ring_buffer_t{

    size_t num;

    size_t block_size;

    gnb_ring_node_t **nodes;

	int head_idx;
	int tail_idx;
    
}gnb_ring_buffer_t;

#define GNB_RING_BUFFER_FULL  1
#define GNB_RING_BUFFER_EMPTY 2

#define GNB_RING_BUFFER_BLOCK_NOT_ENOUGH -1

gnb_ring_buffer_t *gnb_ring_buffer_init(size_t num, size_t block_size);

void gnb_ring_buffer_release(gnb_ring_buffer_t *ring_buffer);

int gnb_ring_buffer_copy_in(gnb_ring_buffer_t *ring_buffer, void *data, size_t size);
int gnb_ring_buffer_copy_out(gnb_ring_buffer_t *ring_buffer, void *data, size_t *size);


gnb_ring_node_t *gnb_ring_buffer_push(gnb_ring_buffer_t *ring_buffer);
void gnb_ring_buffer_push_submit(gnb_ring_buffer_t *ring_buffer);


gnb_ring_node_t *gnb_ring_buffer_pop(gnb_ring_buffer_t *ring_buffer);
void gnb_ring_buffer_pop_submit(gnb_ring_buffer_t *ring_buffer);

#endif
