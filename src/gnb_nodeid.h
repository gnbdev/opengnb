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

#ifndef GNB_NODEID_H
#define GNB_NODEID_H

#include "gnb_node_type.h"

#define GNB_MAX_NODEID_DEC_STRING_SIZE  (sizeof("18446744073709551615") - 1)
#define GNB_MAX_NODEID_HEX_STRING_SIZE  (sizeof("$FFFFFFFFFFFFFFFF") - 1)
#define GNB_MAX_NODEID_STRING_SIZE      (GNB_MAX_NODEID_DEC_STRING_SIZE+GNB_MAX_NODEID_HEX_STRING_SIZE+3)

#define GNB_NODEIS2STR_FMT_DEC (0x1)
#define GNB_NODEIS2STR_FMT_HEX (0x1 << 1)

gnb_uuid_t gnb_str2nodeid(char *nodeidstr);

char *gnb_nodeid2str(gnb_uuid_t nodeid, char *nodeidstr, int fmt);


static char gnb_static_nodeid_string_buffer1[GNB_MAX_NODEID_STRING_SIZE];
static char gnb_static_nodeid_string_buffer2[GNB_MAX_NODEID_STRING_SIZE];
static char gnb_static_nodeid_string_buffer3[GNB_MAX_NODEID_STRING_SIZE];
static char gnb_static_nodeid_string_buffer4[GNB_MAX_NODEID_STRING_SIZE];

#define GNB_NODEID_DEC_STR1(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer1, GNB_NODEIS2STR_FMT_DEC)
#define GNB_NODEID_DEC_STR2(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer2, GNB_NODEIS2STR_FMT_DEC)
#define GNB_NODEID_DEC_STR3(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer3, GNB_NODEIS2STR_FMT_DEC)
#define GNB_NODEID_DEC_STR4(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer4, GNB_NODEIS2STR_FMT_DEC)


#define GNB_NODEID_HEX_STR1(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer1, GNB_NODEIS2STR_FMT_HEX)
#define GNB_NODEID_HEX_STR2(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer2, GNB_NODEIS2STR_FMT_HEX)
#define GNB_NODEID_HEX_STR3(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer3, GNB_NODEIS2STR_FMT_HEX)
#define GNB_NODEID_HEX_STR4(nodeid)  gnb_nodeid2str(nodeid, gnb_static_nodeid_string_buffer4, GNB_NODEIS2STR_FMT_HEX)

#endif
