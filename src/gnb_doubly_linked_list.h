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

#ifndef GNB_DOUBLY_LINKED_LIST_H
#define GNB_DOUBLY_LINKED_LIST_H

#include <stdint.h>

#include "gnb_alloc.h"

typedef struct _gnb_doubly_linked_list_node_t gnb_doubly_linked_list_node_t;

typedef struct _gnb_doubly_linked_list_t gnb_doubly_linked_list_t;

typedef struct _gnb_doubly_linked_list_node_t{
    
    gnb_doubly_linked_list_node_t *pre;
    
    gnb_doubly_linked_list_node_t *nex;
    
    void *data;
    
}gnb_doubly_linked_list_node_t;


typedef struct _gnb_doubly_linked_list_t{
    
    gnb_heap_t *heap;
    
    gnb_doubly_linked_list_node_t *head;
    
    gnb_doubly_linked_list_node_t *tail;
    
    uint32_t num;
    
}gnb_doubly_linked_list_t;


void gnb_doubly_linked_list_node_set(gnb_doubly_linked_list_node_t *dl_node,void *data);


gnb_doubly_linked_list_t* gnb_doubly_linked_list_create(gnb_heap_t *heap);
void gnb_doubly_linked_list_release(gnb_doubly_linked_list_t *doubly_linked_list);


int gnb_doubly_linked_list_add(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node);

gnb_doubly_linked_list_node_t* gnb_doubly_linked_list_pop_head(gnb_doubly_linked_list_t *doubly_linked_list);
gnb_doubly_linked_list_node_t* gnb_doubly_linked_list_pop_tail(gnb_doubly_linked_list_t *doubly_linked_list);


int gnb_doubly_linked_list_pop(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node);
int gnb_doubly_linked_list_move_head(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node);

#endif
