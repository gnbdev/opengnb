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

#ifndef GNB_CONF_FILE_H
#define GNB_CONF_FILE_H

#include "gnb.h"


void local_node_file_config(gnb_core_t *gnb_core);
void gnb_config_file(gnb_core_t *gnb_core);


size_t gnb_get_node_num_from_file(gnb_conf_t *conf);

#endif

