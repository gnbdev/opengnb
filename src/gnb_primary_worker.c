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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "gnb.h"

#ifdef __UNIX_LIKE_OS__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>

#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#define _POSIX
#define __USE_MINGW_ALARM
#endif

#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "gnb_node.h"

#include "gnb_ring_buffer_fixed.h"
#include "gnb_worker_queue_data.h"
#include "gnb_ur1_frame_type.h"
#include "gnb_time.h"
#include "gnb_udp.h"
#include "gnb_binary.h"
#include "crypto/xor/xor.h"

#ifdef __UNIX_LIKE_OS__
void bind_socket_if(gnb_core_t *gnb_core);
#endif

gnb_pf_t* gnb_find_pf_mod_by_name(const char *name);

typedef struct _primary_worker_ctx_t{

    gnb_core_t *gnb_core;

    gnb_pf_core_t  *pf_core;

#ifdef __UNIX_LIKE_OS__
    pthread_t tun_udp_loop_thread;
#endif

#ifdef _WIN32
    pthread_t tun_loop_thread;
    pthread_t udp_loop_thread;
#endif

}primary_worker_ctx_t;


#pragma pack(push, 1)

typedef struct _gnb_ur0_frame_head_t {

    uint8_t  dst_addr4[4];
    uint16_t dst_port4;

    unsigned char passcode[4];

} __attribute__ ((__packed__)) gnb_ur0_frame_head_t;

#pragma pack(pop)


gnb_worker_t * select_pf_worker(gnb_core_t *gnb_core){

    gnb_worker_t *pf_worker = NULL;

    pf_worker = gnb_core->pf_worker_ring->worker[ gnb_core->pf_worker_ring->cur_idx ];

    gnb_core->pf_worker_ring->cur_idx++;

    if ( gnb_core->pf_worker_ring->cur_idx >= gnb_core->pf_worker_ring->size ) {
        gnb_core->pf_worker_ring->cur_idx = 0;
    }

    return pf_worker;

}


void gnb_send_ur0_frame(gnb_core_t *gnb_core, gnb_node_t *dst_node, gnb_payload16_t *payload){

    unsigned char buffer[gnb_core->conf->payload_block_size];

    gnb_payload16_t *fwd_payload = (gnb_payload16_t *)buffer;

    gnb_address_t *dst_address = gnb_select_available_address4(gnb_core, dst_node);

    gnb_address_t node_address_st;

    if( NULL == dst_address ) {

        if ( 0 != dst_node->udp_sockaddr4.sin_port ) {
            node_address_st.type = AF_INET;
            memcpy(&node_address_st.m_address4, &dst_node->udp_sockaddr4.sin_addr.s_addr, 4);
            node_address_st.port = dst_node->udp_sockaddr4.sin_port;
            dst_address = &node_address_st;
        }

    }

    if ( NULL == dst_address ) {
        return;
    }

    gnb_address_t *fwd_address = &gnb_core->fwdu0_address_ring.address_list->array[0];

    gnb_ur0_frame_head_t *ur0_frame_head = (gnb_ur0_frame_head_t *)fwd_payload->data;

    fwd_payload->type = GNB_PAYLOAD_TYPE_UR0;

    memcpy(ur0_frame_head->dst_addr4, &dst_address->m_address4, 4);
    ur0_frame_head->dst_port4 = dst_address->port;

    memcpy(ur0_frame_head->passcode, gnb_core->conf->crypto_passcode, 4);

    uint16_t payload_size = gnb_payload16_size(payload);

    memcpy(fwd_payload->data + sizeof(gnb_ur0_frame_head_t), payload, payload_size);

    gnb_payload16_set_data_len(fwd_payload, sizeof(gnb_ur0_frame_head_t) + payload_size);

    gnb_send_to_address(gnb_core, fwd_address, fwd_payload);

    if ( 1==gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "send ur0 local =>[%s]=>dst_addr[%s:%d]\n", GNB_IP_PORT_STR1(fwd_address), GNB_ADDR4STR2(ur0_frame_head->dst_addr4), ntohs(ur0_frame_head->dst_port4));
    }

}


static void handle_ur0_frame(gnb_core_t *gnb_core, gnb_payload16_t *payload, gnb_sockaddress_t *source_node_addr){

    gnb_ur0_frame_head_t *ur0_frame_head = (gnb_ur0_frame_head_t *)payload->data;

    gnb_address_t dst_address_st;
    dst_address_st.type = AF_INET;
    memcpy(&dst_address_st.address.addr4, ur0_frame_head->dst_addr4, 4);

    dst_address_st.port = ur0_frame_head->dst_port4;

    if ( 0 != memcmp(ur0_frame_head->passcode, gnb_core->conf->crypto_passcode, 4) ) {

        if ( 1==gnb_core->conf->if_dump ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle ur0 =>[%s] passcode invalid\n", GNB_IP_PORT_STR1(&dst_address_st) );
        }
        return;

    }

    gnb_payload16_t *fwd_payload = (gnb_payload16_t *)(payload->data + sizeof(gnb_ur0_frame_head_t));

    gnb_send_to_address(gnb_core, &dst_address_st, fwd_payload);

    if ( 1==gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle ur0 %s => %s\n", GNB_SOCKETADDRSTR1(source_node_addr), GNB_IP_PORT_STR1(&dst_address_st) );
    }

}


