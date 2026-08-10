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
#include "gnb_alloc.h"
#include "gnb_ctl_block.h"
#include "gnb_version.h"

void gnb_ctl_dump_status(gnb_ctl_block_t *ctl_block, gnb_uuid_t in_nodeid, uint8_t online_opt, uint8_t addr_secure);
void gnb_ctl_dump_address_list(gnb_ctl_block_t *ctl_block, gnb_uuid_t in_nodeid, uint8_t online_opt, int address_type, uint8_t format, uint8_t addr_secure);

#define GNB_OPT_INIT                   0x91
#define SET_ADDR_SECURE                (GNB_OPT_INIT + 1)

static void show_useage(int argc,char *argv[]) {
    printf("GNB Ctl\n");
    printf("%s\n", GNB_VERSION_STRING);
    printf("%s\n", GNB_BUILD_STRING);

    printf("Copyright (C) 2019 gnbdev\n");
    printf("Usage: %s -b CTL_BLOCK [OPTION]\n", argv[0]);
    printf("Command Summary:\n");

    printf("  -b, --ctl-block           ctl block mapper file\n");
    printf("  -c, --core                operate core zone\n");
    printf("  -s, --status              dunmp node status\n");
    printf("  -o, --online              dunmp online node\n");
    printf("  -a, --address             dunmp address 0:foramt0,1:foramt1 defalt:0\n");

    printf("  -4, --ipv4-only           Use IPv4 Only\n");
    printf("  -6, --ipv6-only           Use IPv6 Only\n");
    printf("  -n, --node                node id\n");
    printf("      --address-secure      hide part of ip address in logs \"on\",\"off\" default:\"on\"\n");
    printf("      --help\n");

    printf("example:\n");
    printf("%s -b gnb.map -c -s\n",argv[0]);
}

int main (int argc,char *argv[]) {
    int r;
    char *ctl_block_file = NULL;
    
    uint8_t  address_opt      = 0;
    uint8_t  address_type     = 0x3;  // bit0=1:IPV4, bit1=1:IPV6
    uint8_t  core_opt         = 0;
    uint8_t  node_status_opt  = 0;
    uint8_t  online_opt       = 0;
    uint8_t  address_format   = 0;
    uint8_t addr_secure = 0;
    gnb_uuid_t nodeid = 0;

    gnb_heap_t *heap = gnb_heap_create(8192);
    gnb_ctl_block_t *ctl_block = (gnb_ctl_block_t *)gnb_heap_alloc(heap, sizeof(gnb_ctl_block_t));
    ctl_block->mmap_block = (gnb_mmap_block_t *)gnb_heap_alloc(heap, sizeof(gnb_mmap_block_t));

    static struct option long_options[] = {
      { "ctl-block",            required_argument,  0, 'b' },
      { "node",                 required_argument,  0, 'n' },
      { "core",                 no_argument,        0, 'c' },
      { "status",               no_argument,        0, 's' },
      { "address",              required_argument,  0, 'a' },
      { "online",               no_argument,        0, 'o' },
      { "address-secure",       required_argument,  0, SET_ADDR_SECURE },
      { "ipv4-only",            no_argument,        0, '4' },
      { "ipv6-only",            no_argument,        0, '6' },
      { "help",                 no_argument,        0, 'h' },
      { 0, 0, 0, 0 }
    };

    setvbuf(stdout,NULL,_IOLBF,0);
    int opt;
    while (1) {
        int option_index = 0;
        opt = getopt_long (argc, argv, "b:n:csa:o46h",long_options, &option_index);
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
            address_format = (uint8_t)strtoull(optarg, NULL, 10);
            break;
        case 'o':
            online_opt = 1;
            break;
        case '4':
            address_type = 0x1;
            break;
        case '6':
            address_type = 0x2;
            break;
        case SET_ADDR_SECURE:
            if ( !strncmp(optarg, "on", 2) ) {
                addr_secure = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                addr_secure = 0;
            } else {
                addr_secure = 1;
            }
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

    r = gnb_get_ctl_block(ctl_block, ctl_block_file, 0);
    if ( 0 != r ) {
        printf("open ctl block error [%s]\n",ctl_block_file);
        exit(0);
    }

#ifdef _WIN32
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData );
#endif

    if ( node_status_opt ) {
        gnb_ctl_dump_status(ctl_block, nodeid, online_opt, addr_secure);
    }
    if ( address_opt ) {
        if ( 0 != address_format && 1 != address_format ) {
            address_format = 0;
        }
        if ( 1 == address_format ) {
            addr_secure = 0;
        }
        gnb_ctl_dump_address_list(ctl_block, nodeid, online_opt, address_type, address_format, addr_secure);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
