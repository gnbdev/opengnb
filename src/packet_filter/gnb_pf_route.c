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
#include "gnb_pf.h"
#include "gnb_node.h"
#include "gnb_payload16.h"
#include "protocol/network_protocol.h"

#define GNB_PAYLOAD_MAX_TTL     0x05

typedef struct _gnb_route_ctx_t {

    void *udata;

}gnb_route_ctx_t;


gnb_node_t* gnb_query_route4(gnb_core_t *gnb_core, uint32_t dst_ip_int);

#pragma pack(push, 1)

typedef struct _gnb_route_frame_head_t {

    unsigned char magic[2];

    unsigned char pf_type_bits; //用于加密,压缩标识

    uint8_t ttl;

    gnb_uuid_t src_uuid64;

    gnb_uuid_t dst_uuid64;

} __attribute__ ((__packed__)) gnb_route_frame_head_t;

#pragma pack(pop)

#define MIN_ROUTE_FRAME_SIZE ( sizeof(gnb_route_frame_head_t) + sizeof(struct iphdr) )

static void pf_init_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

    gnb_route_ctx_t *ctx = (gnb_route_ctx_t*)gnb_heap_alloc(gnb_core->heap,sizeof(gnb_route_ctx_t));    
    ctx->udata = NULL;

    pf->private_ctx = ctx;

    if ( 0 == gnb_core->tun_payload_offset ) {
        gnb_core->tun_payload_offset = sizeof(gnb_route_frame_head_t);
    }

    gnb_core->route_frame_head_size = sizeof(gnb_route_frame_head_t);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s init\n", pf->name);

}


static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}


/*
 * 创建 route_frame ，填充 ip_frame, 得到dst_node
*/
static int pf_tun_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    if ( NULL==pf_ctx->fwd_payload ) {
        return GNB_PF_ERROR;
    }

    //从payload中得到ip分组的首地址
    struct iphdr  *ip_frame_head;

    ip_frame_head = (struct iphdr*)(pf_ctx->fwd_payload->data + gnb_core->tun_payload_offset);

    //从ip分组中得到 dst ip，用于查路由表获得 dst node
    if ( 0x4 != ip_frame_head->version && 0x6 != ip_frame_head->version ) {
        return GNB_PF_DROP;
    }

    struct ip6_hdr  *ip6_frame_head;
    uint32_t dst_ip_int;

    if ( 0x6 == ip_frame_head->version ) {
        ip6_frame_head = (struct ip6_hdr *)(pf_ctx->fwd_payload->data + gnb_core->tun_payload_offset);
        dst_ip_int = ip6_frame_head->ip6_dst.__in6_u.__u6_addr32[3];
        pf_ctx->ipproto = ip6_frame_head->ip6_ctlun.ip6_un1.ip6_un1_nxt;
        goto handle_ip_frame;
    }

    if ( 0x4 == ip_frame_head->version ) {
        dst_ip_int = *((uint32_t *)&ip_frame_head->daddr);
        pf_ctx->ipproto =ip_frame_head->protocol;
    }

