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


static void pf_init_cb(gnb_core_t *gnb_core){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t*)gnb_heap_alloc(gnb_core->heap,sizeof(gnb_pf_private_ctx_t));

    GNB_PF_SET_CTX(gnb_core,gnb_pf_crypto_xor,ctx);

}


static void pf_conf_cb(gnb_core_t *gnb_core) {


}


/*
 用dst node 的key 加密 ip frmae
*/
static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    if (NULL==pf_ctx->dst_node) {
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
  用 relay node 的key 加密 payload
*/
static int pf_tun_fwd_cb(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ){
        return pf_ctx->pf_status;
    }

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    int i;
    int j = 0;

    unsigned char *p;

    if (GNB_PF_FWD_INET==pf_ctx->pf_fwd) {

        p = (unsigned char *)pf_ctx->fwd_payload->data;

        for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(uint32_t)); i++ ){

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
 用 src_fwd_node 的密钥为payload的子类型是 GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY 的 payload 解密，其他类型不处理
*/
static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    uint32_t *src_fwd_nodeid_ptr;
    uint16_t payload_size;

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    payload_size = gnb_payload16_size(pf_ctx->fwd_payload);

    src_fwd_nodeid_ptr = (uint32_t *)( (void *)pf_ctx->fwd_payload + payload_size - sizeof(uint32_t) );

    pf_ctx->src_fwd_uuid32 = ntohl(*src_fwd_nodeid_ptr);

    pf_ctx->src_fwd_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, pf_ctx->src_fwd_uuid32);

    if ( NULL==pf_ctx->src_fwd_node ) {
        pf_ctx->pf_status = GNB_PF_NOROUTE;
        goto finish;
    }


    int i;
    int j = 0;

    unsigned char *p;

    p = (unsigned char *)pf_ctx->fwd_payload->data;

    for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(uint32_t)); i++ ) {
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


static int pf_inet_route_cb(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    gnb_node_t *src_node;

    int i;
    int j = 0;
    unsigned char *p = (unsigned char *)pf_ctx->ip_frame;

    if (GNB_PF_FWD_TUN==pf_ctx->pf_fwd){

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


static int pf_inet_fwd_cb(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

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

        for ( i=0; i < (gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(uint32_t)); i++ ) {

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


static void pf_release_cb(gnb_core_t *gnb_core){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)GNB_PF_GET_CTX(gnb_core, gnb_pf_crypto_xor);

}


gnb_pf_t gnb_pf_crypto_xor = {
    0,
    "gnb_pf_crypto_xor",
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
