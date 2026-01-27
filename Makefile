include $(TOPDIR)/rules.mk

PKG_NAME:=opengnb
PKG_VERSION:=1.6.1
PKG_RELEASE:=1

PKG_MAINTAINER:=Charles Chan
PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/opengnb
  SECTION:=net
  CATEGORY:=Network
  TITLE:=OpenGNB - Global Network Bridge
  URL:=https://github.com/gnbdev/opengnb
  DEPENDS:=+libpthread
endef

define Package/opengnb/description
  OpenGNB (Global Network Bridge) is a decentralized layer 3 overlay network.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(CURDIR)/src $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/libs $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/Makefile.openwrt $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/Makefile.inc $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		-f Makefile.openwrt \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -ffunction-sections -fdata-sections -Wno-unused-result -I./src -I./libs -I./libs/miniupnpc -I./libs/libnatpmp -I./libs/zlib -D NO_GZIP=1 -D GNB_OPENWRT_BUILD=1" \
		CLI_LDFLAGS="$(TARGET_LDFLAGS) -Wl,--gc-sections -pthread" \
		GNB_ES_LDFLAGS="$(TARGET_LDFLAGS) -Wl,--gc-sections -pthread" \
		all
endef

define Package/opengnb/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gnb $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gnb_crypto $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gnb_ctl $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gnb_es $(1)/usr/bin/
endef

$(eval $(call BuildPackage,opengnb))
