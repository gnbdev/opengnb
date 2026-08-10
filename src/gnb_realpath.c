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
//#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>

#include "gnb_platform.h"
#include "gnb_realpath.h"

#if __UNIX_LIKE_OS__
#define GNB_FILE_SP '/'
#endif

#ifdef _WIN32
#define GNB_FILE_SP '\\'
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#endif

char *gnb_get_file_dir(char *in_file_name, char *file_dir) {
    #if __UNIX_LIKE_OS__
    if ( NULL == realpath(in_file_name, file_dir) ) {
        return NULL;
    }
    #endif

    #ifdef _WIN32
    if ( NULL == _fullpath(file_dir, in_file_name , PATH_MAX) ) {
        return NULL;
    }
    #endif

    size_t string_len = strlen( (const char *)file_dir );
    int i;
    for ( i=(int)string_len-1; i>0; i-- ) {
        if ( '/'==file_dir[i] || '\\'==file_dir[i]) {
            file_dir[i]='\0';
            break;
        }
    }
    return file_dir;
}

char *gnb_file_path_cut(char *filename, size_t len) {
    int i;
    if ( 0==len ) {
        return NULL;
    }
    for ( i=(int)len-1; i>=0; i-- ) {
        if ( '/' == filename[i] || '\\' == filename[i] ) {
            filename[i]='\0';
            return filename;
        }
    }
    return NULL;
}

char *gnb_realpath(char *in_path, char *resolved_path) {
    int ret;
    size_t len;
    char *path;
    struct stat st;
    char *file_basename = NULL;
    char file_dir[PATH_MAX+NAME_MAX];
    int i;
    ret = stat(in_path,&st);

    if ( 0 == ret && S_ISDIR(st.st_mode) ) {
        #if __UNIX_LIKE_OS__
        if ( NULL == realpath(in_path,resolved_path) ) {
            return NULL;
        } else {
            return resolved_path;
        }
        #endif

        #ifdef _WIN32
        if ( NULL == _fullpath(resolved_path, in_path, PATH_MAX) ) {
            return NULL;
        } else {
            return resolved_path;
        }
        #endif
    }

    len = strlen(in_path);
    for ( i=(int)len-1; i>=0; i-- ) {
        if ( '/' == in_path[i] || '\\'== in_path[i] ) {
            file_basename = (char*)(in_path+i+1);
            break;
        }
    }
    if ( NULL == file_basename ) {
        return NULL;
    }
    len = strlen(file_basename);
    if ( len > NAME_MAX ) {
        return NULL;
    }
    strncpy(file_dir, in_path, PATH_MAX+NAME_MAX);
    len = strlen(file_dir);
    path = gnb_file_path_cut(file_dir, len);
    #if __UNIX_LIKE_OS__
    if ( NULL == realpath(path,resolved_path) ) {
        return NULL;
    }
    #endif

    #ifdef _WIN32
    if ( NULL == _fullpath(resolved_path, path, PATH_MAX) ) {
        return NULL;
    }
    #endif
    len = strlen(resolved_path);
    resolved_path[len]   = GNB_FILE_SP;
    resolved_path[len+1] = '\0';
    len++;
    for ( i=0; i<NAME_MAX; i++ ) {
        resolved_path[len+i] = file_basename[i];
        if ( '\0' == file_basename[i] ) {
            break;
        }
    }
    return resolved_path;
}
