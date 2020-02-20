#ifndef gnb_block_h
#define gnb_block_h

#include <stdint.h>


typedef struct _gnb_block32_t {
    uint32_t size;
    unsigned char data[0];
}gnb_block32_t;


#define GNB_BLOCK_VOID(block)  *(void **)&block->data

#endif
