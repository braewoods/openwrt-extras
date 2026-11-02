#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cfatemp
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define KernelPackage/hwmon-cfatemp
  SUBMENU:=Hardware Monitoring Support
  TITLE:=Compact Flash Association cards with temperature sensor
  EXTRA_DEPENDS:=                                          \
    kmod-hwmon-core (=$(LINUX_VERSION)-r$(LINUX_RELEASE)), \
    kmod-ata-core   (=$(LINUX_VERSION)-r$(LINUX_RELEASE)), \
    kmod-scsi-core  (=$(LINUX_VERSION)-r$(LINUX_RELEASE))
  FILES:=$(PKG_BUILD_DIR)/cfatemp.ko
  AUTOLOAD:=$(call AutoLoad,60,cfatemp)
  KCONFIG:=
endef

define KernelPackage/hwmon-cfatemp/description
  Kernel module for Compact Flash Association cards with temperature sensor
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,hwmon-cfatemp))
