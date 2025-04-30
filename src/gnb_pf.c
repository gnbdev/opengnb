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

#include "gnb.h"
#include "gnb_node.h"
#include "gnb_hash32.h"
#include "gnb_pf.h"
#include "gnb_payload16.h"
#include "gnb_unified_forwarding.h"
#include "gnb_binary.h"

void gnb_send_ur0_frame(gnb_core_t *gnb_core, gnb_node_t *dst_node, gnb_payload16_t *payload);


gnb_node_t* gnb_query_route4(gnb_core_t *gnb_core, uint32_t dst_ip_int){

    char ip_string[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &dst_ip_int, ip_string, INET_ADDRSTRLEN);

    gnb_node_t *node=NULL;

    uint32_t dsp_ip_key = dst_ip_int;

    node = GNB_HASH32_UINT32_GET_PTR(gnb_core->ipv4_node_map, dsp_ip_key);

    if ( node ) {
        goto finish;
    }

    dsp_ip_key = dst_ip_int & htonl(IN_CLASSC_NET);
    node = GNB_HASH32_UINT32_GET_PTR(gnb_core->subnetc_node_map, dsp_ip_key);

    if ( node ) {
        goto finish;
    }

    dsp_ip_key = dst_ip_int & htonl(IN_CLASSB_NET);
    node = GNB_HASH32_UINT32_GET_PTR(gnb_core->subnetb_node_map, dsp_ip_key);

    if ( node ) {
        goto finish;
    }

    dsp_ip_key = dst_ip_int & htonl(IN_CLASSA_NET);
    node = GNB_HASH32_UINT32_GET_PTR(gnb_core->subneta_node_map, dsp_ip_key);

    if ( node ) {
        goto finish;
    }

finish:

    return node;

}


#define GNB_PF_TUN_FRAME_INIT        0
#define GNB_PF_TUN_FRAME_ERROR       1
#define GNB_PF_TUN_FRAME_DROP        2
#define GNB_PF_TUN_FRAME_NEXT        3
#define GNB_PF_TUN_FRAME_FINISH      4

#define GNB_PF_TUN_ROUTE_INIT        5
#define GNB_PF_TUN_ROUTE_ERROR       6
#define GNB_PF_TUN_ROUTE_DROP        7
#define GNB_PF_TUN_ROUTE_NOROUTE     8
#define GNB_PF_TUN_ROUTE_NEXT        9
#define GNB_PF_TUN_ROUTE_FINISH     10

#define GNB_PF_TUN_FORWARD_INIT     11
#define GNB_PF_TUN_FORWARD_ERROR    12
#define GNB_PF_TUN_FORWARD_NEXT     13
#define GNB_PF_TUN_FORWARD_FINISH   14


#define GNB_PF_INET_FRAME_INIT      15
#define GNB_PF_INET_FRAME_ERROR     16
#define GNB_PF_INET_FRAME_NOROUTE   17
#define GNB_PF_INET_FRAME_DROP      18
#define GNB_PF_INET_FRAME_NEXT      19
#define GNB_PF_INET_FRAME_FINISH    20


#define GNB_PF_INET_ROUTE_INIT      21
#define GNB_PF_INET_ROUTE_ERROR     22
#define GNB_PF_INET_ROUTE_DROP      23
#define GNB_PF_INET_ROUTE_NOROUTE   24

#define GNB_PF_INET_ROUTE_NEXT      25
#define GNB_PF_INET_ROUTE_FINISH    26

#define GNB_PF_INET_FORWARD_INIT    27
#define GNB_PF_INET_FORWARD_ERROR   28
#define GNB_PF_INET_FORWARD_DROP    29
#define GNB_PF_INET_FORWARD_NEXT    30
#define GNB_PF_INET_FORWARD_FINISH  31
#define GNB_PF_INET_FORWARD_TO_TUN  32
#define GNB_PF_INET_FORWARD_TO_INET 33

static char* gnb_pf_status_strings[34];

