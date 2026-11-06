#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ipp-usb
PKG_VERSION:=0.9.34
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/OpenPrinting/$(PKG_NAME)/archive/refs/tags/$(PKG_VERSION).tar.gz?
PKG_HASH:=4f0ada8593fda3f4239a50fcf24a625842143039de97708281e7b8c0cdff201a

PKG_LICENSE:=BSD-2-Clause
PKG_LICENSE_FILES:=LICENSE
PKG_CPE_ID:=cpe:/a:openprinting:ipp-usb

PKG_BUILD_DEPENDS:=golang/host
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=no-mips16

GO_PKG:=github.com/OpenPrinting/ipp-usb
GO_PKG_INSTALL_BIN_PATH:=/usr/sbin

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/golang/golang-package.mk
#include ../../lang/golang/golang-package.mk

define Package/ipp-usb
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Printing
  TITLE:=IPP reverse proxy for USB printers using IPP-Over-USB
  URL:=https://github.com/OpenPrinting/ipp-usb
  DEPENDS:=$(GO_ARCH_DEPENDS) +libusb-1.0 +libavahi-client
endef

define Package/ipp-usb/description
  ???
endef

define Package/ipp-usb/install
	$(call GoPackage/Package/Install/Bin,$(1))
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/ipp-usb.init $(1)/etc/init.d/ipp-usb
	$(INSTALL_DIR) $(1)/etc/ipp-usb
	$(INSTALL_DATA) $(CURDIR)/files/ipp-usb.conf $(1)/etc/ipp-usb/ipp-usb.conf
endef

$(eval $(call GoBinPackage,ipp-usb))
$(eval $(call BuildPackage,ipp-usb))
