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

#ifndef GNB_KEYS_H
#define GNB_KEYS_H

#include "gnb.h"

int gnb_load_keypair(gnb_core_t *gnb_core);

int gnb_load_public_key(gnb_core_t *gnb_core, gnb_uuid_t uuid64, unsigned char *public_key);

void gnb_update_time_seed(gnb_core_t *gnb_core, uint64_t now_sec);

int gnb_verify_seed_time(gnb_core_t *gnb_core,  uint64_t now_sec);

void gnb_build_crypto_key(gnb_core_t *gnb_core, gnb_node_t *node);

void gnb_build_passcode(void *passcode_bin, char *string_in);

#endif