static void handle_ur1_frame(gnb_core_t *gnb_core, gnb_payload16_t *payload, gnb_sockaddress_t *node_addr){

    gnb_address_t ur1_address_st;
    gnb_uuid_t src_uuid64;
    gnb_uuid_t dst_uuid64;
    gnb_node_t *src_node;
    gnb_node_t *dst_node;
    uint16_t data_size;

    unsigned char *ur1_data;
    uint16_t ur1_data_size;
    unsigned char verifycode[4];

    #define GNB_UR1_SOURCE_UNSET  0
    //payload来自 App
    #define GNB_UR1_SOURCE_APP    1
    //payload来自 gnb node
    #define GNB_UR1_SOURCE_NODE   2

    int ur1_source = GNB_UR1_SOURCE_UNSET;

    data_size = GNB_PAYLOAD16_DATA_SIZE(payload);

    if ( data_size <= sizeof(gnb_ur1_frame_head_t) ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload size error!\n", GNB_SOCKETADDRSTR1(node_addr));
        return;
    }

    gnb_ur1_frame_head_t *ur1_frame_head = (gnb_ur1_frame_head_t *)payload->data;

    if ( ur1_frame_head->ttl > 2 ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload ttl=%d error!\n", ur1_frame_head->ttl, GNB_SOCKETADDRSTR1(node_addr));
        return;
    }

    if ( 0 != memcmp(ur1_frame_head->passcode, gnb_core->conf->crypto_passcode, 4) ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload passcode error!\n", GNB_SOCKETADDRSTR1(node_addr));
        return;
    }

    dst_uuid64 = gnb_ntohll(ur1_frame_head->dst_uuid64);

    if ( 0==dst_uuid64 ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload dst_node=%llu error!\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64);
        return;
    }

    dst_node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, dst_uuid64);

    if ( NULL==dst_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload dst_node=%llu not found!\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64);
        return;
    }

    // 0==src_uuid64 说明 payload 来自应用端, 需要为 payload 设置 src_uuid64,src_addr,ttl
    if ( 0 == ur1_frame_head->src_uuid64 ) {

        ur1_frame_head->src_uuid64 = gnb_htonll(gnb_core->local_node->uuid64);
        ur1_frame_head->ttl = 2;

        if ( AF_INET6 == node_addr->addr_type ) {
            //IPV6:bit=0
            payload->sub_type &= ~(GNB_PAYLOAD_SUB_TYPE_UR1_SRC_ADDRESS4);
            memcpy(ur1_frame_head->src_addr, &node_addr->addr.in6.sin6_addr, 16);
            ur1_frame_head->src_port = node_addr->addr.in6.sin6_port;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s dst_node=%llu src_uuid64=0 set src_addr=%s:%d\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64, GNB_ADDR6STR2(ur1_frame_head->src_addr), ntohs(ur1_frame_head->src_port));
        } else if ( AF_INET == node_addr->addr_type ) {
            //IPV4:bit=1
            payload->sub_type |= GNB_PAYLOAD_SUB_TYPE_UR1_SRC_ADDRESS4;
            memcpy(ur1_frame_head->src_addr, &node_addr->addr.in.sin_addr, 4);
            ur1_frame_head->src_port = node_addr->addr.in.sin_port;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s dst_node=%llu src_uuid64=0 set src_addr=%s:%d\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64, GNB_ADDR4STR2(ur1_frame_head->src_addr), ntohs(ur1_frame_head->src_port));
        } else {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s dst_node=%llu in addr_type error\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64);
            return;
        }

        ur1_source = GNB_UR1_SOURCE_APP;

    } else {

        ur1_source = GNB_UR1_SOURCE_NODE;

        if ( ur1_frame_head->ttl > 0 ) {
            ur1_frame_head->ttl--;
        }

    }


    src_uuid64 = gnb_ntohll(ur1_frame_head->src_uuid64);
    src_node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, src_uuid64);

    if ( NULL==src_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload src_node=%llu not found!\n", GNB_SOCKETADDRSTR1(node_addr), src_uuid64);
        return;
    }

    ur1_data = payload->data + sizeof(gnb_ur1_frame_head_t);
    ur1_data_size = data_size - sizeof(gnb_ur1_frame_head_t);

    //payload 来自一个 gnb node 或 App 设置目的节点就是本地节点
    if ( dst_uuid64 == gnb_core->local_node->uuid64 ) {

        memset(&ur1_address_st, 0, sizeof(gnb_address_t));

        ur1_address_st.port = ur1_frame_head->dst_port;

        if ( !(payload->sub_type & GNB_PAYLOAD_SUB_TYPE_UR1_DST_ADDRESS4) ) {
            //IPV6
            ur1_address_st.type = AF_INET6;
            memcpy(&ur1_address_st.m_address6, ur1_frame_head->dst_addr, 16);
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s src_uuid=%llu dst_node=%llu payload to host dst=%s:%d\n", GNB_SOCKETADDRSTR1(node_addr), gnb_ntohll(ur1_frame_head->src_uuid64), dst_uuid64, GNB_ADDR6STR2(&ur1_address_st.m_address6), ntohs(ur1_address_st.port));
        } else  {
            //IPV4
            ur1_address_st.type = AF_INET;
            memcpy(&ur1_address_st.m_address4, ur1_frame_head->dst_addr, 4);

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s src_uuid=%llu dst_node=%llu payload to host dst=%s:%d\n", GNB_SOCKETADDRSTR1(node_addr), gnb_ntohll(ur1_frame_head->src_uuid64), dst_uuid64, GNB_ADDR4STR2(&ur1_address_st.m_address4), ntohs(ur1_address_st.port));
        }

        if ( ur1_frame_head->src_uuid64 != ur1_frame_head->dst_uuid64) {
            //解密
            xor_crypto(src_node->crypto_key, (unsigned char *)ur1_data, ur1_data_size);

            verifycode[0] = ur1_data[0];
            verifycode[1] = ur1_data[1];
            verifycode[2] = ur1_data[ur1_data_size-2];
            verifycode[3] = ur1_data[ur1_data_size-1];

            if ( 0 != memcmp(ur1_frame_head->verifycode, verifycode, 4) ) {
                
                GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload verifycode error by crypto_key!\n", GNB_SOCKETADDRSTR1(node_addr));

                //尝试用旧通信密钥解密
                xor_crypto(src_node->pre_crypto_key, (unsigned char *)ur1_data, ur1_data_size);

                verifycode[0] = ur1_data[0];
                verifycode[1] = ur1_data[1];
                verifycode[2] = ur1_data[ur1_data_size-2];
                verifycode[3] = ur1_data[ur1_data_size-1];
                if ( 0 != memcmp(ur1_frame_head->verifycode, verifycode, 4) ) {
                    GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload verifycode error by pre_crypto_key!\n", GNB_SOCKETADDRSTR1(node_addr));
                    return;
                }

            }

        }

        //payload 转发到指定 host and port 
        if ( payload->sub_type & GNB_PAYLOAD_SUB_TYPE_UR1_HEADER ) {
            gnb_send_to_address(gnb_core, &ur1_address_st, payload);
            GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame payload relay to %s whith frame header\n", GNB_IP_PORT_STR1(&ur1_address_st) );
        } else {
            //skip frame header
            gnb_send_udata_to_address(gnb_core,&ur1_address_st, payload->data + sizeof(gnb_ur1_frame_head_t), data_size - sizeof(gnb_ur1_frame_head_t));
            GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame payload relay to %s whithout frame header\n", GNB_IP_PORT_STR1(&ur1_address_st) );
        }

        return;

    }

    //payload 转发到指定 gnb node
    if (  GNB_UR1_SOURCE_APP == ur1_source && dst_uuid64 != gnb_core->local_node->uuid64 ) {

        //第一个gnb node,执行加密
        ur1_frame_head->verifycode[0] = ur1_data[0];
        ur1_frame_head->verifycode[1] = ur1_data[1];
        ur1_frame_head->verifycode[2] = ur1_data[ur1_data_size-2];
        ur1_frame_head->verifycode[3] = ur1_data[ur1_data_size-1];

        xor_crypto(dst_node->crypto_key, ur1_data, ur1_data_size);
        gnb_std_uf_forward_payload_to_node(gnb_core, dst_node, payload);
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s to dst node=%llu payload\n", GNB_SOCKETADDRSTR1(node_addr), dst_uuid64);
        return;

    } else if ( GNB_UR1_SOURCE_NODE == ur1_source && dst_uuid64 != gnb_core->local_node->uuid64 ) {

        if ( 0==ur1_frame_head->ttl ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "UR1 frame frome %s payload ttl=%d error!\n", ur1_frame_head->ttl, GNB_SOCKETADDRSTR1(node_addr));
            return;
        }
        gnb_std_uf_forward_payload_to_node(gnb_core, dst_node, payload);
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame from %s src node=%d to dst node=%llu payload relay!\n", GNB_SOCKETADDRSTR1(node_addr), gnb_core->local_node->uuid64, dst_uuid64);

    } else {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "UR1 frame from %s src node=%d to dst node=%llu ur1_source=%d, ttl==%d payload error!\n", GNB_SOCKETADDRSTR1(node_addr), gnb_core->local_node->uuid64, dst_uuid64, ur1_source, ur1_frame_head->ttl);
    }

}


