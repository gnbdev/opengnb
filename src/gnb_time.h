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

#ifndef GNB_TIME_H
#define GNB_TIME_H

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#include <stdint.h>
#include <time.h>
#include <unistd.h>

uint64_t gnb_timestamp_sec();

/*微秒*/
uint64_t gnb_timestamp_usec();


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
