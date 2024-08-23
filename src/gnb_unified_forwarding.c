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

#include "gnb.h"
#include "gnb_node.h"
#include "gnb_hash32.h"
#include "gnb_pf.h"
#include "gnb_payload16.h"
#include "gnb_address.h"
#include "gnb_unified_forwarding.h"

#include "protocol/network_protocol.h"

#pragma pack(push, 1)

typedef struct _gnb_unified_forwarding_frame_foot_t {

    gnb_uuid_t src_nodeid;
    gnb_uuid_t dst_nodeid;
    gnb_uuid_t unified_forwarding_nodeid;

    uint64_t unified_forwarding_seq;

} __attribute__ ((__packed__)) gnb_unified_forwarding_frame_foot_t;

#pragma pack(pop)


static void gnb_setup_unified_forwarding_nodeid(gnb_core_t *gnb_core, gnb_node_t *dst_node){

    int i;

    int select_idx = 0;

    if ( 0 != dst_node->unified_forwarding_nodeid && (gnb_core->now_time_sec - dst_node->unified_forwarding_node_ts_sec) > GNB_UNIFIED_FORWARDING_NODE_EXPIRED_SEC ) {
        return;
    }

    for ( i=1; i<GNB_UNIFIED_FORWARDING_NODE_ARRAY_SIZE; i++ ) {

        if ( (gnb_core->now_time_sec - dst_node->unified_forwarding_node_array[i].last_ts_sec) > GNB_UNIFIED_FORWARDING_NODE_ARRAY_EXPIRED_SEC ) {
            continue;
        }

        if ( dst_node->unified_forwarding_node_array[select_idx].last_ts_sec > dst_node->unified_forwarding_node_array[i].last_ts_sec ) {
            select_idx = i;
        }

    }

    if ( select_idx >= 0 ) {
        dst_node->unified_forwarding_nodeid      = dst_node->unified_forwarding_node_array[select_idx].uuid64;
        dst_node->unified_forwarding_node_ts_sec = gnb_core->now_time_sec;
    }

}


int gnb_unified_forwarding_tun(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    gnb_node_t *dst_node;
    gnb_payload16_t *payload;

    uint16_t in_payload_size;
    gnb_unified_forwarding_frame_foot_t *unified_forwarding_frame_foot;
    gnb_node_t *unified_forwarding_node;

    dst_node = pf_ctx->dst_node;
    payload  = pf_ctx->fwd_payload;

    gnb_setup_unified_forwarding_nodeid(gnb_core, dst_node);

    if ( 0 == dst_node->unified_forwarding_nodeid ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "unified forwarding from tun dst=%llu nodeid not found!\n", dst_node->uuid64);
        return -1;
    }

    unified_forwarding_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, dst_node->unified_forwarding_nodeid);

    if ( NULL == unified_forwarding_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding from tun fwd %llu not found dst=%llu\n", dst_node->unified_forwarding_nodeid, dst_node->uuid64);
        return -2;
    }

    in_payload_size = gnb_payload16_size(payload);

    unified_forwarding_frame_foot = ( (void *)payload ) + in_payload_size;

    payload->sub_type = GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED;

    gnb_payload16_set_size( payload, in_payload_size + sizeof(gnb_unified_forwarding_frame_foot_t) );

    unified_forwarding_frame_foot->src_nodeid = gnb_htonll(gnb_core->local_node->uuid64);
    unified_forwarding_frame_foot->dst_nodeid = gnb_htonll(dst_node->uuid64);
    unified_forwarding_frame_foot->unified_forwarding_nodeid = gnb_htonll(dst_node->unified_forwarding_nodeid);

    if ( dst_node->unified_forwarding_send_seq < gnb_core->now_time_usec ) {
        dst_node->unified_forwarding_send_seq = gnb_core->now_time_usec;
    }

    dst_node->unified_forwarding_send_seq++;

    unified_forwarding_frame_foot->unified_forwarding_seq = gnb_htonll(dst_node->unified_forwarding_send_seq);

    gnb_forward_payload_to_node(gnb_core, unified_forwarding_node, payload);
    dst_node->unified_forwarding_node_ts_sec = gnb_core->now_time_sec;

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*>> Unified Forwarding from tun %llu=>%llu=>%llu seq=%"PRIu64" *>>\n", gnb_core->local_node->uuid64, dst_node->unified_forwarding_nodeid, dst_node->uuid64,  dst_node->unified_forwarding_send_seq);

    return 1;

}


