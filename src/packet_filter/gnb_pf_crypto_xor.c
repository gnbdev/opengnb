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
#include "gnb_payload16.h"
#include "protocol/network_protocol.h"

typedef struct _gnb_pf_private_ctx_t {

    int save_time_seed_update_factor;

}gnb_pf_private_ctx_t;


gnb_pf_t gnb_pf_crypto_xor;


static void pf_init_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t*)gnb_heap_alloc(gnb_core->heap,sizeof(gnb_pf_private_ctx_t));
    pf->private_ctx = ctx;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s init\n", pf->name);

}


static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf) {


}


/*
 用dst node 的key 加密 ip frmae
*/
static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    if ( NULL==pf_ctx->dst_node ) {
        return GNB_PF_ERROR;
    }

    int i;
    int j = 0;

    unsigned char *p = (unsigned char *)pf_ctx->ip_frame;

    for ( i=0; i<pf_ctx->ip_frame_size; i++ ) {

        *p = *p ^ pf_ctx->dst_node->crypto_key[j];

        p++;

        j++;

        if ( j >= 64 ) {
            j = 0;
        }

    }

    return pf_ctx->pf_status;;

}


/*
  只处理有 GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY 标记的 payload
  用 relay node 的key 加密 payload
*/
static int pf_tun_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    int i;
    int j = 0;

    unsigned char *p;

    if ( GNB_PF_FWD_INET==pf_ctx->pf_fwd ) {

        p = (unsigned char *)pf_ctx->fwd_payload->data;

        for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t)); i++ ){

            *p = *p ^ pf_ctx->fwd_node->crypto_key[j];

            p++;

            j++;

            if ( j >= 64 ) {
                j = 0;
            }

        }

        goto finish;

    }

finish:

    return pf_ctx->pf_status;;

}


/*
 只处理有 GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY 标记的 payload
 用上一跳的 relay 节点(src_fwd_nodeb)的密钥为 payload 解密
*/
static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    gnb_uuid_t *src_fwd_nodeid_ptr;
    uint16_t payload_size;

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    payload_size = gnb_payload16_size(pf_ctx->fwd_payload);

    src_fwd_nodeid_ptr = (gnb_uuid_t *)( (void *)pf_ctx->fwd_payload + payload_size - sizeof(gnb_uuid_t) );

    pf_ctx->src_fwd_uuid64 = gnb_ntohll(*src_fwd_nodeid_ptr);

    pf_ctx->src_fwd_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, pf_ctx->src_fwd_uuid64);

    if ( NULL==pf_ctx->src_fwd_node ) {
        pf_ctx->pf_status = GNB_PF_NOROUTE;
        goto finish;
    }

    int i;
    int j = 0;

    unsigned char *p;

    p = (unsigned char *)pf_ctx->fwd_payload->data;

    for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t)); i++ ) {
        *p = *p ^ pf_ctx->src_fwd_node->crypto_key[j];

        p++;

        j++;

        if ( j >= 64 ) {
            j = 0;
        }

    }

    goto finish;


finish:

    return pf_ctx->pf_status;

}


/*
用 src_node 的密钥对 payload 进行解密, 得到来自 src_node 的虚拟网卡的 ip frame,
这些 ip frame 将被写入虚拟网卡
*/
static int pf_inet_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    gnb_node_t *src_node;

    int i;
    int j = 0;
    unsigned char *p = (unsigned char *)pf_ctx->ip_frame;

    if ( GNB_PF_FWD_TUN==pf_ctx->pf_fwd ) {

        src_node = pf_ctx->src_node;

        if ( NULL == src_node ) {
            return GNB_PF_ERROR;
        }

        for ( i=0; i<pf_ctx->ip_frame_size; i++ ) {

            *p = *p ^ src_node->crypto_key[j];

            p++;
            j++;

            if ( j >= 64 ) {
                j = 0;
            }

        }

    }

    return pf_ctx->pf_status;

}


/*
只处理有 GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY 标记的 payload
payload 发往用下一跳前，用下一跳节点的的密钥加密 payload
*/
static int pf_inet_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;
    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    int i;

    int j = 0;

    unsigned char *p;

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    if ( GNB_PF_FWD_INET==pf_ctx->pf_fwd ) {

        if ( NULL==pf_ctx->fwd_node ) {
            pf_ctx->pf_status = GNB_PF_NOROUTE;
            goto finish;
        }

        p = (unsigned char *)pf_ctx->fwd_payload->data;

        for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t)); i++ ) {

            *p = *p ^ pf_ctx->fwd_node->crypto_key[j];

            p++;

            j++;

            if ( j >= 64 ) {
                j = 0;
            }

        }

        goto finish;

    }

finish:

    return pf_ctx->pf_status;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}


gnb_pf_t gnb_pf_crypto_xor = {
    "gnb_pf_crypto_xor",
    NULL,
    pf_init_cb,
    pf_conf_cb,
    NULL,
    pf_tun_route_cb,
    pf_tun_fwd_cb,
    pf_inet_frame_cb,
    pf_inet_route_cb,
    pf_inet_fwd_cb,
    pf_release_cb
};
