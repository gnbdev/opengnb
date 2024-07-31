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
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include "gnb_conf_type.h"
#include "gnb_ctl_block.h"


#ifndef GNB_SKIP_BUILD_TIME
#define GNB_BUILD_STRING  "Build Time ["__DATE__","__TIME__"]"
#else
#define GNB_BUILD_STRING  "Build Time [Hidden]"
#endif


void gnb_ctl_dump_status(gnb_ctl_block_t *ctl_block, gnb_uuid_t in_nodeid, uint8_t online_opt);
void gnb_ctl_dump_address_list(gnb_ctl_block_t *ctl_block, gnb_uuid_t in_nodeid, uint8_t online_opt);

static void show_useage(int argc,char *argv[]){

    printf("GNB Ctl version 1.5.0.a protocol version 1.5.0\n");

    printf("%s\n", GNB_BUILD_STRING);

    printf("Copyright (C) 2019 gnbdev\n");
    printf("Usage: %s -b CTL_BLOCK [OPTION]\n", argv[0]);
    printf("Command Summary:\n");

    printf("  -b, --ctl-block           ctl block mapper file\n");
    printf("  -c, --core                operate core zone\n");
    printf("  -a, --address             dunmp address\n");
    printf("  -s, --status              dunmp node status\n");
    printf("  -o, --online              dunmp online node\n");
    printf("  -n, --node                node id\n");
    printf("      --help\n");

    printf("example:\n");
    printf("%s -b gnb.map -c -s\n",argv[0]);

}


int main (int argc,char *argv[]){

    char *ctl_block_file = NULL;

    gnb_ctl_block_t *ctl_block;    

    uint8_t  address_opt      = 0;
    uint8_t  core_opt         = 0;
    uint8_t  node_status_opt  = 0;
    uint8_t  online_opt       = 0;
    gnb_uuid_t nodeid = 0;

    static struct option long_options[] = {

      { "ctl-block",            required_argument, 0, 'b' },
      { "node",                 required_argument, 0, 'n' },
      { "core",                 no_argument,       0, 'c' },
      { "status",               no_argument,       0, 's' },
      { "address",              no_argument,       0, 'a' },
      { "online",               no_argument,       0, 'o' },
      { "help",                 no_argument,       0, 'h' },
      { 0, 0, 0, 0 }

    };

    setvbuf(stdout,NULL,_IOLBF,0);

    int opt;

    while (1) {

        int option_index = 0;

        opt = getopt_long (argc, argv, "b:n:csaoh",long_options, &option_index);

        if ( opt == -1 ) {
            break;
        }

        switch (opt) {

        case 'b':
            ctl_block_file = optarg;
            break;

        case 'n':
            nodeid = (gnb_uuid_t)strtoull(optarg, NULL, 10);
            break;

        case 'c':
            core_opt = 1;
            break;

        case 's':
            node_status_opt = 1;
            break;

        case 'a':
            address_opt = 1;
            break;

        case 'o':
            online_opt = 1;
            break;

        case 'h':
            show_useage(argc,argv);
            exit(0);

        default:
            break;
        }

    }


    if ( NULL == ctl_block_file ) {
        show_useage(argc,argv);
        exit(0);
    }

    ctl_block = gnb_get_ctl_block(ctl_block_file, 0);

    if ( NULL==ctl_block ) {
        printf("open ctl block error [%s]\n",ctl_block_file);
        exit(0);
    }


#ifdef _WIN32
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData );
#endif


    if ( node_status_opt ) {
        gnb_ctl_dump_status(ctl_block, nodeid, online_opt);
    }

    if ( address_opt ) {
        gnb_ctl_dump_address_list(ctl_block, nodeid, online_opt);
    }


#ifdef _WIN32
    WSACleanup();
#endif

    return 0;

}