static gnb_worker_queue_data_t* make_worker_receive_queue_data(gnb_worker_t *worker, gnb_sockaddress_t *node_addr, uint8_t socket_idx, gnb_payload16_t *payload){

    gnb_worker_queue_data_t *receive_queue_data = (gnb_worker_queue_data_t *)gnb_ring_buffer_fixed_push(worker->ring_buffer_in);

    if ( NULL == receive_queue_data ) {
        return NULL;
    }

    receive_queue_data->type = GNB_WORKER_QUEUE_DATA_TYPE_NODE_IN;

    memcpy(&receive_queue_data->data.node_in.node_addr_st, node_addr, sizeof(gnb_sockaddress_t));

    receive_queue_data->data.node_in.socket_idx = socket_idx;

    memcpy(&receive_queue_data->data.node_in.payload_st, payload, gnb_payload16_size(payload));

    return receive_queue_data;

}


static gnb_worker_queue_data_t* make_worker_send_queue_data(gnb_worker_t *worker, gnb_payload16_t *payload){

    gnb_worker_queue_data_t *send_queue_data = (gnb_worker_queue_data_t *)gnb_ring_buffer_fixed_push(worker->ring_buffer_out);

    if ( NULL == send_queue_data ) {
        return NULL;
    }

    send_queue_data->type = GNB_WORKER_QUEUE_DATA_TYPE_NODE_OUT;

    memcpy(&send_queue_data->data.node_in.payload_st, payload, gnb_payload16_size(payload));

    return send_queue_data;

}


