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
#include <limits.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <winioctl.h>

#include <iphlpapi.h>

#include "tap-windows/tap-windows.h"

#include "gnb.h"
#include "gnb_exec.h"
#include "gnb_tun_drv.h"
#include "gnb_payload16.h"


typedef struct _gnb_core_win_ctx_t{

    char tap_file_name[PATH_MAX];

    char if_name[PATH_MAX];

    char deviceid[PATH_MAX];

    ULONG tun_if_id;

    HANDLE device_handle;

    OVERLAPPED read_olpd;
    OVERLAPPED write_olpd;

    int skip_if_script;

}gnb_core_win_ctx_t;


static int close_tun_win32(gnb_core_t *gnb_core);
static int open_tun_win32(gnb_core_t *gnb_core);


#define MAX_KEY_LENGTH 255

/* {xxxxxxxxxxxx} 形式的字符串 */
static char *get_if_deviceid() {

    HKEY adapters, adapter;
    DWORD i, ret, len;
    char *deviceid = NULL;
    DWORD sub_keys = 0;

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(ADAPTER_KEY), 0, KEY_READ, &adapters);

    if (ret != ERROR_SUCCESS) {
        return NULL;
    }

    ret = RegQueryInfoKey(adapters,    NULL, NULL, NULL, &sub_keys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != ERROR_SUCCESS) {
        return NULL;
    }

    if (sub_keys <= 0) {
        return NULL;
    }

    /* Walk througt all adapters */
    for (i = 0; i < sub_keys; i++) {
        char new_key[MAX_KEY_LENGTH];
        char data[256];
        TCHAR key[MAX_KEY_LENGTH];
        DWORD keylen = MAX_KEY_LENGTH;

        /* Get the adapter key name */
        ret = RegEnumKeyEx(adapters, i, key, &keylen, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS) {
            continue;
        }

        /* Append it to NETWORK_ADAPTERS and open it */
        snprintf(new_key, sizeof(new_key), "%s\\%s", ADAPTER_KEY, key);

        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(new_key), 0, KEY_READ, &adapter);
        if (ret != ERROR_SUCCESS) {
            continue;
        }

        len = sizeof(data);
        ret = RegQueryValueEx(adapter, "DriverDesc", NULL, NULL, (LPBYTE)data, &len);
        if (ret != ERROR_SUCCESS) {
            goto clean;
        }

        if (strncmp(data, "TAP-Windows", sizeof("TAP-Windows")-1) == 0) {

            DWORD type;

            len = sizeof(data);
            ret = RegQueryValueEx(adapter, "NetCfgInstanceId", NULL, &type, (LPBYTE)data, &len);
            if (ret != ERROR_SUCCESS) {
                goto clean;
            }
            deviceid = strdup(data);
            break;
        }

clean:
        RegCloseKey(adapter);
    }

    RegCloseKey(adapters);

    return deviceid;

}

/* 网络连接的名字 */
static char* get_if_devicename(const char *if_deviceid){

    HKEY key;
    DWORD ret;

    char regpath[MAX_KEY_LENGTH];

    char devicename[PATH_MAX];
    long name_len = PATH_MAX;

    snprintf(regpath, MAX_KEY_LENGTH, "%s\\%s\\Connection", NETWORK_CONNECTIONS_KEY, if_deviceid);

    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(regpath), 0, KEY_READ, &key);

    if (ret != ERROR_SUCCESS) {
        RegCloseKey(key);
        return NULL;
    }

    ret = RegQueryValueEx(key, "Name", 0, 0, devicename, &name_len);

    if (ret != ERROR_SUCCESS) {
        RegCloseKey(key);
        return NULL;
    }

    RegCloseKey(key);

    return strdup(devicename);
}


