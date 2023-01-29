%global _empty_manifest_terminate_build 0
Name:		opengnb	
Version: 	1.3.0.c	
Release:	1%{?dist}
Summary:	GNB is open source de-centralized VPN to achieve layer3 network via p2p with the ultimate capability of NAT Traversal.GNB

Group:		System	
License:	GPL	
URL:		https://github.com/gnbdev/opengnb
Source0:	opengnb-1.3.0.c.tar.gz

BuildRequires:	gcc
BuildRequires:	make


%description
OpenGNB is an open source P2P decentralized VPN with extreme intranet penetration capability,Allows you to combine your company-home network into a direct-access LAN.

%prep
%setup -q


%build
mv Makefile.linux Makefile
make  %{?_smp_mflags}

%install
%make_install
mkdir -p %{buildroot}%{_bindir}
install %{_builddir}/%{name}-%{version}/bin/* %{buildroot}%{_bindir}

%files
%{_bindir}*

%changelog
* Thu Nov  3 2022 Wenlong Zhang <zhangwenlong@loongson.cn> - 1.3.0.c-1
- add spec for opengnb 
