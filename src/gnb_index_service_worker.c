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

#ifdef _WIN32
#define _POSIX
#define __USE_MINGW_ALARM
#endif


#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "gnb.h"
#include "gnb_node.h"
#include "gnb_worker.h"
#include "gnb_ring_buffer_fixed.h"
#include "gnb_lru32.h"
#include "gnb_time.h"
#include "gnb_binary.h"
#include "gnb_worker_queue_data.h"
#include "ed25519/ed25519.h"
#include "gnb_index_frame_type.h"


typedef struct _index_service_worker_ctx_t {

    gnb_core_t *gnb_core;

    //注意，lur 中保存的是数据(gnb_key_address_t)拷贝，不是指针
    gnb_lru32_t *lru;

    gnb_payload16_t   *index_frame_payload;

    uint64_t now_time_sec;
    uint64_t now_time_usec;

    pthread_t thread_worker;

}index_service_worker_ctx_t;


typedef struct _gnb_key_address_t{

    uint32_t uuid32;

    // key_node 最后发送 post_addr_frame的时间
    uint64_t last_post_addr4_sec;
    uint64_t last_post_addr6_sec;

    //key_node 最后发送 request_addr_frame 的时间戳
    uint64_t last_send_request_addr_usec;

    //源节点对自身探测的wan地址
    uint8_t  wan6_addr[16];
    uint16_t wan6_port;

    uint8_t  wan4_addr[4];
    uint16_t wan4_port;

    unsigned char node_random_sequence[NODE_RANDOM_SEQUENCE_SIZE];
    unsigned char node_random_sequence_sign[ED25519_SIGN_SIZE];

    unsigned char address6_list_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_KEY_ADDRESS_NUM];
    unsigned char address4_list_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_KEY_ADDRESS_NUM];

    char attachmenta[INDEX_ATTACHMENT_SIZE];  //保存 PAYLOAD_SUB_TYPE_POST_ADDR 的 attachment
    char attachmentb[INDEX_ATTACHMENT_SIZE];  //保存 PAYLOAD_SUB_TYPE_POST_ADDR 的 attachment

}gnb_key_address_t;


static void send_echo_addr_frame(gnb_worker_t *gnb_index_service_worker, unsigned char *key512, uint32_t uuid32, gnb_address_t *address);
static void send_push_addr_frame(gnb_worker_t *gnb_index_service_worker, unsigned char action, unsigned char attachment, unsigned char *src_key, gnb_key_address_t *src_key_address, unsigned char *dst_key, gnb_key_address_t *dst_key_address);
static void handle_post_addr_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *index_service_worker_in_data);
static void handle_request_addr_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *index_service_worker_in_data);