int init_tun_win32(gnb_core_t *gnb_core){

    char *deviceid;

    char *devicename;

    gnb_core_win_ctx_t *tun_win_ctx = (gnb_core_win_ctx_t *)malloc(sizeof(gnb_core_win_ctx_t));

    tun_win_ctx->skip_if_script = 0;

    tun_win_ctx->device_handle = INVALID_HANDLE_VALUE;

    gnb_core->platform_ctx = tun_win_ctx;

    deviceid = get_if_deviceid();

    if ( NULL==deviceid ) {
        return -1;
    }

    snprintf(tun_win_ctx->deviceid, PATH_MAX, "%s", deviceid);

    snprintf(tun_win_ctx->tap_file_name, PATH_MAX, "%s%s%s", USERMODEDEVICEDIR, deviceid, TAP_WIN_SUFFIX);

    devicename = get_if_devicename(deviceid);

    if ( NULL != devicename ) {
        snprintf(tun_win_ctx->if_name, PATH_MAX, "%s", devicename);
        free(devicename);
    }

    if ( tun_win_ctx->if_name && 0 != strncmp(gnb_core->ifname,tun_win_ctx->if_name,256) ) {
        snprintf(gnb_core->ifname, 256, "%s", tun_win_ctx->if_name);
    }

    snprintf(gnb_core->if_device_string, 256, "%s", deviceid);

    free(deviceid);

    return 0;

}


#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* 网络接口id */
static ULONG get_if_id(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    ULONG family = AF_UNSPEC; //AF_INET AF_INET6 AF_UNSPEC

    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_INCLUDE_GATEWAYS;

    outBufLen = WORKING_BUFFER_SIZE;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;

    struct sockaddr_in *sa_in;
    struct sockaddr_in6 *sa_in6;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);

        if (pAddresses == NULL) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal != NO_ERROR) {
        return 0l;
    }

    pCurrAddresses = pAddresses;

    char friendlyname_string[256];
    char description_string[256];

    char ipv4_string[INET_ADDRSTRLEN];
    char ipv6_string[INET6_ADDRSTRLEN];

    tun_win_ctx->tun_if_id = -1;

    while (pCurrAddresses) {

        if (IfOperStatusUp != pCurrAddresses->OperStatus) {
            goto next;
        }

        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,pCurrAddresses->Description,-1,description_string,256,NULL,NULL);
        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,pCurrAddresses->FriendlyName,-1,friendlyname_string,256,NULL,NULL);

        if ( 0==strncmp( description_string,"TAP-Windows Adapter V9", sizeof("TAP-Windows Adapter V9")-1 ) ) {

            //需要比较 pCurrAddresses->AdapterName 找出真正的 tun_if_id
            if ( !strncmp(pCurrAddresses->AdapterName, tun_win_ctx->deviceid, PATH_MAX) ) {
                return pCurrAddresses->IfIndex;
            }

            goto next;
        }

next:

        pCurrAddresses = pCurrAddresses->Next;

    }

    FREE(pAddresses);

    return 0l;

}



static void if_up(gnb_core_t *gnb_core){

    char bin_path[PATH_MAX+NAME_MAX];
    char bin_path_q[PATH_MAX+NAME_MAX];
    char map_path_q[PATH_MAX+NAME_MAX];

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    strncpy(gnb_core->ctl_block->conf_zone->conf_st.ifname, tun_win_ctx->if_name, NAME_MAX);

    snprintf(bin_path,   PATH_MAX+NAME_MAX, "%s\\gnb_es.exe",     gnb_core->ctl_block->conf_zone->conf_st.binary_dir);
    snprintf(bin_path_q, PATH_MAX+NAME_MAX, "\"%s\\gnb_es.exe\"", gnb_core->ctl_block->conf_zone->conf_st.binary_dir);
    snprintf(map_path_q, PATH_MAX+NAME_MAX, "\"%s\"",             gnb_core->ctl_block->conf_zone->conf_st.map_file);

    gnb_arg_list_t *arg_list;
    arg_list = gnb_arg_list_init(32);

    gnb_arg_append(arg_list, bin_path_q);

    gnb_arg_append(arg_list, "-b");
    gnb_arg_append(arg_list, map_path_q);

    gnb_arg_append(arg_list, "--if-up");

    gnb_exec(bin_path, gnb_core->ctl_block->conf_zone->conf_st.binary_dir, arg_list, GNB_EXEC_BACKGROUND|GNB_EXEC_WAIT);

    gnb_arg_list_release(arg_list);

}


