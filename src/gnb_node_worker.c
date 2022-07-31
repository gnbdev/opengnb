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

#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "gnb.h"
#include "gnb_time.h"
#include "gnb_keys.h"
#include "gnb_node.h"
#include "gnb_worker.h"
#include "gnb_ring_buffer.h"
#include "gnb_worker_queue_data.h"
#include "gnb_pingpong_frame_type.h"
#include "gnb_uf_node_frame_type.h"
#include "gnb_unified_forwarding.h"
#include "ed25519/ed25519.h"

//节点同步检测的时间间隔
#define GNB_NODE_SYNC_INTERVAL_TIME_SEC   10

//对一个node的ping时间间隔
#define GNB_NODE_PING_INTERVAL_SEC        20


#define GNB_UF_NODES_NOTIFY_INTERVAL_SEC  35

//对于一个节点必须收到的 ping或 pong 的时间间隔
#define GNB_NODE_UPDATE_INTERVAL_SEC      55

typedef struct _node_worker_ctx_t {

    gnb_core_t *gnb_core;

    gnb_payload16_t   *node_frame_payload;

    uint64_t now_time_usec;
    uint64_t now_time_sec;

    uint64_t last_sync_ts_sec;

    pthread_t thread_worker;

}node_worker_ctx_t;


static void update_unified_forwarding_node_array(gnb_core_t *gnb_core, gnb_node_t *node, gnb_node_t *uf_node) {

    int i;
    int find_idx = -1;
    int pop_idx  = 0;
    uint64_t last_ts_sec = 0l;

    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    for ( i=0; i<GNB_UNIFIED_FORWARDING_NODE_ARRAY_SIZE; i++ ) {

        if ( 0 == node->unified_forwarding_node_array[i].uuid32 ) {
            find_idx = i;
            break;
        }

        if ( uf_node->uuid32 == node->unified_forwarding_node_array[i].uuid32 ) {
            find_idx = i;
            break;
        }

        if ( (node_worker_ctx->now_time_sec - node->unified_forwarding_node_array[i].last_ts_sec) > GNB_UNIFIED_FORWARDING_NODE_ARRAY_EXPIRED_SEC ) {
            find_idx = i;
            break;
        }

        if ( node->unified_forwarding_node_array[i].last_ts_sec > last_ts_sec ) {
            last_ts_sec = node->unified_forwarding_node_array[i].last_ts_sec;
            pop_idx = i;
        }

    }

    if ( -1 == find_idx ) {
        find_idx = pop_idx;
    }

    node->unified_forwarding_node_array[find_idx].uuid32 = uf_node->uuid32;
    node->unified_forwarding_node_array[find_idx].last_ts_sec = node_worker_ctx->now_time_sec;

}


static void handle_uf_node_notify_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *node_worker_in_data){

    gnb_worker_t *main_worker = gnb_core->main_worker;
    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    uf_node_notify_frame_t *uf_node_notify_frame = (uf_node_notify_frame_t *)&node_worker_in_data->payload_st.data;

    uint32_t src_uuid32    = ntohl(uf_node_notify_frame->data.src_uuid32);
    uint32_t dst_uuid32    = ntohl(uf_node_notify_frame->data.dst_uuid32);
    uint32_t df_uuid32     = ntohl(uf_node_notify_frame->data.df_uuid32);

    gnb_sockaddress_t *node_addr = &node_worker_in_data->node_addr_st;

    if ( src_uuid32 == gnb_core->local_node->uuid32 || dst_uuid32 != gnb_core->local_node->uuid32 || df_uuid32 == gnb_core->local_node->uuid32 ) {
        GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_uf_node_notify_frame error local=%u src=%u dst=%u\n", gnb_core->local_node->uuid32, src_uuid32, dst_uuid32);
        return;
    }

    gnb_node_t *src_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, src_uuid32);

    if ( NULL==src_node ) {
        GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_uf_node_notify_frame src node=%u is miss\n", src_uuid32);
        return;
    }

    if ( 0 == gnb_core->conf->lite_mode && !ed25519_verify(uf_node_notify_frame->src_sign, (const unsigned char *)&uf_node_notify_frame->data, sizeof(struct uf_node_notify_frame_data), src_node->public_key) ) {
        GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_uf_node_notify_frame invalid signature src=%u %s\n", src_uuid32, GNB_SOCKETADDRSTR1(node_addr));
        return;
    }

    gnb_node_t *df_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, df_uuid32);

    if ( NULL==df_node ) {
        GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_uf_node_notify_frame df node=%u is miss\n", df_uuid32);
        return;
    }

    GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_uf_node_notify_frame src_uuid32=%u dst_uuid32=%u df_uuid32=%u\n", src_uuid32, dst_uuid32, df_uuid32);

    update_unified_forwarding_node_array(gnb_core, df_node, src_node);

}


