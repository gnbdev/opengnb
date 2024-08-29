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
#include "gnb_hash32.h"
#include "crypto/arc4/arc4.h"
#include "gnb_keys.h"
#include "protocol/network_protocol.h"

typedef struct _gnb_pf_private_ctx_t {

    int save_time_seed_update_factor;
    gnb_hash32_map_t *arc4_ctx_map;

}gnb_pf_private_ctx_t;

gnb_pf_t gnb_pf_crypto_arc4;

static void init_arc4_keys(gnb_core_t *gnb_core,gnb_pf_t *pf){

    int i;

    gnb_node_t *node;

    gnb_pf_private_ctx_t *ctx = pf->private_ctx;

    ctx->save_time_seed_update_factor = gnb_core->time_seed_update_factor;

    int num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0 == num ) {
        return;
    }

    struct arc4_sbox *sbox;

    for (i=0; i < num; i++) {

        node = &gnb_core->ctl_block->node_zone->node[i];

        gnb_build_crypto_key(gnb_core, node);

        sbox = GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, node->uuid64);

        if ( NULL == sbox ) {
            continue;
        }

        arc4_init(sbox, node->crypto_key, 64);

    }

}


static void pf_init_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t*)gnb_heap_alloc(gnb_core->heap,sizeof(gnb_pf_private_ctx_t));

    ctx->arc4_ctx_map = gnb_hash32_create(gnb_core->heap,gnb_core->node_nums,gnb_core->node_nums);

    pf->private_ctx = ctx;

    int num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0 == num ) {
        return;
    }

    int i;

    gnb_node_t *node;

    struct arc4_sbox *sbox;

    for (i=0; i < num; i++) {

        node = &gnb_core->ctl_block->node_zone->node[i];

        gnb_build_crypto_key(gnb_core, node);

        sbox = gnb_heap_alloc(gnb_core->heap, sizeof(struct arc4_sbox));

        arc4_init(sbox, node->crypto_key, 64);

        GNB_HASH32_UINT64_SET(ctx->arc4_ctx_map, node->uuid64, sbox);

    }

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s init\n", pf->name);

}


static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf) {
    init_arc4_keys(gnb_core,pf);
}


static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    if ( ctx->save_time_seed_update_factor != gnb_core->time_seed_update_factor ) {
        init_arc4_keys(gnb_core,pf);
    }

    struct arc4_sbox sbox;

    if ( NULL==pf_ctx->dst_node ) {
        return GNB_PF_ERROR;
    }

    struct arc4_sbox *sbox_init = (struct arc4_sbox *)GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, pf_ctx->dst_uuid64);

    if ( NULL==sbox_init ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "gnb_pf_crypto_arc4 tun_frame node[%llu] miss key\n", pf_ctx->dst_node->uuid64);
        return GNB_PF_ERROR;
    }

    sbox = *sbox_init;

    arc4_crypt(&sbox, pf_ctx->ip_frame, pf_ctx->ip_frame_size);

    return pf_ctx->pf_status;

}


static int pf_tun_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    if ( ctx->save_time_seed_update_factor != gnb_core->time_seed_update_factor ) {
        init_arc4_keys(gnb_core,pf);
    }

    struct arc4_sbox sbox;

    if ( GNB_PF_FWD_INET==pf_ctx->pf_fwd ) {

        struct arc4_sbox *sbox_init = (struct arc4_sbox *)GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, pf_ctx->fwd_node->uuid64 );

        if (NULL==sbox_init) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "gnb_pf_crypto_arc4 tun_frame node[%llu] miss key\n", pf_ctx->dst_node->uuid64);
            return GNB_PF_ERROR;
        }

        sbox = *sbox_init;

        arc4_crypt(&sbox, pf_ctx->fwd_payload->data, gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t));

    }

    return pf_ctx->pf_status;;

}


static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    struct arc4_sbox sbox;
    gnb_uuid_t *src_fwd_nodeid_ptr;
    uint16_t payload_size;

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    if ( ctx->save_time_seed_update_factor != gnb_core->time_seed_update_factor ) {
        init_arc4_keys(gnb_core,pf);
    }

    payload_size = gnb_payload16_size(pf_ctx->fwd_payload);

    src_fwd_nodeid_ptr = (gnb_uuid_t *)( (void *)pf_ctx->fwd_payload + payload_size - sizeof(gnb_uuid_t) );

    pf_ctx->src_fwd_uuid64 = gnb_ntohll(*src_fwd_nodeid_ptr);

    struct arc4_sbox *sbox_init = (struct arc4_sbox *)GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, pf_ctx->src_fwd_uuid64);

    if ( NULL==sbox_init ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "gnb_pf_crypto_arc4 pf_inet_frame_cb node[%llu] miss key\n", pf_ctx->src_fwd_uuid64);
        return GNB_PF_ERROR;
    }

    sbox = *sbox_init;

    arc4_crypt(&sbox, pf_ctx->fwd_payload->data, gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t));

finish:

    return pf_ctx->pf_status;

}


static int pf_inet_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    if ( ctx->save_time_seed_update_factor != gnb_core->time_seed_update_factor ) {
        init_arc4_keys(gnb_core,pf);
    }

    if ( GNB_PF_FWD_TUN==pf_ctx->pf_fwd ) {

        struct arc4_sbox *sbox_init = (struct arc4_sbox *)GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, pf_ctx->src_uuid64);

        if ( NULL==sbox_init ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "gnb_pf_crypto_arc4 inet_route node[%llu] miss key\n", pf_ctx->src_uuid64);
            return GNB_PF_ERROR;
        }

        struct arc4_sbox sbox = *sbox_init;

        arc4_crypt(&sbox, pf_ctx->ip_frame, pf_ctx->ip_frame_size);

    }

    return pf_ctx->pf_status;

}


static int pf_inet_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t *)pf->private_ctx;

    struct arc4_sbox sbox;

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {
        return pf_ctx->pf_status;
    }

    if ( ctx->save_time_seed_update_factor != gnb_core->time_seed_update_factor ) {
        init_arc4_keys(gnb_core,pf);
    }

    if ( GNB_PF_FWD_INET==pf_ctx->pf_fwd ) {

        if ( NULL==pf_ctx->fwd_node ) {
            pf_ctx->pf_status = GNB_PF_NOROUTE;
            goto finish;
        }

        struct arc4_sbox *sbox_init = (struct arc4_sbox *)GNB_HASH32_UINT64_GET_PTR(ctx->arc4_ctx_map, pf_ctx->fwd_node->uuid64);

        if ( NULL==sbox_init ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "gnb_pf_crypto_arc4 pf_inet_frame_cb node[%llu] miss key\n", pf_ctx->fwd_node->uuid64);
            return GNB_PF_ERROR;
        }

        sbox = *sbox_init;

        arc4_crypt(&sbox, pf_ctx->fwd_payload->data, gnb_payload16_data_len(pf_ctx->fwd_payload)-sizeof(gnb_uuid_t));

    }

finish:

    return pf_ctx->pf_status;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){


}


gnb_pf_t gnb_pf_crypto_arc4 = {
    "gnb_pf_crypto_arc4",
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
