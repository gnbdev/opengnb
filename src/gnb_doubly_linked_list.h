#ifndef gnb_doubly_linked_list_h
#define gnb_doubly_linked_list_h

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
