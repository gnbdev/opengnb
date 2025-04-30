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
#include "gnb_doubly_linked_list.h"

void gnb_doubly_linked_list_node_set(gnb_doubly_linked_list_node_t *doubly_linked_list_node,void *data){
    doubly_linked_list_node->data = data;
}


gnb_doubly_linked_list_t* gnb_doubly_linked_list_create(gnb_heap_t *heap){
    
    gnb_doubly_linked_list_t *doubly_linked_list = (gnb_doubly_linked_list_t *)gnb_heap_alloc(heap,sizeof(gnb_doubly_linked_list_t));
    
    memset(doubly_linked_list,0,sizeof(gnb_doubly_linked_list_t));
    doubly_linked_list->heap = heap;
    return doubly_linked_list;
    
}


void gnb_doubly_linked_list_release(gnb_doubly_linked_list_t *doubly_linked_list){
    gnb_heap_free(doubly_linked_list->heap,doubly_linked_list);
}


int gnb_doubly_linked_list_add(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node){

    if ( 0==doubly_linked_list->num ) {
        doubly_linked_list->head = dl_node;
        dl_node->pre = NULL;
        dl_node->nex = NULL;
        doubly_linked_list->tail = dl_node;
        goto finish;
    }

    if ( 1==doubly_linked_list->num ) {
        
        dl_node->pre = NULL;
        dl_node->nex = doubly_linked_list->head;
        doubly_linked_list->head->pre = dl_node;
        
        doubly_linked_list->tail = doubly_linked_list->head;
        
        doubly_linked_list->head = dl_node;
        
        goto finish;
        
    }

    dl_node->pre = NULL;
    dl_node->nex = doubly_linked_list->head;
    
    doubly_linked_list->head->pre = dl_node;
    
    doubly_linked_list->head = dl_node;
    
finish:
    doubly_linked_list->num++;
    
    return 0;
}


gnb_doubly_linked_list_node_t* gnb_doubly_linked_list_pop_head(gnb_doubly_linked_list_t *doubly_linked_list){
    
    gnb_doubly_linked_list_node_t *dl_node;
    
    if ( 0 == doubly_linked_list->num ) {
        return NULL;
    }

    if ( 1 == doubly_linked_list->num ) {
        
        if ( doubly_linked_list->head != doubly_linked_list->tail ) {
            return NULL;
        }

        dl_node = doubly_linked_list->head;

        doubly_linked_list->head = NULL;
        doubly_linked_list->tail = NULL;
        doubly_linked_list->num = 0;

        return dl_node;
    }

    if ( doubly_linked_list->head == doubly_linked_list->tail ) {
        return NULL;
    }
    
    dl_node = doubly_linked_list->head;

    doubly_linked_list->head = dl_node->nex;

    doubly_linked_list->head->pre = NULL;

    doubly_linked_list->num--;

    return dl_node;
    
}


gnb_doubly_linked_list_node_t* gnb_doubly_linked_list_pop_tail(gnb_doubly_linked_list_t *doubly_linked_list){

    gnb_doubly_linked_list_node_t *dl_node;

    if ( 0 == doubly_linked_list->num ) {
        return NULL;
    }

    if ( 1 == doubly_linked_list->num ) {
        
        if ( doubly_linked_list->head != doubly_linked_list->tail ) {
            return NULL;
        }

        dl_node = doubly_linked_list->tail;

        doubly_linked_list->head = NULL;
        doubly_linked_list->tail = NULL;
        doubly_linked_list->num = 0;

        return dl_node;
    }

    if ( doubly_linked_list->head == doubly_linked_list->tail ) {
        return NULL;
    }
    
    dl_node = doubly_linked_list->tail;

    doubly_linked_list->tail = dl_node->pre;
    doubly_linked_list->tail->nex = NULL;

    doubly_linked_list->num--;

    return dl_node;
    
}



int gnb_doubly_linked_list_move_head(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node){
    
    if ( 1==doubly_linked_list->num || 2==doubly_linked_list->num ) {
        return 0;
    }
    
    gnb_doubly_linked_list_node_t *pre_node = dl_node->pre;
    gnb_doubly_linked_list_node_t *nex_node = dl_node->nex;

    if( NULL == pre_node ) {
        //is header
        return 0;
    }
    
    if ( NULL != nex_node ) {
        nex_node->pre = pre_node;
    } else {
        doubly_linked_list->tail = pre_node;
    }
    
    pre_node->nex = nex_node;
    
    dl_node->nex = doubly_linked_list->head;
    
    doubly_linked_list->head->pre = dl_node;
    doubly_linked_list->head = dl_node;
    
    dl_node->pre = NULL;

    return 0;
}


int gnb_doubly_linked_list_pop(gnb_doubly_linked_list_t *doubly_linked_list, gnb_doubly_linked_list_node_t *dl_node){

    if ( 0==doubly_linked_list->num ) {
        return 0;
    }

    gnb_doubly_linked_list_node_t *pre_node = dl_node->pre;
    gnb_doubly_linked_list_node_t *nex_node = dl_node->nex;

    if ( NULL == pre_node && NULL == nex_node ) {

        if ( doubly_linked_list->head != dl_node ) {
            return -1;
        }

        if ( doubly_linked_list->tail != dl_node ) {
            return -1;
        }
        
        if ( 1 != doubly_linked_list->num ) {
            return -1;
        }

        doubly_linked_list->head = NULL;
        doubly_linked_list->tail = NULL;
        doubly_linked_list->num = 0;

        return 0;

    }

    if ( NULL == pre_node ) {
        //is header
        if ( doubly_linked_list->head != dl_node ) {
            return -1;
        }

        if ( 1 == doubly_linked_list->num ) {
            return -1;
        }

        nex_node->pre = NULL;
        doubly_linked_list->head = nex_node;
        doubly_linked_list->num--;

        return 0;
    }

    if ( NULL==nex_node ) {
        //is tail
        if ( doubly_linked_list->tail != dl_node ) {
            return -1;
        }

        if ( 1 == doubly_linked_list->num ) {
            return -1;
        }
        
        pre_node->nex = NULL;
        doubly_linked_list->tail = pre_node;
        doubly_linked_list->num--;

        return 0;
    }
    
    pre_node->nex = nex_node;
    nex_node->pre = pre_node;
    doubly_linked_list->num--;
    
    return 0;
}