handle_ip_frame:

    pf_ctx->dst_node = gnb_query_route4(gnb_core,dst_ip_int);

    if ( NULL==pf_ctx->dst_node ) {
        return GNB_PF_DROP;
    }

    if ( pf_ctx->dst_node == gnb_core->local_node ) {
        return GNB_PF_DROP;
    }

    //如果 dst node 与 本节点的 node 相同，而在tun设备中不会出现这个情况，因此可以判断这个ip frame是有问题的，drop掉
    if ( pf_ctx->dst_node->tun_addr4.s_addr == gnb_core->local_node->tun_addr4.s_addr ) {
        return GNB_PF_DROP;
    }

    gnb_route_frame_head_t *route_frame_head;

    route_frame_head = (gnb_route_frame_head_t *)pf_ctx->fwd_payload->data;

    //在frame head 中填上magic number,接收方要检验
    route_frame_head->magic[0] = 'S';
    route_frame_head->magic[1] = 'U';

    route_frame_head->pf_type_bits = gnb_core->conf->pf_bits;

    pf_ctx->pf_type_bits = &route_frame_head->pf_type_bits;

    route_frame_head->ttl = GNB_PAYLOAD_MAX_TTL;


    //把src和dst的 uuid64 保存在ctx中，route过程要用到
    route_frame_head->src_uuid64 = gnb_htonll(gnb_core->local_node->uuid64);
    route_frame_head->dst_uuid64 = gnb_htonll(pf_ctx->dst_node->uuid64);

    pf_ctx->src_uuid64 = gnb_core->local_node->uuid64;
    pf_ctx->dst_uuid64 = pf_ctx->dst_node->uuid64;

    //把 ip frame 和 size，保存在ctx中，供后面的模块使用
    pf_ctx->ip_frame = pf_ctx->fwd_payload->data + sizeof(gnb_route_frame_head_t);
    pf_ctx->ip_frame_size = gnb_payload16_data_len(pf_ctx->fwd_payload) - sizeof(gnb_route_frame_head_t);

    //这里肯定是 GNB_PF_FWD_INET ，来自本节点tun的分组不可能再发回本节点的tun/tap设备
    pf_ctx->pf_fwd = GNB_PF_FWD_INET;

    return GNB_PF_NEXT;

}


/*
 * route，得到fwd_node
*/
static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    int ret = GNB_PF_NEXT;

    uint8_t relay_count;

    uint16_t org_payload_size;
    uint16_t new_payload_size;

    gnb_uuid_t *src_fwd_nodeid_ptr;
    gnb_uuid_t *relay_nodeid_ptr;

    int relay_nodeid_idx;
    gnb_node_t *last_relay_node;

    gnb_route_frame_head_t *route_frame_head = (gnb_route_frame_head_t *)pf_ctx->fwd_payload->data;

    if ( NULL == pf_ctx->dst_node ) {
        ret = GNB_PF_DROP;
        goto finish;
    }

    if ( 0 == gnb_core->conf->direct_forwarding ) {

        if ( NULL != gnb_core->select_fwd_node ) {
            pf_ctx->fwd_node = gnb_core->select_fwd_node;
            pf_ctx->std_forwarding = 1;
            ret = GNB_PF_NEXT;
            goto handle_relay;
        } else if ( gnb_core->fwdu0_address_ring.address_list->num > 0 ) {
            pf_ctx->universal_udp4_relay = 1;
            ret = GNB_PF_NOROUTE;
            goto finish;
        } else {
            ret = GNB_PF_ERROR;
            goto handle_relay;
        }

    }

    if ( (0 == gnb_core->fwd_node_ring.num ) && GNB_UNIFIED_FORWARDING_OFF==gnb_core->conf->unified_forwarding && ( (pf_ctx->dst_node->udp_addr_status & GNB_NODE_STATUS_IPV6_PING) || (pf_ctx->dst_node->udp_addr_status & GNB_NODE_STATUS_IPV4_PING) ) ) {
        pf_ctx->fwd_node = pf_ctx->dst_node;
        pf_ctx->direct_forwarding = 1;
        ret = GNB_PF_NEXT;
        goto handle_relay;
    }

    //设置了 direct_forward 且 dst_node 状态是激活状态，fwd_node 就是 dst_node
    if ( (pf_ctx->dst_node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) || (pf_ctx->dst_node->udp_addr_status & GNB_NODE_STATUS_IPV4_PONG) ) {
        pf_ctx->fwd_node = pf_ctx->dst_node;
        pf_ctx->direct_forwarding = 1;
        ret = GNB_PF_NEXT;
        goto handle_relay;
    }


    if ( gnb_core->fwdu0_address_ring.address_list->num > 0 && NULL == gnb_core->select_fwd_node ) {
        ret = GNB_PF_NOROUTE;
        goto handle_relay;
    }

    if ( NULL == gnb_core->select_fwd_node ) {
        ret = GNB_PF_DROP;
        goto handle_relay;
    }

    if ( (gnb_core->select_fwd_node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) || (gnb_core->select_fwd_node->udp_addr_status & GNB_NODE_STATUS_IPV4_PONG) ) {
        pf_ctx->fwd_node = gnb_core->select_fwd_node;
        pf_ctx->fwd_payload->sub_type |= GNB_PAYLOAD_SUB_TYPE_IPFRAME_STD;
        pf_ctx->std_forwarding = 1;
        ret = GNB_PF_NEXT;
        goto handle_relay;
    } else {
        ret = GNB_PF_DROP;
        goto handle_relay;
    }