void gnb_pf_status_strings_init() {

    gnb_pf_status_strings[GNB_PF_TUN_FRAME_INIT]       = "TUN_FRAME_INIT";
    gnb_pf_status_strings[GNB_PF_TUN_FRAME_ERROR]      = "TUN_FRAME_ERROR";
    gnb_pf_status_strings[GNB_PF_TUN_FRAME_DROP]       = "TUN_FRAME_DROP";
    gnb_pf_status_strings[GNB_PF_TUN_FRAME_NEXT]       = "TUN_FRAME_NEXT";
    gnb_pf_status_strings[GNB_PF_TUN_FRAME_FINISH]     = "TUN_FRAME_FINISH";

    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_INIT]       = "TUN_ROUTE_INIT";
    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_ERROR]      = "TUN_ROUTE_ERROR";
    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_DROP]       = "TUN_ROUTE_DROP";
    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_NOROUTE]    = "TUN_ROUTE_NOROUTE";
    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_NEXT]       = "TUN_ROUTE_NEXT";
    gnb_pf_status_strings[GNB_PF_TUN_ROUTE_FINISH]     = "TUN_ROUTE_FINISH";

    gnb_pf_status_strings[GNB_PF_TUN_FORWARD_INIT]     = "TUN_FORWARD_INIT";
    gnb_pf_status_strings[GNB_PF_TUN_FORWARD_ERROR]    = "TUN_FORWARD_ERROR";
    gnb_pf_status_strings[GNB_PF_TUN_FORWARD_NEXT]     = "TUN_FORWARD_NEXT";
    gnb_pf_status_strings[GNB_PF_TUN_FORWARD_FINISH]   = "TUN_FORWARD_FINISH";

    gnb_pf_status_strings[GNB_PF_INET_FRAME_INIT]      = "INET_FRAME_INIT";
    gnb_pf_status_strings[GNB_PF_INET_FRAME_ERROR]     = "INET_FRAME_ERROR";
    gnb_pf_status_strings[GNB_PF_INET_FRAME_NOROUTE]   = "INET_FRAME_NOROUTE";
    gnb_pf_status_strings[GNB_PF_INET_FRAME_DROP]      = "INET_FRAME_DROP";
    gnb_pf_status_strings[GNB_PF_INET_FRAME_NEXT]      = "INET_FRAME_NEXT";
    gnb_pf_status_strings[GNB_PF_INET_FRAME_FINISH]    = "INET_FRAME_FINISH";

    gnb_pf_status_strings[GNB_PF_INET_ROUTE_INIT]      = "INET_ROUTE_INIT";
    gnb_pf_status_strings[GNB_PF_INET_ROUTE_ERROR]     = "INET_ROUTE_ERROR";
    gnb_pf_status_strings[GNB_PF_INET_ROUTE_DROP]      = "INET_ROUTE_DROP";
    gnb_pf_status_strings[GNB_PF_INET_ROUTE_NOROUTE]   = "INET_ROUTE_NOROUTE";

    gnb_pf_status_strings[GNB_PF_INET_ROUTE_NEXT]      = "INET_ROUTE_NEXT";
    gnb_pf_status_strings[GNB_PF_INET_ROUTE_FINISH]    = "INET_ROUTE_FINISH";

    gnb_pf_status_strings[GNB_PF_INET_FORWARD_INIT]    = "INET_FORWARD_INIT";
    gnb_pf_status_strings[GNB_PF_INET_FORWARD_ERROR]   = "INET_FORWARD_ERROR";
    gnb_pf_status_strings[GNB_PF_INET_FORWARD_DROP]    = "INET_FORWARD_DROP";
    gnb_pf_status_strings[GNB_PF_INET_FORWARD_NEXT]    = "INET_FORWARD_NEXT";
    gnb_pf_status_strings[GNB_PF_INET_FORWARD_TO_TUN]  = "INET_FORWARD_TO_TUN";
    gnb_pf_status_strings[GNB_PF_INET_FORWARD_TO_INET] = "INET_FORWARD_TO_INET";

}


static gnb_pf_t* find_pf_in_array(gnb_pf_array_t *pf_array, const char *pf_name)  {
    
    int i;

    for ( i=0; i<pf_array->num; i++ ) {

        if ( NULL==pf_array->pf[i] ) {
            break;
        }

        if ( 0 == strncmp(pf_array->pf[i]->name, pf_name, 128) ) {
            return pf_array->pf[i];
        }        

    }

    return NULL;

}


