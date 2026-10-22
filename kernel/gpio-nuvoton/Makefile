#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpio-nuvoton
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define KernelPackage/gpio-nuvoton
  SUBMENU:=GPIO support
  TITLE:=GPIO Nuvoton Support
  DEPENDS:=@GPIO_SUPPORT @TARGET_x86
  FILES:=$(PKG_BUILD_DIR)/gpio-nuvoton.ko
  AUTOLOAD:=$(call AutoLoad,30,gpio-nuvoton,1)
  KCONFIG:=
endef

define KernelPackage/gpio-nuvoton/description
  Support for GPIO functionality of Nuvoton Super I/O Chips.
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,gpio-nuvoton))
