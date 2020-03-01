@echo off

setlocal enabledelayedexpansion

cd /d %~dp0


set nodeid=%1

if defined nodeid (

    echo nodeid is !nodeid!

) else (

    echo nodeid is NULL
    goto FINISH

)

start ..\bin\Window10_x86_64\gnb_es.exe -s -b ..\conf\!nodeid!\gnb.map --dump-address --upnp

..\bin\Window10_x86_64\gnb.exe -i WindowsTun -c ..\conf\!nodeid! --port-detect-start=500 --port-detect-end=65535

:FINISH