static gnb_pf_array_t* gnb_pf_array_init(gnb_heap_t *heap, int size){

    gnb_pf_array_t *pf_array;

    pf_array = (gnb_pf_array_t *)gnb_heap_alloc(heap, sizeof(gnb_pf_array_t) + sizeof(gnb_pf_t)*size);
    pf_array->size = size;
    pf_array->num = 0;

    return pf_array;

}


gnb_pf_core_t* gnb_pf_core_init(gnb_heap_t *heap, int size){
    
    gnb_pf_core_t *pf_core;

    pf_core = gnb_heap_alloc(heap, sizeof(gnb_pf_core_t));

    pf_core->pf_install_array    = gnb_pf_array_init(heap, size);

    pf_core->pf_tun_frame_array  = gnb_pf_array_init(heap, size);
    pf_core->pf_tun_route_array  = gnb_pf_array_init(heap, size);
    pf_core->pf_tun_fwd_array    = gnb_pf_array_init(heap, size);

    pf_core->pf_inet_frame_array = gnb_pf_array_init(heap, size);
    pf_core->pf_inet_route_array = gnb_pf_array_init(heap, size);
    pf_core->pf_inet_fwd_array   = gnb_pf_array_init(heap, size);

    return pf_core;

}


void gnb_pf_core_release(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core) {

    int i;

    for ( i=0; i<pf_core->pf_install_array->num; i++ ) {

        if ( NULL==pf_core->pf_install_array->pf[i]->pf_release ) {
            continue;
        }

        pf_core->pf_install_array->pf[i]->pf_release(gnb_core, pf_core->pf_install_array->pf[i]);

    }

}


void gnb_pf_core_conf(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core){

    gnb_pf_array_t *pf_array;

    pf_array = pf_core->pf_install_array;

    gnb_pf_t *pf_dump;
    gnb_pf_t *pf_route;
    gnb_pf_t *pf_zip;
    gnb_pf_t *pf_crypto;    

    pf_dump   = find_pf_in_array(pf_array, "gnb_pf_dump");
    pf_route  = find_pf_in_array(pf_array, gnb_core->conf->pf_route);



    pf_zip    = find_pf_in_array(pf_array, "gnb_pf_zip");
    pf_crypto = find_pf_in_array(pf_array, "gnb_pf_crypto_xor");

    if ( NULL == pf_crypto ) {
        pf_crypto = find_pf_in_array(pf_array, "gnb_pf_crypto_arc4");
    }

    int idx;

    //pf_tun
    //pf_tun_frame             gnb_pf_dump -> gnb_pf_route
    idx = 0;    
    if ( NULL != pf_dump ) {
        pf_core->pf_tun_frame_array->pf[idx++] = pf_dump;
    }
    pf_core->pf_tun_frame_array->pf[idx++] = pf_route;
    pf_core->pf_tun_frame_array->num = idx;


    //pf_tun_route            gnb_pf_route[+] ->   gnb_pf_zip -> gnb_pf_crypto(p2p)
    idx = 0;
    pf_core->pf_tun_route_array->pf[idx++] = pf_route;
    if ( NULL != pf_zip) {
        pf_core->pf_tun_route_array->pf[idx++] = pf_zip;
    }
    if ( NULL != pf_crypto ) {
        pf_core->pf_tun_route_array->pf[idx++] = pf_crypto; //p2p
    }
    pf_core->pf_tun_route_array->num = idx;


    //pf_tun_fwd      gnb_pf_crypto(relay)
    idx = 0;
    if ( NULL != pf_crypto ) {
        pf_core->pf_tun_fwd_array->pf[idx++] = pf_crypto; //relay
    }
    pf_core->pf_tun_fwd_array->num = idx;


    //pf_inet
    //pf_inet_frame   gnb_pf_crypto(relay) -> gnb_pf_route
    idx = 0;
    if ( NULL != pf_crypto ) {
        pf_core->pf_inet_frame_array->pf[idx++] = pf_crypto; //relay
    }    
    pf_core->pf_inet_frame_array->pf[idx++] = pf_route;
    pf_core->pf_inet_frame_array->num = idx;

    //pf_inet_route    gnb_pf_route -> gnb_pf_crypto(p2p) -> gnb_pf_zip
    idx = 0;
    pf_core->pf_inet_route_array->pf[idx++] = pf_route;
    if ( NULL != pf_crypto ) {
        pf_core->pf_inet_route_array->pf[idx++] = pf_crypto; //p2p
    }
    if ( NULL != pf_zip ) {
        pf_core->pf_inet_route_array->pf[idx++] = pf_zip;
    }
    pf_core->pf_inet_route_array->num   = idx;

    //pf_inet_fwd              gnb_pf_dump -> gnb_pf_route -> gnb_pf_crypto(relay)
    idx = 0;
    if ( NULL != pf_dump ) {
        pf_core->pf_inet_fwd_array->pf[idx++] = pf_dump;
    }    
    pf_core->pf_inet_fwd_array->pf[idx++] = pf_route;
    if ( NULL != pf_crypto ) {
        pf_core->pf_inet_fwd_array->pf[idx++] = pf_crypto; //relay
    }
    pf_core->pf_inet_fwd_array->num = idx;

    return;

}


