#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

//unistd.h 定义了 _POSIX_THREAD_SAFE_FUNCTIONS 使得 localtime_r 有效
#include <unistd.h>
#endif

#include <time.h>
#include <sys/time.h>

#include "gnb_time.h"

/*秒*/
int64_t gnb_timestamp_sec(){
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