static void handle_post_addr_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *index_service_worker_in_data){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_core->index_service_worker->ctx;

    gnb_key_address_t *key_address;

    post_addr_frame_t *post_addr_frame = (post_addr_frame_t *)&index_service_worker_in_data->payload_st.data;

    gnb_sockaddress_t *sockaddress = &index_service_worker_in_data->node_addr_st;

    key_address = GNB_LRU32_HASH_GET_VALUE(index_service_worker_ctx->lru, post_addr_frame->data.src_key512, 64);

    gnb_address_list_t *address6_list;
    gnb_address_list_t *address4_list;

    if ( NULL == key_address ) {

        key_address = (gnb_key_address_t *)alloca( sizeof(gnb_key_address_t) );

        memset(key_address,0,sizeof(gnb_key_address_t));

        GNB_LRU32_FIXED_STORE(index_service_worker_ctx->lru, post_addr_frame->data.src_key512, 64, key_address);

        key_address = GNB_LRU32_HASH_GET_VALUE(index_service_worker_ctx->lru, post_addr_frame->data.src_key512, 64);

        address6_list = (gnb_address_list_t *)key_address->address6_list_block;
        address6_list->size = GNB_KEY_ADDRESS_NUM;

        address4_list = (gnb_address_list_t *)key_address->address4_list_block;
        address4_list->size = GNB_KEY_ADDRESS_NUM;

    } else {

        address6_list = (gnb_address_list_t *)key_address->address6_list_block;
        address4_list = (gnb_address_list_t *)key_address->address4_list_block;

    }

    key_address->uuid32 = ntohl(post_addr_frame->data.src_uuid32);

    gnb_address_t *address = alloca(sizeof(gnb_address_t));

    address->ts_sec = index_service_worker_ctx->now_time_sec;

    if ( AF_INET6 == sockaddress->addr_type ) {

        if ( index_service_worker_ctx->now_time_sec - key_address->last_post_addr6_sec < GNB_POST_ADDR_LIMIT_SEC ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE POST receive addr=%u now_time_sec=%"PRIu64" last_post_addr6_sec=%"PRIu64" LIMIT\n", key_address->uuid32, index_service_worker_ctx->now_time_sec, key_address->last_post_addr6_sec);
            key_address->last_post_addr6_sec = index_service_worker_ctx->now_time_sec;
            return;
        }

        key_address->last_post_addr6_sec = index_service_worker_ctx->now_time_sec;

        gnb_set_address6(address, &sockaddress->addr.in6);

        gnb_address_list3_fifo(address6_list, address);

    }

    if ( AF_INET == sockaddress->addr_type ) {

        if ( index_service_worker_ctx->now_time_sec - key_address->last_post_addr4_sec < GNB_POST_ADDR_LIMIT_SEC ) {
            GNB_LOG2(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE POST receive addr=%u now_time_sec=%"PRIu64" last_post_addr4_sec=%"PRIu64" LIMIT\n", key_address->uuid32, index_service_worker_ctx->now_time_sec, key_address->last_post_addr4_sec);
            key_address->last_post_addr4_sec = index_service_worker_ctx->now_time_sec;
            return;
        }

        key_address->last_post_addr4_sec = index_service_worker_ctx->now_time_sec;

        gnb_set_address4(address, &sockaddress->addr.in);

        gnb_address_list3_fifo(address4_list, address);

    }


    if ( 0 != post_addr_frame->data.wan6_port ) {
        memcpy(key_address->wan6_addr,post_addr_frame->data.wan6_addr,16);
        key_address->wan6_port = post_addr_frame->data.wan6_port;
    }

    if ( 'p' == post_addr_frame->data.arg0 ) {

        if ( 'a' == post_addr_frame->data.arg1 ) {
            memcpy(key_address->attachmenta, post_addr_frame->data.attachment, INDEX_ATTACHMENT_SIZE);
        } else if ( 'b' == post_addr_frame->data.arg1 ) {
            memcpy(key_address->attachmentb, post_addr_frame->data.attachment, INDEX_ATTACHMENT_SIZE);
        }

    }

    send_echo_addr_frame(gnb_core->index_service_worker, post_addr_frame->data.src_key512, key_address->uuid32, address);

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE POST addr [%u][%s]\n", key_address->uuid32, GNB_SOCKETADDRSTR1(sockaddress));

}


static void send_echo_addr_frame(gnb_worker_t *gnb_index_service_worker, unsigned char *key512, uint32_t uuid32, gnb_address_t *address){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_index_service_worker->ctx;
    gnb_core_t *gnb_core = index_service_worker_ctx->gnb_core;

    index_service_worker_ctx->index_frame_payload->sub_type = PAYLOAD_SUB_TYPE_ECHO_ADDR;

    gnb_payload16_set_data_len( index_service_worker_ctx->index_frame_payload,  sizeof(echo_addr_frame_t) );

    echo_addr_frame_t *echo_addr_frame = (echo_addr_frame_t *)index_service_worker_ctx->index_frame_payload->data;

    memset(echo_addr_frame, 0, sizeof(echo_addr_frame_t));

    memcpy(echo_addr_frame->data.dst_key512, key512, 64);

    echo_addr_frame->data.dst_uuid32 = htonl(uuid32);

    echo_addr_frame->data.src_ts_usec = gnb_htonll(index_service_worker_ctx->now_time_usec);

    if ( AF_INET6 == address->type ) {
        echo_addr_frame->data.addr_type = '6';
        memcpy(echo_addr_frame->data.addr, address->m_address6, 16);
        //debug_text
        snprintf(echo_addr_frame->data.text, 80, "ECHO ADDR [%s:%d][%u]", GNB_ADDR6STR_PLAINTEXT1(address->m_address6), ntohs(address->port), uuid32);
    } else if ( AF_INET == address->type ) {
        echo_addr_frame->data.addr_type = '4';
        memcpy(echo_addr_frame->data.addr, address->m_address4, 4);
        //debug_text
        snprintf(echo_addr_frame->data.text, 80, "ECHO ADDR [%s:%d][%u]", GNB_ADDR4STR_PLAINTEXT1(address->m_address4), ntohs(address->port), uuid32 );
    }

    echo_addr_frame->data.port = address->port;

    gnb_send_to_address(gnb_core, address, index_service_worker_ctx->index_frame_payload);

}


/*
 * 把 src_key_address里nodeid及ip地址 发到 dst_key_address 对于的nodeid的节点
*/
static void send_push_addr_frame(gnb_worker_t *gnb_index_service_worker, unsigned char action, unsigned char attachment, unsigned char *src_key, gnb_key_address_t *src_key_address, unsigned char *dst_key, gnb_key_address_t *dst_key_address){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_index_service_worker->ctx;

    gnb_core_t *gnb_core = index_service_worker_ctx->gnb_core;

    index_service_worker_ctx->index_frame_payload->sub_type = PAYLOAD_SUB_TYPE_PUSH_ADDR;

    gnb_payload16_set_data_len( index_service_worker_ctx->index_frame_payload,  sizeof(push_addr_frame_t) );

    push_addr_frame_t *push_addr_frame = (push_addr_frame_t *)index_service_worker_ctx->index_frame_payload->data;

    memset(push_addr_frame, 0, sizeof(push_addr_frame_t));

    memcpy(push_addr_frame->data.node_key, src_key, 64);

    push_addr_frame->data.node_uuid32 = htonl(src_key_address->uuid32);

    gnb_address_list_t *address6_list = (gnb_address_list_t *)src_key_address->address6_list_block;
    gnb_address_list_t *address4_list = (gnb_address_list_t *)src_key_address->address4_list_block;

    if ( 0 != address6_list->array[0].port && ( index_service_worker_ctx->now_time_sec - address6_list->array[0].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr6_a, &address6_list->array[0].address, 16);
        push_addr_frame->data.port6_a = address6_list->array[0].port;
    }

    if ( 0 != address6_list->array[1].port && ( index_service_worker_ctx->now_time_sec - address6_list->array[1].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr6_b, &address6_list->array[1].address, 16);
        push_addr_frame->data.port6_b = address6_list->array[1].port;
    }

    if ( 0 != address6_list->array[2].port && ( index_service_worker_ctx->now_time_sec - address6_list->array[2].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr6_c, &address6_list->array[2].address, 16);
        push_addr_frame->data.port6_c = address6_list->array[2].port;
    }

    if ( 0 != address4_list->array[0].port && ( index_service_worker_ctx->now_time_sec - address4_list->array[0].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr4_a, &address4_list->array[0].address, 4);
        push_addr_frame->data.port4_a = address4_list->array[0].port;
    }

    if ( 0 != address4_list->array[1].port && ( index_service_worker_ctx->now_time_sec - address4_list->array[1].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr4_b, &address4_list->array[1].address, 4);
        push_addr_frame->data.port4_b = address4_list->array[1].port;
    }

    if ( 0 != address4_list->array[2].port && ( index_service_worker_ctx->now_time_sec - address4_list->array[2].ts_sec < GNB_ADDRESS_LIFE_TIME_TS_SEC) ) {
        memcpy(&push_addr_frame->data.addr4_c, &address4_list->array[2].address, 4);
        push_addr_frame->data.port4_c = address4_list->array[2].port;
    }

    //找一个空闲的位置，把节点自探测的 wan_addr6 写入
    if ( 0 == push_addr_frame->data.port6_a ) {
        memcpy(&push_addr_frame->data.addr6_a, &src_key_address->wan6_addr, 16);
        push_addr_frame->data.port6_a = src_key_address->wan6_port;
        goto finish_fill_address;
    }

    if ( 0 == push_addr_frame->data.port6_b ) {
        memcpy(&push_addr_frame->data.addr6_b, &src_key_address->wan6_addr, 16);
        push_addr_frame->data.port6_b = src_key_address->wan6_port;
        goto finish_fill_address;
    }

    if ( 0 == push_addr_frame->data.port6_c ) {
        memcpy(&push_addr_frame->data.addr6_c, &src_key_address->wan6_addr, 16);
        push_addr_frame->data.port6_c = src_key_address->wan6_port;
        goto finish_fill_address;
    }

finish_fill_address:

    push_addr_frame->data.arg0 = action;

    if ( 'a' == attachment ) {
        memcpy(push_addr_frame->data.attachment, src_key_address->attachmenta, INDEX_ATTACHMENT_SIZE);
    } else if ( 'b' == attachment ) {
        memcpy(push_addr_frame->data.attachment, src_key_address->attachmentb, INDEX_ATTACHMENT_SIZE);
    }

    //debug_text
    snprintf(push_addr_frame->data.text, 32, "INDEX PUSH ADDR[%u]=>[%u]", src_key_address->uuid32, dst_key_address->uuid32);

    gnb_address_list_t *dst_address6_list = (gnb_address_list_t *)dst_key_address->address6_list_block;
    gnb_address_list_t *dst_address4_list = (gnb_address_list_t *)dst_key_address->address4_list_block;

    //发给节点所有的活跃地址
    gnb_send_available_address_list(gnb_core, dst_address6_list, index_service_worker_ctx->index_frame_payload, index_service_worker_ctx->now_time_sec);
    gnb_send_available_address_list(gnb_core, dst_address4_list, index_service_worker_ctx->index_frame_payload, index_service_worker_ctx->now_time_sec);

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "SEND PUSH ADDR [%u]->[%u]\n", src_key_address->uuid32, dst_key_address->uuid32);

}