static void send_uf_node_notify_frame(gnb_core_t *gnb_core, gnb_node_t *dst_node, gnb_node_t *direct_forwarding_node){

    gnb_worker_t *main_worker = gnb_core->main_worker;

    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    node_worker_ctx->node_frame_payload->sub_type = PAYLOAD_SUB_TYPE_NODE_UNIFIED_NOTIFY;

    gnb_payload16_set_data_len(node_worker_ctx->node_frame_payload, sizeof(uf_node_notify_frame_t));

    uf_node_notify_frame_t *uf_node_notify_frame = (uf_node_notify_frame_t *)node_worker_ctx->node_frame_payload->data;

    memset(uf_node_notify_frame, 0, sizeof(uf_node_notify_frame_t));

    uf_node_notify_frame->data.src_uuid32 = htonl(gnb_core->local_node->uuid32);
    uf_node_notify_frame->data.dst_uuid32 = htonl(dst_node->uuid32);
    uf_node_notify_frame->data.df_uuid32  = htonl(direct_forwarding_node->uuid32);

    snprintf((char *)uf_node_notify_frame->data.text, 64, "UNIFIED_NOTIFY src=%u dst=%u df=%u", gnb_core->local_node->uuid32, dst_node->uuid32, direct_forwarding_node->uuid32);

    if ( 0 == gnb_core->conf->lite_mode ) {
        ed25519_sign(uf_node_notify_frame->src_sign, (const unsigned char *)&uf_node_notify_frame->data, sizeof(struct uf_node_notify_frame_data), gnb_core->ed25519_public_key, gnb_core->ed25519_private_key);
    }

    gnb_forward_payload_to_node(gnb_core, dst_node, node_worker_ctx->node_frame_payload);

    GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "send_uf_node_notify_frame src=%u notify=%u direct_forwarding=%u\n", gnb_core->local_node->uuid32, dst_node->uuid32, direct_forwarding_node->uuid32);

}


static void unifield_forwarding_notify(gnb_core_t *gnb_core, gnb_node_t *dst_node){

    size_t num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0 == num ) {
        return;
    }

    int i;

    gnb_node_t *df_node;

    for ( i=0; i<num; i++ ) {

        df_node = &gnb_core->ctl_block->node_zone->node[i];

        if ( dst_node->uuid32 == df_node->uuid32 ) {
            continue;
        }

        if ( gnb_core->local_node->uuid32 == df_node->uuid32 ) {
            continue;
        }

        if ( !( (GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & df_node->udp_addr_status ) ) {
            continue;
        }

        send_uf_node_notify_frame(gnb_core, dst_node, df_node);

    }

}


static void send_ping_frame(gnb_core_t *gnb_core, gnb_node_t *node){

    int ret;

    gnb_worker_t *main_worker = gnb_core->main_worker;

    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    if ( INADDR_ANY == node->udp_sockaddr4.sin_addr.s_addr && 0 == memcmp(&node->udp_sockaddr6.sin6_addr,&in6addr_any,sizeof(struct in6_addr)) ) {
        return;
    }

    if ( GNB_NODE_STATUS_UNREACHABL == node->udp_addr_status ) {

        if ( !(node->type & GNB_NODE_TYPE_IDX) && !(node->type & GNB_NODE_TYPE_FWD) && !(node->type & GNB_NODE_TYPE_RELAY) && !(node->type & GNB_NODE_TYPE_STATIC_ADDR) ) {
            return;
        }

    }

    node_worker_ctx->node_frame_payload->sub_type = PAYLOAD_SUB_TYPE_PING;
    gnb_payload16_set_data_len(node_worker_ctx->node_frame_payload, sizeof(node_ping_frame_t));

    node_ping_frame_t *node_ping_frame = (node_ping_frame_t *)node_worker_ctx->node_frame_payload->data;
    memset(node_ping_frame, 0, sizeof(node_ping_frame_t));

    node_ping_frame->data.src_uuid32 = htonl(gnb_core->local_node->uuid32);
    node_ping_frame->data.dst_uuid32 = htonl(node->uuid32);

    node_ping_frame->data.src_ts_usec = gnb_htonll(node_worker_ctx->now_time_usec);

    snprintf((char *)node_ping_frame->data.text,32,"%d --PING-> %d", gnb_core->local_node->uuid32, node->uuid32);

    if ( 0 == gnb_core->conf->lite_mode ) {
        ed25519_sign(node_ping_frame->src_sign, (const unsigned char *)&node_ping_frame->data, sizeof(struct ping_frame_data), gnb_core->ed25519_public_key, gnb_core->ed25519_private_key);
    }

    //PING frame 尽可能 ipv4 和 ipv6 都发送
    gnb_send_to_node(gnb_core, node, node_worker_ctx->node_frame_payload, GNB_ADDR_TYPE_IPV6|GNB_ADDR_TYPE_IPV4);

    //更新 node 的ping 时间戳
    node->ping_ts_sec  = node_worker_ctx->now_time_sec;
    node->ping_ts_usec = node_worker_ctx->now_time_usec;

    GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "ping src[%u]->dst[%u] %s %s ping_ts=%"PRIu64"\n",
            gnb_core->local_node->uuid32, node->uuid32,
            GNB_SOCKADDR6STR1(&node->udp_sockaddr6), GNB_SOCKADDR4STR2(&node->udp_sockaddr4),
            node->ping_ts_sec
    );

}


