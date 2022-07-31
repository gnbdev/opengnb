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
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "gnb_platform.h"
#include "gnb_dir.h"

#if __UNIX_LIKE_OS__
#define GNB_FILE_SP '/'
#endif

#ifdef _WIN32

#define GNB_FILE_SP '\\'

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#endif


char *gnb_get_file_dir(char *in_file_name, char *file_dir){

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

    for ( i=string_len-1; i>0; i-- ) {

        if ( GNB_FILE_SP==file_dir[i] ) {
            file_dir[i]='\0';
            break;
        }

    }

    return file_dir;

}


char *gnb_get_file_dir_dup(char *file_name){

    char *string = strdup(file_name);

    char *resolved_path = (char *)malloc(PATH_MAX);

    size_t string_len = strlen( (const char *)string );

    int i;

    for ( i=string_len-1; i>0; i-- ) {

        if ( '/'==string[i] || '\\'==string[i] ) {
            string[i]='\0';
            break;
        }

    }

    #if __UNIX_LIKE_OS__
    if ( NULL == realpath(string,resolved_path) ) {
        free(string);
        free(resolved_path);
        return NULL;
    }
    free(string);
    return resolved_path;
    #endif

    #ifdef _WIN32
    if ( NULL == _fullpath(resolved_path, string, PATH_MAX) ) {
        free(string);
        free(resolved_path);
        return NULL;
    }
    free(string);
    return resolved_path;
    #endif

}