static void handle_udp(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core, uint8_t socket_idx, int af){

    ssize_t n_recv;
    uint16_t payload_size;
    gnb_sockaddress_t node_addr_st;
    gnb_worker_t *pf_worker;
    gnb_worker_queue_data_t *receive_queue_data;
    gnb_payload16_t *inet_payload;

    if ( !gnb_core->conf->activate_tun ) {
        inet_payload = gnb_core->inet_payload;
        goto skip_tun;
    }

    if ( gnb_core->pf_worker_ring->size == 0 ) {

        inet_payload = gnb_core->inet_payload;

    } else {

        pf_worker = select_pf_worker(gnb_core);

        receive_queue_data = (gnb_worker_queue_data_t *)gnb_ring_buffer_fixed_push(pf_worker->ring_buffer_in);
        
        if ( NULL == receive_queue_data ) {
            return;
        }

        inet_payload = &receive_queue_data->data.node_in.payload_st;

    }

skip_tun:

    switch (af) {

        case AF_INET6:

            node_addr_st.socklen = sizeof(struct sockaddr_in6);

            n_recv = recvfrom(gnb_core->udp_ipv6_sockets[socket_idx], (void *)inet_payload, gnb_core->conf->payload_block_size, 0, (struct sockaddr *)&node_addr_st.addr.in6, &node_addr_st.socklen);
            node_addr_st.addr_type = AF_INET6;

            break;

        case AF_INET:

            node_addr_st.socklen = sizeof(struct sockaddr_in);

            n_recv = recvfrom(gnb_core->udp_ipv4_sockets[socket_idx], (void *)inet_payload, gnb_core->conf->payload_block_size, 0, (struct sockaddr *)&node_addr_st.addr.in, &node_addr_st.socklen);
            node_addr_st.addr_type = AF_INET;

            break;

        default:
            n_recv = 0;
            break;

    }

    if ( n_recv <= 0 ) {
        goto finish;
    }

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Payload INET in buffer[%s..]\n", GNB_HEX2_BYTE256((void *)inet_payload));
    }

    node_addr_st.protocol = SOCK_DGRAM;

    payload_size = gnb_payload16_size(inet_payload);


    if ( payload_size != n_recv ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle_udp n_recv=%lu payload_size=%u payload invalid!\n", n_recv, payload_size);
        goto finish;
    }


    if ( 1 == gnb_core->conf->activate_tun && GNB_PAYLOAD_TYPE_IPFRAME == inet_payload->type ) {

        if ( gnb_core->pf_worker_ring->size == 0 ) {

            gnb_pf_inet(gnb_core, pf_core, inet_payload, &node_addr_st);

        } else {

            receive_queue_data->type = GNB_WORKER_QUEUE_DATA_TYPE_NODE_IN;
            memcpy(&receive_queue_data->data.node_in.node_addr_st, &node_addr_st, sizeof(gnb_sockaddress_t));
            receive_queue_data->data.node_in.socket_idx = socket_idx;
            gnb_ring_buffer_fixed_push_submit(pf_worker->ring_buffer_in);
            pf_worker->notify(pf_worker);

        }

        goto finish;

    }


    //收到 index 类型的paload 就放到 index_worker 或 index_service_worker queue 中
    if( GNB_PAYLOAD_TYPE_INDEX == inet_payload->type ) {

        switch ( inet_payload->sub_type ) {

        case PAYLOAD_SUB_TYPE_POST_ADDR    :
        case PAYLOAD_SUB_TYPE_REQUEST_ADDR :

                if ( 0 == gnb_core->conf->activate_index_service_worker ) {
                    goto finish;
                }

                receive_queue_data = make_worker_receive_queue_data(gnb_core->index_service_worker, &node_addr_st, socket_idx, inet_payload);

                if ( NULL == receive_queue_data ) {
                    //ringbuffer is full
                    GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle_udp index_service_worker ringbuffer is full!\n");
                    goto finish;
                }

                gnb_ring_buffer_fixed_push_submit(gnb_core->index_service_worker->ring_buffer_in);

                gnb_core->index_service_worker->notify(gnb_core->index_service_worker);

             break;

        case PAYLOAD_SUB_TYPE_ECHO_ADDR    :
        case PAYLOAD_SUB_TYPE_PUSH_ADDR    :
        case PAYLOAD_SUB_TYPE_DETECT_ADDR  :

                if ( 0 == gnb_core->conf->activate_index_worker ) {
                    goto finish;
                }


                receive_queue_data = make_worker_receive_queue_data(gnb_core->index_worker, &node_addr_st, socket_idx, inet_payload);


                if ( NULL == receive_queue_data ) {
                    //ringbuffer is full
                    GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle_udp index_worker ringbuffer is full!\n");
                    goto finish;
                }

                gnb_ring_buffer_fixed_push_submit(gnb_core->index_worker->ring_buffer_in);

                gnb_core->index_worker->notify(gnb_core->index_worker);

            break;

        default :

            break;

        }

        goto finish;

    }

    //收到 node 类型的paload 就放到 node_worker queue 中
    if ( GNB_PAYLOAD_TYPE_NODE == inet_payload->type ) {

        if ( 0 == gnb_core->conf->activate_node_worker ) {
            goto finish;
        }

        receive_queue_data = make_worker_receive_queue_data(gnb_core->node_worker, &node_addr_st, socket_idx, inet_payload);

        if ( NULL == receive_queue_data ) {
            //ringbuffer is full
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "handle_udp node_worker ringbuffer is full!\n");
            goto finish;
        }

        gnb_ring_buffer_fixed_push_submit(gnb_core->node_worker->ring_buffer_in);
        gnb_core->node_worker->notify(gnb_core->node_worker);

        goto finish;

    }

    if ( GNB_PAYLOAD_TYPE_UR1 == inet_payload->type && 1 == gnb_core->conf->universal_relay1 ) {
        handle_ur1_frame(gnb_core, inet_payload, &node_addr_st);
        goto finish;
    }

    if ( GNB_PAYLOAD_TYPE_UR0 == inet_payload->type && 1 == gnb_core->conf->universal_relay0 ) {
        handle_ur0_frame(gnb_core, inet_payload, &node_addr_st);
        goto finish;
    }

