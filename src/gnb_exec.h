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

#ifndef GNB_EXEC_H
#define GNB_EXEC_H

#include "gnb_platform.h"

#include "gnb_arg_list.h"

#define GNB_EXEC_FOREGROUND  (0x1)
#define GNB_EXEC_BACKGROUND  (0x1 << 1)
#define GNB_EXEC_WAIT        (0x1 << 2)

#ifdef __UNIX_LIKE_OS__

#include <sys/types.h>
#include <unistd.h>

pid_t gnb_exec(char *app_filename, char *current_path, gnb_arg_list_t *arg_list, int flag);
void gnb_kill(pid_t pid);

#endif



#ifdef _WIN32

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#include <windows.h>

int gnb_exec(char *app_filename, char *current_path, gnb_arg_list_t *gnb_arg_list, int flag);
int gnb_kill(DWORD pid);

#endif


#endif