handle_relay:

    if ( GNB_NODE_RELAY_DISABLE == pf_ctx->dst_node->node_relay_mode ) {
        goto finish_relay;
    }

    if  ( GNB_PF_NEXT == ret && (GNB_NODE_RELAY_AUTO & pf_ctx->dst_node->node_relay_mode) ) {
        goto finish_relay;
    }

    if ( !( (GNB_NODE_RELAY_FORCE|GNB_NODE_RELAY_AUTO) & pf_ctx->dst_node->node_relay_mode ) ) {
        goto finish_relay;
    }

    relay_count = pf_ctx->dst_node->route_node_ttls[pf_ctx->dst_node->selected_route_node];

    if ( 0 == relay_count || relay_count > GNB_MAX_NODE_RELAY ) {
        goto finish_relay;
    }

    pf_ctx->fwd_payload->sub_type |= GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY;

    if ( GNB_NODE_RELAY_STATIC & pf_ctx->dst_node->node_relay_mode ) {
        pf_ctx->dst_node->selected_route_node = 0;
    }

    relay_nodeid_ptr = (gnb_uuid_t *)( pf_ctx->fwd_payload->data + sizeof(gnb_route_frame_head_t) + pf_ctx->ip_frame_size );

    for ( relay_nodeid_idx=0; relay_nodeid_idx < relay_count; relay_nodeid_idx++ ) {
        relay_nodeid_ptr[ relay_nodeid_idx ] = gnb_htonll( pf_ctx->dst_node->route_node[ pf_ctx->dst_node->selected_route_node ][ relay_nodeid_idx ] );
    }

    relay_nodeid_ptr[ relay_nodeid_idx ] = gnb_htonll(gnb_core->local_node->uuid64);

    org_payload_size = gnb_payload16_size(pf_ctx->fwd_payload);

    route_frame_head->ttl = relay_count + 1;

    new_payload_size = org_payload_size + relay_count*sizeof(gnb_uuid_t) + sizeof(gnb_uuid_t);

    if ( new_payload_size > GNB_MAX_PAYLOAD_SIZE ) {
        ret = GNB_PF_DROP;
        goto finish_relay;
    }

    gnb_payload16_set_size(pf_ctx->fwd_payload, new_payload_size);

    pf_ctx->fwd_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, pf_ctx->dst_node->route_node[ pf_ctx->dst_node->selected_route_node ][ relay_count-1 ]);

    if ( NULL==pf_ctx->fwd_node ) {
        ret = GNB_PF_NOROUTE;
        goto finish_relay;
    }

    pf_ctx->relay_forwarding = 1;

    route_frame_head->pf_type_bits = gnb_core->conf->pf_bits;

    ret = GNB_PF_NEXT;

    if ( GNB_NODE_RELAY_BALANCE & pf_ctx->dst_node->node_relay_mode ) {

        pf_ctx->dst_node->selected_route_node++;

        if ( 0 == pf_ctx->dst_node->route_node[pf_ctx->dst_node->selected_route_node][0] ) {
            pf_ctx->dst_node->selected_route_node = 0;
        }

    }

    if ( 1==gnb_core->conf->if_dump ) {

        for ( relay_nodeid_idx=0; relay_nodeid_idx<relay_count; relay_nodeid_idx++ ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_tun_route_cb idx=%u relay node=%llu\n", relay_nodeid_idx, pf_ctx->dst_node->route_node[ pf_ctx->dst_node->selected_route_node ][ relay_nodeid_idx ]);
        }

    }