static void handle_request_addr_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *index_service_worker_in_data){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_core->index_service_worker->ctx;
    request_addr_frame_t *request_addr_frame = (request_addr_frame_t *)&index_service_worker_in_data->payload_st.data;
    gnb_sockaddress_t *sockaddress = &index_service_worker_in_data->node_addr_st;

    uint32_t src_uuid32 = ntohl(request_addr_frame->data.src_uuid32);
    uint32_t dst_uuid32 = ntohl(request_addr_frame->data.dst_uuid32);

    gnb_key_address_t *l_key_address;
    gnb_key_address_t *r_key_address;

    l_key_address = GNB_LRU32_HASH_GET_VALUE(index_service_worker_ctx->lru, request_addr_frame->data.src_key512, 64);

    if ( NULL==l_key_address ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE REQUEST src[%u] => [%u] l_key_address[%s] is Not Founded\n", src_uuid32, dst_uuid32, GNB_HEX1_BYTE128(request_addr_frame->data.src_key512));
        return;
    }

    if ( (index_service_worker_ctx->now_time_sec - l_key_address->last_post_addr6_sec) < GNB_POST_ADDR_INTERVAL_TIME_SEC*2 || (index_service_worker_ctx->now_time_sec - l_key_address->last_post_addr4_sec) < GNB_POST_ADDR_INTERVAL_TIME_SEC*2 ) {
        // l_key_address 里面的地址未超时，将其移到双向链表的首部
        GNB_LRU32_MOVETOHEAD(index_service_worker_ctx->lru, request_addr_frame->data.src_key512, 64);
    } else {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE REQUEST src[%u] => [%u] l_key_address[%s] time out now[%"PRIu64"] lastpost6[%"PRIu64"] lastpost4[%"PRIu64"]\n", src_uuid32, dst_uuid32,
                GNB_HEX1_BYTE128(request_addr_frame->data.src_key512), index_service_worker_ctx->now_time_sec, l_key_address->last_post_addr6_sec, l_key_address->last_post_addr4_sec);
        return;
    }