int gnb_pf_install(gnb_pf_array_t *pf_array, gnb_pf_t *pf){

    if ( pf_array->num >= pf_array->size ) {
        return -1;
    }

    pf_array->pf[pf_array->num] = pf;
    pf_array->num++;

    return 0;

}


void gnb_pf_init(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array){

    int i;

    for ( i=0; i<pf_array->num; i++ ) {

        if ( NULL==pf_array->pf[i]->pf_init ) {
            continue;
        }

        pf_array->pf[i]->pf_init(gnb_core,pf_array->pf[i]);
    }

}


void gnb_pf_conf(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array){

    int i;

    for ( i=0; i<pf_array->num; i++ ) {

        if ( NULL==pf_array->pf[i]->pf_conf ) {
            continue;
        }

        pf_array->pf[i]->pf_conf(gnb_core,pf_array->pf[i]);

    }

}


/*
把输入的 payload 加上offset，这样pf模块处理的时候，就可以在offset之前填充pf的头部，减少一次通过 memcpy 重组payload
*/
void gnb_pf_tun(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core, gnb_payload16_t *payload){

    int i;
    int ret;

    gnb_pf_ctx_t pf_ctx_st;

    gnb_pf_array_t *pf_tun_frame_array = pf_core->pf_tun_frame_array;
    gnb_pf_array_t *pf_tun_route_array = pf_core->pf_tun_route_array;
    gnb_pf_array_t *pf_tun_fwd_array   = pf_core->pf_tun_fwd_array;


    memset(&pf_ctx_st,0,sizeof(gnb_pf_ctx_t));

    pf_ctx_st.pf_fwd = GNB_PF_FWD_INIT;

    pf_ctx_st.fwd_payload = payload;

    pf_ctx_st.fwd_payload->type = GNB_PAYLOAD_TYPE_IPFRAME;
    pf_ctx_st.fwd_payload->sub_type = GNB_PAYLOAD_SUB_TYPE_IPFRAME_INIT;


    int pf_tun_frame_status   = GNB_PF_TUN_FRAME_INIT;
    int pf_tun_route_status   = GNB_PF_TUN_ROUTE_INIT;
    int pf_tun_forward_status = GNB_PF_TUN_FORWARD_INIT;

    gnb_uuid_t fwd_uuid64 = 0;
    gnb_core->select_fwd_node = gnb_select_forward_node(gnb_core);

    pf_ctx_st.pf_status = GNB_PF_TUN_FRAME_INIT;

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "----- GNB PF TUN BEGIN -----\n");
    }

    for ( i=0; i<pf_tun_frame_array->num; i++ ) {

        if ( NULL==pf_tun_frame_array->pf[i]->pf_tun_frame ) {
            continue;
        }

        pf_ctx_st.pf_status = pf_tun_frame_array->pf[i]->pf_tun_frame(gnb_core, pf_tun_frame_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_ERROR:
                pf_tun_frame_status = GNB_PF_TUN_FRAME_ERROR;
                break;
            case GNB_PF_DROP:
                pf_tun_frame_status = GNB_PF_TUN_FRAME_DROP;
                break;
            case GNB_PF_NEXT:
                pf_tun_frame_status = GNB_PF_TUN_FRAME_NEXT;
                break;
            case GNB_PF_FINISH:
                pf_tun_frame_status = GNB_PF_TUN_FRAME_FINISH;
                break;
            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">>> tun payload pf_tun_frame:[%s] => %s\n", pf_tun_frame_array->pf[i]->name, gnb_pf_status_strings[pf_tun_frame_status]);

        if ( GNB_PF_FINISH == pf_ctx_st.pf_status ) {
            break;
        }

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status || GNB_PF_DROP == pf_ctx_st.pf_status ) {
            goto pf_tun_finish;
        }        

    }

    pf_ctx_st.pf_status = GNB_PF_TUN_ROUTE_INIT;

    for ( i=0; i<pf_tun_route_array->num; i++ ) {

        if ( NULL == pf_tun_route_array->pf[i]->pf_tun_route ) {
            continue;
        }

        pf_ctx_st.pf_status = pf_tun_route_array->pf[i]->pf_tun_route(gnb_core, pf_tun_route_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_ERROR:
                pf_tun_route_status = GNB_PF_TUN_ROUTE_ERROR;
                break;
            case GNB_PF_NOROUTE:
                pf_tun_route_status = GNB_PF_TUN_ROUTE_NOROUTE;
                break;
            case GNB_PF_DROP:
                pf_tun_route_status = GNB_PF_TUN_ROUTE_DROP;
                break;
            case GNB_PF_NEXT:
                pf_tun_route_status = GNB_PF_TUN_ROUTE_NEXT;
                break;
            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">>> tun payload pf_tun_route:[%s] %s\n", pf_tun_route_array->pf[i]->name, gnb_pf_status_strings[pf_tun_route_status]);

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status ) {
            goto pf_tun_finish;
        }

        
    }

    /*
     * 一般的,forwarding 的优先级是 relay_forwarding > unified_forwarding > direct_forwarding > std_forwarding
     * 以下条件跳转是确保 relay_forwarding > unified_forwarding
     * */
    if ( NULL != pf_ctx_st.fwd_node && 1 == pf_ctx_st.relay_forwarding ) {
        goto pf_tun_fwd;
    }


    if ( GNB_UNIFIED_FORWARDING_OFF == gnb_core->conf->unified_forwarding ) {

        goto skip_unified_forwarding;
    }

    if ( GNB_UNIFIED_FORWARDING_FORCE == gnb_core->conf->unified_forwarding ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding Force src=%llu dst=%llu\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64);

        ret = gnb_unified_forwarding_tun(gnb_core, &pf_ctx_st);

        if ( ret > 0 ) {
            pf_ctx_st.unified_forwarding = 1;
        } else {
            pf_ctx_st.unified_forwarding = 0;
        }

        goto pf_tun_finish;

    }

    if ( NULL == pf_ctx_st.fwd_node && GNB_UNIFIED_FORWARDING_AUTO == gnb_core->conf->unified_forwarding ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding Auto src=%llu dst=%llu\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64);

        ret = gnb_unified_forwarding_tun(gnb_core, &pf_ctx_st);

        if ( ret > 0 ) {
            pf_ctx_st.unified_forwarding = 1;
        } else {
            pf_ctx_st.unified_forwarding = 0;
        }

        goto pf_tun_finish;

    }

    if ( GNB_UNIFIED_FORWARDING_SUPER == gnb_core->conf->unified_forwarding || GNB_UNIFIED_FORWARDING_HYPER == gnb_core->conf->unified_forwarding ) {

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "Unified Forwarding Multi-Path src=%llu dst=%llu\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64);

        ret = gnb_unified_forwarding_with_multi_path_tun(gnb_core, &pf_ctx_st);

        if ( 0 == ret ) {
            pf_ctx_st.unified_forwarding = 1;
        } else {
            pf_ctx_st.unified_forwarding = 0;
        }

        goto pf_tun_finish;

    }


