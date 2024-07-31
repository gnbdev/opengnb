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

#include <stdio.h>

#include "gnb_es_type.h"

#ifdef __UNIX_LIKE_OS__
#include <net/if.h>       /* for ifconf */
#include <netinet/in.h>   /* for sockaddr_in */
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <pthread.h>
#endif

#include "gnb_udp.h"
#include "gnb_address.h"
#include "gnb_node_type.h"
#include "gnb_payload16.h"
#include "gnb_discover_in_lan_frame_type.h"
#include "gnb_pingpong_frame_type.h"
#include "gnb_worker_type.h"

#include "ed25519/ed25519.h"

typedef struct _discover_in_lan_worker_ctx_t {

    gnb_es_ctx *es_ctx;

    pthread_t thread_worker;

}discover_in_lan_worker_ctx_t;


static void send_broadcast4(gnb_es_ctx *es_ctx, struct sockaddr_in *src_address_in, struct sockaddr_in *if_broadcast_address){

    char payload_buffer[ PAYLOAD_DISCOVER_LAN_IN_FRAME_PAYLOAD_SIZE ];
    gnb_payload16_t *payload;
    gnb_node_t *local_node;

    struct sockaddr_in broadcast_address_st;
    int broadcast_socket;

    int ret;
    int on;

#if 0
    local_node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(es_ctx->uuid_node_map, es_ctx->ctl_block->core_zone->local_uuid);

    if ( NULL == local_node ) {
        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "send broadcast4 error local node=%llu\n", es_ctx->ctl_block->core_zone->local_uuid);
        return;
    }
#endif

    local_node = es_ctx->local_node;

    payload = (gnb_payload16_t *)payload_buffer;

    payload->type = GNB_PAYLOAD_TYPE_LAN_DISCOVER;
    payload->sub_type = 0;
    gnb_payload16_set_data_len( payload, sizeof(discover_lan_in_frame_t) );

    discover_lan_in_frame_t *discover_lan_in_frame = (discover_lan_in_frame_t *)payload->data;

    memset(discover_lan_in_frame, 0, sizeof(discover_lan_in_frame_t));
    memcpy(discover_lan_in_frame->data.src_key512, local_node->key512, 64);
    discover_lan_in_frame->data.src_uuid64 = gnb_htonll(local_node->uuid64);
    memcpy(discover_lan_in_frame->data.src_addr4, &src_address_in->sin_addr.s_addr, sizeof(struct in_addr));
    discover_lan_in_frame->data.src_port4 = htons(es_ctx->ctl_block->conf_zone->conf_st.udp4_ports[0]);
    discover_lan_in_frame->data.src_ts_usec = gnb_htonll(es_ctx->now_time_usec);

    if ( NULL != if_broadcast_address ) {
        memcpy(&broadcast_address_st, if_broadcast_address, sizeof(struct sockaddr_in));
        broadcast_address_st.sin_family = AF_INET;
        broadcast_address_st.sin_port = htons(DISCOVER_LAN_IN_BROADCAST_PORT);
    } else {
        memset(&broadcast_address_st, 0, sizeof(struct sockaddr_in));
        broadcast_address_st.sin_family = AF_INET;
        broadcast_address_st.sin_port = htons(DISCOVER_LAN_IN_BROADCAST_PORT);
        broadcast_address_st.sin_addr.s_addr = INADDR_BROADCAST;
    }

    snprintf(discover_lan_in_frame->data.text, 256, "GNB LAN DISCOVER node=%llu address=%s,port=%d,broadcast_address=%s",
                                                    local_node->uuid64,
                                                    gnb_get_address4string(&src_address_in->sin_addr.s_addr,        gnb_static_ip_port_string_buffer1, 0),
                                                    es_ctx->ctl_block->conf_zone->conf_st.udp4_ports[0],
                                                    gnb_get_address4string(&broadcast_address_st.sin_addr.s_addr,   gnb_static_ip_port_string_buffer2, 0) );

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);

    on = 1;

    ret = setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on));

    ret = bind(broadcast_socket, (struct sockaddr *)src_address_in, sizeof(struct sockaddr_in));

    ret = sendto(broadcast_socket, (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr*)&broadcast_address_st, sizeof(struct sockaddr_in));

    GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "send broadcast4 %s\n", discover_lan_in_frame->data.text);

