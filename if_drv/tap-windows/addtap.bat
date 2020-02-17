@rem Add a new TAP virtual ethernet adapter


echo "这是一个绿色软件，但在Windows上需要依赖一个第三方的虚拟网卡驱动（OpenVPN开发），如果不打算安装，可以按[Ctrl+c]终止"

cd /d %~dp0

tapinstall.exe install OemVista.inf tap0901

echo "安装虚拟网卡驱动后需要重新启动程序..."

pause

