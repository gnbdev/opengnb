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


#ifdef _WIN32
#include <windows.h>
//unistd.h 定义了 _POSIX_THREAD_SAFE_FUNCTIONS 使得 localtime_r 有效
#include <unistd.h>
#endif

#include <sys/time.h>

#include "gnb_random.h"
#include "gnb_binary.h"

static uint64_t comput_random_seed(){

    int ret;

    struct timeval cur_time;

    ret = gettimeofday(&cur_time,NULL);

    if (0!=ret) {
        return 0;
    }

    return cur_time.tv_usec;

}


unsigned char *gnb_random_data(unsigned char *buffer, size_t buffer_size){

    uint64_t seed_u64;

    seed_u64 = comput_random_seed();

    int seed = (int)seed_u64;

    srand(seed);

    int r;
    int i;
    for (i = 0; i < buffer_size; i++) {
        r = rand();
        buffer[i] = r % 256;
    }

    return buffer;

}
