#  Created by Zz on 10/29/14.
#  Copyright (c) 2014 Kiban Labs Inc. All rights reserved.

#include ../../build/config.mk
#include ../../build/module.mk
#include ../../build/gccconfig.mk


#compile: beetle.c
#	$(CC) -o beetle -I$(BUILD_DIR)/bluez-5.24/ipkg-install/usr/include/ -lbluetooth -L$(BUILD_DIR)/bluez-5.24/ipkg-install/usr/lib/

#clean:
#	rm -f beetle


include $(TOPDIR)/rules.mk

USE_SOURCE_DIR:=$(CURDIR)/pkg

PKG_NAME:=beetle
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://s3-us-west-1.amazonaws.com/zzbucket
PKG_MD5SUM:=99842d3ee0c2ce729ea48560d00aba26

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/beetle
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Kiban hub bluetooth interface
  DEPENDS:=+bluez +libbluetooth
  URL:=http://www.kibanlabs.com/
endef

CONFIGURE_ARGS+=CPPFLAGS=-I$(BUILD_DIR)/bluez-5.24/ipkg-install/usr/include/ \
		LDFLAGS=-L$(BUILD_DIR)/bluez-5.24/ipkg-install/usr/lib/

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LDFLAGS="$(TARGET_LDFLAGS) \
			-L$(ICONV_PREFIX)/lib \
			-L$(INTL_PREFIX)/lib -lm \
			-L$(BUILD_DIR)/bluez-5.24/ipkg-install/usr/lib/" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install
endef

define Package/beetle/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_BIN) $(BUILD_DIR)/bluez-5.24/ipkg-install/usr/lib/libbluetooth.so* $(1)/lib
	$(INSTALL_BIN) $$(TOOLCHAIN_DIR)/lib/libstdc++.so* $(1)/lib

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/beetle $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/lib/firmware
	$(CP) -rp $(CURDIR)/pkg/files/* $(1)
endef

$(eval $(call BuildPackage,beetle))

