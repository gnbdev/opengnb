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

#ifndef GNB_TUN_DRV_H
#define GNB_TUN_DRV_H

typedef struct _gnb_core_t gnb_core_t;

typedef int (*init_tun_func_t)(gnb_core_t *gnb_core);

typedef int (*open_tun_func_t)(gnb_core_t *gnb_core);

typedef int (*read_tun_func_t)(gnb_core_t *gnb_core, void *buf, size_t buf_size);

typedef int (*write_tun_func_t)(gnb_core_t *gnb_core, void *buf, size_t buf_size);

typedef int (*close_tun_func_t)(gnb_core_t *gnb_core);

typedef int (*loop_tun_func_t)(gnb_core_t *gnb_core);

typedef int (*release_tun_func_t)(gnb_core_t *gnb_core);

typedef struct _gnb_tun_drv_t {

	init_tun_func_t  init_tun;

	open_tun_func_t  open_tun;

	read_tun_func_t  read_tun;

	write_tun_func_t write_tun;

	close_tun_func_t close_tun;

	release_tun_func_t release_tun;

}gnb_tun_drv_t;


#if defined(__FreeBSD__)
extern gnb_tun_drv_t gnb_tun_drv_freebsd;
#endif


#if defined(__OpenBSD__)
extern gnb_tun_drv_t gnb_tun_drv_openbsd;
#endif


#if defined(__APPLE__)
extern gnb_tun_drv_t gnb_tun_drv_darwin;
#endif

#if defined(__linux__)
extern gnb_tun_drv_t gnb_tun_drv_linux;
#endif


#if defined(_WIN32)
extern gnb_tun_drv_t gnb_tun_drv_win32;
extern gnb_tun_drv_t gnb_tun_drv_wintun;
#endif

#endif
