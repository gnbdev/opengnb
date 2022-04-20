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
#define WIN32_LEAN_AND_MEAN
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <ws2ipdef.h>
#include <mstcpip.h>
#include <winternl.h>
#include <netioapi.h>

#include "wintun/wintun.h"

#include "gnb.h"
#include "gnb_exec.h"
#include "gnb_tun_drv.h"
#include "gnb_payload16.h"


#define MAX_TRIES 3
#define WINTUN_VERSION (14U)
#define WINTUN_POOL_NAME L"GNB"
#define WINTUN_RING_CAPACITY 0x400000 /* 4 MiB */




static WINTUN_CREATE_ADAPTER_FUNC *WintunCreateAdapter;
static WINTUN_CLOSE_ADAPTER_FUNC *WintunCloseAdapter;
static WINTUN_OPEN_ADAPTER_FUNC *WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC *WintunGetAdapterLUID;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC *WintunGetRunningDriverVersion;
static WINTUN_DELETE_DRIVER_FUNC *WintunDeleteDriver;
static WINTUN_SET_LOGGER_FUNC *WintunSetLogger;
static WINTUN_START_SESSION_FUNC *WintunStartSession;
static WINTUN_END_SESSION_FUNC *WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC *WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC *WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC *WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC *WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC *WintunSendPacket;


static HMODULE
InitializeWintun(gnb_core_t *gnb_core) {
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] LoadLibrary wintun.dll Start...\n");
    HMODULE Wintun = LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!Wintun) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] LoadLibrary wintun.dll Failed ...\n");
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] Please check the wintun.dll Exists in the application directory.\n");
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] Wintun driver download link: https://www.wintun.net/builds/wintun-0.14.1.zip\n");
        return NULL;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] GetProcAddress wintun.dll Start...\n");
#define X(Name) ((*(FARPROC *)&Name = GetProcAddress(Wintun, #Name)) == NULL)
    if (X(WintunCreateAdapter) || X(WintunCloseAdapter) || X(WintunOpenAdapter) || X(WintunGetAdapterLUID) ||
        X(WintunGetRunningDriverVersion) || X(WintunDeleteDriver) || X(WintunSetLogger) || X(WintunStartSession) ||
        X(WintunEndSession) || X(WintunGetReadWaitEvent) || X(WintunReceivePacket) || X(WintunReleaseReceivePacket) ||
        X(WintunAllocateSendPacket) || X(WintunSendPacket))
#undef X
    {
        DWORD LastError = GetLastError();
        FreeLibrary(Wintun);
        SetLastError(LastError);
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] GetProcAddress wintun.dll  Failed...\n");
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] Please check the wintun.dll Exists with the right version v0.14\n");
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[InitializeWintun] Wintun driver download link: https://www.wintun.net/builds/wintun-0.14.1.zip\n");

        return NULL;
    }
    return Wintun;
}



typedef struct _gnb_core_wintun_ctx_t{
    char if_name[PATH_MAX];
    HMODULE drv_module;
    WINTUN_ADAPTER_HANDLE adapter_handle;
    WINTUN_SESSION_HANDLE session;

    HANDLE hQuitEvent;
    HANDLE hReadEvent;

    HANDLE hInterfaceUpEvent;
    HANDLE hInterfaceNotify;

    BOOL  ifDisabled;

    int skip_if_script;

}gnb_core_wintun_ctx_t;


//Static Callback function for NotifyIpInterfaceChange API.
static void WINAPI OnInterfaceChange(PVOID callerContext,
                                     PMIB_IPINTERFACE_ROW row,
                                     MIB_NOTIFICATION_TYPE notificationType)
{
    NET_LUID wintun_luid;
    ULONG wintun_if_index;
    gnb_core_t *gnb_core = (gnb_core_t*)callerContext;
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    WintunGetAdapterLUID(tun_wintun_ctx->adapter_handle, &wintun_luid);
    ConvertInterfaceLuidToIndex(&wintun_luid, &wintun_if_index);
    if(wintun_if_index == row->InterfaceIndex)
    {
        //网卡发生变化
        char *status = "";
        switch (notificationType)
        {
        case MibAddInstance:
            //网卡被启用
            status = "Enabled";
            tun_wintun_ctx->ifDisabled = FALSE;
            SetEvent(tun_wintun_ctx->hInterfaceUpEvent);
            break;
        case MibDeleteInstance:
            //网卡被禁用
            status = "Disabled";
            tun_wintun_ctx->ifDisabled = TRUE;
            break;
        case MibParameterNotification:
        case MibInitialNotification:
        default:
            return;
        }
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[OnInterfaceChange] Interface index:%lu status:%s\n", wintun_if_index, status);
    }

}


