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
#include "gnb_hash32.h"


typedef struct _gnb_hash32_bucket_t{
    uint32_t chain_len;
    gnb_kv32_t *kv_chain;
}gnb_hash32_bucket_t;


uint32_t murmurhash_hash(unsigned char *data, size_t len);


gnb_hash32_map_t *gnb_hash32_create(gnb_heap_t *heap, uint32_t bucket_num, uint32_t kv_num){
    
    gnb_hash32_map_t *hash32_map = gnb_heap_alloc(heap, sizeof(gnb_hash32_map_t));
    
    if ( NULL == hash32_map ) {
        return NULL;
    }

    memset(hash32_map, 0, sizeof(gnb_hash32_map_t));

    hash32_map->heap = heap;

    hash32_map->buckets = gnb_heap_alloc(hash32_map->heap, sizeof(gnb_hash32_bucket_t *) * bucket_num );

    if ( NULL == hash32_map->buckets ) {
        return NULL;
    }

    hash32_map->bucket_num = bucket_num;

    uint32_t i;

    void *p = gnb_heap_alloc(hash32_map->heap, sizeof(gnb_hash32_bucket_t)*bucket_num );

    if ( NULL == p ) {
        return NULL;
    }

    memset(p, 0, sizeof(gnb_hash32_bucket_t)*bucket_num);
    
    for ( i=0; i<bucket_num; i++ ) {
        hash32_map->buckets[i] = p;
        p += sizeof(gnb_hash32_bucket_t);
    }

    return hash32_map;
    
}


void gnb_hash32_release(gnb_hash32_map_t *hash32_map){
    gnb_heap_free(hash32_map->heap, hash32_map);
}


gnb_kv32_t *gnb_kv32_create(gnb_hash32_map_t *hash32_map, u_char *key, uint32_t key_len, u_char *value, uint32_t value_len){

    uint32_t r_key_len;
    uint32_t r_value_len;
    
    if ( 0 != key_len ) {
        r_key_len = key_len;
    }else{
        r_key_len = sizeof(void *);
    }

    if ( 0 != value_len ) {
        r_value_len = value_len;
    }else{
        r_value_len = sizeof(void *);
    }

    gnb_kv32_t *kv = gnb_heap_alloc(hash32_map->heap, sizeof(gnb_kv32_t) + sizeof(gnb_block32_t) + r_key_len + sizeof(gnb_block32_t) + r_value_len );
    
    if ( NULL == kv ) {
        return NULL;
    }
    
    void *p = (void *)kv;
    kv->key   = p + sizeof(gnb_kv32_t);
    kv->value = p + sizeof(gnb_kv32_t) + sizeof(gnb_block32_t) + key_len;
    kv->nex = NULL;
    
    memcpy(kv->key->data, key, key_len);
    kv->key->size = key_len;

    if ( 0 != value_len ){
        memcpy(kv->value->data, value, value_len);
        kv->value->size = value_len;
    } else {
        //直接存 void*
        GNB_BLOCK_VOID(kv->value) = value;
        kv->value->size = 0;
    }
    
    return kv;
}


void gnb_kv32_release(gnb_hash32_map_t *hash_map, gnb_kv32_t *kv){

    gnb_heap_free(hash_map->heap, kv);

}


gnb_kv32_t *gnb_hash32_set(gnb_hash32_map_t *hash32_map, u_char *key, uint32_t key_len, void *value, uint32_t value_len){

    gnb_kv32_t *kv = NULL;

    uint32_t hashcode = murmurhash_hash(key, key_len);

    uint32_t bucket_idx = hashcode % hash32_map->bucket_num;
    
    gnb_hash32_bucket_t *bucket = hash32_map->buckets[bucket_idx];

    if ( NULL == bucket->kv_chain ){
        bucket->kv_chain = gnb_kv32_create(hash32_map, key, key_len, value, value_len);
        bucket->kv_chain->nex = NULL;
        bucket->chain_len = 1;
        hash32_map->kv_num++;
        goto finish;
    }

    gnb_kv32_t *kv_chain = bucket->kv_chain;
    gnb_kv32_t *pre_kv_chain = kv_chain;

    do{
    
        if ( key_len != kv_chain->key->size ) {
            pre_kv_chain = kv_chain;
            kv_chain = kv_chain->nex;
            continue;
        }

        if ( !memcmp(kv_chain->key->data, key, key_len) ) {

            kv = kv_chain;

            if ( 0 != value_len ){
                 memcpy(kv->value->data, value, value_len);
                 kv->value->size = value_len;
            } else {
                 //直接存 void*
                 GNB_BLOCK_VOID(kv->value) = value;
                 kv->value->size = 0;
            }

            break;

        }

        pre_kv_chain = kv_chain;
        kv_chain = kv_chain->nex;
        
    }while (kv_chain);

    if ( NULL == kv ){
        pre_kv_chain->nex = gnb_kv32_create(hash32_map, key, key_len, value, value_len);
        bucket->chain_len++;
        hash32_map->kv_num++;
        goto finish;
    }

finish:

    return kv;
    
}


