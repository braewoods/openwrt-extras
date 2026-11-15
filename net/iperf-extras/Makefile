#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iperf-extras
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Build/Compile/Default
endef

define Package/iperf-extras
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Extra files for iperf
  EXTRA_DEPENDS:=   \
    iperf (>=2.2.1)
endef

define Package/iperf-extras/description
  This package contains extra files for iperf, such as an init file.
endef

define Package/iperf-extras/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iperf.init $(1)/etc/init.d/iperf
endef

define Package/iperf3-extras
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Extra files for iperf3
  EXTRA_DEPENDS:=   \
    iperf3 (>=3.22)
endef

define Package/iperf3-extras/description
  This package contains extra files for iperf3, such as an init file.
endef

define Package/iperf3-extras/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/iperf3.init $(1)/etc/init.d/iperf3
endef

$(eval $(call BuildPackage,iperf-extras))
$(eval $(call BuildPackage,iperf3-extras))
