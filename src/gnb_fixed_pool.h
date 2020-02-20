#ifndef gnb_fixed_pool_h
#define gnb_fixed_pool_h

#include <stdint.h>
#include "gnb_alloc.h"

typedef struct _gnb_fixed_pool_t gnb_fixed_pool_t;

gnb_fixed_pool_t* gnb_fixed_pool_create(gnb_heap_t *heap, uint32_t array_len, uint32_t bsize);

void* gnb_fixed_pool_pop(gnb_fixed_pool_t *fixed_pool);

uint32_t gnb_fixed_pool_push(gnb_fixed_pool_t *fixed_pool, void *block);

void gnb_fixed_pool_release(gnb_heap_t *heap,gnb_fixed_pool_t *fixed_pool);

#endif
