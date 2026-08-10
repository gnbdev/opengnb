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

#ifndef GNB_MMAP_H
#define GNB_MMAP_H

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#include <stddef.h>
#include <limits.h>
#include "gnb_alloc.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct _gnb_mmap_block_t {
#ifdef __UNIX_LIKE_OS__
	int fd;
#endif
#ifdef _WIN32
	HANDLE file_descriptor;
	HANDLE map_handle;
#endif
	char filename[PATH_MAX];
	void *block;
	size_t block_size;
	int mmap_type;
} gnb_mmap_block_t;

#define GNB_MMAP_TYPE_READONLY               (0x0)
#define GNB_MMAP_TYPE_READWRITE              (0x1)
#define GNB_MMAP_TYPE_CREATE                 (0x1 << 1)
#define GNB_MMAP_TYPE_CLEANEXIT              (0x1 << 2)

int gnb_mmap(gnb_mmap_block_t *mmap_block,const char *filename, size_t block_size, int mmap_type);
void gnb_munmap(gnb_mmap_block_t *mmap_block);

void* gnb_mmap_get_block(gnb_mmap_block_t *mmap_block);

#endif