static int close_tun_wintun(gnb_core_t *gnb_core);
static int open_tun_wintun(gnb_core_t *gnb_core);
static int release_tun_wintun(gnb_core_t *gnb_core);




int init_tun_wintun(gnb_core_t *gnb_core) {
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Notes: gnb_tun_drv_wintun is a Third party module; technical support:  https://www.github.com/wuqiong\n");

    HMODULE module = InitializeWintun(gnb_core);

    if(NULL == module) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "[init_tun_wintun] InitializeWintun  Failed.\n");
        return -1;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[init_tun_wintun] InitializeWintun  Successful...\n");

    HANDLE quit_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!quit_event) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "[init_tun_wintun] Failed to Create QuitEvent.\n");
        WintunDeleteDriver();
        FreeLibrary(module);
        return -2;
    }
    gnb_core_wintun_ctx_t *tun_wintun_ctx = (gnb_core_wintun_ctx_t *)malloc(sizeof(gnb_core_wintun_ctx_t));
    memset(tun_wintun_ctx, 0, sizeof(gnb_core_wintun_ctx_t));
    gnb_core->platform_ctx = tun_wintun_ctx;
    tun_wintun_ctx->skip_if_script = 0;
    tun_wintun_ctx->hQuitEvent = quit_event;
    tun_wintun_ctx->drv_module = module;


    snprintf(tun_wintun_ctx->if_name, PATH_MAX, "%s", "P2PNet");


    if ( tun_wintun_ctx->if_name && 0 != strncmp(gnb_core->ifname,tun_wintun_ctx->if_name,256) ) {
        snprintf(gnb_core->ifname, 256, "%s", tun_wintun_ctx->if_name);
    }

    if(gnb_core->if_device_string!=NULL) {
        snprintf(gnb_core->if_device_string, 256, "%s", "GNB Device");
    }

    return 0;

}




static void if_up(gnb_core_t *gnb_core) {

    char bin_path[PATH_MAX+NAME_MAX];
    char bin_path_q[PATH_MAX+NAME_MAX];
    char map_path_q[PATH_MAX+NAME_MAX];

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return;

    strncpy(gnb_core->ctl_block->conf_zone->conf_st.ifname, tun_wintun_ctx->if_name, NAME_MAX);

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


static void if_down(gnb_core_t *gnb_core) {

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return;

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


static int set_addr4(gnb_core_t *gnb_core) {

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    unsigned long status = 0;

    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );

    WintunGetAdapterLUID(tun_wintun_ctx->adapter_handle, &ipRow.InterfaceLuid);
    ipRow.Address.si_family  = AF_INET;
    ipRow.OnLinkPrefixLength = (UINT8)ntod( gnb_core->local_node->tun_netmask_addr4.s_addr );

    InetPton(AF_INET, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4), &ipRow.Address.Ipv4.sin_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    //remove old same address on any interface.
    MIB_UNICASTIPADDRESS_TABLE *table = NULL;
    status = GetUnicastIpAddressTable(AF_INET, &table);
    if (status == NO_ERROR) {
        for (ULONG i = 0; i < (table->NumEntries); ++i) {
            if (0 == memcmp(&table->Table[i].Address.Ipv4.sin_addr, &ipRow.Address.Ipv4.sin_addr, sizeof(IN_ADDR)) ) {
                GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr4] Remove Old IPv4 address: %s\n", GNB_ADDR4STR_PLAINTEXT1(&(table->Table[i].Address.Ipv4.sin_addr)));
                DeleteUnicastIpAddressEntry(&table->Table[i]);
            }
        }
        FreeMibTable(table);
    }


    status = CreateUnicastIpAddressEntry(&ipRow);
    if (status != NO_ERROR) {
        char *errorMsg = NULL;
        switch(status)
        {
            case ERROR_INVALID_PARAMETER:
                errorMsg = "ERROR_INVALID_PARAMETER";
                break;
            case ERROR_NOT_FOUND:
                errorMsg = "ERROR_NOT_FOUND";
                break;
            case ERROR_NOT_SUPPORTED:
                errorMsg = "ERROR_NOT_SUPPORTED";
                break;
            case ERROR_OBJECT_ALREADY_EXISTS:
                errorMsg = "ERROR_OBJECT_ALREADY_EXISTS";
                break;
            default:
                errorMsg = "ERROR_Other";
                break;
        }
        GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr4] Failed to set IPv4 address: %s  error:%s\n", GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4), errorMsg);
        return -3;
    }else {
         GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr4] Finished to set IPv4 address: %s\n", GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4));
    }
    return 0;

}