skip_unified_forwarding:

    if ( NULL == pf_ctx_st.fwd_node ) {
        goto pf_tun_finish;
    }

pf_tun_fwd:

    pf_ctx_st.pf_status = GNB_PF_TUN_FORWARD_INIT;

    for ( i=0; i < pf_tun_fwd_array->num;  i++ ) {

        if ( NULL==pf_tun_fwd_array->pf[i]->pf_tun_fwd ) {
            continue;
        }

        pf_ctx_st.pf_status = pf_tun_fwd_array->pf[i]->pf_tun_fwd(gnb_core, pf_tun_fwd_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_ERROR:
                pf_tun_forward_status = GNB_PF_TUN_FORWARD_ERROR;
                break;
            case GNB_PF_NEXT:
                pf_tun_forward_status = GNB_PF_TUN_FORWARD_NEXT;
                break;
            case GNB_PF_FINISH:
                pf_tun_forward_status = GNB_PF_TUN_FORWARD_FINISH;
                break;
            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">>> tun payload pf_tun_fwd:[%s] %s\n", pf_tun_fwd_array->pf[i]->name, gnb_pf_status_strings[pf_tun_forward_status]);

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status ) {
            goto pf_tun_finish;
        }

        if ( GNB_PF_FINISH == pf_ctx_st.pf_status ) {
            break;
        }

    }


    gnb_p2p_forward_payload_to_node(gnb_core, pf_ctx_st.fwd_node, pf_ctx_st.fwd_payload);


    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "payload frome TUN to INET node=%llu [%s]\n", pf_ctx_st.fwd_node->uuid64, GNB_HEX2_BYTE256((void *)pf_ctx_st.fwd_payload) );
    }


    pf_ctx_st.fwd_node->in_bytes     += pf_ctx_st.ip_frame_size;
    gnb_core->local_node->out_bytes  += pf_ctx_st.ip_frame_size;

    if ( pf_ctx_st.dst_uuid64 == pf_ctx_st.fwd_node->uuid64 ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, ">>> tun payload forward to inet src=%llu dst=%llu >>>\n", pf_ctx_st.src_uuid64, pf_ctx_st.fwd_node->uuid64);
    } else {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "*>> tun payload forward to inet src=%llu dst=%llu fwd=%llu *>>\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64, pf_ctx_st.fwd_node->uuid64);
    }


