@echo off

cd /d %~dp0

setlocal enabledelayedexpansion

rd /S /Q conf

xcopy /Y /E /I conf_tpl conf

set /a nodeid=1000

for /l %%c in (1 1 %1) do (

	set /a nodeid += 1
	
	md conf\!nodeid!\ed25519
	md conf\!nodeid!\security
		
	bin\Window10_x86_64\gnb_crypto.exe -c -p !nodeid!.private -k !nodeid!.public
	
	copy !nodeid!.private conf\!nodeid!\security\
	copy !nodeid!.public  conf\!nodeid!\security\

)

set /a nodeid=1000

for /l %%c in (1 1 %1) do (
	set /a nodeid += 1
	copy *.public conf\!nodeid!\ed25519\
)

del *.public
del *.private