finish:

    return;

}


static int handle_tun(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core){

    ssize_t rlen;

    gnb_worker_t *pf_worker;
    gnb_worker_queue_data_t *send_queue_data;

    //tun模式下这里得到的payload是ip分组, tap模式下是以太网分组,现在都是tun模式
    rlen = gnb_core->drv->read_tun(gnb_core, gnb_core->tun_payload->data + gnb_core->tun_payload_offset, gnb_core->conf->payload_block_size);

    if ( rlen<=0 ) {
        goto finish;
    }

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Payload TUN out buffer[%s..]\n", GNB_HEX2_BYTE128((void *)(gnb_core->tun_payload->data + gnb_core->tun_payload_offset)));
    }

    gnb_payload16_set_size(gnb_core->tun_payload, GNB_PAYLOAD16_HEAD_SIZE + gnb_core->tun_payload_offset + rlen);

    if ( gnb_core->pf_worker_ring->size == 0 ) {

        gnb_pf_tun(gnb_core, pf_core, gnb_core->tun_payload);

    } else {

        pf_worker = select_pf_worker(gnb_core);

        send_queue_data = make_worker_send_queue_data(pf_worker, gnb_core->tun_payload);

        if ( NULL == send_queue_data ) {
            //ringbuffer is full            
            goto finish;
        }

        gnb_ring_buffer_fixed_push_submit(pf_worker->ring_buffer_out);

        pf_worker->notify(pf_worker);

    }

finish:

    return rlen;

}


#ifdef _WIN32

