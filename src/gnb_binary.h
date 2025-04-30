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

#ifndef GNB_BINARY_H
#define GNB_BINARY_H

#include <stdio.h>

#define FE_BIN2HEX_LOWERCASE 0
#define FE_BIN2HEX_CAPITALS  1

char *gnb_bin2hex_case_string(void *bin, size_t bin_size, char *hex_string,  int letter_case);
char *gnb_bin2hex_string(void *bin, size_t bin_size, char *hex_string);


char *gnb_bin2hex_case(void *bin, size_t bin_size, char *hex_string,  int letter_case);
char *gnb_bin2hex(void *bin, size_t bin_size, char *hex_string);

void *gnb_hex2bin(char *hex_string, void *bin, size_t bin_size);


typedef struct _gnb_hex_string8_t{
	char value[8+1];
}gnb_hex_string8_t;


typedef struct _gnb_hex_string16_t{
	char value[16+1];
}gnb_hex_string16_t;


typedef struct _gnb_hex_string32_t{
	char value[32+1];
}gnb_hex_string32_t;


typedef struct _gnb_hex_string64_t{
	char value[64+1];
}gnb_hex_string64_t;


typedef struct _gnb_hex_string128_t{
	char value[128+1];
}gnb_hex_string128_t;


typedef struct _gnb_hex_string256_t{
	char value[256+1];
}gnb_hex_string256_t;


char * gnb_get_hex_string8(void *byte4, char *dest);
char * gnb_get_hex_string16(void *byte8, char *dest);
char * gnb_get_hex_string32(void *byte16, char *dest);
char * gnb_get_hex_string64(void *byte32, char *dest);
char * gnb_get_hex_string128(void *byte64, char *dest);
char * gnb_get_hex_string256(void *byte128, char *dest);


static char gnb_hex1_string256[256+1];
static char gnb_hex2_string256[256+1];

#define GNB_HEX1_BYTE8(bytes)   gnb_get_hex_string8(bytes,   gnb_hex1_string256)
#define GNB_HEX1_BYTE16(bytes)  gnb_get_hex_string16(bytes,  gnb_hex1_string256)
#define GNB_HEX1_BYTE32(bytes)  gnb_get_hex_string32(bytes,  gnb_hex1_string256)
#define GNB_HEX1_BYTE64(bytes)  gnb_get_hex_string64(bytes,  gnb_hex1_string256)
#define GNB_HEX1_BYTE128(bytes) gnb_get_hex_string128(bytes, gnb_hex1_string256)
#define GNB_HEX1_BYTE256(bytes) gnb_get_hex_string256(bytes, gnb_hex1_string256)

#define GNB_HEX2_BYTE8(bytes)   gnb_get_hex_string8(bytes,   gnb_hex2_string256)
#define GNB_HEX2_BYTE16(bytes)  gnb_get_hex_string16(bytes,  gnb_hex2_string256)
#define GNB_HEX2_BYTE32(bytes)  gnb_get_hex_string32(bytes,  gnb_hex2_string256)
#define GNB_HEX2_BYTE64(bytes)  gnb_get_hex_string64(bytes,  gnb_hex2_string256)
#define GNB_HEX2_BYTE128(bytes) gnb_get_hex_string128(bytes, gnb_hex2_string256)
#define GNB_HEX2_BYTE256(bytes) gnb_get_hex_string256(bytes, gnb_hex2_string256)

#endif