finish_relay:

    if ( 0==pf_ctx->relay_forwarding && 1==pf_ctx->direct_forwarding ) {

        if ( 0 == pf_ctx->dst_node->last_relay_nodeid ) {
            goto finish;
        }

        if ( 0 == pf_ctx->dst_node->last_relay_node_ts_sec || (gnb_core->now_time_sec - pf_ctx->dst_node->last_relay_node_ts_sec) > GNB_LAST_RELAY_NODE_EXPIRED_SEC ) {
            goto finish;
        }

        last_relay_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, pf_ctx->dst_node->last_relay_nodeid);

        if ( NULL==last_relay_node ) {
            goto finish;
        }

        if ( (last_relay_node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) || (last_relay_node->udp_addr_status & GNB_NODE_STATUS_IPV4_PONG) ) {
            pf_ctx->fwd_node = last_relay_node;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_tun_route_cb forward through last relay node %llu => %llu => %llu\n", gnb_core->local_node->uuid64, last_relay_node->uuid64, pf_ctx->dst_node->uuid64 );
        }

    }

finish:

    GNB_LOG4(gnb_core->log, GNB_LOG_ID_PF, "pf_tun_route_cb [%llu]>[%llu] pf_ctx->in_ttl[%u] route_frame_head->ttl[%u] ip_frame_size[%d]\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, pf_ctx->in_ttl, route_frame_head->ttl, pf_ctx->ip_frame_size);

    return ret;

}


static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    int ret = GNB_PF_NEXT;

    uint16_t payload_data_size;
    gnb_uuid_t *pre_src_fwd_nodeid_ptr;

    //gnb_route_ctx_t *ctx = (gnb_route_ctx_t *)GNB_PF_GET_CTX(gnb_core,gnb_pf_route);

    gnb_route_frame_head_t *route_frame_head;

    int i;
    gnb_uuid_t *nodeid_ptr;

    if ( NULL == pf_ctx->fwd_payload ) {
        ret = GNB_PF_ERROR;
        goto finish;
    }

    payload_data_size = gnb_payload16_data_len(pf_ctx->fwd_payload);

    if( payload_data_size < MIN_ROUTE_FRAME_SIZE ) {
        ret = GNB_PF_ERROR;
        goto finish;
    }

    pre_src_fwd_nodeid_ptr = (gnb_uuid_t *)( pf_ctx->fwd_payload->data + payload_data_size );

    pf_ctx->src_fwd_uuid64 = gnb_ntohll(*pre_src_fwd_nodeid_ptr);

    //从payload中得到 route_frame 首地址
    route_frame_head = (gnb_route_frame_head_t *)pf_ctx->fwd_payload->data;

    //检查magic number是否正确，如果不正确，那有可能是对端发来的payload不对，或者前面接收模块对数据处理不对
    if ( route_frame_head->magic[0] != 'S' || route_frame_head->magic[1] != 'U' ) {
        ret = GNB_PF_ERROR;
        goto finish;
    }

    pf_ctx->src_uuid64 = gnb_ntohll(route_frame_head->src_uuid64);
    pf_ctx->dst_uuid64 = gnb_ntohll(route_frame_head->dst_uuid64);

    pf_ctx->pf_type_bits = &route_frame_head->pf_type_bits;

    if ( route_frame_head->ttl > GNB_PAYLOAD_MAX_TTL ) {
        ret = GNB_PF_DROP;
        goto finish;
    }

    //把 ip frame 和 size，保存在ctx中
    pf_ctx->ip_frame = pf_ctx->fwd_payload->data + sizeof(gnb_route_frame_head_t);

    if ( GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY & pf_ctx->fwd_payload->sub_type ) {
        pf_ctx->ip_frame_size = gnb_payload16_data_len(pf_ctx->fwd_payload) - sizeof(gnb_route_frame_head_t) - route_frame_head->ttl*sizeof(gnb_uuid_t);
    } else {
        pf_ctx->ip_frame_size = gnb_payload16_data_len(pf_ctx->fwd_payload) - sizeof(gnb_route_frame_head_t);
    }

    pf_ctx->in_ttl = route_frame_head->ttl;

    if ( route_frame_head->ttl > 0) {
        route_frame_head->ttl--;
    }

    if (gnb_core->local_node->uuid64 == pf_ctx->dst_uuid64) {
        route_frame_head->ttl = 0;
    }

    ret = GNB_PF_NEXT;

