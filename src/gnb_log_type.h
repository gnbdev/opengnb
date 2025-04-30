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

#ifndef GNB_LOG_TYPE_H
#define GNB_LOG_TYPE_H

#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#ifdef _WIN32

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#endif

typedef struct _gnb_log_config_t {

    char log_name[20];

    uint8_t console_level;

    uint8_t file_level;

    uint8_t udp_level;

}gnb_log_config_t;

#define GNB_MAX_LOG_ID 128

typedef struct _gnb_log_ctx_t {

	#define GNB_LOG_OUTPUT_NONE      (0x0)
	#define GNB_LOG_OUTPUT_STDOUT    (0x1)
	#define GNB_LOG_OUTPUT_FILE      (0x1 << 1)
	#define GNB_LOG_OUTPUT_UDP       (0x1 << 2)

	unsigned char output_type;

	char log_file_path[PATH_MAX];

	//char log_file_prefix[32];

	char log_file_name_std[PATH_MAX+NAME_MAX];
	char log_file_name_debug[PATH_MAX+NAME_MAX];
	char log_file_name_error[PATH_MAX+NAME_MAX];


	int std_fd;
	int debug_fd;
	int error_fd;


	int pre_std_fd;
	int pre_debug_fd;
	int pre_error_fd;


	int pre_mday;

	uint8_t  addr4[4];
	uint16_t port4;

	uint8_t  addr6[16];
	uint16_t port6;

	int socket6_fd;
	int socket4_fd;


	#define GNB_LOG_UDP_TYPE_TEXT     0
	#define GNB_LOG_UDP_TYPE_BINARY   1
	uint8_t log_udp_type;

	//如果 log_udp_type 为 GNB_LOG_UDP_TYPE_BINARY，就需要提供gnb_payload的类型
	char    log_payload_type;

	gnb_log_config_t config_table[GNB_MAX_LOG_ID];

}gnb_log_ctx_t;

#endif
