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


void gnb_es_upnp(gnb_conf_t *conf, gnb_log_ctx_t *log){

    gnb_es_upnp_em(conf, log);

}
