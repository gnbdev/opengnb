#ifndef gnb_time_h
#define gnb_time_h

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#include <stdint.h>
#include <time.h>
#include <unistd.h>

int64_t gnb_timestamp_sec();

/*微秒*/
uint64_t gnb_timestamp_usec();

/*
 例子，休眠 100 毫秒
 gnb_sleep(0, 100*1000000l);
 */
//void gnb_sleep(long sec, long nanosec);

//format:"%Y_%m_%d_%H.%M.%S"
void gnb_now_timef(const char *format, char *buffer, size_t buffer_size);

void gnb_timef(const char *format, time_t t, char *buffer, size_t buffer_size);

/*
 获得当前月的当天数字
 */
int gnb_now_mday();

int gnb_now_yday();

#define GNB_TIME_STRING_MAX 64

#ifdef __UNIX_LIKE_OS__
#define GNB_SLEEP_MILLISECOND(millisecond) usleep(1000 * millisecond)
#endif

#if defined(_WIN32)
#define GNB_SLEEP_MILLISECOND(millisecond)	Sleep(millisecond)
#endif


#endif

