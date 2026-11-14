#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iperfs
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Build/Compile/Default
endef

define Package/iperfs
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Init script for iperf server
  EXTRA_DEPENDS:=   \
    iperf (>=2.2.1)
endef

define Package/iperfs/description
  This package contains an init script to allow running an
  iperf server as a system service.
endef

define Package/iperfs/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iperf.init $(1)/etc/init.d/iperf
endef

define Package/iperf3s
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Init script for iperf3 server
  EXTRA_DEPENDS:=   \
    iperf3 (>=3.22)
endef

define Package/iperf3s/description
  This package contains an init script to allow running an
  iperf3 server as a system service.
endef

define Package/iperf3s/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iperf3.init $(1)/etc/init.d/iperf3
endef

$(eval $(call BuildPackage,iperfs))
$(eval $(call BuildPackage,iperf3s))
