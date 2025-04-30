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
#include <limits.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include "gnb_es_type.h"

#ifdef _WIN32

static void windows_if_up(gnb_es_ctx *es_ctx){

    gnb_conf_t *conf;
    gnb_ctl_core_zone_t *core_zone;

    conf      = &es_ctx->ctl_block->conf_zone->conf_st;
    core_zone = es_ctx->ctl_block->core_zone;

    char cmd[1024];

    char *devicename;
    char *if_device_string;

    devicename = core_zone->ifname;
    if_device_string   = core_zone->if_device_string;

    //修改tun设备的MTU
    snprintf(cmd,1024,"netsh interface ipv4 set subinterface interface=\"%s\" mtu=%d store=active >NUL 2>&1", devicename, conf->mtu);
    system(cmd);

}


static void windows_if_down(gnb_es_ctx *es_ctx){


}

#endif


void gnb_es_if_up(gnb_es_ctx *es_ctx){

#ifdef _WIN32
    windows_if_up(es_ctx);
#endif

}


void gnb_es_if_down(gnb_es_ctx *es_ctx){

#ifdef _WIN32
    windows_if_down(es_ctx);
#endif

}

void gnb_es_if_loop(gnb_es_ctx *es_ctx){

}
