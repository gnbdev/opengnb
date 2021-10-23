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
#include <limits.h>
#include <stddef.h>
#include <inttypes.h>

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_address.h"
#include "gnb_binary.h"
#include "gnb_time.h"
#include "gnb_udp.h"
#include "gnb_ctl_block.h"
#include "ed25519/sha512.h"


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