static void handle_ping_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *node_worker_in_data){

    gnb_worker_t *main_worker = gnb_core->main_worker;
    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;
    node_ping_frame_t *node_ping_frame = (node_ping_frame_t *)&node_worker_in_data->payload_st.data;

    uint32_t src_uuid32 = ntohl(node_ping_frame->data.src_uuid32);
    uint32_t dst_uuid32 = ntohl(node_ping_frame->data.dst_uuid32);

    //收到ping frame，需要更新node的addr
    gnb_sockaddress_t *node_addr = &node_worker_in_data->node_addr_st;

    uint8_t addr_update = 0;

    if (src_uuid32 == gnb_core->local_node->uuid32) {
        //目标节点和当前节点在同一个子网里，当前节点发 ping frame 经过路由的nat又转回给了当前节点
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_ping_frame local_node[%u] src[%u]=>dst[%u]\n", gnb_core->local_node->uuid32, src_uuid32, dst_uuid32);
        return;
    }

    gnb_node_t *src_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, src_uuid32);

    if ( NULL==src_node ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_ping_frame node=%u is miss\n", src_uuid32);
        return;
    }

    if ( src_node->type & GNB_NODE_TYPE_SLIENCE ) {
        return;
    }

    //本地节点如果有 GNB_NODE_TYPE_SLIENCE 属性 将只响应 带有 GNB_NODE_TYPE_FWD 属性的节点
    if ( (gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE) && !(src_node->type & GNB_NODE_TYPE_FWD) ) {
        return;
    }

    if ( 0 == gnb_core->conf->lite_mode && !ed25519_verify(node_ping_frame->src_sign, (const unsigned char *)&node_ping_frame->data, sizeof(struct ping_frame_data), src_node->public_key) ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_ping_frame invalid signature src=%u %s\n", src_uuid32, GNB_SOCKETADDRSTR1(node_addr));
        return;
    }

    uint64_t src_ts_usec = gnb_ntohll(node_ping_frame->data.src_ts_usec);

    int64_t latency_usec = node_worker_ctx->now_time_usec - src_ts_usec;

    if (AF_INET6 == node_addr->addr_type) {

        src_node->udp_addr_status |= GNB_NODE_STATUS_IPV6_PING;

        //只在发生改变的时候才更新
        if ( 0 != gnb_cmp_sockaddr_in6(&src_node->udp_sockaddr6, &node_addr->addr.in6) || node_worker_in_data->socket_idx != src_node->socket6_idx ) {
            src_node->udp_sockaddr6 = node_addr->addr.in6;
            src_node->socket6_idx   = node_worker_in_data->socket_idx;
            addr_update = 1;
        }

        src_node->addr6_update_ts_sec = node_worker_ctx->now_time_sec;

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_ping_frame IPV6 src[%u]->dst[%u] idx=%u %s now=%"PRIu64" src_ts=%"PRIu64" up=%u different=%"PRId64"\n",
                src_node->uuid32, dst_uuid32,
                node_worker_in_data->socket_idx,
                GNB_SOCKADDR6STR1(&src_node->udp_sockaddr6),
                node_worker_ctx->now_time_usec, src_ts_usec, addr_update, latency_usec);


    }

    if (AF_INET == node_addr->addr_type) {

        src_node->udp_addr_status |= GNB_NODE_STATUS_IPV4_PING;

        //只在发生改变的时候才更新
        if ( PAYLOAD_SUB_TYPE_PING == node_worker_in_data->payload_st.sub_type && (0 != gnb_cmp_sockaddr_in(&src_node->udp_sockaddr4, &node_addr->addr.in) || node_worker_in_data->socket_idx != src_node->socket4_idx) ) {
            src_node->udp_sockaddr4 = node_addr->addr.in;
            src_node->socket4_idx   = node_worker_in_data->socket_idx;
            addr_update = 1;

            GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_ping_frame IPV4 src[%u]->dst[%u] idx=%u %s now=%"PRIu64" src_ts=%"PRIu64" up=%u different=%"PRId64"\n",
                    src_node->uuid32, dst_uuid32,
                    node_worker_in_data->socket_idx,
                    GNB_SOCKADDR4STR1(&src_node->udp_sockaddr4),
                    node_worker_ctx->now_time_usec, src_ts_usec, addr_update, latency_usec);
        }

        //处理来自 LAN 的 ping frame
        if ( PAYLOAD_SUB_TYPE_LAN_PING == node_worker_in_data->payload_st.sub_type ) {

            src_node->udp_sockaddr4 = node_addr->addr.in;
            //LAN ping 的端口存放在 attachment 中
            memcpy(&src_node->udp_sockaddr4.sin_port, node_ping_frame->data.attachment, sizeof(uint16_t));

            src_node->socket4_idx   = node_worker_in_data->socket_idx;
            addr_update = 1;

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_ping_frame IPV4 LAN src[%u]->dst[%u] idx=%u %s now=%"PRIu64" src_ts=%"PRIu64" up=%u different=%"PRId64"\n",
                    src_node->uuid32, dst_uuid32,
                    node_worker_in_data->socket_idx,
                    GNB_SOCKADDR4STR1(&src_node->udp_sockaddr4),
                    node_worker_ctx->now_time_usec, src_ts_usec, addr_update, latency_usec);

        }

        src_node->addr4_update_ts_sec = node_worker_ctx->now_time_sec;

    }

    node_worker_ctx->node_frame_payload->sub_type = PAYLOAD_SUB_TYPE_PONG;

    gnb_payload16_set_data_len(node_worker_ctx->node_frame_payload, sizeof(node_pong_frame_t));

    node_pong_frame_t *node_pong_frame = (node_pong_frame_t *)node_worker_ctx->node_frame_payload->data;

    memset(node_pong_frame, 0, sizeof(node_pong_frame_t));

    node_pong_frame->data.src_uuid32 = htonl(gnb_core->local_node->uuid32);
    node_pong_frame->data.dst_uuid32 = htonl(src_node->uuid32);

    node_pong_frame->data.src_ts_usec = gnb_htonll(node_worker_ctx->now_time_usec);
    node_pong_frame->data.dst_ts_usec = node_ping_frame->data.src_ts_usec;

    //把本节点的 upd 端口 发送给对端，这个端口在节点连通的情况下用虚拟ip访问到
    gnb_payload16_t *payload_attachment = (gnb_payload16_t *)node_pong_frame->data.attachment;
    gnb_payload16_set_data_len(payload_attachment, sizeof(node_attachment_tun_sockaddress_t));
    payload_attachment->type = GNB_NODE_ATTACHMENT_TYPE_TUN_SOCKADDRESS;

    node_attachment_tun_sockaddress_t *attachment_tun_sockaddress = (node_attachment_tun_sockaddress_t *)payload_attachment->data;
    memcpy(&attachment_tun_sockaddress->tun_addr4, &gnb_core->local_node->tun_addr4.s_addr, 4);
    attachment_tun_sockaddress->tun_sin_port4 = gnb_core->local_node->tun_sin_port4;

    snprintf((char *)node_pong_frame->data.text,32,"%d --PONG-> %d",gnb_core->local_node->uuid32,src_node->uuid32);

    if ( 0 == gnb_core->conf->lite_mode ) {
        ed25519_sign(node_pong_frame->src_sign, (const unsigned char *)&node_pong_frame->data, sizeof(struct pong_frame_data), gnb_core->ed25519_public_key, gnb_core->ed25519_private_key);
    }

    unsigned char addr_type_bits;

    //根据源地址选择发送的类型
    if (AF_INET == node_addr->addr_type) {
        addr_type_bits = GNB_ADDR_TYPE_IPV4;
    }

    if (AF_INET6 == node_addr->addr_type) {
        addr_type_bits = GNB_ADDR_TYPE_IPV6;
    }

    gnb_send_to_node(gnb_core, src_node, node_worker_ctx->node_frame_payload, addr_type_bits);

}


