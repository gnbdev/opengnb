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
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

#define  _POSIX_C_SOURCE 1
//使得 localtime_r 等函数有效

#endif

#include <time.h>
#include <sys/time.h>

#include "gnb_time.h"


uint64_t gnb_timestamp_sec(){

    int ret;
    struct timeval cur_time;
    uint64_t u64;

    ret = gettimeofday(&cur_time,NULL);

    if (0!=ret) {
        return 0;
    }

    u64 = (uint64_t)cur_time.tv_sec;

    return u64;

}


uint64_t gnb_timestamp_usec(){

    int ret;
    struct timeval cur_time;
    uint64_t u64;

    ret = gettimeofday(&cur_time,NULL);

    if (0!=ret) {
        return 0;
    }

    u64 = (uint64_t)cur_time.tv_sec*1000000 + cur_time.tv_usec;

    return u64;
}


void gnb_now_timef(const char *format, char *buffer, size_t buffer_size){

    time_t t;
    
    struct tm ltm;
    
    time (&t);
    
    localtime_r(&t, &ltm);
    
    strftime (buffer,buffer_size,format,&ltm);

}


void gnb_timef(const char *format, time_t t, char *buffer, size_t buffer_size){

    struct tm ltm;

    localtime_r(&t, &ltm);

    strftime (buffer,buffer_size,format,&ltm);

}


int gnb_now_mday(){
    
    time_t t;
    
    struct tm ltm;

    time (&t);
    
    localtime_r(&t, &ltm);

    return ltm.tm_mday;
    
}


int gnb_now_yday(){

    time_t t;

    struct tm ltm;

    time (&t);

    localtime_r(&t, &ltm);

    return ltm.tm_yday;

}