pf_tun_finish:

    if ( 0 == pf_ctx_st.unified_forwarding && NULL != pf_ctx_st.dst_node && NULL == pf_ctx_st.fwd_node && 1 == pf_ctx_st.universal_udp4_relay ) {
        gnb_send_ur0_frame(gnb_core, pf_ctx_st.dst_node, pf_ctx_st.fwd_payload);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "tun try to universal relay src=%llu dst=%llu\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64);
    }


if_dump:

    if ( 1 == gnb_core->conf->if_dump ) {

        fwd_uuid64 = NULL != pf_ctx_st.fwd_node ? pf_ctx_st.fwd_node->uuid64:0;

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "tun src=%llu dst=%llu fwd=%llu [%s] [%s] relay=%d,unified=%d,direct=%d,forward=%d ip_frame_size=%u\n",
               pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64, fwd_uuid64,
               gnb_pf_status_strings[pf_tun_frame_status], gnb_pf_status_strings[pf_tun_route_status],
               pf_ctx_st.relay_forwarding, pf_ctx_st.unified_forwarding, pf_ctx_st.direct_forwarding, pf_ctx_st.std_forwarding,
               pf_ctx_st.ip_frame_size);

    }

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "----- GNB PF TUN   END -----\n");
    }

finish:

    return;

}