static void handle_pong_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *node_worker_in_data){

    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    node_pong_frame_t *node_pong_frame = (node_pong_frame_t *)&node_worker_in_data->payload_st.data;

    uint32_t src_uuid32 = ntohl(node_pong_frame->data.src_uuid32);
    uint32_t dst_uuid32 = ntohl(node_pong_frame->data.dst_uuid32);

    uint64_t dst_ts_usec = gnb_ntohll(node_pong_frame->data.dst_ts_usec);

    //收到pong frame，需要更新node的addr
    gnb_sockaddress_t *node_addr = &node_worker_in_data->node_addr_st;

    uint8_t addr_update = 0;

    if ( dst_uuid32 != gnb_core->local_node->uuid32 ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_pong_frame dst_uuid32[%u] != local_node[%u] addr_type=%d\n", dst_uuid32, gnb_core->local_node->uuid32, node_worker_in_data->node_addr_st.addr_type);
        return;
    }

    gnb_node_t *src_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, src_uuid32);

    if ( NULL==src_node ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_pong_frame node=%u is miss\n", src_uuid32);
        return;
    }

    if ( src_node->type & GNB_NODE_TYPE_SLIENCE ) {
        return;
    }

    //本地节点如果有 GNB_NODE_TYPE_SLIENCE 属性 将只响应 带有 GNB_NODE_TYPE_FWD 属性的节点
    if ( (gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE) && !(src_node->type & GNB_NODE_TYPE_FWD) ) {
        return;
    }

    if ( 0 == gnb_core->conf->lite_mode && !ed25519_verify(node_pong_frame->src_sign, (const unsigned char *)&node_pong_frame->data, sizeof(struct pong_frame_data), src_node->public_key) ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER,"handle_pong_frame node=%u invalid signature\n", src_uuid32);
        return;
    }

    if (AF_INET6 == node_addr->addr_type) {

        src_node->udp_addr_status |= GNB_NODE_STATUS_IPV6_PONG;

        //只在发生改变的时候才更新
        if ( 0 != gnb_cmp_sockaddr_in6(&src_node->udp_sockaddr6, &node_addr->addr.in6) || node_worker_in_data->socket_idx != src_node->socket6_idx ) {
            src_node->udp_sockaddr6 = node_addr->addr.in6;
            src_node->socket6_idx   = node_worker_in_data->socket_idx;
            addr_update = 1;
        }

        src_node->addr6_update_ts_sec = node_worker_ctx->now_time_sec;

        if ( dst_ts_usec == src_node->ping_ts_usec ) {

            src_node->addr6_ping_latency_usec = node_worker_ctx->now_time_usec - dst_ts_usec + 1;
            if ( 0 == src_node->addr6_ping_latency_usec ) {
                GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER,"addr6_ping_latency_usec==0 now=%"PRIu64" dst_ts=%"PRIu64"\n",node_worker_ctx->now_time_usec, dst_ts_usec);
            }

        }

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_pong_frame IPV6 src[%u]->dst[%u] idx=%u %s now=%"PRIu64" dst_ts=%"PRIu64" up=%u latency=%"PRId64"\n",
                src_node->uuid32, dst_uuid32,
                node_worker_in_data->socket_idx,
                GNB_SOCKADDR6STR1(&src_node->udp_sockaddr6),
                node_worker_ctx->now_time_usec, dst_ts_usec, addr_update, src_node->addr6_ping_latency_usec);


    }

    if (AF_INET == node_addr->addr_type) {

        src_node->udp_addr_status |= GNB_NODE_STATUS_IPV4_PONG;

        //只在发生改变的时候才更新
        if ( 0 != gnb_cmp_sockaddr_in(&src_node->udp_sockaddr4, &node_addr->addr.in) || node_worker_in_data->socket_idx != src_node->socket4_idx ) {
            src_node->udp_sockaddr4 = node_addr->addr.in;
            src_node->socket4_idx   = node_worker_in_data->socket_idx;
            addr_update = 1;
        }

        src_node->addr4_update_ts_sec = node_worker_ctx->now_time_sec;

        if ( dst_ts_usec == src_node->ping_ts_usec ) {

            src_node->addr4_ping_latency_usec = node_worker_ctx->now_time_usec - dst_ts_usec  + 1;
            if ( 0 == src_node->addr4_ping_latency_usec ) {
                GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER,"addr4_ping_latency_usec==0 now=%"PRIu64" dst_ts=%"PRIu64"\n",node_worker_ctx->now_time_usec, dst_ts_usec);
            }

        }

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_pong_frame IPV4 src[%u]->dst[%u] idx=%u %s now=%"PRIu64" dst_ts=%"PRIu64" up=%u latency=%"PRId64"\n",
                src_node->uuid32, dst_uuid32,
                node_worker_in_data->socket_idx,
                GNB_SOCKADDR4STR1(&src_node->udp_sockaddr4),
                node_worker_ctx->now_time_usec, dst_ts_usec, addr_update, src_node->addr4_ping_latency_usec);

    }


    //处理附件
    gnb_payload16_t *payload_attachment = (gnb_payload16_t *)node_pong_frame->data.attachment;

    node_attachment_tun_sockaddress_t *attachment_tun_sockaddress;

    if ( GNB_NODE_ATTACHMENT_TYPE_TUN_SOCKADDRESS == payload_attachment->type ) {

        attachment_tun_sockaddress = (node_attachment_tun_sockaddress_t *)payload_attachment->data;

        if ( src_node->tun_sin_port4 != attachment_tun_sockaddress->tun_sin_port4 ) {
            src_node->tun_sin_port4 = attachment_tun_sockaddress->tun_sin_port4;
        }

    }

    if ( PAYLOAD_SUB_TYPE_PONG2 == node_worker_in_data->payload_st.sub_type ) {

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_NODE_WORKER, "handle_pong2_frame  src[%u]->dst[%u] idx=%u\n",
                src_node->uuid32, dst_uuid32,
                node_worker_in_data->socket_idx);

        return;
    }

    node_worker_ctx->node_frame_payload->sub_type = PAYLOAD_SUB_TYPE_PONG2;

    gnb_payload16_set_data_len(node_worker_ctx->node_frame_payload, sizeof(node_pong_frame_t));

    node_pong_frame_t *node_pong2_frame = (node_pong_frame_t *)node_worker_ctx->node_frame_payload->data;

    memset(node_pong2_frame,0,sizeof(node_pong_frame_t));

    node_pong2_frame->data.src_uuid32 = htonl(gnb_core->local_node->uuid32);
    node_pong2_frame->data.dst_uuid32 = htonl(src_node->uuid32);

    node_pong2_frame->data.src_ts_usec = gnb_htonll(node_worker_ctx->now_time_usec);
    node_pong2_frame->data.dst_ts_usec = node_pong2_frame->data.src_ts_usec;

    gnb_payload16_t *payload_attachment2 = (gnb_payload16_t *)node_pong2_frame->data.attachment;
    gnb_payload16_set_data_len(payload_attachment2, 0);
    payload_attachment2->type = GNB_NODE_ATTACHMENT_TYPE_TUN_EMPTY;

    snprintf((char *)node_pong2_frame->data.text,32,"%d --PONG2-> %d",gnb_core->local_node->uuid32,src_node->uuid32);

    if ( 0 == gnb_core->conf->lite_mode ) {
        ed25519_sign(node_pong2_frame->src_sign, (const unsigned char *)&node_pong2_frame->data, sizeof(struct pong_frame_data), gnb_core->ed25519_public_key, gnb_core->ed25519_private_key);
    }

    unsigned char addr_type_bits;

    //根据源地址选择发送的类型
    if (AF_INET == node_addr->addr_type) {
        addr_type_bits = GNB_ADDR_TYPE_IPV4;
    }

    if (AF_INET6 == node_addr->addr_type) {
        addr_type_bits = GNB_ADDR_TYPE_IPV6;
    }

    gnb_send_to_node(gnb_core, src_node, node_worker_ctx->node_frame_payload, addr_type_bits);

}


