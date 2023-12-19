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
#include "gnb_payload16.h"
#include "protocol/network_protocol.h"

#ifdef __UNIX_LIKE_OS__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#endif


gnb_pf_t gnb_pf_dump;

static void pf_init_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}

static void pf_conf_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){

}


static int pf_tun_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    char src_ipv4_string[INET_ADDRSTRLEN];
    char dst_ipv4_string[INET_ADDRSTRLEN];

    char src_ipv6_string[INET6_ADDRSTRLEN];
    char dst_ipv6_string[INET6_ADDRSTRLEN];

    struct iphdr   *ip_frame_head  = (struct iphdr*  )(pf_ctx->fwd_payload->data + gnb_core->tun_payload_offset);
    struct ip6_hdr *ip6_frame_head = (struct ip6_hdr*)(pf_ctx->fwd_payload->data + gnb_core->tun_payload_offset);

    static uint64_t seq = 0;

    seq++;

    if ( 0x4 != ip_frame_head->version && 0x6 != ip_frame_head->version ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"dump pf_tun_frame_cb 0x4!=ip_frame_head->version 0x6!=ip_frame_head->version version[%x]\n", ip_frame_head->version);
        return pf_ctx->pf_status;
    }

    if ( 0x4 == ip_frame_head->version ) {

        inet_ntop(AF_INET, &ip_frame_head->saddr, src_ipv4_string, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip_frame_head->daddr, dst_ipv4_string, INET_ADDRSTRLEN);

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"tun seq[%"PRIu64"]%s > %s out:%u\n",seq,
                  src_ipv4_string, dst_ipv4_string,
                  gnb_payload16_size(pf_ctx->fwd_payload));

        return pf_ctx->pf_status;
    }

    uint32_t src_ip_int;
    uint32_t dst_ip_int;

    if ( 0x6 == ip_frame_head->version ) {

        src_ip_int = ip6_frame_head->ip6_src.__in6_u.__u6_addr32[3];
        dst_ip_int = ip6_frame_head->ip6_dst.__in6_u.__u6_addr32[3];

        inet_ntop(AF_INET, &src_ip_int, src_ipv4_string, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &dst_ip_int, dst_ipv4_string, INET_ADDRSTRLEN);

        inet_ntop(AF_INET6, &ip6_frame_head->ip6_src, src_ipv6_string, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &ip6_frame_head->ip6_dst, dst_ipv6_string, INET6_ADDRSTRLEN);

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"tun seq[%"PRIu64"][%s][%s] > [%s][%s] out:%u\n", seq,
                  src_ipv6_string, src_ipv4_string,
                  dst_ipv6_string, dst_ipv4_string,
                  gnb_payload16_size(pf_ctx->fwd_payload));

    }

    return pf_ctx->pf_status;

}


static int pf_tun_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){
    return pf_ctx->pf_status;
}


static int pf_tun_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){
    return pf_ctx->pf_status;;
}


static int pf_inet_frame_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){
    return pf_ctx->pf_status;
}


static int pf_inet_route_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){
    return pf_ctx->pf_status;
}


static int pf_inet_fwd_cb(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx){

    char src_ipv4_string[INET_ADDRSTRLEN];
    char dst_ipv4_string[INET_ADDRSTRLEN];

    char src_ipv6_string[INET6_ADDRSTRLEN];
    char dst_ipv6_string[INET6_ADDRSTRLEN];

    struct iphdr   *ip_frame_head  = (struct iphdr  *)pf_ctx->ip_frame;
    struct ip6_hdr *ip6_frame_head = (struct ip6_hdr*)pf_ctx->ip_frame;

    static uint64_t seq = 0;

    seq++;

    if ( gnb_core->local_node->uuid32 != pf_ctx->dst_uuid32 ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"inet dump[%"PRIu64"] [%u] < [%u] in:%u\n",seq, pf_ctx->dst_uuid32, pf_ctx->src_uuid32, gnb_payload16_size(pf_ctx->fwd_payload));
        return pf_ctx->pf_status;
    }


    if ( 0x4 != ip_frame_head->version && 0x6 != ip_frame_head->version ) {
        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"dump pf_inet_fwd_cb 0x4!=ip_frame_head->version 0x6!=ip_frame_head->version version[%x]\n", ip_frame_head->version);
        return pf_ctx->pf_status;
    }

    if ( 0x4 == ip_frame_head->version ) {

        inet_ntop(AF_INET, &ip_frame_head->saddr, src_ipv4_string, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip_frame_head->daddr, dst_ipv4_string, INET_ADDRSTRLEN);

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"inet dump[%"PRIu64"] [%u]%s < [%u]%s in:%u\n",seq,
                  pf_ctx->dst_uuid32,dst_ipv4_string,
                  pf_ctx->src_uuid32,src_ipv4_string,
                  gnb_payload16_size(pf_ctx->fwd_payload));

        return pf_ctx->pf_status;
    }

    if ( 0x6 == ip_frame_head->version ) {

        inet_ntop(AF_INET6, &ip6_frame_head->ip6_src, src_ipv6_string, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &ip6_frame_head->ip6_dst, dst_ipv6_string, INET6_ADDRSTRLEN);

        GNB_LOG3(gnb_core->log,GNB_LOG_ID_PF,"inet dump[%"PRIu64"] [%u]%s < [%u]%s in:%u\n",seq,
                  pf_ctx->dst_uuid32, dst_ipv6_string,
                  pf_ctx->src_uuid32, src_ipv6_string,
                  gnb_payload16_size(pf_ctx->fwd_payload));

    }

    return pf_ctx->pf_status;

}


static void pf_release_cb(gnb_core_t *gnb_core, gnb_pf_t *pf){


}

gnb_pf_t gnb_pf_dump = {
    "gnb_pf_dump",
    NULL,
    pf_init_cb,
    pf_conf_cb,
    pf_tun_frame_cb,
    pf_tun_route_cb,
    pf_tun_fwd_cb,
    pf_inet_frame_cb,
    pf_inet_route_cb,
    pf_inet_fwd_cb,
    pf_release_cb
};