int gnb_hash32_store(gnb_hash32_map_t *hash32_map, u_char *key, uint32_t key_len, void *value, uint32_t value_len){

    gnb_kv32_t *kv = NULL;

    uint32_t hashcode = murmurhash_hash(key, key_len);

    uint32_t bucket_idx = hashcode % hash32_map->bucket_num;
    
    gnb_hash32_bucket_t *bucket = hash32_map->buckets[bucket_idx];

    if ( NULL == bucket->kv_chain ){
        bucket->kv_chain = gnb_kv32_create(hash32_map, key, key_len, value, value_len);
        bucket->chain_len = 1;
        hash32_map->kv_num++;
        goto finish;
    }

    gnb_kv32_t *kv_chain = bucket->kv_chain;
    gnb_kv32_t *pre_kv_chain = kv_chain;
    
    do{
        
        if ( key_len != kv_chain->key->size ) {
            pre_kv_chain = kv_chain;
            kv_chain = kv_chain->nex;
            continue;
        }

        if ( !memcmp(kv_chain->key->data, key, key_len) ) {
            
            gnb_kv32_release(hash32_map, kv_chain);

            if ( bucket->kv_chain != kv_chain ) {

                pre_kv_chain->nex = gnb_kv32_create(hash32_map, key, key_len, value, value_len);

            } else {

                bucket->kv_chain = gnb_kv32_create(hash32_map, key, key_len, value, value_len);
            }

            break;
        }

        pre_kv_chain = kv_chain;
        kv_chain = kv_chain->nex;
        
    }while (kv_chain);

    if ( NULL == kv ){
        pre_kv_chain->nex = gnb_kv32_create(hash32_map, key, key_len, value, value_len);
        bucket->chain_len++;
        hash32_map->kv_num++;
        goto finish;
    }

finish:

    return 0;
    
}


gnb_kv32_t *gnb_hash32_get(gnb_hash32_map_t *hash32_map, u_char *key, uint32_t key_len){
    
    gnb_kv32_t *kv = NULL;
    
    uint32_t hashcode = murmurhash_hash(key, key_len);

    uint32_t bucket_idx = hashcode % hash32_map->bucket_num;
    
    gnb_hash32_bucket_t *bucket = hash32_map->buckets[bucket_idx];

    if ( NULL == bucket->kv_chain ){
        goto finish;
    }

    gnb_kv32_t *kv_chain = bucket->kv_chain;

    do{
    
        if ( key_len != kv_chain->key->size ) {
            kv_chain = kv_chain->nex;
            continue;
        }
        
        if ( !memcmp(kv_chain->key->data, key, key_len) ) {
            kv = kv_chain;
            break;
        }

        kv_chain = kv_chain->nex;
        
    }while (kv_chain);

finish:

    return kv;
    
}


gnb_kv32_t *gnb_hash32_del(gnb_hash32_map_t *hash32_map, u_char *key, uint32_t key_len){

    gnb_kv32_t *old_kv = NULL;

    uint32_t hashcode = murmurhash_hash(key, key_len);

    uint32_t bucket_idx = hashcode % hash32_map->bucket_num;
    
    gnb_hash32_bucket_t *bucket = hash32_map->buckets[bucket_idx];
    
    if ( NULL == bucket->kv_chain ){
        goto finish;
    }

    gnb_kv32_t *kv_chain = bucket->kv_chain;
    
    if ( !memcmp(kv_chain->key->data, key, key_len) ) {

        old_kv = kv_chain;

        hash32_map->kv_num--;
        
        if ( 1 == bucket->chain_len ){
            bucket->kv_chain = NULL;
            bucket->chain_len = 0;
        }else{
            bucket->kv_chain = kv_chain->nex;
            bucket->chain_len--;
        }
        goto finish;

    }
    
    gnb_kv32_t *pre_kv_chain = kv_chain;
    
    kv_chain = kv_chain->nex;
    
    if ( NULL == kv_chain ) {
        goto finish;
    }

    do{
    
        if ( key_len != kv_chain->key->size ) {
            pre_kv_chain = kv_chain;
            kv_chain = kv_chain->nex;
            continue;
        }

        if ( !memcmp(kv_chain->key->data, key, key_len) ) {
            pre_kv_chain->nex = kv_chain->nex;
            old_kv = kv_chain;
            bucket->chain_len--;
            hash32_map->kv_num--;
            break;
        }
        
        pre_kv_chain = kv_chain;
        kv_chain = kv_chain->nex;
        
    }while (kv_chain);

finish:

    return old_kv;

}


