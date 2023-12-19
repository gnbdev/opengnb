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

#include "zlib/zlib.h"


typedef struct _gnb_pf_private_ctx_t {
    
    uint32_t max_deflate_chunk_size;
    uint32_t max_inflate_chunk_size;

    z_stream deflate_strm;
    gnb_payload16_t *deflated_payload;

    z_stream inflate_strm;    
    gnb_payload16_t *inflate_payload;

}gnb_pf_private_ctx_t;


gnb_pf_t gnb_pf_zip;


static void pf_init_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

    int ret;

    gnb_pf_private_ctx_t *ctx = (gnb_pf_private_ctx_t*)gnb_heap_alloc(gnb_core->heap,sizeof(gnb_pf_private_ctx_t));


    ctx->max_deflate_chunk_size = gnb_core->conf->payload_block_size;
    ctx->max_inflate_chunk_size = gnb_core->conf->payload_block_size;


    if ( 0==gnb_core->conf->pf_worker_num  ) {
        ctx->deflated_payload = (gnb_payload16_t *)gnb_core->ctl_block->core_zone->pf_worker_payload_blocks;
    } else {
        ctx->deflated_payload = (gnb_payload16_t *)(gnb_core->ctl_block->core_zone->pf_worker_payload_blocks + 
        (sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size + sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size ) * (1+gnb_core->pf_worker_ring->cur_idx));
    }

    ctx->inflate_payload  = (gnb_payload16_t *)((unsigned char *)ctx->deflated_payload + sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size);

    ctx->deflate_strm.zalloc = Z_NULL;
    ctx->deflate_strm.zfree  = Z_NULL;
    ctx->deflate_strm.opaque = Z_NULL;

    if ( 0==gnb_core->conf->zip_level ) {
        ret = deflateInit(&ctx->deflate_strm, Z_DEFAULT_COMPRESSION);    
    } else {
        ret = deflateInit(&ctx->deflate_strm, gnb_core->conf->zip_level);    
    }

    if ( ret != Z_OK ) {
        GNB_LOG1(gnb_core->log,GNB_LOG_ID_PF, "%s deflateInit error\n", gnb_pf_zip.name);
    }

    ctx->inflate_strm.zalloc   = Z_NULL;
    ctx->inflate_strm.zfree    = Z_NULL;
    ctx->inflate_strm.opaque   = Z_NULL;

    ret = inflateInit(&ctx->inflate_strm);

    if ( ret != Z_OK ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s inflateInit error\n", gnb_pf_zip.name);
    }

    pf->private_ctx = ctx;

}


static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){


}


/*
 deflate 压缩 payload
*/
static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    int ret;

    int deflate_chunk_size;

    uint16_t in_payload_data_len;

    if ( 0==gnb_core->conf->zip_level ) {
        return pf_ctx->pf_status;
    }

    gnb_pf_private_ctx_t *ctx = pf->private_ctx;

    deflateReset(&ctx->deflate_strm);

    in_payload_data_len = gnb_payload16_data_len(pf_ctx->fwd_payload);

    ctx->deflate_strm.next_in   = pf_ctx->fwd_payload->data;
    ctx->deflate_strm.avail_in  = in_payload_data_len;
    ctx->deflate_strm.avail_out = gnb_core->conf->payload_block_size;
    ctx->deflate_strm.next_out  = ctx->deflated_payload->data;

    ret = deflate(&ctx->deflate_strm, Z_FINISH);

    if ( ret != Z_STREAM_END ) {
        return GNB_PF_ERROR;
    }

    deflate_chunk_size = gnb_core->conf->payload_block_size - ctx->deflate_strm.avail_out;

    if ( deflate_chunk_size >= gnb_core->conf->payload_block_size ) {
        return GNB_PF_ERROR;
    }

    if( deflate_chunk_size >= gnb_core->conf->payload_block_size ) {
        return GNB_PF_ERROR;
    }

    if ( GNB_ZIP_AUTO == gnb_core->conf->zip && deflate_chunk_size >= in_payload_data_len ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Deflate Skip in payload size=%d deflate chunk size=%d\n", in_payload_data_len, deflate_chunk_size);
        goto skip_deflate;
    }


    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Deflate in payload size=%d deflate_chunk_size=%d\n", in_payload_data_len, deflate_chunk_size);

    ctx->deflated_payload->type     = pf_ctx->fwd_payload->type;
    ctx->deflated_payload->sub_type = pf_ctx->fwd_payload->sub_type | GNB_PAYLOAD_SUB_TYPE_IPFRAME_ZIP;    

    pf_ctx->fwd_payload = ctx->deflated_payload;
    gnb_payload16_set_data_len(pf_ctx->fwd_payload, deflate_chunk_size);

skip_deflate:

    return GNB_PF_NEXT;

}


/*
  inflate 解压 payload
*/
static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){


    int ret;

    int inflate_chunk_size;

    uint16_t in_payload_data_len;

    if ( 0==gnb_core->conf->zip_level ) {
        return pf_ctx->pf_status;
    }

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_ZIP) ) {
        return pf_ctx->pf_status;
    }

    gnb_pf_private_ctx_t *ctx = pf->private_ctx;

    inflateReset(&ctx->inflate_strm);

    in_payload_data_len = gnb_payload16_data_len(pf_ctx->fwd_payload);

    ctx->inflate_strm.next_in   = pf_ctx->fwd_payload->data;
    ctx->inflate_strm.avail_in  = in_payload_data_len;
    ctx->inflate_strm.avail_out = gnb_core->conf->payload_block_size;
    ctx->inflate_strm.next_out  = ctx->inflate_payload->data;

    ret = inflate(&ctx->inflate_strm, Z_FINISH);

    if ( ret != Z_STREAM_END ) {
        return  GNB_PF_ERROR;
    }

    inflate_chunk_size = gnb_core->conf->payload_block_size - ctx->inflate_strm.avail_out;

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Inflate payload size=%d inflate_chunk_size=%d\n", in_payload_data_len, inflate_chunk_size);

    ctx->inflate_payload->type     = pf_ctx->fwd_payload->type;
    ctx->inflate_payload->sub_type = pf_ctx->fwd_payload->sub_type;

    pf_ctx->fwd_payload = ctx->inflate_payload;

    gnb_payload16_set_data_len(pf_ctx->fwd_payload, inflate_chunk_size);

    return GNB_PF_NEXT;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}


gnb_pf_t gnb_pf_zip = {
    "gnb_pf_zip",
    NULL,
    pf_init_cb,
    pf_conf_cb,
    NULL,
    pf_tun_route_cb,
    NULL,
    pf_inet_frame_cb,
    NULL,
    NULL,
    pf_release_cb
};