#if 0
    //一个节点确实可能需要请求很多节点的信息，没设计好之前暂时不做限制
    if ( (l_key_address->last_send_request_addr_usec - index_service_worker_ctx->now_time_usec) < GNB_REQUEST_ADDR_LIMIT_USEC ) {
        return;
    }
#endif

    l_key_address->last_send_request_addr_usec = index_service_worker_ctx->now_time_usec;

    r_key_address = GNB_LRU32_HASH_GET_VALUE(index_service_worker_ctx->lru, request_addr_frame->data.dst_key512, 64);
    if ( NULL==r_key_address ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE REQUEST src[%u] => [%u] r_key_address[%s] is  Not Founded\n", src_uuid32, dst_uuid32, GNB_HEX1_BYTE128(request_addr_frame->data.dst_key512));
        return;
    }

    if ( (index_service_worker_ctx->now_time_sec - r_key_address->last_post_addr6_sec) < GNB_POST_ADDR_INTERVAL_TIME_SEC*2 || (index_service_worker_ctx->now_time_sec - r_key_address->last_post_addr4_sec) < GNB_POST_ADDR_INTERVAL_TIME_SEC*2 ) {
        // r_key_address 里面的地址未超时，将其移到双向链表的首部
        GNB_LRU32_MOVETOHEAD(index_service_worker_ctx->lru, request_addr_frame->data.dst_key512, 64);

    } else {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE REQUEST src[%u] => [%u] r_key_address[%s] time out now[%"PRIu64"] lastpost6[%"PRIu64"] lastpost4[%"PRIu64"]\n", src_uuid32, dst_uuid32,
                GNB_HEX1_BYTE128(request_addr_frame->data.dst_key512), index_service_worker_ctx->now_time_sec, r_key_address->last_post_addr6_sec, r_key_address->last_post_addr4_sec);
        return;
    }

    unsigned char attachment = 'a';
    if ( 'g' == request_addr_frame->data.arg0 ) {
        if ( 'a' == request_addr_frame->data.arg1 ) {
            attachment = 'a';
        } else if( 'b' == request_addr_frame->data.arg1 ) {
            attachment = 'b';
        }
    }

    //即使节点开启了多个 socket ，index server 只存最近一份地址
    gnb_address_list_t *address6_list = (gnb_address_list_t *)l_key_address->address6_list_block;
    gnb_address_list_t *address4_list = (gnb_address_list_t *)l_key_address->address4_list_block;    

    gnb_address_t *address = alloca(sizeof(gnb_address_t));

    address->ts_sec = index_service_worker_ctx->now_time_sec;

    if ( AF_INET6 == sockaddress->addr_type ) {
        gnb_set_address6(address, &sockaddress->addr.in6);
        gnb_address_list3_fifo(address6_list, address);
    }

    if ( AF_INET == sockaddress->addr_type ) {
        gnb_set_address4(address, &sockaddress->addr.in);
        gnb_address_list3_fifo(address4_list, address);
    }

    send_push_addr_frame(gnb_core->index_service_worker, PUSH_ADDR_ACTION_CONNECT, attachment, request_addr_frame->data.src_key512, l_key_address, request_addr_frame->data.dst_key512, r_key_address);
    send_push_addr_frame(gnb_core->index_service_worker, PUSH_ADDR_ACTION_CONNECT, attachment, request_addr_frame->data.dst_key512, r_key_address, request_addr_frame->data.src_key512, l_key_address);

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "HANDLE REQUEST push addr src[%u] => [%u] r_key_address[%s] now[%"PRIu64"] lastpost6[%"PRIu64"] lastpost4[%"PRIu64"]\n", src_uuid32, dst_uuid32,
                    GNB_HEX1_BYTE128(request_addr_frame->data.dst_key512), index_service_worker_ctx->now_time_sec, r_key_address->last_post_addr6_sec, r_key_address->last_post_addr4_sec);

}