static void if_down(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    char bin_path[PATH_MAX+NAME_MAX];
    char bin_path_q[PATH_MAX+NAME_MAX];
    char map_path_q[PATH_MAX+NAME_MAX];

    snprintf(bin_path,   PATH_MAX+NAME_MAX, "%s\\gnb_es.exe",     gnb_core->ctl_block->conf_zone->conf_st.binary_dir);
    snprintf(bin_path_q, PATH_MAX+NAME_MAX, "\"%s\\gnb_es.exe\"", gnb_core->ctl_block->conf_zone->conf_st.binary_dir);
    snprintf(map_path_q, PATH_MAX+NAME_MAX, "\"%s\"",             gnb_core->ctl_block->conf_zone->conf_st.map_file);

    gnb_arg_list_t *arg_list;
    arg_list = gnb_arg_list_init(32);

    gnb_arg_append(arg_list, bin_path_q);

    gnb_arg_append(arg_list, "-b");
    gnb_arg_append(arg_list, map_path_q);

    gnb_arg_append(arg_list, "--if-down");

    gnb_exec(bin_path, gnb_core->ctl_block->conf_zone->conf_st.binary_dir, arg_list, GNB_EXEC_BACKGROUND|GNB_EXEC_WAIT);

    gnb_arg_list_release(arg_list);

}


static int ntod(uint32_t mask) {
    int i, n = 0;
    int bits = sizeof(uint32_t) * 8;

    for (i = bits - 1; i >= 0; i--) {
        if (mask & (0x01 << i))
            n++;
    }

    return n;
}


static int set_addr4(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    DWORD dwRetVal = 0;

    DWORD dwSize = 0;
    unsigned long status = 0;

    DWORD lastError = 0;
    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );

    ipRow.InterfaceIndex = tun_win_ctx->tun_if_id;

    ipRow.Address.si_family  = AF_INET;

    ipRow.OnLinkPrefixLength = (UINT8)ntod( gnb_core->local_node->tun_netmask_addr4.s_addr );

    InetPton(AF_INET, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4), &ipRow.Address.Ipv4.sin_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    status = CreateUnicastIpAddressEntry(&ipRow);

    return 0;

}


static int set_addr6(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    DWORD dwRetVal = 0;

    DWORD dwSize = 0;
    unsigned long status = 0;

    DWORD lastError = 0;
    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );

    ipRow.InterfaceIndex = tun_win_ctx->tun_if_id;

    ipRow.Address.si_family = AF_INET6;
    ipRow.OnLinkPrefixLength = 96;

    InetPton(AF_INET6, GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), &ipRow.Address.Ipv6.sin6_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    status = CreateUnicastIpAddressEntry(&ipRow);

    return 0;

}


static int open_tun_win32(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    int ret;

    if ( INVALID_HANDLE_VALUE != tun_win_ctx->device_handle ) {
        return -1;
    }

    tun_win_ctx->device_handle = CreateFile(tun_win_ctx->tap_file_name, GENERIC_WRITE|GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM|FILE_FLAG_OVERLAPPED, 0);

    if ( INVALID_HANDLE_VALUE == tun_win_ctx->device_handle ) {
        return -2;
    }

    ULONG data[4];
    DWORD len;

    data[0] = 1;
    ret = DeviceIoControl(tun_win_ctx->device_handle, TAP_WIN_IOCTL_SET_MEDIA_STATUS, data, sizeof(data), data, sizeof(data), &len, NULL);

    if ( ret == 0 ) {
        //GetLastError();
        return -4;
    }

    if ( data[0]>0 ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Device[%s] up succeed\n", tun_win_ctx->tap_file_name);
    }

    tun_win_ctx->read_olpd.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

    tun_win_ctx->write_olpd.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    tun_win_ctx->tun_if_id = get_if_id(gnb_core);

    gnb_core->ctl_block->core_zone->tun_if_id = tun_win_ctx->tun_if_id;


    int netmask = gnb_core->local_node->tun_netmask_addr4.s_addr;
    data[0] = gnb_core->local_node->tun_addr4.s_addr;
    data[1] = data[0] & netmask;
    data[2] = netmask;

    ret = DeviceIoControl(tun_win_ctx->device_handle, TAP_WIN_IOCTL_CONFIG_TUN, &data, sizeof(data), &data, sizeof(data), &len, NULL);

    if ( 0==ret ) {
        return -5;
    }

    if ( tun_win_ctx->tun_if_id > 0 ) {
        set_addr4(gnb_core);
        set_addr6(gnb_core);

    }

    if ( !tun_win_ctx->skip_if_script ) {
        if_up(gnb_core);
    }

    return 0;

}