static int set_addr6(gnb_core_t *gnb_core) {

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    unsigned long status = 0;

    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );
    WintunGetAdapterLUID(tun_wintun_ctx->adapter_handle, &ipRow.InterfaceLuid);
    ipRow.Address.si_family = AF_INET6;
    ipRow.OnLinkPrefixLength = 96;

    InetPton(AF_INET6, GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), &ipRow.Address.Ipv6.sin6_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    //remove old same address on any interface.
    MIB_UNICASTIPADDRESS_TABLE *table = NULL;
    status = GetUnicastIpAddressTable(AF_INET6, &table);
    if (status == NO_ERROR) {
        for (ULONG i = 0; i < (table->NumEntries); ++i) {
            if (0 == memcmp(&table->Table[i].Address.Ipv6.sin6_addr, &ipRow.Address.Ipv6.sin6_addr, sizeof(IN6_ADDR)) ) {
                GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr6] Remove Old IPv6 address: %s\n", GNB_ADDR6STR_PLAINTEXT1(&(table->Table[i].Address.Ipv6.sin6_addr)));
                DeleteUnicastIpAddressEntry(&table->Table[i]);
            }
        }
        FreeMibTable(table);
    }


    status = CreateUnicastIpAddressEntry(&ipRow);
    if (status != NO_ERROR) {
        char *errorMsg = NULL;
        switch(status)
        {
            case ERROR_INVALID_PARAMETER:
                errorMsg = "ERROR_INVALID_PARAMETER";
                break;
            case ERROR_NOT_FOUND:
                errorMsg = "ERROR_NOT_FOUND";
                break;
            case ERROR_NOT_SUPPORTED:
                errorMsg = "ERROR_NOT_SUPPORTED";
                break;
            case ERROR_OBJECT_ALREADY_EXISTS:
                errorMsg = "ERROR_OBJECT_ALREADY_EXISTS";
                break;
            default:
                errorMsg = "ERROR_Other";
                break;
        }
        GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr6] Failed to set IPv6 address: %s error:%s\n", GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), errorMsg);
        return -3;
    }else {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[set_addr6] Finished to set IPv6 address: %s\n", GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr));
    }
    return 0;

}