finish:

    if ( 1==gnb_core->conf->if_dump ) {

        if( GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY & pf_ctx->fwd_payload->sub_type ) {

            nodeid_ptr = (gnb_uuid_t *)(pf_ctx->fwd_payload->data + sizeof(gnb_route_frame_head_t) + pf_ctx->ip_frame_size);

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_frame_cb src_fwd[%llu] [%llu]>[%llu] in_ttl[%u] ip_frame_size[%u]\n", pf_ctx->src_fwd_uuid64, pf_ctx->src_uuid64, pf_ctx->dst_uuid64, pf_ctx->in_ttl, pf_ctx->ip_frame_size);

            for ( i=0; i<pf_ctx->in_ttl; i++ ) {
                GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_frame_cb [%llu]>[%llu] relay[%llu]\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, gnb_ntohll(*nodeid_ptr) );
                nodeid_ptr++;
            }

        } else {

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_frame_cb [%llu]>[%llu] in_ttl[%u] ip_frame_size[%u]\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, pf_ctx->in_ttl, pf_ctx->ip_frame_size);

        }

    }

    return ret;

}


static int pf_inet_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    //gnb_route_ctx_t *ctx = (gnb_route_ctx_t *)GNB_PF_GET_CTX(gnb_core,gnb_pf_route);

    gnb_route_frame_head_t *route_frame_head;

    gnb_payload16_t *payload_in = pf_ctx->fwd_payload;

    route_frame_head = (gnb_route_frame_head_t *)payload_in->data;

    int ret = GNB_PF_NEXT;

    gnb_uuid_t *src_fwd_nodeid_ptr;
    gnb_uuid_t *relay_nodeid_ptr;
    gnb_uuid_t  relay_nodeid;

    gnb_uuid_t  current_nodeid;

    uint16_t payload_data_size;

    int i;
    gnb_uuid_t *nodeid_ptr;

    payload_data_size = GNB_PAYLOAD16_DATA_SIZE(pf_ctx->fwd_payload);

    pf_ctx->src_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, pf_ctx->src_uuid64);

    if ( NULL==pf_ctx->src_node ) {
        ret = GNB_PF_DROP;
        goto finish;
    }

    if ( gnb_core->local_node->uuid64 == pf_ctx->dst_uuid64 ) {

        pf_ctx->pf_fwd = GNB_PF_FWD_TUN;
        ret = GNB_PF_NEXT;

        if ( GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY & pf_ctx->fwd_payload->sub_type ) {
            src_fwd_nodeid_ptr = (gnb_uuid_t *)(pf_ctx->fwd_payload->data + payload_data_size - sizeof(gnb_uuid_t));
            pf_ctx->src_node->last_relay_nodeid = gnb_ntohll( *(src_fwd_nodeid_ptr) );
            pf_ctx->src_node->last_relay_node_ts_sec = gnb_core->now_time_sec;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_route_cb GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY src_nodeid=%llu set last_relay_nodeid=%llu\n", pf_ctx->src_node->uuid64, pf_ctx->src_node->last_relay_nodeid);
        } else {
            pf_ctx->src_node->last_relay_nodeid = 0;
            pf_ctx->src_node->last_relay_node_ts_sec = 0l;
        }

        goto finish;

    }

    if ( 0x0 == route_frame_head->ttl ) {
        ret = GNB_PF_DROP;
        goto finish;
    }

    if ( GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY & pf_ctx->fwd_payload->sub_type ) {
        goto route_relay;
    }