static void* tun_loop_thread_func( void *data ) {

    gnb_worker_t *gnb_worker = (gnb_worker_t *)data;
    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;
    gnb_core_t *gnb_core = primary_worker_ctx->gnb_core;
    gnb_pf_core_t *pf_core = primary_worker_ctx->pf_core;


    ssize_t rlen;

    gnb_core->loop_flag = 1;

    while ( gnb_core->loop_flag ) {

        rlen = gnb_core->drv->read_tun(gnb_core, gnb_core->tun_payload->data + gnb_core->tun_payload_offset, gnb_core->conf->payload_block_size);

        if ( rlen<=0 ) {
            continue;
        }

        if ( 1 == gnb_core->conf->if_dump ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Payload TUN out buffer[%s..]\n", GNB_HEX2_BYTE128((void *)(gnb_core->tun_payload->data + gnb_core->tun_payload_offset)));
        }

        gnb_payload16_set_size(gnb_core->tun_payload, GNB_PAYLOAD16_HEAD_SIZE + gnb_core->tun_payload_offset + rlen);

        gnb_pf_tun(gnb_core, pf_core, gnb_core->tun_payload);

    }

    return NULL;

}


static void* udp_loop_thread_func( void *data ) {

    gnb_worker_t *gnb_worker = (gnb_worker_t *)data;
    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;
    gnb_core_t *gnb_core = primary_worker_ctx->gnb_core;
    gnb_pf_core_t *pf_core = primary_worker_ctx->pf_core;

    int n_ready;

    struct timeval timeout;

    fd_set readfds;
    fd_set allset;

    FD_ZERO(&readfds);
    FD_ZERO(&allset);

    int maxfd = 0;

    int i;

    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6 ) {

        for ( i=0; i < gnb_core->conf->udp6_socket_num; i++ ) {

            FD_SET(gnb_core->udp_ipv6_sockets[i], &allset);

            if ( gnb_core->udp_ipv6_sockets[i] > maxfd ) {
                maxfd = gnb_core->udp_ipv6_sockets[i];
            }

        }

    }

    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4 ) {

        for ( i=0; i < gnb_core->conf->udp4_socket_num; i++ ) {

            FD_SET(gnb_core->udp_ipv4_sockets[i], &allset);

            if ( gnb_core->udp_ipv4_sockets[i] > maxfd ) {
                maxfd = gnb_core->udp_ipv4_sockets[i];
            }

        }

    }

    int ret = 0;

    gnb_worker->thread_worker_flag     = 1;
    gnb_worker->thread_worker_run_flag = 1;

    gnb_core->loop_flag = 1;
    uint64_t pre_usec = 0l;

    while (gnb_core->loop_flag) {

        readfds = allset;

        //由于在Windows下pthread_kill不起作用，因此不能依靠信号打断 select，因此把select的超时时间设短
        timeout.tv_sec  = 0l;
        timeout.tv_usec = 10000l;

        n_ready = select( maxfd + 1, &readfds, NULL, NULL, &timeout );

        if ( -1 == n_ready ) {
            if ( EINTR == errno ) {
                //udp_loop_thread_func 被信号打断，可能队列里面被投放了数据
                continue;
            } else {
                break;
            }
        }

        if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6 ) {

            for ( i=0; i<gnb_core->conf->udp6_socket_num; i++ ) {

                if ( FD_ISSET( gnb_core->udp_ipv6_sockets[i], &readfds ) ) {
                    handle_udp(gnb_core, pf_core, i, AF_INET6);
                }

            }

        }

        if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4 ) {

            for ( i=0; i<gnb_core->conf->udp4_socket_num; i++ ) {

                if ( FD_ISSET( gnb_core->udp_ipv4_sockets[i], &readfds ) ) {
                    handle_udp(gnb_core, pf_core, i, AF_INET);
                }

            }

        }

    }//while()

    if ( (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6) ) {

        for ( i=0; i<gnb_core->conf->udp6_socket_num; i++ ) {
            FD_CLR(gnb_core->udp_ipv6_sockets[i], &allset);
        }

    }

    if ( (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4) ) {

        for ( i=0; i<gnb_core->conf->udp4_socket_num; i++ ) {
            FD_CLR(gnb_core->udp_ipv4_sockets[i], &allset);
        }

    }

    return NULL;

}

#endif