char *gnb_realpath(char *in_path, char *resolved_path){

    int ret;
    size_t len;
    char *path;
    struct stat st;
    char *file_basename = NULL;
    char file_dir[PATH_MAX+NAME_MAX];

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

    int i;

    len = strlen(in_path);

    for ( i=(int)len-1; i>=0; i-- ) {

        if ( GNB_FILE_SP == in_path[i] ) {
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

    for (i=0; i<NAME_MAX; i++) {

        resolved_path[len+i] = file_basename[i];

        if ( '\0' == file_basename[i]) {
            break;
        }

    }

    return resolved_path;

}


char *gnb_realpath_dup(char *path){

    char *resolved_path = (char *)malloc(PATH_MAX);

    #if __UNIX_LIKE_OS__
    if ( NULL == realpath(path,resolved_path) ) {
        free(resolved_path);
        return NULL;
    }
    return resolved_path;
    #endif

    #ifdef _WIN32
    if ( NULL == _fullpath(resolved_path, path, PATH_MAX) ) {
        free(resolved_path);
        return NULL;
    }
    return resolved_path;
    #endif

}


char *gnb_make_realpath_dup(char *base_path, char *sub_path){

    char *tmp_buffer;
    char *realpath;

    tmp_buffer = (char *)malloc(PATH_MAX);

    snprintf(tmp_buffer, PATH_MAX,"%s/%s", base_path, sub_path);
    realpath = gnb_realpath_dup(tmp_buffer);

    free(tmp_buffer);

    return realpath;

}



//获取目录下的子目录，不包含普通文件
//计划废弃
int gnb_get_subdirname(char *path, int max_filename_len, char **filename_lst, int *filename_lst_len){

    DIR *dir;

    struct dirent *sub_dirent;

    #ifdef _WIN32
    struct stat st;
    char test_file_name[FILENAME_MAX];
    int ret;
    #endif

    dir = opendir (path);

    if ( NULL==dir ) {
        *filename_lst_len = 0;
        return -1;
    }

    int list_len = 0;

    while ( NULL !=( sub_dirent = readdir( dir ) ) ) {


        #if __UNIX_LIKE_OS__
        if ( DT_DIR != sub_dirent->d_type ) {
          continue;
        }
        #endif

        #ifdef _WIN32
        snprintf(test_file_name, max_filename_len, "%s/%s", path, sub_dirent->d_name);

        ret = stat(test_file_name,&st);

        if (-1==ret) {
            continue;
        }

        if ( !S_ISDIR(st.st_mode) ) {
            continue;
        }
        #endif

        if (strlen(sub_dirent->d_name)>max_filename_len) {
            continue;
        }

        if (0==strncmp(sub_dirent->d_name,".",max_filename_len)) {
            continue;
        }

        if (0==strncmp(sub_dirent->d_name,"..",max_filename_len)) {
            continue;
        }

        memset(filename_lst[list_len],0,max_filename_len);

        strncpy(filename_lst[list_len],sub_dirent->d_name,256);

        if ( list_len >= *filename_lst_len ) {
            break;
        }

        list_len++;

    }

    closedir( dir );

    *filename_lst_len = list_len;

   return 0;

}



int gnb_get_dir_file_names(char *path, gnb_file_info_t **sub_file_info_lst, int *lst_len_ptr){

    struct stat st;

    DIR *dir;

    int filename_len;
    int abs_filename_len;

    char abs_filename[GNB_MAX_FILE_NAME_LEN];

    struct dirent *sub_dirent;

    uint8_t file_type;

    int ret;

    size_t path_len = strlen(path);

    if ( 0 == path_len ) {
        return -1;
    }

    dir = opendir(path);

    if ( NULL==dir ) {
        *lst_len_ptr = 0;
        return -1;
    }

    int idx = 0;

    while ( NULL !=( sub_dirent = readdir( dir ) ) ) {

        if ( idx >= *lst_len_ptr ) {
            break;
        }

        if ( 0==strncmp(sub_dirent->d_name,".",GNB_MAX_FILE_NAME_LEN) ) {
            continue;
        }

        if (0==strncmp(sub_dirent->d_name,"..",GNB_MAX_FILE_NAME_LEN)) {
            continue;
        }

        #if __UNIX_LIKE_OS__

        if ( '/' == path[path_len-1] || '\\' == path[path_len-1] ) {
            snprintf(abs_filename, GNB_MAX_FILE_NAME_LEN, "%s%s", path, sub_dirent->d_name);
        } else {
            snprintf(abs_filename, GNB_MAX_FILE_NAME_LEN, "%s/%s", path, sub_dirent->d_name);
        }

        ret = lstat(abs_filename,&st);

        #endif

        #ifdef _WIN32

        if ( '/' == path[path_len-1] || '\\' == path[path_len-1] ) {
            snprintf(abs_filename, GNB_MAX_FILE_NAME_LEN, "%s%s", path, sub_dirent->d_name);
        } else {
            snprintf(abs_filename, GNB_MAX_FILE_NAME_LEN, "%s\\%s", path, sub_dirent->d_name);
        }

        ret = stat(abs_filename,&st);
        #endif

        if ( -1==ret ) {
            continue;
        }

        if ( S_ISREG(st.st_mode) ) {
            file_type = GNB_FILE_TYPE_REG;
        } else if ( S_ISDIR(st.st_mode) ) {
            file_type = GNB_FILE_TYPE_DIR;

        #if __UNIX_LIKE_OS__
        } else if (S_ISLNK(st.st_mode)) {
            file_type = GNB_FILE_TYPE_LNK;
        #endif
        } else {
            file_type = GNB_FILE_TYPE_INIT;
        }

        if ( GNB_FILE_TYPE_REG != file_type && GNB_FILE_TYPE_DIR != file_type ) {
            continue;
        }

        sub_file_info_lst[idx] = (gnb_file_info_t *)malloc(sizeof(gnb_file_info_t));

        filename_len      = strlen(sub_dirent->d_name)  + 1;
        abs_filename_len  = strlen(path) + filename_len + 1;

        sub_file_info_lst[idx]->name     = (char *)malloc(filename_len);
        snprintf(sub_file_info_lst[idx]->name, filename_len, "%s",   sub_dirent->d_name);

        sub_file_info_lst[idx]->abs_name = (char *)malloc(abs_filename_len);

        snprintf(sub_file_info_lst[idx]->abs_name, abs_filename_len, "%s", abs_filename);

        sub_file_info_lst[idx]->type = file_type;

        if ( idx >= *lst_len_ptr ) {
            break;
        }

        idx++;

    }

    closedir(dir);

    *lst_len_ptr = idx;

    return 0;

}


int gnb_scan_dir(char *path, gnb_file_info_t **file_info_lst, int *lst_len_ptr){

    gnb_file_info_t **sub_file_info_lst = file_info_lst;

    int max_lst_len = *lst_len_ptr;

    int sub_lst_len = max_lst_len;

    int lst_len = 0;

    gnb_get_dir_file_names(path, sub_file_info_lst, &sub_lst_len);

    lst_len += sub_lst_len;

    sub_file_info_lst += sub_lst_len;

    sub_lst_len        = max_lst_len - lst_len;

    int i;

    for ( i=0; i<lst_len; i++ ) {

        if ( GNB_FILE_TYPE_DIR != file_info_lst[i]->type ) {
            continue;
        }

        gnb_get_dir_file_names(file_info_lst[i]->abs_name, sub_file_info_lst, &sub_lst_len);

        if ( (lst_len+sub_lst_len) >= max_lst_len ) {
            break;
        }

        lst_len += sub_lst_len;

        sub_file_info_lst += sub_lst_len;

        sub_lst_len        = max_lst_len - lst_len;

    }

    *lst_len_ptr = lst_len;

    return 0;

}


char *gnb_file_path_cut(char *filename, size_t len){

    int i;

    if ( 0==len ) {
        return NULL;
    }

    for (i=(int)len-1; i>=0; i--) {

        if ( GNB_FILE_SP == filename[i] ) {
            filename[i]='\0';
            return filename;
        }
    }

    return NULL;

}


char *gnb_file_path_dup(const char *filename, size_t len){

    int i;

    if (0==len) {
        return NULL;
    }

    char *path_dup=strdup(filename);

    for (i=(int)len-1; i>=0; i--) {
        if ( GNB_FILE_SP == path_dup[i] ) {
            path_dup[i]='\0';
            return path_dup;
        }
    }

    free(path_dup);

    return NULL;

}


void gnb_release_file_info_lst(gnb_file_info_t **file_info_lst, int lst_len){

    int i;

    for ( i=0; i<lst_len; i++ ) {

        free(file_info_lst[i]->name);
        free(file_info_lst[i]->abs_name);
        free(file_info_lst[i]);

    }

    free(file_info_lst);

}