int gnb_unified_forwarding_with_multi_path_tun(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    int ret;
    int i;
    int c = 0;

    gnb_node_t *dst_node;
    gnb_payload16_t *payload;
    uint16_t in_payload_size;
    gnb_unified_forwarding_frame_foot_t *unified_forwarding_frame_foot;
    gnb_node_t *unified_forwarding_node;

    if ( GNB_UNIFIED_FORWARDING_HYPER != gnb_core->conf->unified_forwarding ) {

        if ( IPPROTO_TCP != pf_ctx->ipproto ) {

            ret = gnb_unified_forwarding_tun(gnb_core, pf_ctx);

            if ( 1 != ret && NULL != pf_ctx->dst_node ) {
                gnb_forward_payload_to_node(gnb_core, pf_ctx->dst_node, pf_ctx->fwd_payload);
            }

            return ret;
        }

    }

    dst_node = pf_ctx->dst_node;
    payload  = pf_ctx->fwd_payload;

    in_payload_size = gnb_payload16_size(payload);

    unified_forwarding_frame_foot = ( (void *)payload ) + in_payload_size;

    payload->sub_type = GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED_MULTI_PATH;

    gnb_payload16_set_size( payload, in_payload_size + sizeof(gnb_unified_forwarding_frame_foot_t) );

    unified_forwarding_frame_foot->src_nodeid = gnb_htonll(gnb_core->local_node->uuid64);
    unified_forwarding_frame_foot->dst_nodeid = gnb_htonll(dst_node->uuid64);

    if ( dst_node->unified_forwarding_send_seq < gnb_core->now_time_usec ) {
        dst_node->unified_forwarding_send_seq = gnb_core->now_time_usec;
    }

    dst_node->unified_forwarding_send_seq++;

    unified_forwarding_frame_foot->unified_forwarding_seq = gnb_htonll(dst_node->unified_forwarding_send_seq);

    if ( NULL != pf_ctx->dst_node ) {
        // 先发送到目的地，然后是 unified forwarding  节点
        gnb_forward_payload_to_node(gnb_core, pf_ctx->dst_node, payload);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*>> Unified Forwarding with Multi-Path to tun local %llu=>%llu seq=%"PRIu64" *>>\n", gnb_core->local_node->uuid64, dst_node->uuid64, dst_node->unified_forwarding_send_seq);

    }

    for ( i=0; i<GNB_UNIFIED_FORWARDING_NODE_ARRAY_SIZE; i++ ) {

        if ( (gnb_core->now_time_sec - dst_node->unified_forwarding_node_array[i].last_ts_sec) > GNB_UNIFIED_FORWARDING_NODE_ARRAY_EXPIRED_SEC ) {
            continue;
        }

        dst_node->unified_forwarding_nodeid = dst_node->unified_forwarding_node_array[i].uuid64;
        unified_forwarding_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, dst_node->unified_forwarding_nodeid);

        if ( NULL == unified_forwarding_node ) {
            continue;
        }

        unified_forwarding_frame_foot->unified_forwarding_nodeid = gnb_htonll(dst_node->unified_forwarding_nodeid);
        gnb_forward_payload_to_node(gnb_core, unified_forwarding_node, payload);

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*>> Unified Forwarding with Multi-Path to tun %llu=>%llu=>%llu seq=%"PRIu64" *>>\n", gnb_core->local_node->uuid64, dst_node->uuid64, dst_node->unified_forwarding_nodeid, dst_node->unified_forwarding_send_seq);

        //最多转发5个节点
        if ( c >= 5 ) {
            break;
        }

        c++;

    }

    dst_node->unified_forwarding_node_ts_sec = gnb_core->now_time_sec;

    return c;

}


