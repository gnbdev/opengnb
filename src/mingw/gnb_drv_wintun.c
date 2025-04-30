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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#define WIN32_LEAN_AND_MEAN

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

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

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
InitializeWintun(gnb_core_t *gnb_core){
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "LoadLibrary wintun.dll  Start ...\n");
    HMODULE Wintun =
        LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!Wintun)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "LoadLibrary wintun.dll  Failed ...\n");
        return NULL;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "GetProcAddress wintun.dll  Start...\n");
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
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "GetProcAddress wintun.dll  Failed...\n");
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

    int skip_if_script;

}gnb_core_wintun_ctx_t;


static int close_tun_wintun(gnb_core_t *gnb_core);
static int open_tun_wintun(gnb_core_t *gnb_core);
static int release_tun_wintun(gnb_core_t *gnb_core);


#define MAX_KEY_LENGTH 255



int init_tun_wintun(gnb_core_t *gnb_core){

    gnb_core_wintun_ctx_t *tun_wintun_ctx = (gnb_core_wintun_ctx_t *)malloc(sizeof(gnb_core_wintun_ctx_t));
    memset(tun_wintun_ctx, 0, sizeof(gnb_core_wintun_ctx_t));

    tun_wintun_ctx->skip_if_script = 0;

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Notes: gnb_drv_wintun.c is a Third party module; technical support:  https://www.github.com/wuqiong\n");

    HMODULE module = InitializeWintun(gnb_core);

    if(NULL == module) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "InitializeWintun  failed!\n");
        WintunDeleteDriver();
        return -1;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "InitializeWintun  success...\n");


    HANDLE quit_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!quit_event)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Failed to create quitevent!\n");
        WintunDeleteDriver();
        return -2;
    }

    tun_wintun_ctx->hQuitEvent = quit_event;


    tun_wintun_ctx->drv_module = module;

    gnb_core->platform_ctx = tun_wintun_ctx;

    snprintf(tun_wintun_ctx->if_name, PATH_MAX, "%s", "P2PNet");


    if ( tun_wintun_ctx->if_name && 0 != strncmp(gnb_core->ifname,tun_wintun_ctx->if_name,256) ) {
        snprintf(gnb_core->ifname, 256, "%s", tun_wintun_ctx->if_name);
    }

    if(gnb_core->if_device_string!=NULL)
    {
        snprintf(gnb_core->if_device_string, 256, "%s", "P2PNet Device");
    }

    return 0;

}





static void if_up(gnb_core_t *gnb_core){

    char bin_path[PATH_MAX+NAME_MAX];
    char bin_path_q[PATH_MAX+NAME_MAX];
    char map_path_q[PATH_MAX+NAME_MAX];

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

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


static void if_down(gnb_core_t *gnb_core){

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

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


static int ntod(uint32_t mask){
    int i, n = 0;
    int bits = sizeof(uint32_t) * 8;

    for (i = bits - 1; i >= 0; i--) {
        if (mask & (0x01 << i))
            n++;
    }

    return n;
}


static int set_addr4(gnb_core_t *gnb_core){

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

    DWORD dwRetVal = 0;

    DWORD dwSize = 0;
    unsigned long status = 0;

    DWORD lastError = 0;
    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );

    WintunGetAdapterLUID(tun_wintun_ctx->adapter_handle, &ipRow.InterfaceLuid);

    ipRow.Address.si_family  = AF_INET;

    ipRow.OnLinkPrefixLength = (UINT8)ntod( gnb_core->local_node->tun_netmask_addr4.s_addr );

    InetPton(AF_INET, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4), &ipRow.Address.Ipv4.sin_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    status = CreateUnicastIpAddressEntry(&ipRow);
    if (status != ERROR_SUCCESS && status != ERROR_OBJECT_ALREADY_EXISTS)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Failed to set IPv4 address: %s\n", GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4));
        return -3;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Success to set IPv4 address: %s\n", GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4));
    return 0;

}


static int set_addr6(gnb_core_t *gnb_core){

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

    DWORD dwRetVal = 0;

    DWORD dwSize = 0;
    unsigned long status = 0;

    DWORD lastError = 0;
    SOCKADDR_IN localAddress;

    MIB_UNICASTIPADDRESS_ROW ipRow;

    InitializeUnicastIpAddressEntry( &ipRow );

    WintunGetAdapterLUID(tun_wintun_ctx->adapter_handle, &ipRow.InterfaceLuid);

    ipRow.Address.si_family = AF_INET6;
    ipRow.OnLinkPrefixLength = 96;

    InetPton(AF_INET6, GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), &ipRow.Address.Ipv6.sin6_addr);

    status = DeleteUnicastIpAddressEntry(&ipRow);

    status = CreateUnicastIpAddressEntry(&ipRow);
    if (status != ERROR_SUCCESS && status != ERROR_OBJECT_ALREADY_EXISTS)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Failed to set IPv6 address: %s\n", GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr));
        return -3;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Success to set IPv6 address: %s\n", GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr));
    return 0;

}