static void update_node_crypto_key(gnb_core_t *gnb_core, uint64_t now_sec){

    int need_update_time_seed;

    size_t num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0 == num ) {
        return;
    }

    need_update_time_seed = gnb_verify_seed_time(gnb_core, now_sec);

    if ( 0 == need_update_time_seed ) {
        return;
    }

    gnb_update_time_seed(gnb_core, now_sec);

    int i;

    gnb_node_t *node;

    for ( i=0; i<num; i++ ) {

        node = &gnb_core->ctl_block->node_zone->node[i];

        if ( gnb_core->local_node->uuid32 == node->uuid32 ) {
            continue;
        }

        gnb_build_crypto_key(gnb_core, node);

    }

}


static void sync_node(gnb_worker_t *gnb_node_worker){

    node_worker_ctx_t *node_worker_ctx = gnb_node_worker->ctx;

    gnb_core_t *gnb_core = node_worker_ctx->gnb_core;

    size_t num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0 == num ) {
        return;
    }

    int i;

    gnb_node_t *node;

    for ( i=0; i<num; i++ ) {

        node = &gnb_core->ctl_block->node_zone->node[i];

        if ( gnb_core->local_node->uuid32 == node->uuid32 ) {
            continue;
        }

        if ( node->type & GNB_NODE_TYPE_SLIENCE ) {
            continue;
        }

        //如果本地节点带有 GNB_NODE_TYPE_SLIENCE 属性 将只请求带有 GNB_NODE_TYPE_FWD 属性的节点的地址
        if ( (gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE) && !(node->type & GNB_NODE_TYPE_FWD) ) {
            continue;
        }

        if( INADDR_ANY==node->udp_sockaddr4.sin_addr.s_addr &&
            0 == memcmp(&node->udp_sockaddr6.sin6_addr,&in6addr_any,sizeof(struct in6_addr)) &&
            gnb_core->index_address_ring.address_list->num > 0 )
        {

            if ( (node_worker_ctx->now_time_sec - node->ping_ts_sec) >= GNB_NODE_PING_INTERVAL_SEC ) {
                //如果地址为 0.0.0.0 或 :: , 需要向 index node 发送 PAYLOAD_SUB_TYPE_ADDR_QUERY
                node->udp_addr_status = GNB_NODE_STATUS_UNREACHABL;
                node->ping_ts_sec = node_worker_ctx->now_time_sec;
            }

            continue;

        }

        if ( (node_worker_ctx->now_time_sec - node->ping_ts_sec) >= GNB_NODE_PING_INTERVAL_SEC ) {
            send_ping_frame(gnb_core, node);
        }

        if ( (node_worker_ctx->now_time_sec - node->last_notify_uf_nodes_ts_sec) >= GNB_UF_NODES_NOTIFY_INTERVAL_SEC ) {

            if ( !( (GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status ) ) {
                continue;
            }

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "unifield_forwarding_notify nodeid=%u\n", node->uuid32);
            unifield_forwarding_notify(gnb_core, node);
            node->last_notify_uf_nodes_ts_sec = node_worker_ctx->now_time_sec;

        }

        if ( (node_worker_ctx->now_time_sec - node->addr4_update_ts_sec) > GNB_NODE_UPDATE_INTERVAL_SEC ) {
            //节点状态超时，且不是idx node, 可能目标node已经下线或者更换了ip
            //IPV4 需要向 idx node 发送 PAYLOAD_SUB_TYPE_ADDR_QUERY
            if ( !(node->type & GNB_NODE_TYPE_IDX) ) {
                node->udp_addr_status &= ~(GNB_NODE_STATUS_IPV4_PONG | GNB_NODE_STATUS_IPV4_PING);
            }

        }

        if ( (node_worker_ctx->now_time_sec - node->addr6_update_ts_sec) > GNB_NODE_UPDATE_INTERVAL_SEC ) {
            //节点状态超时，且不是idx node, 可能目标node已经下线或者更换了ip
            if ( !(node->type & GNB_NODE_TYPE_IDX) ){
                node->udp_addr_status &= ~(GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV6_PING);
            }

        }

    }

}