static int open_tun_wintun(gnb_core_t *gnb_core) {
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -2;

    DWORD LastError;
    if ( NULL != tun_wintun_ctx->adapter_handle ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] Wintun Adapter Handle Exist.\n");
        return -1;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunCreateAdapter Start...\n");

    struct in_addr addr4 = gnb_core->local_node->tun_addr4;
    GUID Guid = { 0xdeadbabe, 0xcafe, 0xbeef, { 0x01, 0x23, 0x45, 0x67, addr4.S_un.S_un_b.s_b1, addr4.S_un.S_un_b.s_b2, addr4.S_un.S_un_b.s_b3, addr4.S_un.S_un_b.s_b4 } };

    const size_t cSize = strlen(tun_wintun_ctx->if_name) + 1;
    wchar_t w_if_name[cSize];
    mbstowcs (w_if_name, tun_wintun_ctx->if_name, cSize);

    WINTUN_ADAPTER_HANDLE Adapter = NULL;
    int try_count = MAX_TRIES;
    while (NULL == Adapter && try_count > 0) {
        Adapter =  WintunCreateAdapter(w_if_name, WINTUN_POOL_NAME, &Guid);
        try_count--;
    }

    if (NULL == Adapter) {
        DWORD lastError = GetLastError();
        if( lastError == ERROR_OBJECT_ALREADY_EXISTS) {
            GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunCreateAdapter Failed! Already Exist.\n");
            return -1;
        }
        else {
            GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunCreateAdapter Failed: %ld\n", lastError);
            return -2;
        }
    }
    DWORD version = WintunGetRunningDriverVersion();
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] Wintun Driver Version Loaded: v%u.%u\n", (version >> 16) & 0xff, (version >> 0) & 0xff);
    if(version != WINTUN_VERSION) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] Wintun Driver Version Mismatch, Expected: v%u.%u\n", (WINTUN_VERSION >> 16) & 0xff, (WINTUN_VERSION >> 0) & 0xff);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] Wintun Driver Download Link: https://www.wintun.net/builds/wintun-0.14.1.zip\n");
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunCreateAdapter Successful...\n");
    tun_wintun_ctx->adapter_handle = Adapter;

    DWORD status4 = set_addr4(gnb_core);
    if(0 != status4) {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return status4;
    }
    DWORD status6 = set_addr6(gnb_core);
    if(0 != status6) {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return status6;
    }

    WINTUN_SESSION_HANDLE Session = WintunStartSession(tun_wintun_ctx->adapter_handle, WINTUN_RING_CAPACITY);
    if (!Session) {
        GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunStartSession Failed:%ld\n", GetLastError());
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return -4;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[open_tun_wintun] WintunStartSession Successful.\n");
    tun_wintun_ctx->session = Session;

    tun_wintun_ctx->hReadEvent = WintunGetReadWaitEvent(Session);

    if ( !tun_wintun_ctx->skip_if_script ) {
        if_up(gnb_core);
    }

    //注册网卡状态变化通知,监控网卡被禁用启动事件
    HANDLE hInterfaceNotify = NULL;
    DWORD ret = NotifyIpInterfaceChange(
                AF_INET, // IPv4 and IPv6
                (PIPINTERFACE_CHANGE_CALLBACK)OnInterfaceChange,
                gnb_core,  // pass to callback
                FALSE, // no initial notification
                &hInterfaceNotify);
    if(NO_ERROR == ret) {
        tun_wintun_ctx->hInterfaceNotify = hInterfaceNotify;
        tun_wintun_ctx->hInterfaceUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    return 0;
}


static int read_tun_wintun(gnb_core_t *gnb_core, void *buf, size_t buf_size) {
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    if(NULL == tun_wintun_ctx->adapter_handle) return -1;

    if(TRUE == tun_wintun_ctx->ifDisabled) {
        if(WAIT_TIMEOUT == WaitForSingleObject(tun_wintun_ctx->hInterfaceUpEvent, INFINITE)) {
            return 0;
        }
    }

    WINTUN_SESSION_HANDLE session = (WINTUN_SESSION_HANDLE)tun_wintun_ctx->session;
    if(NULL == session)
    {
        //可能中途虚拟网卡或虚拟设备被禁用,Session被关闭,
        //网卡状态恢复启动时, 需要重新打开Session.
        session = WintunStartSession(tun_wintun_ctx->adapter_handle, WINTUN_RING_CAPACITY);
        if (NULL == session) {
            //GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] WintunStartSession Failed:%ld\n", GetLastError());
            return -1;
        }
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] WintunStartSession Successful.\n");
        tun_wintun_ctx->session = session;
        tun_wintun_ctx->hReadEvent = WintunGetReadWaitEvent(session);

        set_addr4(gnb_core);
        set_addr6(gnb_core);
    }

    HANDLE WaitHandles[] = { tun_wintun_ctx->hReadEvent, tun_wintun_ctx->hQuitEvent };

    DWORD packet_size;
    BYTE *packet = WintunReceivePacket(session, &packet_size);
    if(NULL != packet) {
        size_t real_size = (packet_size > buf_size)?buf_size:packet_size;
        memcpy(buf, packet, real_size);
        WintunReleaseReceivePacket(session, packet);
        //GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] bufsize:%d, WintunReceivePacket Actual Size:%d\n", buf_size,  packet_size);
        return real_size;
    }else {
        DWORD LastError  = GetLastError();
        switch (LastError) {
            case ERROR_NO_MORE_ITEMS:
                //GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] WintunReceivePacket NO_MORE_ITEMS:%ld\n", LastError);
                if (WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0) {
                    return ERROR_SUCCESS;
                }
                break;
            case ERROR_HANDLE_EOF:
                //控制面板->网络链接中   禁用了该网卡
                //设备管理器->网络适配器  禁用了该设备
                //此两种场景需要关闭Session,后续尝试再次打开Session.
                GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] WintunReceivePacket ERROR_HANDLE_EOF:%ld\n", LastError);
                WintunEndSession(session);
                tun_wintun_ctx->session = NULL;
                tun_wintun_ctx->hReadEvent = NULL;
                return ERROR_SUCCESS;
            default:
                GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[read_tun_wintun] WintunReceivePacket Failed:%ld\n", LastError);
                return ERROR_SUCCESS;
        }
    }
    return ERROR_SUCCESS;
}