static int open_tun_wintun(gnb_core_t *gnb_core){

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

    int ret;
    DWORD LastError;
    if ( NULL != tun_wintun_ctx->adapter_handle ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Wintun Adapter Exist!\n");
        return -1;
    }
    
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "WintunCreateAdapter Start...\n");
    struct in_addr addr4 = gnb_core->local_node->tun_addr4;
    GUID Guid = { 0xdeadbabe, 0xcafe, 0xbeef, { 0x01, 0x23, 0x45, 0x67, addr4.S_un.S_un_b.s_b1, addr4.S_un.S_un_b.s_b2, addr4.S_un.S_un_b.s_b3, addr4.S_un.S_un_b.s_b4 } };
    //WINTUN_ADAPTER_HANDLE Adapter = WintunCreateAdapter(tun_wintun_ctx->tap_file_name, tun_wintun_ctx->if_name, &Guid);
    const size_t cSize = strlen(tun_wintun_ctx->if_name) + 1;
    wchar_t w_if_name[cSize];
    mbstowcs (w_if_name, tun_wintun_ctx->if_name, cSize);
    WINTUN_ADAPTER_HANDLE Adapter = NULL;
    int try_count = 2;
    while (NULL == Adapter && try_count > 0)
    {
        Adapter =  WintunCreateAdapter(w_if_name, WINTUN_POOL_NAME, &Guid);
        try_count--;
    }
    
    if (NULL == Adapter)
    {
        DWORD lastErr = GetLastError();
        if( lastErr == ERROR_OBJECT_ALREADY_EXISTS)
        {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "WintunCreateAdapter  Failed! already exist!\n"); 
            return -1;
        }
        else
        {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "WintunCreateAdapter  Failed: %ld\n", lastErr);
            return -2;
        }
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "WintunCreateAdapter Success...\n");




    tun_wintun_ctx->adapter_handle = Adapter;

    DWORD status4 = set_addr4(gnb_core);
    if(0 != status4)
    {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return status4;
    }
    DWORD status6 = set_addr6(gnb_core);
    if(0 != status6)
    {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return status6;
    }


    WINTUN_SESSION_HANDLE Session = WintunStartSession(tun_wintun_ctx->adapter_handle, WINTUN_RING_CAPACITY);
    if (!Session)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Failed to WintunStartSession!\n");
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        tun_wintun_ctx->adapter_handle = NULL;
        return -4;
    }
    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Success to WintunStartSession!\n");
    tun_wintun_ctx->session = Session;



    tun_wintun_ctx->hReadEvent = WintunGetReadWaitEvent(Session);






    if ( !tun_wintun_ctx->skip_if_script ) {
        if_up(gnb_core);
    }

    return 0;

}


static int read_tun_wintun(gnb_core_t *gnb_core, void *buf, size_t buf_size){
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    WINTUN_SESSION_HANDLE session = (WINTUN_SESSION_HANDLE)tun_wintun_ctx->session;
    HANDLE WaitHandles[] = { tun_wintun_ctx->hReadEvent, tun_wintun_ctx->hQuitEvent };

    DWORD packet_size;
    BYTE *packet = WintunReceivePacket(session, &packet_size);
    if(NULL != packet){
        size_t real_size = (packet_size > buf_size)?buf_size:packet_size;
        memcpy(buf, packet, real_size);
        WintunReleaseReceivePacket(session, packet);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "read_tun_wintun bufsize:%d, real WintunReceivePacket size:%d!\n", buf_size,  packet_size);
        return real_size;
    }else{
        DWORD LastError  = GetLastError();
        switch (LastError )
        {
            case ERROR_NO_MORE_ITEMS:
                GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "read_tun_wintun Packet read ERROR_NO_MORE_ITEMS:%ld!\n", LastError);
                if (WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0){
                    return ERROR_SUCCESS;
                }
                break;        
            default:
                GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "read_tun_wintun Packet read failed:%ld!\n", LastError);
                return LastError;
        }
    }
    return ERROR_SUCCESS;
}


static int write_tun_wintun(gnb_core_t *gnb_core, void *buf, size_t buf_size){
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    WINTUN_SESSION_HANDLE session = (WINTUN_SESSION_HANDLE)tun_wintun_ctx->session;

    BYTE *Packet = WintunAllocateSendPacket(session, buf_size);
    memcpy(Packet, buf, buf_size);
    DWORD LastError = GetLastError();
    if (NULL != Packet)
    {
        WintunSendPacket(session, Packet);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "write_tun_wintun WintunAllocateSendPacket size:%d!\n", buf_size);
        return 0;
    }
    else if (LastError != ERROR_BUFFER_OVERFLOW)
    {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "write_tun_wintun WintunAllocateSendPacket size:%d, Packet write failed!\n", buf_size);
        return 0;
    }
    else 
    {
        return 0;
    }
}


static int close_tun_wintun(gnb_core_t *gnb_core){

    gnb_core->loop_flag = 0;

    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;

    if ( !tun_wintun_ctx->skip_if_script ) {
        if_down(gnb_core);
    }

    if(NULL != tun_wintun_ctx->hReadEvent)
    {
        SetEvent(tun_wintun_ctx->hReadEvent);
        tun_wintun_ctx->hReadEvent = NULL;
    }

    if(NULL != tun_wintun_ctx->hQuitEvent)
    {
        SetEvent(tun_wintun_ctx->hQuitEvent);
        tun_wintun_ctx->hQuitEvent = NULL;
    }

    if(NULL != tun_wintun_ctx->session)
    {
        WintunEndSession(tun_wintun_ctx->session);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Close Tun, WintunEndSession!\n");
        tun_wintun_ctx->session = NULL;
    }

    if(NULL != tun_wintun_ctx->adapter_handle)
    {
        WintunCloseAdapter(tun_wintun_ctx->adapter_handle);
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Close Tun, WintunCloseAdapter!\n");
        tun_wintun_ctx->adapter_handle = NULL;
    }

    

    return release_tun_wintun(gnb_core);

}

static int release_tun_wintun(gnb_core_t *gnb_core){
    gnb_core_wintun_ctx_t *tun_wintun_ctx = gnb_core->platform_ctx;
    if (NULL != tun_wintun_ctx)
    {
        if (NULL != tun_wintun_ctx->drv_module)
        {
            FreeLibrary(tun_wintun_ctx->drv_module);
            tun_wintun_ctx->drv_module = NULL;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "Release Tun, FreeLibrary!\n");
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