gnb_kv32_t **gnb_hash32_array(gnb_hash32_map_t *hash32_map, uint32_t *num){
    
    gnb_kv32_t *kv;

    gnb_kv32_t ** kv_array = NULL;

    gnb_hash32_bucket_t *bucket;
    
    if ( 0 == hash32_map->kv_num ){
        *num = 0;
        return NULL;
    }

    if ( hash32_map->kv_num < (*num) ){
        *num = hash32_map->kv_num;
    }

    kv_array = gnb_heap_alloc(hash32_map->heap, sizeof(gnb_kv32_t *) * (*num) );

    uint32_t i,j;
    uint32_t idx = 0;

    for (i=0; i<hash32_map->bucket_num; i++) {

        bucket = hash32_map->buckets[i];

        if ( 0 == bucket->chain_len ) {
            continue;
        }

        kv = bucket->kv_chain;

        for (j=0; j<bucket->chain_len; j++) {
            kv_array[idx] = kv;
            idx++;
            kv = kv->nex;
        }

    }

    return kv_array;

}


uint32_t* gnb_hash32_uint32_keys(gnb_hash32_map_t *hash32_map, uint32_t *num){
    
    gnb_kv32_t *kv;

    uint32_t *uint32_array = NULL;

    gnb_hash32_bucket_t *bucket;
    
    if ( 0 == hash32_map->kv_num ){
        *num = 0;
        return NULL;
    }

    if ( hash32_map->kv_num < (*num) ){
        *num = hash32_map->kv_num;
    }

    uint32_array = gnb_heap_alloc(hash32_map->heap, sizeof(uint32_t) * (*num) );

    uint32_t i,j;
    uint32_t idx = 0;

    for (i=0; i<hash32_map->bucket_num; i++) {

        bucket = hash32_map->buckets[i];

        if ( 0 == bucket->chain_len ) {
            continue;
        }

        kv = bucket->kv_chain;

        for (j=0; j<bucket->chain_len; j++) {
            uint32_array[idx] = GNB_HASH32_UINT32_KEY(kv);
            idx++;
            kv = kv->nex;
        }

    }

    return uint32_array;
    
}


uint64_t* gnb_hash32_uint64_keys(gnb_hash32_map_t *hash32_map, uint32_t *num){

    gnb_kv32_t *kv;
    uint64_t *uint64_array = NULL;

    gnb_hash32_bucket_t *bucket;
    
    if ( 0 == hash32_map->kv_num ){
        *num = 0;
        return NULL;
    }

    if ( hash32_map->kv_num < (*num) ){
        *num = hash32_map->kv_num;
    }

    uint64_array = gnb_heap_alloc(hash32_map->heap, sizeof(uint64_t) * (*num) );

    uint32_t i,j;
    uint32_t idx = 0;

    for (i=0; i<hash32_map->bucket_num; i++) {

        bucket = hash32_map->buckets[i];

        if ( 0 == bucket->chain_len ) {
            continue;
        }

        kv = bucket->kv_chain;

        for (j=0; j<bucket->chain_len; j++) {
            uint64_array[idx] = GNB_HASH32_UINT64_KEY(kv);
            idx++;
            kv = kv->nex;
        }

    }

    return uint64_array;
    
}


u_char** gnb_hash32_string_keys(gnb_hash32_map_t *hash32_map, uint32_t *num){

    gnb_kv32_t *kv;
    u_char** string_array = NULL;

    gnb_hash32_bucket_t *bucket;
    
    if ( 0 == hash32_map->kv_num ){
        *num = 0;
        return NULL;
    }
    
    if ( hash32_map->kv_num < (*num) ){
        *num = hash32_map->kv_num;
    }

    string_array = gnb_heap_alloc(hash32_map->heap, sizeof(u_char*) * (*num) );

    uint32_t i,j;
    uint32_t idx = 0;

    for (i=0; i<hash32_map->bucket_num; i++) {

        bucket = hash32_map->buckets[i];

        if ( 0 == bucket->chain_len ) {
            continue;
        }

        kv = bucket->kv_chain;

        for (j=0; j<bucket->chain_len; j++) {
            string_array[idx] = GNB_HASH32_STRING_KEY(kv);
            idx++;
            kv = kv->nex;
        }

    }

    return string_array;

}