static void handle_node_frame(gnb_core_t *gnb_core, gnb_worker_in_data_t *node_worker_in_data){

    gnb_payload16_t *payload = &node_worker_in_data->payload_st;

    if ( GNB_PAYLOAD_TYPE_NODE != payload->type ) {
        GNB_LOG2(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "handle_node_frame GNB_PAYLOAD_TYPE_NODE != payload->type[%x]\n", payload->type);
        return;
    }

    switch ( payload->sub_type ) {

        case PAYLOAD_SUB_TYPE_PING:
        case PAYLOAD_SUB_TYPE_LAN_PING:

            handle_ping_frame(gnb_core, node_worker_in_data);
            break;

        case PAYLOAD_SUB_TYPE_PONG  :
        case PAYLOAD_SUB_TYPE_PONG2 :

            handle_pong_frame(gnb_core, node_worker_in_data);
            break;

        case PAYLOAD_SUB_TYPE_NODE_UNIFIED_NOTIFY:

            handle_uf_node_notify_frame(gnb_core, node_worker_in_data);
            break;

        default:
            break;

    }

    return;

}


static void handle_recv_queue(gnb_core_t *gnb_core){

    int i;

    node_worker_ctx_t *node_worker_ctx = gnb_core->node_worker->ctx;

    gnb_ring_node_t *ring_node;
    gnb_worker_queue_data_t *receive_queue_data;

    int ret;

    for ( i=0; i<1024; i++ ) {

        ring_node = gnb_ring_buffer_pop( gnb_core->node_worker->ring_buffer );

        if ( NULL == ring_node) {
            break;
        }

        receive_queue_data = (gnb_worker_queue_data_t *)ring_node->data;

        handle_node_frame(gnb_core, &receive_queue_data->data.node_in);

        gnb_ring_buffer_pop_submit( gnb_core->node_worker->ring_buffer );

    }

}