static int read_tun_win32(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    int res;

    DWORD frame_size = 0;

    res = ReadFile(tun_win_ctx->device_handle, buf, buf_size, &frame_size, &tun_win_ctx->read_olpd);

    if (!res) {

        res = WaitForSingleObject(tun_win_ctx->read_olpd.hEvent, INFINITE);
        //res = WaitForSingleObject(tun_win_ctx->read_olpd.hEvent, 100);

        if ( WAIT_TIMEOUT == res ) {
            frame_size = -1;
            goto finish;
        }

        res = GetOverlappedResult(tun_win_ctx->device_handle, &tun_win_ctx->read_olpd, (LPDWORD) &frame_size, FALSE);

        if ( frame_size<=0 ) {

            tun_win_ctx->skip_if_script = 0;

            close_tun_win32(gnb_core);

            open_tun_win32(gnb_core);

            tun_win_ctx->skip_if_script = 1;

            GNB_ERROR3(gnb_core->log,GNB_LOG_ID_CORE,"read_tun_win32 ERROR frame_size=%d\n", frame_size);

            goto finish;

        }

    } else {

        goto finish;

    }

finish:

    return frame_size;

}


static int write_tun_win32(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    DWORD written;

    int res;

    res = WriteFile(tun_win_ctx->device_handle, buf, buf_size, &written, &tun_win_ctx->write_olpd);

    if (!res && GetLastError() == ERROR_IO_PENDING) {

        WaitForSingleObject(tun_win_ctx->write_olpd.hEvent, INFINITE);

        res = GetOverlappedResult(tun_win_ctx->device_handle, &tun_win_ctx->write_olpd, &written, FALSE);

        if ( written != buf_size ) {
          GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "write_tun_win32 ERROR buf_size=%d written=%d\n", buf_size,written);
          return -1;
        }

    }

    return 0;

}


static int close_tun_win32(gnb_core_t *gnb_core){

    gnb_core_win_ctx_t *tun_win_ctx = gnb_core->platform_ctx;

    if (INVALID_HANDLE_VALUE==tun_win_ctx->device_handle){
        GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE,"if[%s] aready closeed\n", gnb_core->ifname);
        return 0;
    }

    int ret;

    unsigned long status = 0l;

    DWORD len;

    ret = DeviceIoControl(tun_win_ctx->device_handle, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof(status), &status, sizeof(status), &len, NULL);

    if ( ret == 0 ) {
        //GetLastError();
        return -1;
    }

    if ( status>0 ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Device[%s] down succeed\n",tun_win_ctx->tap_file_name);
    }

    CloseHandle(tun_win_ctx->device_handle);

    tun_win_ctx->device_handle = INVALID_HANDLE_VALUE;

    if ( !tun_win_ctx->skip_if_script ) {
        if_down(gnb_core);
    }

    return 0;

}

static int release_tun_win32(gnb_core_t *gnb_core){

    free(gnb_core->platform_ctx);

    return 0;
}

gnb_tun_drv_t gnb_tun_drv_win32 = {

    init_tun_win32,

    open_tun_win32,

    read_tun_win32,

    write_tun_win32,

    close_tun_win32,

    release_tun_win32

};
