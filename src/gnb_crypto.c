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
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "gnb.h"
#include "gnb_keys.h"

#include "gnb_binary.h"
#include "crypto/random/gnb_random.h"
#include "ed25519/ed25519.h"

static void show_useage(int argc,char *argv[]){

    printf("Build[%s %s]\n", __DATE__, __TIME__);

    printf("usage: %s -c -p private_key_file -k public_key_file\n",argv[0]);
    printf("example:\n");
    printf("%s -c -p 1001.private -k 1001.public\n",argv[0]);

}

static void create_keypair(uint32_t uuid32, const char *private_key_file, const char *public_key_file){

    int private_file_fd;
    int public_file_fd;

    unsigned char seed[32];

    unsigned char ed25519_private_key[64];
    unsigned char ed25519_public_key[32];

    char hex_string[128];

    void *p;

    ssize_t rlen;
    ssize_t wlen;
    gnb_random_data(seed, 32);

    ed25519_create_keypair(ed25519_public_key, ed25519_private_key, (const unsigned char *)seed);

    gnb_bin2hex(ed25519_public_key,  32, hex_string);

    public_file_fd = open(public_key_file, O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);

    if ( -1 == public_file_fd ) {
        exit(0);
    }

    wlen = write(public_file_fd,hex_string,64);

    if ( -1 == wlen) {
        perror("write public_file");
    }

    close(public_file_fd);

    gnb_bin2hex(ed25519_private_key, 64, hex_string);

    private_file_fd = open(private_key_file, O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);

    if ( -1 == private_file_fd ) {
        exit(0);
    }

    wlen = write(private_file_fd,hex_string,128);

    if ( -1 == wlen) {
        perror("write private_file");
    }

    close(private_file_fd);

}


int main (int argc,char *argv[]){

    static struct option long_options[] = {
      { "create",      required_argument, 0, 'c' },
      { "private_key", required_argument, 0, 'p' },
      { "public_key",  required_argument, 0, 'k' },
      { 0, 0, 0, 0 }
    };

    int opt;

    uint32_t uuid32 = 0;

    char *public_key_file  = NULL;
    char *private_key_file = NULL;

    char *cmd = NULL;

    while (1) {

        int option_index = 0;

        opt = getopt_long (argc, argv, "ck:p:h",long_options, &option_index);

        if (opt == -1) {
            break;
        }

        switch (opt) {

        case 'c':
            cmd = "create";
            break;

        case 'p':
            private_key_file = optarg;
            break;

        case 'k':
            public_key_file = optarg;
            break;

        case 'h':
            break;

        default:
            break;
        }

    }

    if ( NULL==cmd || NULL==private_key_file || NULL==public_key_file ) {
        show_useage(argc, argv);
        exit(0);
    }

    create_keypair(uuid32, private_key_file, public_key_file);

    return 0;

}

/*
./gnb_crypto -c -p 1010.private -k 1010.public
*/