static void* thread_worker_func( void *data ) {

    gnb_worker_t *gnb_node_worker = (gnb_worker_t *)data;

    node_worker_ctx_t *node_worker_ctx = gnb_node_worker->ctx;

    gnb_core_t *gnb_core = node_worker_ctx->gnb_core;

    gnb_node_worker->thread_worker_flag     = 1;
    gnb_node_worker->thread_worker_run_flag = 1;

    gnb_worker_wait_main_worker_started(gnb_core);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_NODE_WORKER, "start %s success!\n", gnb_node_worker->name);

    do{

        gnb_worker_sync_time(&node_worker_ctx->now_time_sec, &node_worker_ctx->now_time_usec);

        update_node_crypto_key(gnb_core, node_worker_ctx->now_time_sec);

        handle_recv_queue(gnb_core);

        //每经过一个时间间隔就检查一次各节点的状态
        if ( node_worker_ctx->now_time_sec - node_worker_ctx->last_sync_ts_sec > GNB_NODE_SYNC_INTERVAL_TIME_SEC ) {

            sync_node(gnb_node_worker);

            node_worker_ctx->last_sync_ts_sec = node_worker_ctx->now_time_sec;

        }

        GNB_SLEEP_MILLISECOND(100);

    }while(gnb_node_worker->thread_worker_flag);

    gnb_node_worker->thread_worker_run_flag = 0;

    return NULL;

}


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_core_t *gnb_core = (gnb_core_t *)ctx;

    node_worker_ctx_t *node_worker_ctx =  (node_worker_ctx_t *)gnb_heap_alloc(gnb_core->heap, sizeof(node_worker_ctx_t));

    memset(node_worker_ctx, 0, sizeof(node_worker_ctx_t));

    node_worker_ctx->node_frame_payload =  gnb_payload16_init(0,1600);
    node_worker_ctx->node_frame_payload->type = GNB_PAYLOAD_TYPE_NODE;

    gnb_worker->ring_buffer = gnb_ring_buffer_init(gnb_core->conf->node_woker_queue_length, GNB_WORKER_QUEUE_BLOCK_SIZE);

    node_worker_ctx->gnb_core = gnb_core;

    gnb_worker->ctx = node_worker_ctx;

    GNB_LOG1(gnb_core->log,GNB_LOG_ID_NODE_WORKER,"%s init finish\n", gnb_worker->name);
}


static void release(gnb_worker_t *gnb_worker){

    node_worker_ctx_t *node_worker_ctx =  (node_worker_ctx_t *)gnb_worker->ctx;

    gnb_ring_buffer_release(gnb_worker->ring_buffer);

}


static int start(gnb_worker_t *gnb_worker){

    node_worker_ctx_t *node_worker_ctx = gnb_worker->ctx;

    pthread_create(&node_worker_ctx->thread_worker, NULL, thread_worker_func, gnb_worker);

    pthread_detach(node_worker_ctx->thread_worker);

    return 0;
}


static int stop(gnb_worker_t *gnb_worker){

    node_worker_ctx_t *node_worker_ctx = gnb_worker->ctx;

    gnb_core_t *gnb_core = node_worker_ctx->gnb_core;

    gnb_worker_t *node_worker = gnb_core->node_worker;

    node_worker->thread_worker_flag = 0;

    return 0;
}


static int notify(gnb_worker_t *gnb_worker){

    int ret;

    node_worker_ctx_t *node_worker_ctx = gnb_worker->ctx;
    ret = pthread_kill(node_worker_ctx->thread_worker,SIGALRM);

    return 0;

}

gnb_worker_t gnb_node_worker_mod = {

    .name      = "gnb_node_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL

};