int gnb_unified_forwarding_inet(gnb_core_t *gnb_core, gnb_payload16_t *payload){

    gnb_unified_forwarding_frame_foot_t *unified_forwarding_frame_foot;

    gnb_uuid_t src_nodeid;
    gnb_node_t *src_node;

    gnb_uuid_t dst_nodeid;
    gnb_node_t *dst_node;

    gnb_uuid_t unified_forwarding_nodeid;
    gnb_node_t *unified_forwarding_node;
    uint64_t unified_forwarding_seq;

    uint16_t in_payload_size;

    in_payload_size = gnb_payload16_size(payload);

    if ( in_payload_size <= sizeof(gnb_unified_forwarding_frame_foot_t) ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet frame error size=%u\n", in_payload_size);
        return UNIFIED_FORWARDING_ERROR;
    }

    unified_forwarding_frame_foot = ( (void *)payload ) + ( in_payload_size - sizeof(gnb_unified_forwarding_frame_foot_t) );

    src_nodeid = gnb_ntohll(unified_forwarding_frame_foot->src_nodeid);
    src_node   = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, src_nodeid);

    if ( NULL == src_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet src node %llu not found!\n", src_nodeid);
        return UNIFIED_FORWARDING_ERROR;
    }

    dst_nodeid = gnb_ntohll(unified_forwarding_frame_foot->dst_nodeid);
    dst_node   = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, dst_nodeid);

    unified_forwarding_nodeid = gnb_ntohll(unified_forwarding_frame_foot->unified_forwarding_nodeid);

    if ( NULL == dst_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet dst node %llu not found!\n", dst_nodeid);
        return UNIFIED_FORWARDING_ERROR;
    }

    unified_forwarding_seq = gnb_ntohll(unified_forwarding_frame_foot->unified_forwarding_seq);

    //payload to inet
    if ( gnb_core->local_node->uuid64 != dst_nodeid ) {

        if ( gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE ) {
            return UNIFIED_FORWARDING_DROP;
        }

        gnb_forward_payload_to_node(gnb_core, dst_node, payload);

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">*> Unified Forwarding to inet %llu=>%llu=>%llu seq=%"PRIu64" >*>\n", src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq);

        return UNIFIED_FORWARDING_TO_INET;

    }

    //payload to tun
    src_node->unified_forwarding_nodeid      = unified_forwarding_nodeid;
    src_node->unified_forwarding_node_ts_sec = gnb_core->now_time_sec;

    gnb_payload16_set_size( payload, in_payload_size - sizeof(gnb_unified_forwarding_frame_foot_t) );

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*<< Unified Forwarding to tun %llu=>%llu=>%llu seq=%"PRIu64" *<<\n", src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq);

    return UNIFIED_FORWARDING_TO_TUN;

}


