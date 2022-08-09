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


void gnb_ctl_dump_status(gnb_ctl_block_t *ctl_block,int reachabl_opt);
void gnb_ctl_dump_address_list(gnb_ctl_block_t *ctl_block,int reachabl_opt);
void gnb_ctl_dump_node_wan_address(gnb_ctl_block_t *ctl_block);


static void show_useage(int argc,char *argv[]){

    printf("GNB Ctl version 1.3.0.0  protocol version 1.2.0\n");
    printf("Build[%s %s]\n", __DATE__, __TIME__);

    printf("Copyright (C) 2019 gnbdev\n");
    printf("Usage: %s -b CTL_BLOCK [OPTION]\n", argv[0]);
    printf("Command Summary:\n");

    printf("  -b, --ctl-block           ctl block mapper file\n");
    printf("  -a, --address             operate address zone\n");
    printf("  -w, --wan-address         operate wan address zone\n");
    printf("  -c, --core                operate core zone\n");
    printf("  -r, --reachabl            only output reachabl node\n");
    printf("  -s, --show                show\n");

    printf("      --help\n");

    printf("example:\n");
    printf("%s --ctl-block=./gnb.map\n",argv[0]);

}


int main (int argc,char *argv[]){

    char *ctl_block_file = NULL;

    gnb_ctl_block_t *ctl_block;

    setvbuf(stdout,NULL,_IOLBF,0);

    int address_opt     = 0;
    int wan_address_opt = 0;
    int core_opt        = 0;
    int show_opt        = 0;
    int reachabl_opt    = 0;

    static struct option long_options[] = {

      { "ctl-block",            required_argument, 0, 'b' },
      { "address",              no_argument, 0, 'a' },
      { "wan-address",          no_argument, 0, 'w' },
      { "core",                 no_argument, 0, 'c' },
      { "show",                 no_argument, 0, 's' },
      { "reachabl",             no_argument, 0, 'r' },
      { "help",                 no_argument, 0, 'h' },

      { 0, 0, 0, 0 }

    };


    int opt;

    while (1) {

        int option_index = 0;

        opt = getopt_long (argc, argv, "b:awcrsh",long_options, &option_index);

        if (opt == -1) {
            break;
        }

        switch (opt) {

        case 'b':
            ctl_block_file = optarg;
            break;

        case 'a':
            address_opt = 1;
            break;

        case 'w':
            wan_address_opt = 1;
            break;

        case 'c':
            core_opt = 1;
            break;

        case 'r':
            reachabl_opt = 1;
            break;

        case 's':
            show_opt = 1;
            break;

        case 'h':
            show_useage(argc,argv);
            exit(0);

        default:
            break;
        }

    }


    if ( NULL == ctl_block_file ){
        show_useage(argc,argv);
        exit(0);
    }

    ctl_block = gnb_get_ctl_block(ctl_block_file, 0);

    if ( NULL==ctl_block ){
        printf("open ctl block error [%s]\n",ctl_block_file);
        exit(0);
    }


#ifdef _WIN32
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData );
#endif


    if (core_opt){
        gnb_ctl_dump_status(ctl_block,reachabl_opt);
    }

    if (address_opt){
        gnb_ctl_dump_address_list(ctl_block,reachabl_opt);
    }

    if (wan_address_opt){
        gnb_ctl_dump_node_wan_address(ctl_block);
    }



#ifdef _WIN32
    WSACleanup();
#endif

    return 0;

}
