@echo 这是一个绿色软件，但在Windows上需要依赖一个第三方的虚拟网卡驱动（OpenVPN开发），如果不打算安装，可以按[Ctrl+c]终止

@echo 如果打算安装，请按任意键

@pause

cd /d %~dp0

cd if_drv\tap-windows

tapinstall.exe install OemVista.inf tap0901

@echo 安装虚拟网卡驱动后需要重新启动程序...

@echo 请按任意键退出安装程序

@pause
