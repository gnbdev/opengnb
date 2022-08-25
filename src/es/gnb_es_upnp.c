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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_ctl_block.h"
#include "gnb_time.h"
#include "es/gnb_es_type.h"

#include "miniupnpc.h"
#include "natpmp.h"
#include "upnpcommands.h"

static void gnb_es_upnp_em(gnb_conf_t *conf, gnb_log_ctx_t *log){

    const char *rootdescurl = NULL;

    struct UPNPDev *devlist = NULL;

    const char *multicastif = NULL;

    const char *minissdpdpath = NULL;

    int localport = UPNP_LOCAL_PORT_ANY;

    int ipv6 = 0;

    unsigned char ttl = 2;

    const char *description = NULL;

    int error = 0;

    int r = 1;

    struct UPNPUrls urls;

    struct IGDdatas data;

    char lan_addr[64] = "unset";
    char lan_addr_port[6];

    //查询时用
    char intClient[16];
    char intPort[6];
    char duration[16];

    uint16_t last_extPort = 0;

    snprintf(lan_addr_port, 6, "%d", conf->udp4_ports[0]);

    uint16_t test_extPort;

    char ext_port_string[6];
    char in_port_string[6];

    devlist = upnpDiscover(2000, multicastif, minissdpdpath, localport, ipv6, ttl, &error);

    GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Try to UPNP_GetValidIGD.\n");

    if ( NULL == devlist ) {
        return;
    }

    r = UPNP_GetValidIGD(devlist, &urls, &data, lan_addr, sizeof(lan_addr));

    switch (r) {

    case 0:
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC UPNP_GetValidIGD NO IGD found\n");
        GNB_SLEEP_MILLISECOND(300 * 1000);
        return;

    case 1:
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP,"UPNPC Found valid IGD : %s\n", urls.controlURL);
        break;

    case 2:
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Found a (not connected?) IGD : %s\n", urls.controlURL); //有时得到 ipv6的 urls？然后失败
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Trying to continue anyway\n");
        break;

    case 3:
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Trying to continue anyway\n");
        break;

    default:
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Found device (igd ?) : %s\n", urls.controlURL);
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Trying to continue anyway\n");
        break;
    }

    if ( 0 == strncmp(lan_addr, "unset", 64) ) {
        goto next;
    }

    GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNPC Local LAN ip address : %s\n", lan_addr);

    if ( 0 == strncmp(lan_addr, "unset", 64) ) {
        goto next;
    }

    int i;

    for ( i = 0; i<conf->udp4_socket_num; i++ ) {

        if( conf->udp4_ports[i] <= 1 || conf->udp4_ports[i] >= 65535 ) {
            continue;
        }

        snprintf(in_port_string,  6, "%d", conf->udp4_ports[i]);
        snprintf(ext_port_string, 6, "%d", conf->udp4_ext_ports[i]);

        r = UPNP_GetSpecificPortMappingEntry(urls.controlURL,
                data.first.servicetype, ext_port_string, "UDP",
                NULL/*remoteHost*/, intClient, intPort, NULL/*desc*/,
                NULL/*enabled*/, duration);

        if ( UPNPCOMMAND_SUCCESS == r) {

            if ( 0 == strncmp(lan_addr, intClient, 16) ) {

                GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "already redirected loacl[%s:%s] extPort[%d]\n", intClient, intPort, conf->udp4_ports[i]);

            } else {

                GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "already redirected other[%s:%s] extPort[%d]\n",intClient, intPort, conf->udp4_ports[i]);
                //conf->udp4_ext_ports[i] += 1;
                snprintf(ext_port_string, 6, "%d", conf->udp4_ext_ports[i]);

            }

            //续约
            goto doPortMapping;

        } else {

            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "UPNP_GetSpecificPortMappingEntry [%d]\n", r);

        }

doPortMapping:

        r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, ext_port_string, in_port_string, lan_addr, "GNB", "UDP", NULL, "3600");

        if ( 0 == r ) {
            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "now redirected [%s:%d] extPort[%d]\n", lan_addr, conf->udp4_ports[i], conf->udp4_ext_ports[i]);
        } else {
            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "Port Mapping Error return[%d] [%s:%d] extPort[%d]\n", r, lan_addr, conf->udp4_ports[i], conf->udp4_ext_ports[i]);
        }

    }

next:

    FreeUPNPUrls(&urls);

    freeUPNPDevlist(devlist);

    return;
}


int gnb_es_natpnpc(gnb_conf_t *conf, gnb_log_ctx_t *log){

    int forcegw = 0;
    in_addr_t gateway = 0;
    natpmp_t natpmp;
    natpmpresp_t response;

    struct in_addr gateway_in_use;

    uint32_t lifetime = 3600;

    int i;
    int r;
    int sav_errno;

    int ret = 0;

    r = initnatpmp(&natpmp, forcegw, gateway);

    if ( r < 0 ) {
        ret = -1;
        goto finish;        
    }

    gateway_in_use.s_addr = natpmp.gateway;

    r = sendpublicaddressrequest(&natpmp);

    if ( r < 0 ) {
        ret = -2;
        goto finish;
    }

    struct timeval timeout;
    fd_set fds;

    do {

        FD_ZERO(&fds);

        FD_SET(natpmp.s, &fds);

        getnatpmprequesttimeout(&natpmp, &timeout);

        r = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

        if ( r < 0 ) {
            ret = -3;
            goto finish;
        }

        r = readnatpmpresponseorretry(&natpmp, &response);

        sav_errno = errno;

        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "readnatpmpresponseorretry returned %d (%s)\n", r, r==0?"OK":(r==NATPMP_TRYAGAIN?"TRY AGAIN":"FAILED"));

        if( r<0 && r!=NATPMP_TRYAGAIN ) {

#ifdef ENABLE_STRNATPMPERR
            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "readnatpmpresponseorretry() failed : %s\n", strnatpmperr(r));
#endif

            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "errno=%d '%s'\n", sav_errno, strerror(sav_errno));

        }

    } while (r==NATPMP_TRYAGAIN);

    if ( r < 0 ) {
        ret = -4;
        goto finish;
    }

    for ( i = 0; i < conf->udp4_socket_num; i++ ) {

        r = sendnewportmappingrequest(&natpmp, NATPMP_PROTOCOL_UDP, conf->udp4_ports[i], conf->udp4_ext_ports[i], lifetime);

        if ( r<0 ) {
            continue;
        }

        do {

            FD_ZERO(&fds);

            FD_SET(natpmp.s, &fds);

            getnatpmprequesttimeout(&natpmp, &timeout);
            
            select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
            
            r = readnatpmpresponseorretry(&natpmp, &response);
            
            GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "readnatpmpresponseorretry returned %d (%s)\n", r, r==0?"OK":(r==NATPMP_TRYAGAIN?"TRY AGAIN":"FAILED"));

        } while(r==NATPMP_TRYAGAIN);

    }

finish:

    r = closenatpmp(&natpmp);

    return ret;

}

void gnb_es_upnp(gnb_conf_t *conf, gnb_log_ctx_t *log){

    int ret;
    
    ret = gnb_es_natpnpc(conf, log);

    if ( 0 == ret ) {    
        GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "gnb_es_natpnpc finish upnp\n");
        return;
    }

    GNB_LOG1(log, GNB_LOG_ID_ES_UPNP, "gnb_es_natpnpc ret=%d gnb_es_upnp_em\n");

    gnb_es_upnp_em(conf, log);

}