#ifdef __UNIX_LIKE_OS__
    close(broadcast_socket);
#endif

#ifdef _WIN32
    closesocket(broadcast_socket);
#endif

}


static void handle_discover_lan_in_frame(gnb_es_ctx *es_ctx, gnb_payload16_t *in_payload, gnb_sockaddress_t *node_addr){

    gnb_uuid_t in_src_uuid64;
    gnb_node_t *local_node;
    gnb_node_t *dst_node;

    uint16_t src_port4;

    discover_lan_in_frame_t *discover_lan_in_frame;
    node_ping_frame_t *node_ping_frame;

    gnb_payload16_t *payload;
    unsigned char payload_buffer[ NODE_PING_FRAME_PAYLOAD_SIZE ];

    struct sockaddr_in udp_sockaddr4;

    if ( GNB_PAYLOAD_TYPE_LAN_DISCOVER != in_payload->type ) {
        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame error in payload type=%d GNB_PAYLOAD_TYPE_LAN_DISCOVER=%d\n", in_payload->type, GNB_PAYLOAD_TYPE_LAN_DISCOVER);
        return;
    }

    local_node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(es_ctx->uuid_node_map, es_ctx->ctl_block->core_zone->local_uuid);

    if ( NULL == local_node ) {
        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame error local_uuid=%llu\n", es_ctx->ctl_block->core_zone->local_uuid);
        return;
    }

    discover_lan_in_frame = (discover_lan_in_frame_t *)in_payload->data;

    in_src_uuid64 = gnb_ntohll(discover_lan_in_frame->data.src_uuid64);

    GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame src_node[%llu]\n", in_src_uuid64);

    if ( in_src_uuid64 == local_node->uuid64 ) {
        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame error in_src_uuid64=%llu local_node->uuid64=%llu\n", in_src_uuid64, local_node->uuid64);
        return;
    }

    dst_node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(es_ctx->uuid_node_map, in_src_uuid64);

    if ( NULL == dst_node ) {
        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame error dst node[%llu] not found!\n", in_src_uuid64);
        return;
    }

    payload = (gnb_payload16_t *)payload_buffer;

    payload->type     = GNB_PAYLOAD_TYPE_NODE;
    payload->sub_type = PAYLOAD_SUB_TYPE_LAN_PING;
    gnb_payload16_set_data_len( payload, sizeof(node_ping_frame_t) );

    node_ping_frame = (node_ping_frame_t *)payload->data;
    memset(node_ping_frame, 0, sizeof(node_ping_frame_t));

    node_ping_frame->data.src_uuid64 = gnb_htonll(local_node->uuid64);
    node_ping_frame->data.dst_uuid64 = discover_lan_in_frame->data.src_uuid64;
    node_ping_frame->data.src_ts_usec = gnb_htonll(es_ctx->now_time_usec);

    src_port4 = htons(es_ctx->ctl_block->conf_zone->conf_st.udp4_ports[0]);
    memcpy(node_ping_frame->data.attachment, &src_port4, sizeof(uint16_t));

    snprintf((char *)node_ping_frame->data.text, 32, "(LAN)%llu --PING-> %llu", local_node->uuid64, dst_node->uuid64);

    if ( 0 == es_ctx->ctl_block->conf_zone->conf_st.lite_mode ) {
        ed25519_sign(node_ping_frame->src_sign, (const unsigned char *)&node_ping_frame->data, sizeof(struct ping_frame_data), es_ctx->ctl_block->core_zone->ed25519_public_key, es_ctx->ctl_block->core_zone->ed25519_private_key);
    }

    memset(&udp_sockaddr4, 0, sizeof(struct sockaddr_in));

    udp_sockaddr4.sin_family = AF_INET;
    memcpy(&udp_sockaddr4.sin_addr.s_addr, &discover_lan_in_frame->data.src_addr4, 4);
    udp_sockaddr4.sin_port = discover_lan_in_frame->data.src_port4;

    sendto(es_ctx->udp_socket4, (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&udp_sockaddr4, sizeof(struct sockaddr_in));

    GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "handle_discover_lan_in_frame send %s\n", node_ping_frame->data.text);

    return;

}