void gnb_pf_inet(gnb_core_t *gnb_core, gnb_pf_core_t *pf_core, gnb_payload16_t *payload, gnb_sockaddress_t *source_node_addr){

    gnb_pf_ctx_t pf_ctx_st;

    gnb_pf_array_t *pf_inet_frame_array = pf_core->pf_inet_frame_array;
    gnb_pf_array_t *pf_inet_route_array = pf_core->pf_inet_route_array;
    gnb_pf_array_t *pf_inet_fwd_array   = pf_core->pf_inet_fwd_array;


    memset(&pf_ctx_st,0,sizeof(gnb_pf_ctx_t));

    pf_ctx_st.pf_fwd = GNB_PF_FWD_INIT;
    pf_ctx_st.fwd_payload = payload;
    pf_ctx_st.source_node_addr = source_node_addr;

    int i;

    int pf_inet_frame_status  = GNB_PF_INET_FRAME_INIT;
    int pf_inet_route_status  = GNB_PF_INET_ROUTE_INIT;
    int pf_inet_forwad_status = GNB_PF_INET_FORWARD_INIT;

    int ret;

    gnb_uuid_t fwd_uuid64 = 0;

    gnb_core->select_fwd_node = gnb_select_forward_node(gnb_core);

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "----- GNB PF INET BEGIN -----\n");
    }


    if ( GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED == payload->sub_type ) {

        //如果 ip分组 不是转发到本节点，就转发到目的节点
        ret = gnb_unified_forwarding_inet(gnb_core, payload);

        if ( UNIFIED_FORWARDING_TO_TUN != ret ) {
            goto pf_inet_finish;
        }

    }


    if ( GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED_MULTI_PATH == payload->sub_type ) {

        //如果 ip分组 不是转发到本节点，就转发到目的节点
        ret = gnb_unified_forwarding_multi_path_inet(gnb_core, payload);

        if ( UNIFIED_FORWARDING_TO_TUN != ret ) {
            goto pf_inet_finish;
        }

    }


    for ( i=0; i<pf_inet_frame_array->num; i++ ) {

        if ( NULL == pf_inet_frame_array->pf[i]->pf_inet_frame ) {
            continue;
        }


        pf_ctx_st.pf_status = pf_inet_frame_array->pf[i]->pf_inet_frame(gnb_core, pf_inet_frame_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_NOROUTE:
                pf_inet_frame_status = GNB_PF_INET_FRAME_NOROUTE;
                break;
            case GNB_PF_ERROR:
                pf_inet_frame_status = GNB_PF_INET_FRAME_ERROR;
                break;
            case GNB_PF_DROP:
                pf_inet_frame_status = GNB_PF_INET_FRAME_DROP;
                break;
            case GNB_PF_FINISH:
                pf_inet_frame_status = GNB_PF_INET_FRAME_FINISH;
                break;

            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "<<< inet payload pf_inet_frame:[%s] %s\n", pf_inet_frame_array->pf[i]->name, gnb_pf_status_strings[pf_inet_frame_status]);

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status || GNB_PF_DROP == pf_ctx_st.pf_status ) {
            goto pf_inet_finish;
        }

        if ( GNB_PF_FINISH == pf_ctx_st.pf_status ) {
            break;
        }

    }


    for ( i=0; i<pf_inet_route_array->num; i++ ) {

        if ( NULL == pf_inet_route_array->pf[i]->pf_inet_route ) {
            continue;
        }

        pf_ctx_st.pf_status = pf_inet_route_array->pf[i]->pf_inet_route(gnb_core, pf_inet_route_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_ERROR:
                pf_inet_route_status = GNB_PF_INET_ROUTE_ERROR;
                break;
            case GNB_PF_DROP:
                pf_inet_route_status = GNB_PF_INET_ROUTE_DROP;
                break;
            case GNB_PF_NOROUTE:
                pf_inet_route_status = GNB_PF_INET_ROUTE_NOROUTE;
                break;
            case GNB_PF_NEXT:
                pf_inet_route_status = GNB_PF_INET_ROUTE_NEXT;
                break;
            case GNB_PF_FINISH:
                pf_inet_route_status = GNB_PF_INET_ROUTE_FINISH;
                break;
            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "<<< inet payload pf_inet_route:[%s] %s\n", pf_inet_route_array->pf[i]->name, gnb_pf_status_strings[pf_inet_route_status]);

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status || GNB_PF_DROP == pf_ctx_st.pf_status || GNB_PF_NOROUTE == pf_ctx_st.pf_status ) {
            goto pf_inet_finish;
        }

        if ( GNB_PF_FINISH == pf_ctx_st.pf_status ) {
            break;
        }

    }



    for ( i=0; i<pf_inet_fwd_array->num; i++ ) {

        if ( NULL == pf_inet_fwd_array->pf[i]->pf_inet_fwd ) {
            continue;
        }

        pf_ctx_st.pf_status = pf_inet_fwd_array->pf[i]->pf_inet_fwd(gnb_core, pf_inet_fwd_array->pf[i], &pf_ctx_st);

        switch (pf_ctx_st.pf_status) {

            case GNB_PF_ERROR:
                pf_inet_forwad_status = GNB_PF_INET_FORWARD_ERROR;
                break;
            case GNB_PF_DROP:
                pf_inet_forwad_status = GNB_PF_INET_FORWARD_DROP;
                break;
            case GNB_PF_NEXT:
                pf_inet_forwad_status = GNB_PF_INET_FORWARD_NEXT;
                break;
            case GNB_PF_FINISH:
                pf_inet_forwad_status = GNB_PF_INET_FORWARD_FINISH;
                break;
            default:
                break;

        }

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "<<< inet payload pf_inet_fwd:[%s] %s\n", pf_inet_fwd_array->pf[i]->name, gnb_pf_status_strings[pf_inet_forwad_status]);

        if ( GNB_PF_ERROR == pf_ctx_st.pf_status || GNB_PF_DROP == pf_ctx_st.pf_status ) {
            goto pf_inet_finish;
        }

        if ( GNB_PF_FINISH == pf_ctx_st.pf_status ) {            
            break;
        }


    }

    fwd_uuid64 = NULL!=pf_ctx_st.fwd_node ? pf_ctx_st.fwd_node->uuid64:0;

    if ( NULL == pf_ctx_st.src_node ) {
        goto pf_inet_finish;
    }

    if ( gnb_core->conf->activate_tun && GNB_PF_FWD_TUN == pf_ctx_st.pf_fwd ) {

        gnb_core->drv->write_tun(gnb_core, pf_ctx_st.ip_frame, pf_ctx_st.ip_frame_size);

        if ( 1 == gnb_core->conf->if_dump ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "payload frome INET to TUN src node=%llu [%s]\n", pf_ctx_st.src_node->uuid64, GNB_HEX2_BYTE256((void *)pf_ctx_st.fwd_payload) );
        }


        fwd_uuid64 = pf_ctx_st.dst_uuid64;

        pf_inet_forwad_status = GNB_PF_INET_FORWARD_TO_TUN;

        gnb_core->local_node->in_bytes += pf_ctx_st.ip_frame_size;
        pf_ctx_st.src_node->out_bytes  += pf_ctx_st.ip_frame_size;

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "<<< inet payload forward to tun src=%llu dst=%llu <<<\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64);

        goto pf_inet_finish;

    }

    if ( GNB_PF_FWD_INET == pf_ctx_st.pf_fwd && NULL != pf_ctx_st.fwd_node && NULL != pf_ctx_st.fwd_payload ) {

        gnb_p2p_forward_payload_to_node(gnb_core, pf_ctx_st.fwd_node, pf_ctx_st.fwd_payload);

        if ( 1 == gnb_core->conf->if_dump ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "payload frome INET to INET dst node=%llu [%s]\n", pf_ctx_st.fwd_node->uuid64, GNB_HEX2_BYTE256((void *)pf_ctx_st.fwd_payload) );
        }

        pf_inet_forwad_status = GNB_PF_INET_FORWARD_TO_INET;

        gnb_core->local_node->out_bytes += pf_ctx_st.ip_frame_size;
        pf_ctx_st.fwd_node->in_bytes    += pf_ctx_st.ip_frame_size;

        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "<*< inet payload forward to inet src=%llu dst=%llu fwd=%llu >*>\n", pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64, fwd_uuid64);

    }

pf_inet_finish:

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "inet src=%llu dst=%llu fwd=%llu [%s] [%s] [%s] ip_frame_size=%u\n",
                   pf_ctx_st.src_uuid64, pf_ctx_st.dst_uuid64, fwd_uuid64,
                   gnb_pf_status_strings[pf_inet_frame_status], gnb_pf_status_strings[pf_inet_route_status], gnb_pf_status_strings[pf_inet_forwad_status],
                   pf_ctx_st.ip_frame_size);
    }

    if ( 1 == gnb_core->conf->if_dump ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF,"----- GNB PF INET END -----\n");
    }

    return;

}
