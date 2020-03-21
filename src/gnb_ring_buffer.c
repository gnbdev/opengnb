#include <stdlib.h>
#include <string.h>

#include "gnb_ring_buffer.h"


gnb_ring_buffer_t *gnb_ring_buffer_init(size_t num, size_t block_size) {
    
	gnb_ring_buffer_t *ring_buffer = (gnb_ring_buffer_t *)malloc(sizeof(gnb_ring_buffer_t));
    
    ring_buffer->num = num;
    
    ring_buffer->block_size = block_size;

    ring_buffer->nodes = malloc( sizeof(gnb_ring_node_t *) * num);
    
    int i = 0;
    
    for (i=0;i<ring_buffer->num;i++) {
        
        ring_buffer->nodes[i] = malloc( sizeof(gnb_ring_node_t) );
        ring_buffer->nodes[i]->data = malloc(block_size);

    }
    
    ring_buffer->head_idx = 0;
    ring_buffer->tail_idx = 0;
    
    return ring_buffer;
    
}


void gnb_ring_buffer_release(gnb_ring_buffer_t *ring_buffer){

	int i=0;

	void *p;

	for (i=0;i<ring_buffer->num;i++){
		free(ring_buffer->nodes[i]->data);
		free(ring_buffer->nodes[i]);
	}

	free(ring_buffer->nodes);

	free(ring_buffer);

}


int gnb_ring_buffer_copy_in(gnb_ring_buffer_t *ring_buffer, void *data, size_t size){
    
	if ( size > ring_buffer->block_size ){
		return GNB_RING_BUFFER_BLOCK_NOT_ENOUGH;
	}

	int tail_next_idx = ring_buffer->tail_idx + 1;

	if ( tail_next_idx >= ring_buffer->num ){
		tail_next_idx = 0;
	}

	if ( tail_next_idx == ring_buffer->head_idx ){
		return  GNB_RING_BUFFER_FULL;
	}

	memcpy(ring_buffer->nodes[ring_buffer->tail_idx]->data , data, size);

	ring_buffer->nodes[ring_buffer->tail_idx]->size = size;

	ring_buffer->tail_idx = tail_next_idx;

    return 0;
}


int gnb_ring_buffer_copy_out(gnb_ring_buffer_t *ring_buffer, void *data, size_t *size){

	if ( *size > ring_buffer->block_size ){
		return GNB_RING_BUFFER_BLOCK_NOT_ENOUGH;
	}

	if ( ring_buffer->head_idx == ring_buffer->tail_idx ){
		return GNB_RING_BUFFER_EMPTY;
	}

	int head_next_idx = ring_buffer->head_idx + 1;

	if ( head_next_idx >= ring_buffer->num ){
		head_next_idx = 0;
	}

	memcpy(data, ring_buffer->nodes[ring_buffer->head_idx]->data, ring_buffer->nodes[ring_buffer->head_idx]->size);

	ring_buffer->head_idx = head_next_idx;

    return 0;

}


gnb_ring_node_t *gnb_ring_buffer_push(gnb_ring_buffer_t *ring_buffer){

	int tail_next_idx = ring_buffer->tail_idx + 1;

	if ( tail_next_idx >= ring_buffer->num ){
		tail_next_idx = 0;
	}

	if ( tail_next_idx == ring_buffer->head_idx ){
		return  NULL;
	}

	return ring_buffer->nodes[ring_buffer->tail_idx];
}

void gnb_ring_buffer_push_submit(gnb_ring_buffer_t *ring_buffer){

	int tail_next_idx = ring_buffer->tail_idx + 1;

	if ( tail_next_idx >= ring_buffer->num ){
		tail_next_idx = 0;
	}

	ring_buffer->tail_idx = tail_next_idx;

}



gnb_ring_node_t *gnb_ring_buffer_pop(gnb_ring_buffer_t *ring_buffer){

	if ( ring_buffer->head_idx == ring_buffer->tail_idx ){
		return NULL;
	}

	return ring_buffer->nodes[ring_buffer->head_idx];

}

void gnb_ring_buffer_pop_submit(gnb_ring_buffer_t *ring_buffer){

	int head_next_idx = ring_buffer->head_idx + 1;

	if ( head_next_idx >= ring_buffer->num ){
		head_next_idx = 0;
	}

	ring_buffer->head_idx = head_next_idx;
}


