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

#include <ctype.h>

#include "gnb_binary.h"

static const char *bin2hex_convtab_down_case = "0123456789abcdef";
static const char *bin2hex_convtab_up_case   = "0123456789ABCDEF";

char *gnb_bin2hex_case_string(void *bin, size_t bin_size, char *hex_string,  int letter_case) {
    
    int i;
    char *p;
    const char *convtab;

    if ( FE_BIN2HEX_LOWERCASE == letter_case ) {
        convtab = bin2hex_convtab_down_case;
    } else {
        convtab = bin2hex_convtab_up_case;
    }
    
    p = hex_string;
    
    for ( i=0; i<bin_size; i++ ) {
        *p++ = convtab[ *(unsigned char *)bin >>4 ];
        *p++ = convtab[ *(unsigned char *)bin & 0xF ];
        bin++;
    }
    
    hex_string[i*2]='\0';
    
    return hex_string;
    
}


char *gnb_bin2hex_string(void *bin, size_t bin_size, char *hex_string) {

    int i;
    char *p;
    const char *convtab;

    convtab = bin2hex_convtab_down_case;

    p = hex_string;

    for ( i=0; i<bin_size; i++ ) {
        *p++ = convtab[ *(unsigned char *)bin >>4 ];
        *p++ = convtab[ *(unsigned char *)bin & 0xF ];
        bin++;
    }

    hex_string[i*2]='\0';

    return hex_string;

}


char *gnb_bin2hex_case(void *bin, size_t bin_size, char *hex_string,  int letter_case) {

    int i;
    char *p;
    const char *convtab;

    if ( FE_BIN2HEX_LOWERCASE == letter_case ) {
        convtab = bin2hex_convtab_down_case;
    } else {
        convtab = bin2hex_convtab_up_case;
    }

    p = hex_string;

    for ( i=0; i<bin_size; i++ ) {
        *p++ = convtab[ *(unsigned char *)bin >>4 ];
        *p++ = convtab[ *(unsigned char *)bin & 0xF ];
        bin++;
    }

    return hex_string;

}


char *gnb_bin2hex(void *bin, size_t bin_size, char *hex_string) {

    int i;

    char *p;

    const char *convtab;

    convtab = bin2hex_convtab_down_case;

    p = hex_string;

    for ( i=0; i<bin_size; i++ ) {
        *p++ = convtab[ *(unsigned char *)bin >>4 ];
        *p++ = convtab[ *(unsigned char *)bin & 0xF ];
        bin++;
    }

    return hex_string;

}


void *gnb_hex2bin(char *hex_string, void *bin, size_t bin_size){

    char *p = (char *)bin;

    int h;
    int l;
    int i;

    for ( i = 0; i < bin_size; i++ ) {

        if ( !isxdigit(hex_string[i * 2]) || !isxdigit(hex_string[i * 2 + 1]) ) {
            return NULL;
        }

        if ( isdigit( hex_string[i * 2] ) ) {
            h = hex_string[i * 2] - '0';
        } else {
            h = toupper(hex_string[i * 2]) - 'A' + 10;
        }

        if ( isdigit( hex_string[i * 2 + 1] ) ) {
            l = hex_string[i * 2 + 1] - '0';
        } else {
            l = toupper(hex_string[i * 2 + 1]) - 'A' + 10;
        }

        ((char *)bin)[i] = h*16 + l;

    }

    return bin;

}


char * gnb_get_hex_string8(void *byte4,char *dest){

    gnb_hex_string8_t hex_string8;

    gnb_bin2hex(byte4, 4, dest);

    dest[8] = '\0';

    return dest;

}


char * gnb_get_hex_string16(void *byte8, char *dest){

    gnb_bin2hex(byte8, 8, dest);

    dest[16] = '\0';

    return dest;

}


char * gnb_get_hex_string32(void *byte16, char *dest){

    gnb_bin2hex(byte16, 16, dest);

    dest[32] = '\0';

    return dest;

}


char * gnb_get_hex_string64(void *byte32, char *dest){

    gnb_bin2hex(byte32, 32, dest);

    dest[64] = '\0';

    return dest;

}


char * gnb_get_hex_string128(void *byte64, char *dest){

    gnb_bin2hex(byte64, 64, dest);

    dest[128] = '\0';

    return dest;

}


char * gnb_get_hex_string256(void *byte128, char *dest){

    gnb_bin2hex(byte128, 128, dest);

    dest[256] = '\0';

    return dest;

}