#ifdef __UNIX_LIKE_OS__
static void* tun_udp_loop_thread_func(void *data){

    gnb_worker_t *gnb_worker = (gnb_worker_t *)data;
    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;
    gnb_core_t *gnb_core = primary_worker_ctx->gnb_core;
    gnb_pf_core_t  *pf_core = primary_worker_ctx->pf_core;


    int n_ready;
    struct timeval timeout;
    fd_set readfds;
    fd_set allset;

    FD_ZERO(&readfds);
    FD_ZERO(&allset);

    int maxfd = 0;

    if ( gnb_core->conf->activate_tun ) {

        if ( -1 == gnb_core->tun_fd ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "tun_fd[%d] err\n", gnb_core->tun_fd);
            exit(1);
        }

        FD_SET(gnb_core->tun_fd, &allset);
        maxfd = gnb_core->tun_fd;

    }

    int i;

    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6 ) {

        for ( i=0; i<gnb_core->conf->udp6_socket_num; i++ ) {

            FD_SET(gnb_core->udp_ipv6_sockets[i], &allset);

            if ( gnb_core->udp_ipv6_sockets[i] > maxfd ) {
                maxfd = gnb_core->udp_ipv6_sockets[i];
            }

        }

    }

    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4 ) {

        for ( i=0; i<gnb_core->conf->udp4_socket_num; i++ ) {

            FD_SET(gnb_core->udp_ipv4_sockets[i], &allset);

            if ( gnb_core->udp_ipv4_sockets[i] > maxfd ) {
                maxfd = gnb_core->udp_ipv4_sockets[i];
            }

        }

    }

    int ret = 0;

    gnb_core->loop_flag = 1;

    gnb_worker->thread_worker_flag     = 1;
    gnb_worker->thread_worker_run_flag = 1;

    static unsigned long c = 0;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "start %s success!\n", gnb_worker->name);

    while ( gnb_core->loop_flag ) {

        readfds = allset;

        timeout.tv_sec  = 1l;
        timeout.tv_usec = 10000l;

        n_ready = select( maxfd + 1, &readfds, NULL, NULL, &timeout );

        if ( -1 == n_ready ) {

            if ( EINTR == errno ) {
                //检查一下有没到这里
                //被信号打断，可能队列里面被投放了数据
                continue;
            } else {
                break;
            }

        }

        if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6 ) {

            for ( i=0; i < gnb_core->conf->udp6_socket_num; i++ ) {

                if ( FD_ISSET( gnb_core->udp_ipv6_sockets[i], &readfds ) ) {
                    handle_udp(gnb_core, pf_core, i, AF_INET6);
                }

            }

        }

        if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4 ) {

            for ( i=0; i < gnb_core->conf->udp4_socket_num; i++ ) {

                if ( FD_ISSET( gnb_core->udp_ipv4_sockets[i], &readfds ) ) {
                    handle_udp(gnb_core, pf_core, i, AF_INET);
                }

            }

        }

        if ( gnb_core->conf->activate_tun ) {

            if ( FD_ISSET( gnb_core->tun_fd, &readfds ) ) {
                handle_tun(gnb_core, pf_core);
            }

        }

    }//while()

    if ( (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6) ) {

        for (i=0; i<gnb_core->conf->udp6_socket_num; i++) {
            FD_CLR(gnb_core->udp_ipv6_sockets[i], &allset);
        }

    }

    if ( (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4) ) {

        for (i=0; i<gnb_core->conf->udp4_socket_num; i++) {
            FD_CLR(gnb_core->udp_ipv4_sockets[i], &allset);
        }

    }

    if ( gnb_core->conf->activate_tun ) {
        FD_CLR(gnb_core->tun_fd, &allset);
    }

    return NULL;

}
#endif


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_core_t *gnb_core = (gnb_core_t *)ctx;

    primary_worker_ctx_t *primary_worker_ctx = (primary_worker_ctx_t *)gnb_heap_alloc(gnb_core->heap, sizeof(primary_worker_ctx_t));

    memset(primary_worker_ctx, 0, sizeof(primary_worker_ctx_t));

    //没有线程需要投递数据到这个线程
    gnb_worker->ring_buffer_in = NULL;
    gnb_worker->ring_buffer_out = NULL;

    primary_worker_ctx->gnb_core = (gnb_core_t *)ctx;
    gnb_worker->ctx = primary_worker_ctx;

    primary_worker_ctx->pf_core = gnb_pf_core_init(gnb_core->heap, 32);

    gnb_pf_core_t *pf_core = primary_worker_ctx->pf_core;

    gnb_pf_t *pf;

    if ( 1==gnb_core->conf->if_dump ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_dump");
        gnb_pf_install(primary_worker_ctx->pf_core->pf_install_array, pf);
    }

    pf = gnb_find_pf_mod_by_name(gnb_core->conf->pf_route);

    if ( NULL == pf ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "pf_route '%s' not exist\n", gnb_core->conf->pf_route);
        exit(1);
    }

    gnb_pf_install(pf_core->pf_install_array, pf);

    if ( 0 != gnb_core->conf->zip_level ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_zip");
        gnb_pf_install(pf_core->pf_install_array, pf);        
    }

    if ( !(GNB_PF_BITS_CRYPTO_XOR & gnb_core->conf->pf_bits) && !(GNB_PF_BITS_CRYPTO_ARC4 & gnb_core->conf->pf_bits) ) {
        goto skip_crypto;
    }

    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_XOR ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_xor");
        gnb_pf_install(pf_core->pf_install_array, pf);
    }

    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_ARC4 ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_arc4");
        gnb_pf_install(pf_core->pf_install_array, pf);
    }

