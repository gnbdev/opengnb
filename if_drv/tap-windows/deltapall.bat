echo WARNING: this script will delete ALL TAP virtual adapters (use the device manager to delete adapters one at a time)
pause
cd /d %~dp0
tapinstall.exe remove tap0901
pause