int gnb_unified_forwarding_multi_path_inet(gnb_core_t *gnb_core, gnb_payload16_t *payload){

    gnb_unified_forwarding_frame_foot_t *unified_forwarding_frame_foot;

    gnb_uuid_t src_nodeid;
    gnb_node_t *src_node;

    gnb_uuid_t dst_nodeid;
    gnb_node_t *dst_node;

    gnb_uuid_t unified_forwarding_nodeid;
    gnb_node_t *unified_forwarding_node;
    uint64_t unified_forwarding_seq;

    uint16_t in_payload_size;

    in_payload_size = gnb_payload16_size(payload);

    if ( in_payload_size <= sizeof(gnb_unified_forwarding_frame_foot_t) ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet frame error size=%u\n", in_payload_size);
        return UNIFIED_FORWARDING_ERROR;
    }

    unified_forwarding_frame_foot = ( (void *)payload ) + ( in_payload_size - sizeof(gnb_unified_forwarding_frame_foot_t) );

    src_nodeid = gnb_ntohll(unified_forwarding_frame_foot->src_nodeid);
    src_node   = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, src_nodeid);

    if ( NULL == src_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet src node %llu not found!\n", src_nodeid);
        return UNIFIED_FORWARDING_ERROR;
    }

    dst_nodeid = gnb_ntohll(unified_forwarding_frame_foot->dst_nodeid);
    dst_node   = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, dst_nodeid);

    unified_forwarding_nodeid = gnb_ntohll(unified_forwarding_frame_foot->unified_forwarding_nodeid);

    if ( NULL == dst_node ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding inet error dst node %llu not found!\n", dst_nodeid);
        return UNIFIED_FORWARDING_ERROR;
    }

    unified_forwarding_seq = gnb_ntohll(unified_forwarding_frame_foot->unified_forwarding_seq);

    //payload to inet
    if ( gnb_core->local_node->uuid64 != dst_nodeid ) {

        if ( gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">*> Unified Forwarding Multi Path inet local node is slience dst=%llu frame drop! >*>\n", dst_nodeid);
            return UNIFIED_FORWARDING_DROP;
        }

        gnb_forward_payload_to_node(gnb_core, dst_node, payload);

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">*> Unified Forwarding Multi Path %llu=>%llu=>%llu seq=%"PRIu64" frame to inet >*>\n", src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq);

        return UNIFIED_FORWARDING_TO_INET;

    }

    //scan the dst node unified_forwarding_seq_array
    int max_seq_idx=0;
    int min_seq_idx=UNIFIED_FORWARDING_RECV_SEQ_ARRAY_SIZE-1;
    int i;

    for ( i=0; i<UNIFIED_FORWARDING_RECV_SEQ_ARRAY_SIZE; i++ ) {

        if ( unified_forwarding_seq == dst_node->unified_forwarding_recv_seq_array[i] ) {

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*<< Unified Forwarding Multi Path inet in_seq=%"PRIu64" == recv_seq[%d]=%"PRIu64" frame drop! *<<\n",
                    unified_forwarding_seq, i, dst_node->unified_forwarding_recv_seq_array[i]);

            return UNIFIED_FORWARDING_DROP;

        }

        if ( dst_node->unified_forwarding_recv_seq_array[i] > dst_node->unified_forwarding_recv_seq_array[max_seq_idx] ) {
            max_seq_idx = i;
        }

        if ( dst_node->unified_forwarding_recv_seq_array[i] < dst_node->unified_forwarding_recv_seq_array[min_seq_idx] ) {
            min_seq_idx = i;
        }

    }


    if ( unified_forwarding_seq < dst_node->unified_forwarding_recv_seq_array[min_seq_idx] ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*<< Unified Forwarding Multi Path inet %llu=>%llu=>%llu in_seq=%"PRIu64" < min_seq=%"PRIu64" frame drop! *<<\n",
                src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq, dst_node->unified_forwarding_recv_seq_array[min_seq_idx]);

        return UNIFIED_FORWARDING_DROP;

    }


    if ( unified_forwarding_seq > dst_node->unified_forwarding_recv_seq_array[max_seq_idx] ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*<< Unified Forwarding multi path inet %llu=>%llu=>%llu in_seq=%"PRIu64" > max_seq=%"PRIu64" frame to tun *<<\n",
                src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq, dst_node->unified_forwarding_recv_seq_array[max_seq_idx]);

    } else {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*<< Unified Forwarding Multi Path inet %llu=>%llu=>%llu in_seq=%"PRIu64" < max_seq=%"PRIu64" frame to tun *<<\n",
                src_nodeid, unified_forwarding_nodeid, dst_nodeid, unified_forwarding_seq, dst_node->unified_forwarding_recv_seq_array[max_seq_idx]);

    }


    dst_node->unified_forwarding_recv_seq_array[min_seq_idx] = unified_forwarding_seq;

    //payload to tun
    src_node->unified_forwarding_nodeid      = unified_forwarding_nodeid;
    src_node->unified_forwarding_node_ts_sec = gnb_core->now_time_sec;

    gnb_payload16_set_size( payload, in_payload_size - sizeof(gnb_unified_forwarding_frame_foot_t) );

    return UNIFIED_FORWARDING_TO_TUN;

}
