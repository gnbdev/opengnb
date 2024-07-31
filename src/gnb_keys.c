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

#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

#define  _POSIX_C_SOURCE 1
//使得 localtime_r 等函数有效

#endif

#include <time.h>

#include "gnb_binary.h"
#include "crypto/random/gnb_random.h"

#include "ed25519/ed25519.h"
#include "ed25519/sha512.h"

#include "gnb_keys.h"


/*
固定长度
unsigned char seed[32];
unsigned char signature[64];
unsigned char public_key[32];
unsigned char private_key[64];
unsigned char scalar[32];
unsigned char shared_secret[32];
*/

int gnb_load_keypair(gnb_core_t *gnb_core){

    int private_file_fd;
    int public_file_fd;

    char node_private_file_name[PATH_MAX+NAME_MAX];
    char node_public_file_name[PATH_MAX+NAME_MAX];

    unsigned char seed[32];

    char hex_string[129];

    void *p;

    ssize_t rlen;

    snprintf(node_private_file_name, PATH_MAX+NAME_MAX, "%s/security/%llu.private", gnb_core->conf->conf_dir, gnb_core->conf->local_uuid);
    snprintf(node_public_file_name,  PATH_MAX+NAME_MAX, "%s/security/%llu.public",  gnb_core->conf->conf_dir, gnb_core->conf->local_uuid);

    private_file_fd = open(node_private_file_name, O_RDONLY);

    if ( -1 == private_file_fd ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "load node private file[%s] error\n", node_private_file_name);
        exit(0);
    }

    rlen = read(private_file_fd, hex_string, 128);

    close(private_file_fd);

    if ( 128 != rlen ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "load node private file[%s] key error\n", node_private_file_name);
        exit(0);
    }

    p = gnb_hex2bin(hex_string, gnb_core->ed25519_private_key, 64);

    if ( NULL==p ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "setup private key error\n");
        exit(0);
    }

    public_file_fd = open(node_public_file_name, O_RDONLY);

    if ( -1 == public_file_fd ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "load node public file [%s] error\n", node_public_file_name);
        exit(0);
    }

    rlen = read(public_file_fd, hex_string, 64);

    close(public_file_fd);

    if ( 64 != rlen ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "load node public file[%s] key error\n", node_public_file_name);
        exit(0);
    }

    p = gnb_hex2bin(hex_string, gnb_core->ed25519_public_key, 32);

    if ( NULL==p ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "setup public key error\n");
        exit(0);
    }

    return 0;

}


int gnb_load_public_key(gnb_core_t *gnb_core, gnb_uuid_t uuid64, unsigned char *public_key){

    char node_public_file_name[PATH_MAX+NAME_MAX];

    int public_file_fd;

    char hex_string[129];

    ssize_t rlen;

    snprintf(node_public_file_name, PATH_MAX+NAME_MAX, "%s/ed25519/%llu.public", gnb_core->conf->conf_dir, uuid64);

    public_file_fd = open(node_public_file_name, O_RDONLY);

    if ( -1 == public_file_fd ) {
        return -1;
    }

    rlen = read(public_file_fd, hex_string, 64);

    close(public_file_fd);

    if ( 64 != rlen ) {
        return -2;
    }

    gnb_hex2bin(hex_string, public_key, 32);

    return 0;

}


void gnb_build_crypto_key(gnb_core_t *gnb_core, gnb_node_t *node){

    //passcode 将在这个函数中发挥比较重要的作用
    unsigned char buffer[64+4];

    if ( GNB_CRYPTO_KEY_UPDATE_INTERVAL_NONE != gnb_core->conf->crypto_key_update_interval ) {
        memcpy(buffer,gnb_core->time_seed,32);
    } else {
        memcpy(buffer,node->shared_secret,32);
    }

    memcpy(buffer+32,node->shared_secret,32);

    memcpy(buffer+64, gnb_core->conf->crypto_passcode, 4);

    sha512(buffer, 64+4, node->crypto_key);

}


/*
gnb_update_time_seed gnb_verify_seed_time
用于根据时钟更新加密的密钥
*/
void gnb_update_time_seed(gnb_core_t *gnb_core, uint64_t now_sec){

    time_t t;

    struct tm ltm;

    uint32_t time_seed;

    t = (time_t)now_sec;

    gmtime_r(&t, &ltm);

    time_seed = ltm.tm_year + ltm.tm_mon + ltm.tm_yday;

    if ( GNB_CRYPTO_KEY_UPDATE_INTERVAL_HOUR == gnb_core->conf->crypto_key_update_interval ) {
        time_seed += ltm.tm_hour;
    } else if ( GNB_CRYPTO_KEY_UPDATE_INTERVAL_MINUTE == gnb_core->conf->crypto_key_update_interval ) {
        time_seed += ltm.tm_hour;
        time_seed += ltm.tm_min;
    } else {
        time_seed += ltm.tm_hour;
    }

    time_seed = htonl(time_seed);

    sha512((const unsigned char *)(&time_seed),  sizeof(uint32_t), gnb_core->time_seed);

}


int gnb_verify_seed_time(gnb_core_t *gnb_core, uint64_t now_sec){

    time_t t;
    struct tm ltm;
    int r = 0;

    t = (time_t)now_sec;

    gmtime_r(&t, &ltm);

    if ( GNB_CRYPTO_KEY_UPDATE_INTERVAL_HOUR == gnb_core->conf->crypto_key_update_interval ) {
        r = (ltm.tm_hour + 1) - gnb_core->time_seed_update_factor;
        gnb_core->time_seed_update_factor = ltm.tm_hour+1;
    } else if ( GNB_CRYPTO_KEY_UPDATE_INTERVAL_MINUTE == gnb_core->conf->crypto_key_update_interval ) {
        r = (ltm.tm_min+1) - gnb_core->time_seed_update_factor;
        gnb_core->time_seed_update_factor = (ltm.tm_min+1);
    } else {
        r = (ltm.tm_hour+1) - gnb_core->time_seed_update_factor;
        gnb_core->time_seed_update_factor = ltm.tm_hour+1;
    }

    return r;
}


void gnb_build_passcode(void *passcode_bin, char *string_in) {

    char   passcode_string[9];
    size_t passcode_string_len;
    char   *passcode_string_offset;

    memset(passcode_string, 0, 9);

    passcode_string_len = strlen(string_in);

    if ( passcode_string_len>2 && '0' == string_in[0] && 'x' == string_in[1] ) {
        passcode_string_offset = string_in + 2;
        passcode_string_len -= 2;
    } else {
        passcode_string_offset = string_in;
    }

    if ( passcode_string_len > 8 ) {
        passcode_string_len = 8;
    }

    if ( 0 != passcode_string_len ) {
        memcpy(passcode_string, passcode_string_offset, passcode_string_len);
    }

    if ( NULL == gnb_hex2bin(passcode_string, passcode_bin, 4) ) {
        memcpy(passcode_bin, passcode_string, 4);
    }

}