#ifdef _WIN32

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

void gnb_discover_in_lan_ipv4(gnb_es_ctx *es_ctx){

    int i;

    ULONG family = AF_INET;

    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_INCLUDE_GATEWAYS;

    outBufLen = WORKING_BUFFER_SIZE;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;

    PIP_ADAPTER_DNS_SERVER_ADDRESS pDnServer = NULL;

    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

    struct sockaddr_in *sa_in;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);

        if (pAddresses == NULL) {
            GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal != NO_ERROR) {
        return;
    }

    pCurrAddresses = pAddresses;

    char friendlyname_string[256];
    char description_string[256];

    while (pCurrAddresses) {

        if ( IF_TYPE_ETHERNET_CSMACD != pCurrAddresses->IfType ) {
            goto next;
        }

        if ( IfOperStatusUp != pCurrAddresses->OperStatus ) {
            goto next;
        }

        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,pCurrAddresses->Description,-1,description_string,256,NULL,NULL);
        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,pCurrAddresses->FriendlyName,-1,friendlyname_string,256,NULL,NULL);

        if ( 0==strncmp( description_string, "TAP-Windows Adapter V9", sizeof("TAP-Windows Adapter V9")-1 ) ) {
            goto next;
        }

        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "IfIndex[%u] Adapter name: [%s] Adapter Description[%s]\n", pCurrAddresses->IfIndex, pCurrAddresses->AdapterName, description_string);

        pUnicast = pCurrAddresses->FirstUnicastAddress;

        if (NULL==pUnicast) {
            goto next;
        }

        for ( i=0; pUnicast!=NULL; i++ ) {

            if ( pUnicast->Address.lpSockaddr->sa_family == AF_INET ) {
                sa_in = (struct sockaddr_in *)pUnicast->Address.lpSockaddr;
                send_broadcast4(es_ctx, sa_in, NULL);
            }

            pUnicast = pUnicast->Next;

        }

        next:

        pCurrAddresses = pCurrAddresses->Next;

    }

}

#endif


#ifdef __UNIX_LIKE_OS__

void gnb_discover_in_lan_ipv4(gnb_es_ctx *es_ctx){

    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;

    struct sockaddr_in *in;
    struct sockaddr_in *broadcast_in;

    int ret;
    int n;

    ret = getifaddrs(&ifaddr);

    if ( -1 == ret ) {
        return;
    }

    for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next ) {

        if ( ifa->ifa_addr == NULL ) {
            continue;
        }

        if ( AF_INET != ifa->ifa_addr->sa_family ) {
            continue;
        }

        if ( IFF_LOOPBACK & ifa->ifa_flags ) {
            continue;
        }

        if ( IFF_POINTOPOINT & ifa->ifa_flags ) {
            continue;
        }

        if ( !(IFF_UP & ifa->ifa_flags) ) {
            continue;
        }

        if ( !(IFF_RUNNING & ifa->ifa_flags) ) {
            continue;
        }

        if ( !(IFF_BROADCAST & ifa->ifa_flags) ) {
            continue;
        }

        in = (struct sockaddr_in *)ifa->ifa_addr;

        broadcast_in = (struct sockaddr_in *)ifa->ifa_broadaddr;


        GNB_LOG1(es_ctx->log, GNB_LOG_ID_ES_DISCOVER_IN_LAN, "ifa->ifa_name[%s] [%s] broadcast[%s]\n", ifa->ifa_name,
                gnb_get_address4string(&in->sin_addr.s_addr,           gnb_static_ip_port_string_buffer1, 0),
                gnb_get_address4string(&broadcast_in->sin_addr.s_addr, gnb_static_ip_port_string_buffer2, 0) );


        send_broadcast4(es_ctx, in, broadcast_in);

    }

    freeifaddrs(ifaddr);

}

#endif