static int write_tun_wintun(gnb_core_t *gnb_core, void *buf, size_t buf_size){
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    WINTUN_SESSION_HANDLE session = (WINTUN_SESSION_HANDLE)tun_wintun_ctx->session;
    if(NULL == session) return -1;

    BYTE *Packet = WintunAllocateSendPacket(session, buf_size);
    memcpy(Packet, buf, buf_size);
    DWORD LastError = GetLastError();
    if (NULL != Packet){
        WintunSendPacket(session, Packet);
        //GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[write_tun_wintun] WintunAllocateSendPacket Size:%d.\n", buf_size);
        return 0;
    }
    else if (LastError != ERROR_BUFFER_OVERFLOW){
        //GNB_ERROR3(gnb_core->log, GNB_LOG_ID_CORE, "[write_tun_wintun] WintunAllocateSendPacket Size:%d, Packet Write Failed.\n", buf_size);
        return 0;
    }
    else{
        return 0;
    }
}


static int close_tun_wintun(gnb_core_t *gnb_core){
    gnb_core->loop_flag = 0;
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    if ( !tun_wintun_ctx->skip_if_script ) {
        if_down(gnb_core);
    }

    if(NULL != tun_wintun_ctx->hReadEvent) {
        SetEvent(tun_wintun_ctx->hReadEvent);
        tun_wintun_ctx->hReadEvent = NULL;
    }

    if(NULL != tun_wintun_ctx->hInterfaceNotify)
    {
        CancelMibChangeNotify2(tun_wintun_ctx->hInterfaceNotify);
        tun_wintun_ctx->hInterfaceNotify = NULL;
    }

    if(NULL != tun_wintun_ctx->hInterfaceUpEvent)
    {
        SetEvent(tun_wintun_ctx->hInterfaceUpEvent);
        CloseHandle(tun_wintun_ctx->hInterfaceUpEvent);
        tun_wintun_ctx->hInterfaceUpEvent = NULL;
    }

    if(NULL != tun_wintun_ctx->hQuitEvent) {
        SetEvent(tun_wintun_ctx->hQuitEvent);
        CloseHandle(tun_wintun_ctx->hReadEvent);
        tun_wintun_ctx->hQuitEvent = NULL;
    }

    if(NULL != tun_wintun_ctx->session) {
        WintunEndSession(tun_wintun_ctx->session);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[close_tun_wintun] WintunEndSession.\n");
        tun_wintun_ctx->session = NULL;
    }

    if(NULL != tun_wintun_ctx->adapter_handle) {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[close_tun_wintun] WintunCloseAdapter.\n");
        tun_wintun_ctx->adapter_handle = NULL;
    }

    return release_tun_wintun(gnb_core);
}

static int release_tun_wintun(gnb_core_t *gnb_core) {
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if(NULL == tun_wintun_ctx) return -1;

    if (NULL != tun_wintun_ctx) {
        if (NULL != tun_wintun_ctx->drv_module) {
            FreeLibrary(tun_wintun_ctx->drv_module);
            tun_wintun_ctx->drv_module = NULL;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "[release_tun_wintun] FreeLibrary.\n");
        }
        free(gnb_core->platform_ctx);
        gnb_core->platform_ctx = NULL;
    }
    return 0;
}

gnb_tun_drv_t gnb_tun_drv_wintun = {

    init_tun_wintun,

    open_tun_wintun,

    read_tun_wintun,

    write_tun_wintun,

    close_tun_wintun,

    release_tun_wintun

};
