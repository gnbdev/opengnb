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

#ifndef GNB_ARG_LIST_H
#define GNB_ARG_LIST_H

#include <stddef.h>

#define GNB_ARG_STRING_MAX_SIZE 1024*4
#define GNB_ARG_MAX_SIZE        1024

typedef struct _gnb_arg_list_t {
    
    size_t size;
    
    int argc;
    
    char *argv[0];
    
}gnb_arg_list_t;


gnb_arg_list_t *gnb_arg_list_init(int size);

void gnb_arg_list_release(gnb_arg_list_t *arg_list);

int gnb_arg_append(gnb_arg_list_t *arg_list,const char *arg);

int gnb_arg_list_to_string(gnb_arg_list_t *arg_list, char *string, size_t string_len);

gnb_arg_list_t *gnb_arg_string_to_list(char *string, int num);

#endif