static void handle_index_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *index_service_worker_in_data){

    gnb_payload16_t *payload = &index_service_worker_in_data->payload_st;

    if ( GNB_PAYLOAD_TYPE_INDEX != payload->type ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "handle_index_frame GNB_PAYLOAD_TYPE_INDEX != payload->type[%x]\n", payload->type);
        return;
    }

    switch( payload->sub_type ) {

        case PAYLOAD_SUB_TYPE_POST_ADDR:

            handle_post_addr_frame(gnb_core, index_service_worker_in_data);
            break;

        case PAYLOAD_SUB_TYPE_REQUEST_ADDR:

            handle_request_addr_frame(gnb_core, index_service_worker_in_data);
            break;

        default:
            break;

    }

    return;

}


static void handle_recv_queue(gnb_core_t *gnb_core){

    int i;

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_core->index_service_worker->ctx;
    
    gnb_worker_queue_data_t *receive_queue_data;

    int ret;

    for ( i=0; i<1024; i++ ) {

        receive_queue_data = (gnb_worker_queue_data_t *)gnb_ring_buffer_fixed_pop( gnb_core->index_service_worker->ring_buffer_in );

        if ( NULL==receive_queue_data ) {
            break;
        }        

        handle_index_frame(gnb_core, &receive_queue_data->data.node_in);

        gnb_ring_buffer_fixed_pop_submit( gnb_core->index_service_worker->ring_buffer_in );

    }

}