route_default:

    pf_ctx->dst_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, pf_ctx->dst_uuid64);

    if ( NULL != pf_ctx->dst_node ) {
        pf_ctx->fwd_node = pf_ctx->dst_node;
        pf_ctx->pf_fwd = GNB_PF_FWD_INET;
    } else {
        pf_ctx->fwd_node = gnb_core->select_fwd_node;
        pf_ctx->pf_fwd = GNB_PF_FWD_INET;
    }

    ret = GNB_PF_NEXT;

    if ( 1==gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_route_cb [%llu]>[%llu] out_ttl[%u] ip_frame_size[%u] route node\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, route_frame_head->ttl, pf_ctx->ip_frame_size);
    }

    goto finish;

route_relay:

    if ( 1 == pf_ctx->in_ttl ) {
        //不可能到这里
        ret = GNB_PF_ERROR;
        goto finish;
    }

    src_fwd_nodeid_ptr = (gnb_uuid_t *)(pf_ctx->fwd_payload->data + payload_data_size - sizeof(gnb_uuid_t));

    current_nodeid = gnb_ntohll( *(src_fwd_nodeid_ptr-1) );

    if ( pf_ctx->in_ttl > 2 ) {

        relay_nodeid_ptr = src_fwd_nodeid_ptr-2;

        if ( current_nodeid != gnb_core->local_node->uuid64 ) {
            ret = GNB_PF_ERROR;
            goto finish;
        }

    } else {
        relay_nodeid_ptr = NULL;
    }

    if ( NULL != relay_nodeid_ptr ) {

        relay_nodeid = gnb_ntohll( *relay_nodeid_ptr );

        if ( relay_nodeid==gnb_core->local_node->uuid64 ) {
            ret = GNB_PF_ERROR;
            goto finish;
        }

    } else {

        relay_nodeid = pf_ctx->dst_uuid64;

    }

    pf_ctx->dst_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, relay_nodeid);

    if ( NULL == pf_ctx->dst_node ) {
        ret = GNB_PF_NOROUTE;
        goto finish;
    }

    pf_ctx->fwd_node = pf_ctx->dst_node;
    pf_ctx->pf_fwd = GNB_PF_FWD_INET;

    gnb_payload16_set_data_len(pf_ctx->fwd_payload, payload_data_size - sizeof(gnb_uuid_t) );

    ret = GNB_PF_NEXT;

    if ( 1==gnb_core->conf->if_dump ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "pf_inet_route_cb [%llu]>[%llu] out_ttl[%u] ip_frame_size[%u] route relay node\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, route_frame_head->ttl, pf_ctx->ip_frame_size);

        nodeid_ptr = (gnb_uuid_t *)(pf_ctx->fwd_payload->data + sizeof(gnb_route_frame_head_t) + pf_ctx->ip_frame_size);

        for ( i=0; i<(pf_ctx->in_ttl-1); i++ ) {
            GNB_LOG3( gnb_core->log,GNB_LOG_ID_PF, "pf_inet_frame_cb [%llu]>[%llu] in_ttl[%u] relay[%llu]\n", pf_ctx->src_uuid64, pf_ctx->dst_uuid64, pf_ctx->in_ttl, gnb_ntohll(*nodeid_ptr) );
            nodeid_ptr++;
        }

    }

finish:

    return ret;

}


static int pf_inet_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    return pf_ctx->pf_status;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){


}


gnb_pf_t gnb_pf_route = {
    "gnb_pf_route",
    NULL,
    pf_init_cb,
    pf_conf_cb,

    pf_tun_frame_cb,
    pf_tun_route_cb,
    NULL,

    pf_inet_frame_cb,
    pf_inet_route_cb,
    NULL,

    pf_release_cb
};
