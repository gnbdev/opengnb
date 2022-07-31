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

#ifndef GNB_DIR_H
#define GNB_DIR_H

#include <stdint.h>

char *gnb_get_file_dir(char *file_name, char *file_dir);

char *gnb_get_file_dir_dup(char *file_name);

char *gnb_realpath_dup(char *path);

char *gnb_make_realpath_dup(char *base_path, char *sub_path);

int gnb_get_subdirname(char *path, int max_filename_len, char **filename_lst, int *filename_lst_len);

#define GNB_MAX_FILE_NAME_LEN 1024

typedef struct _gnb_file_info_t{

	char *name;

	//绝对路径
	char *abs_name;

	#define GNB_FILE_TYPE_INIT 0
	#define GNB_FILE_TYPE_REG  1
	#define GNB_FILE_TYPE_DIR  2
	#define GNB_FILE_TYPE_LNK  3

	uint8_t type;

}gnb_file_info_t;


int gnb_get_dir_file_names(char *path, gnb_file_info_t **sub_file_info_lst, int *lst_len_ptr);

int gnb_scan_dir(char *path, gnb_file_info_t **file_info_lst, int *lst_len_ptr);

char *gnb_file_path_cut(char *filename, size_t len);

char *gnb_file_path_dup(const char *filename, size_t len);

/* 从传入的文件路径中得到文件的目录和文件名，通过 realpath(Unix 平台) 或 _fullpath (Windows平台) 获得文件目录的绝对路径，与文件名合并成一个字符串返回 */
char *gnb_realpath(char *in_path, char *resolved_path);

void gnb_release_file_info_lst(gnb_file_info_t **file_info_lst, int lst_len);

#endif
