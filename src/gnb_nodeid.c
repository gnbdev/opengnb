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
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>

#include "gnb_nodeid.h"

gnb_uuid_t gnb_str2nodeid(char *nodeidstr){

    gnb_uuid_t nodeid;
    char *endptr;
    char *p;
    int is_hex = 0;
    int i;

    p = nodeidstr;

    if ( '$' == *nodeidstr ) {
        is_hex = 1;
        p = nodeidstr+1;
        goto convert_begin;
    }

    if ( '0' == *nodeidstr && ( 'x' == *(nodeidstr+1) || 'X' == *(nodeidstr+1) ) ) {
        is_hex = 1;
        p = nodeidstr+2;
        goto convert_begin;
    }

    for ( i=0; i<16; i++ ) {

        if ( *p == '\0' ) {
            break;
        }

        if ( (*p >='A' && *p <='F') || (*p >='a' && *p <='f') ) {
            is_hex = 1;
            p = nodeidstr;
            goto convert_begin;
        }

        p++;

    }

convert_begin:

    if ( is_hex ) {
        nodeid = strtoull(p, &endptr, 16);
    } else {
        nodeid = strtoull(nodeidstr, &endptr, 10);
    }

    if ( ERANGE == errno ) {
        nodeid = 0xFFFFFFFFFFFFFFFF;
        goto convert_end;
    }

    if ( endptr == p ) {
        nodeid = 0xFFFFFFFFFFFFFFFF;
        goto convert_end;
    }

convert_end:

    return nodeid;

}


char *gnb_nodeid2str(gnb_uuid_t nodeid, char *nodeidstr, int fmt){
#if 0
    char buf[GNB_MAX_NODEID_STRING_SIZE];

    switch (fmt) {
        case GNB_NODEIS2STR_FMT_DEC|GNB_NODEIS2STR_FMT_HEX:
            snprintf(nodeidstr, GNB_MAX_NODEID_STRING_SIZE, "%016"PRIX64"(%"PRIu64")", nodeid, nodeid);
            break;
        case GNB_NODEIS2STR_FMT_DEC:
            snprintf(nodeidstr, GNB_MAX_NODEID_STRING_SIZE, "%"PRIu64"", nodeid);
            break;
        case GNB_NODEIS2STR_FMT_HEX:
            snprintf(nodeidstr, GNB_MAX_NODEID_STRING_SIZE, "%016"PRIX64"", nodeid);
            break;
        default:
            break;

    }
#endif
    return nodeidstr;

}
