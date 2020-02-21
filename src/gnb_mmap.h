#ifndef gnb_mmap_h
#define gnb_mmap_h

#include <stddef.h>

typedef struct _gnb_mmap_block_t gnb_mmap_block_t;


#define GNB_MMAP_TYPE_READONLY               (0x0)
#define GNB_MMAP_TYPE_READWRITE              (0x1)
#define GNB_MMAP_TYPE_CREATE                 (0x1 << 1)
#define GNB_MMAP_TYPE_CLEANEXIT              (0x1 << 2)


gnb_mmap_block_t* gnb_mmap_create(const char *filename, size_t block_size, int mmap_type);

void gnb_mmap_release(gnb_mmap_block_t *mmap_block);

void* gnb_mmap_get_block(gnb_mmap_block_t *mmap_block);

size_t gnb_mmap_get_size(gnb_mmap_block_t *mmap_block);

#endif
