/*
   Copyright (C) 2019 gnbdev

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

#ifndef GNB_REALPATH_H
#define GNB_REALPATH_H

#include <stdint.h>
#include <sys/stat.h>

char *gnb_get_file_dir(char *file_name, char *file_dir);
char *gnb_file_path_cut(char *filename, size_t len);
/* 从传入的文件路径中得到文件的目录和文件名，通过 realpath(Unix 平台) 或 _fullpath (Windows平台) 获得文件目录的绝对路径，与文件名合并成一个字符串返回 */
char *gnb_realpath(char *in_path, char *resolved_path);

#endif