void gnb_discover_in_lan_service(gnb_es_ctx *es_ctx){

    char buffer[ PAYLOAD_DISCOVER_LAN_IN_FRAME_PAYLOAD_SIZE ];
    gnb_payload16_t *payload;

    int n_ready;
    int maxfd = 0;

    struct timeval timeout;

    fd_set readfds;
    fd_set allset;

    ssize_t n_recv;

    gnb_sockaddress_t node_addr_st;

    payload = (gnb_payload16_t *)buffer;

    FD_ZERO(&readfds);
    FD_ZERO(&allset);

    FD_SET(es_ctx->udp_discover_recv_socket4, &allset);

    maxfd = es_ctx->udp_discover_recv_socket4;

    readfds = allset;

    timeout.tv_sec  = 0l;
    timeout.tv_usec = 10000l;

    n_ready = select( maxfd + 1, &readfds, NULL, NULL, &timeout );

    if ( -1==n_ready ) {
        return;
    }

    if ( FD_ISSET( es_ctx->udp_discover_recv_socket4, &readfds ) ) {

        node_addr_st.socklen = sizeof(struct sockaddr_in);

        n_recv = recvfrom(es_ctx->udp_discover_recv_socket4, (void *)payload, PAYLOAD_DISCOVER_LAN_IN_FRAME_PAYLOAD_SIZE, 0, (struct sockaddr *)&node_addr_st.addr.in, &node_addr_st.socklen);

        if ( -1 == n_recv ) {
            return;
        }

        if ( PAYLOAD_DISCOVER_LAN_IN_FRAME_PAYLOAD_SIZE != n_recv ) {
            return;
        }

        node_addr_st.addr_type = AF_INET;

        handle_discover_lan_in_frame(es_ctx, payload, &node_addr_st);

    }

}


static void* thread_worker_func( void *data ) {

    gnb_worker_t   *discover_in_lan_worker = (gnb_worker_t *)data;
    discover_in_lan_worker_ctx_t *discover_in_lan_worker_ctx = (discover_in_lan_worker_ctx_t *)discover_in_lan_worker->ctx;
    gnb_es_ctx *es_ctx = discover_in_lan_worker_ctx->es_ctx;

    discover_in_lan_worker->thread_worker_flag     = 1;
    discover_in_lan_worker->thread_worker_run_flag = 1;

    do{

        gnb_discover_in_lan_service(es_ctx);

    }while(discover_in_lan_worker->thread_worker_flag);

    discover_in_lan_worker->thread_worker_run_flag = 0;

    return NULL;

}


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_es_ctx *es_ctx;
    discover_in_lan_worker_ctx_t *discover_in_lan_worker_ctx;
    int on;
    int ret;
    struct sockaddr_in svr_addr;

    discover_in_lan_worker_ctx =  (discover_in_lan_worker_ctx_t *)malloc(sizeof(discover_in_lan_worker_ctx_t));

    memset(discover_in_lan_worker_ctx, 0, sizeof(discover_in_lan_worker_ctx_t));

    es_ctx = (gnb_es_ctx *)ctx;
    discover_in_lan_worker_ctx->es_ctx = es_ctx;
    es_ctx->udp_discover_recv_socket4 = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&svr_addr, 0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    svr_addr.sin_port = htons(DISCOVER_LAN_IN_BROADCAST_PORT);

    ret = bind(es_ctx->udp_discover_recv_socket4, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in));

    on = 1;
    ret = setsockopt(es_ctx->udp_discover_recv_socket4, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on));

    gnb_worker->ctx = discover_in_lan_worker_ctx;

}


static void release(gnb_worker_t *gnb_worker){


}


static int start(gnb_worker_t *gnb_worker){

    discover_in_lan_worker_ctx_t *discover_in_lan_worker_ctx = (discover_in_lan_worker_ctx_t *)gnb_worker->ctx;

    pthread_create(&discover_in_lan_worker_ctx->thread_worker, NULL, thread_worker_func, gnb_worker);
    pthread_detach(discover_in_lan_worker_ctx->thread_worker);

    return 0;
}


static int stop(gnb_worker_t *gnb_worker){

    return 0;

}


static int notify(gnb_worker_t *gnb_worker){

    return 0;

}


gnb_worker_t gnb_discover_in_lan_worker_mod = {

    .name      = "gnb_discover_in_lan_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL

};
