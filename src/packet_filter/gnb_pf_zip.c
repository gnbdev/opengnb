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
#include "gnb_binary.h"

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

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s init\n", pf->name);

}


static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){


}


/*
 deflate 压缩 payload
对 pf_ctx->ip_frame 起 pf_ctx->ip_frame_size 个字节压缩
对于包含 GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY 标志的 payload 尾部的relay node id 数组需要保留
*/
static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    int ret;

    int deflate_chunk_size;

    //uint16_t in_payload_data_len;
    uint16_t frame_header_size;
    uint16_t frame_tail_size;

    if ( 0==gnb_core->conf->zip_level ) {
        return pf_ctx->pf_status;
    }

    frame_header_size = gnb_core->tun_payload_offset;

    gnb_pf_private_ctx_t *ctx = pf->private_ctx;

    deflateReset(&ctx->deflate_strm);

    ctx->deflate_strm.next_in   = pf_ctx->ip_frame;
    ctx->deflate_strm.avail_in  = pf_ctx->ip_frame_size;

    ctx->deflate_strm.next_out  = ctx->deflated_payload->data + frame_header_size;
    ctx->deflate_strm.avail_out = gnb_core->conf->payload_block_size;

    ret = deflate(&ctx->deflate_strm, Z_FINISH);

    if ( ret != Z_STREAM_END ) {
        return GNB_PF_ERROR;
    }


    // deflate_chunk_size is new ip_frame_size
    deflate_chunk_size = gnb_core->conf->payload_block_size - ctx->deflate_strm.avail_out;

    if ( deflate_chunk_size >= gnb_core->conf->payload_block_size ) {
        return GNB_PF_ERROR;
    }

    if( deflate_chunk_size >= gnb_core->conf->payload_block_size ) {
        return GNB_PF_ERROR;
    }

    if ( GNB_ZIP_AUTO == gnb_core->conf->zip && deflate_chunk_size >= pf_ctx->ip_frame_size ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Deflate Skip in payload ip_frame_size=%d deflate chunk size=%d\n", pf_ctx->ip_frame_size, deflate_chunk_size);
        goto skip_deflate;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Deflate in payload size=%d deflate_chunk_size=%d\n", pf_ctx->ip_frame_size, deflate_chunk_size);

    ctx->deflated_payload->type     = pf_ctx->fwd_payload->type;
    ctx->deflated_payload->sub_type = pf_ctx->fwd_payload->sub_type | GNB_PAYLOAD_SUB_TYPE_IPFRAME_ZIP;    

    
    //拷贝 ip_frame 前的 frame header 数据
    memcpy(ctx->deflated_payload->data, pf_ctx->fwd_payload->data, gnb_core->tun_payload_offset);

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY) ) {

        gnb_payload16_set_data_len(ctx->deflated_payload, frame_header_size + deflate_chunk_size);

    } else {

        //GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY
        //拷贝 ip_frame 后的数据

        frame_tail_size   = gnb_payload16_data_len(pf_ctx->fwd_payload) - frame_header_size - pf_ctx->ip_frame_size;

        memcpy((ctx->deflated_payload->data + frame_header_size + deflate_chunk_size), (pf_ctx->ip_frame + pf_ctx->ip_frame_size), frame_tail_size);
        gnb_payload16_set_data_len(ctx->deflated_payload, frame_header_size + deflate_chunk_size + frame_tail_size);
    
    }

    pf_ctx->fwd_payload = ctx->deflated_payload;

    //重新指定 ip_frame 位置 和 ip_frame_size
    pf_ctx->ip_frame = pf_ctx->fwd_payload->data + frame_header_size;
    pf_ctx->ip_frame_size = deflate_chunk_size;


skip_deflate:

    return GNB_PF_NEXT;

}



/*
  inflate 解压 payload
*/
static int pf_inet_route(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    int ret;

    int inflate_chunk_size;

    uint16_t in_payload_data_len;
    uint16_t frame_header_size;

    if ( 0==gnb_core->conf->zip_level ) {
        return pf_ctx->pf_status;
    }

    if ( !(pf_ctx->fwd_payload->sub_type & GNB_PAYLOAD_SUB_TYPE_IPFRAME_ZIP) ) {
        return pf_ctx->pf_status;
    }

    //目标节点不是本地节点，就不要解压数据
    if ( pf_ctx->dst_uuid64 != gnb_core->local_node->uuid64 ) {
        return pf_ctx->pf_status;
    }

    frame_header_size = gnb_core->tun_payload_offset;

    gnb_pf_private_ctx_t *ctx = pf->private_ctx;

    inflateReset(&ctx->inflate_strm);

    in_payload_data_len = gnb_payload16_data_len(pf_ctx->fwd_payload);

    ctx->inflate_strm.next_in   = pf_ctx->fwd_payload->data + frame_header_size;
    ctx->inflate_strm.avail_in  = in_payload_data_len - frame_header_size;

    ctx->inflate_strm.next_out  = ctx->inflate_payload->data + frame_header_size;
    ctx->inflate_strm.avail_out = gnb_core->conf->payload_block_size;


    ret = inflate(&ctx->inflate_strm, Z_FINISH);

    if ( ret != Z_STREAM_END ) {
        return  GNB_PF_ERROR;
    }

    inflate_chunk_size = gnb_core->conf->payload_block_size - ctx->inflate_strm.avail_out;

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Inflate payload size=%d inflate_chunk_size=%d\n", in_payload_data_len, inflate_chunk_size);

    ctx->inflate_payload->type     = pf_ctx->fwd_payload->type;
    ctx->inflate_payload->sub_type = pf_ctx->fwd_payload->sub_type;

    memcpy(ctx->inflate_payload->data, pf_ctx->fwd_payload->data, frame_header_size);


    pf_ctx->fwd_payload = ctx->inflate_payload;

    //new ip_frame_size
    pf_ctx->ip_frame_size = inflate_chunk_size;
    //new payload size
    gnb_payload16_set_data_len(pf_ctx->fwd_payload, frame_header_size + pf_ctx->ip_frame_size);
    
    pf_ctx->ip_frame = pf_ctx->fwd_payload->data + frame_header_size;

    return GNB_PF_NEXT;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}


gnb_pf_t gnb_pf_zip = {
    .name          = "gnb_pf_zip",
    .type          = GNB_PF_TYEP_UNSET,
    .private_ctx   = NULL,
    .pf_init       = pf_init_cb,
    .pf_conf       = pf_conf_cb,
    .pf_tun_frame  = NULL,                  // pf_tun_frame
    .pf_tun_route  = pf_tun_route_cb,       // pf_tun_route
    .pf_tun_fwd    = NULL,                  // pf_tun_fwd
    .pf_inet_frame = NULL,                  // pf_inet_frame
    .pf_inet_route = pf_inet_route,         // pf_inet_route
    .pf_inet_fwd   = NULL,                  // pf_inet_fwd
    .pf_release    = pf_release_cb          // pf_release
};
