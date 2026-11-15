#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lxc-extras
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Build/Compile/Default
endef

define Package/lxc-extras
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Extra files for lxc
  EXTRA_DEPENDS:=             \
    rsync          (>=3.5.2), \
    lxc-create     (>=7.0.0), \
    lxc-destroy    (>=7.0.0), \
    lxc-copy       (>=7.0.0), \
    lxc-attach     (>=7.0.0), \
    lxc-console    (>=7.0.0), \
    lxc-info       (>=7.0.0), \
    lxc-ls         (>=7.0.0), \
    lxc-top        (>=7.0.0), \
    lxc-freeze     (>=7.0.0), \
    lxc-unfreeze   (>=7.0.0), \
    lxc-wait       (>=7.0.0), \
    lxc-usernsexec (>=7.0.0), \
    lxc-start      (>=7.0.0), \
    lxc-stop       (>=7.0.0), \
    lxc-autostart  (>=7.0.0)
endef

define Package/lxc-extras/description
  This package contains extra files for lxc, such as an init file.
endef

define Package/lxc-extras/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lxc.init $(1)/etc/init.d/lxc
endef

$(eval $(call BuildPackage,lxc-extras))
