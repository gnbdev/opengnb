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
#include <windows.h>
#include <processthreadsapi.h>
#include <limits.h>

#include "gnb_exec.h"
#include "gnb_arg_list.h"


int gnb_get_pid(){
    int pid;
    pid = GetCurrentProcessId();
    return pid;
}


void gnb_set_env(const char *name, const char *value){

    _putenv_s(name, value);

}


int gnb_exec(char *app_filename, char *current_path, gnb_arg_list_t *gnb_arg_list, int flag){

    int rc;

    wchar_t app_filename_w[PATH_MAX+NAME_MAX];
    wchar_t gnb_arg_string_w[PATH_MAX+NAME_MAX];
    wchar_t current_path_w[PATH_MAX+NAME_MAX];

    STARTUPINFOW si;
    PROCESS_INFORMATION pi = {0};
    DWORD creation_flags;

    ZeroMemory(&si,sizeof(si));

    si.cb = sizeof(si);

    char gnb_arg_string[GNB_ARG_STRING_MAX_SIZE];

    rc = gnb_arg_list_to_string(gnb_arg_list, gnb_arg_string, GNB_ARG_STRING_MAX_SIZE);

    if ( flag & GNB_EXEC_BACKGROUND ) {

        si.wShowWindow = SW_HIDE;
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        creation_flags = CREATE_NO_WINDOW;

    } else {

        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;
        creation_flags = CREATE_NEW_CONSOLE;
    }

    memset(app_filename_w, 0, PATH_MAX+NAME_MAX);
    MultiByteToWideChar(CP_ACP, 0, app_filename,     -1, app_filename_w,   PATH_MAX+NAME_MAX);

    memset(current_path_w, 0, PATH_MAX+NAME_MAX);
    MultiByteToWideChar(CP_ACP, 0, current_path,     -1, current_path_w,   PATH_MAX+NAME_MAX);

    memset(gnb_arg_string_w, 0, PATH_MAX+NAME_MAX);
    MultiByteToWideChar(CP_ACP, 0, gnb_arg_string,   -1, gnb_arg_string_w, PATH_MAX+NAME_MAX);

    rc = CreateProcessW(app_filename_w, gnb_arg_string_w, NULL,NULL,TRUE, creation_flags, NULL, current_path_w, &si,&pi);

    if ( !rc ) {
        return -1;
    }

    if ( flag & GNB_EXEC_WAIT ) {

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitcode;

        rc = GetExitCodeProcess(pi.hProcess, &exitcode);

    }

    if ( pi.hProcess ) {
      CloseHandle(pi.hProcess);
      pi.hProcess = NULL;
    }

    if ( pi.hThread ) {
       CloseHandle(pi.hThread);
       pi.hThread = NULL;
    }

    return pi.dwProcessId;

}


int gnb_kill(DWORD pid){

    HANDLE hProcess=OpenProcess(PROCESS_TERMINATE,FALSE, pid);

    TerminateProcess(hProcess, 0);

    return 0;
}