skip_crypto:

    gnb_pf_core_conf(gnb_core, pf_core);
    gnb_pf_init(gnb_core, pf_core->pf_install_array);
    gnb_pf_conf(gnb_core, pf_core->pf_install_array);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_MAIN_WORKER, "%s init finish\n", gnb_worker->name);

}


static void release(gnb_worker_t *gnb_worker){

    primary_worker_ctx_t *primary_worker_ctx = (primary_worker_ctx_t *)gnb_worker->ctx;
    gnb_core_t *gnb_core = primary_worker_ctx->gnb_core;
    gnb_pf_core_t *pf_core = primary_worker_ctx->pf_core;
    gnb_pf_core_release(gnb_core, pf_core);

}


static int start(gnb_worker_t *gnb_worker){

    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;

    gnb_core_t *gnb_core = primary_worker_ctx->gnb_core;

    int i;

    struct sockaddr_in6 sockaddr6;
    struct sockaddr_in  sockaddr;

    socklen_t sockaddr_len;

    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6 ) {

        for ( i=0; i < gnb_core->conf->udp6_socket_num; i++ ) {

            gnb_core->udp_ipv6_sockets[i] = socket(AF_INET6, SOCK_DGRAM, 0);

            gnb_bind_udp_socket_ipv6(gnb_core->udp_ipv6_sockets[i], gnb_core->conf->listen_address6_string,  gnb_core->conf->udp6_ports[i]);

            sockaddr_len = sizeof(struct sockaddr_in6);
            getsockname( gnb_core->udp_ipv6_sockets[i], (struct sockaddr *)&sockaddr6, &sockaddr_len );
            gnb_core->conf->udp6_ports[i] = ntohs(sockaddr6.sin6_port);

            GNB_LOG1(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "bind addr6 [%s:%d]\n", gnb_core->conf->listen_address6_string, gnb_core->conf->udp6_ports[i]);
        }

    }


    if ( gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4 ) {

        for ( i=0; i < gnb_core->conf->udp4_socket_num; i++ ) {

            gnb_core->udp_ipv4_sockets[i] = socket(AF_INET, SOCK_DGRAM, 0);

            gnb_bind_udp_socket_ipv4(gnb_core->udp_ipv4_sockets[i], gnb_core->conf->listen_address4_string, gnb_core->conf->udp4_ports[i]);

            if ( 0==gnb_core->conf->udp4_ports[i] ) {
                sockaddr_len = sizeof(struct sockaddr_in);
                getsockname( gnb_core->udp_ipv4_sockets[i], (struct sockaddr *)&sockaddr, &sockaddr_len );
                gnb_core->conf->udp4_ports[i] = ntohs(sockaddr.sin_port);
            }

            gnb_core->conf->udp4_ext_ports[i] = gnb_core->conf->udp4_ports[i];

            GNB_LOG1(gnb_core->log,GNB_LOG_ID_MAIN_WORKER, "bind addr4[%s:%d]\n", gnb_core->conf->listen_address4_string, gnb_core->conf->udp4_ports[i]);

        }

    }


#ifdef __UNIX_LIKE_OS__

    //尝试绑定网卡
    bind_socket_if(gnb_core);

    pthread_create(&primary_worker_ctx->tun_udp_loop_thread, NULL, tun_udp_loop_thread_func, gnb_worker);
    pthread_detach(primary_worker_ctx->tun_udp_loop_thread);

#endif

#ifdef _WIN32
    pthread_create(&primary_worker_ctx->udp_loop_thread, NULL, udp_loop_thread_func, gnb_worker);
    pthread_detach(primary_worker_ctx->udp_loop_thread);

    if ( !gnb_core->conf->activate_tun ) {
        return 0;
    }

    pthread_create(&primary_worker_ctx->tun_loop_thread, NULL, tun_loop_thread_func, gnb_worker);
    pthread_detach(primary_worker_ctx->tun_loop_thread);

    /*在Windows下如果执行 pthread_detach， pthread_kill 会返回 ESRCH
     * 事实上，在Windows下执行pthread_kill没发现可以发送信号到线程
    */
#endif

    return 0;

}


static int stop(gnb_worker_t *gnb_worker){

    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;

    return 0;
}


static int notify(gnb_worker_t *gnb_worker){

    int ret;

    primary_worker_ctx_t *primary_worker_ctx = gnb_worker->ctx;

#ifdef __UNIX_LIKE_OS__
    ret = pthread_kill(primary_worker_ctx->tun_udp_loop_thread,SIGALRM);
#endif

#ifdef _WIN32
    ret = pthread_kill(primary_worker_ctx->udp_loop_thread,SIGHUP);
#endif

    return 0;

}


gnb_worker_t gnb_primary_worker_mod = {
    .name      = "gnb_primary_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL
};