static void* thread_worker_func( void *data ) {

    int ret;

    gnb_worker_t *gnb_index_service_worker = (gnb_worker_t *)data;
    index_service_worker_ctx_t *index_service_worker_ctx = gnb_index_service_worker->ctx;
    gnb_core_t *gnb_core = index_service_worker_ctx->gnb_core;

    gnb_index_service_worker->thread_worker_flag     = 1;
    gnb_index_service_worker->thread_worker_run_flag = 1;

    gnb_worker_wait_primary_worker_started(gnb_core);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "start %s success!\n", gnb_index_service_worker->name);

    do{

        gnb_worker_sync_time(&index_service_worker_ctx->now_time_sec, &index_service_worker_ctx->now_time_usec);

        handle_recv_queue(gnb_core);

        GNB_SLEEP_MILLISECOND(150);

    }while(gnb_index_service_worker->thread_worker_flag);

    return NULL;

}


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_core_t *gnb_core = (gnb_core_t *)ctx;

    void *memory;
    size_t memory_size;

    index_service_worker_ctx_t *index_service_worker_ctx = (index_service_worker_ctx_t *)gnb_heap_alloc(gnb_core->heap, sizeof(index_service_worker_ctx_t));
    memset(index_service_worker_ctx, 0, sizeof(index_service_worker_ctx_t));
    index_service_worker_ctx->gnb_core = gnb_core;
    //可以改小一点
    index_service_worker_ctx->index_frame_payload = (gnb_payload16_t *)gnb_heap_alloc(gnb_core->heap,GNB_MAX_PAYLOAD_SIZE);
    index_service_worker_ctx->index_frame_payload->type = GNB_PAYLOAD_TYPE_INDEX;
    index_service_worker_ctx->lru  = gnb_lru32_create(gnb_core->heap, 4096, sizeof(gnb_key_address_t));

    memory_size = gnb_ring_buffer_fixed_sum_size(GNB_INDEX_SERVICE_WORKER_QUEUE_BLOCK_SIZE, gnb_core->conf->index_service_woker_queue_length);
    memory = gnb_heap_alloc(gnb_core->heap, memory_size);
    gnb_worker->ring_buffer_in = gnb_ring_buffer_fixed_init(memory, GNB_INDEX_SERVICE_WORKER_QUEUE_BLOCK_SIZE, gnb_core->conf->index_service_woker_queue_length);
    gnb_worker->ring_buffer_out = NULL;
    gnb_worker->ctx = index_service_worker_ctx;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_INDEX_SERVICE_WORKER, "%s init finish\n", gnb_worker->name);

}

static void release(gnb_worker_t *gnb_worker){

    index_service_worker_ctx_t *index_service_worker_ctx =  (index_service_worker_ctx_t *)gnb_worker->ctx;

}

static int start(gnb_worker_t *gnb_worker){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_worker->ctx;

    pthread_create(&index_service_worker_ctx->thread_worker, NULL, thread_worker_func, gnb_worker);

    pthread_detach(index_service_worker_ctx->thread_worker);

    return 0;
}

static int stop(gnb_worker_t *gnb_worker){

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_worker->ctx;

    gnb_core_t *gnb_core = index_service_worker_ctx->gnb_core;

    gnb_worker_t *index_service_worker = gnb_core->index_service_worker;

    index_service_worker->thread_worker_flag = 0;

    return 0;
}

static int notify(gnb_worker_t *gnb_worker){

    int ret;

    index_service_worker_ctx_t *index_service_worker_ctx = gnb_worker->ctx;

    ret = pthread_kill(index_service_worker_ctx->thread_worker,SIGALRM);

    return 0;

}

gnb_worker_t gnb_index_service_worker_mod = {

    .name      = "gnb_index_service_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL

};
