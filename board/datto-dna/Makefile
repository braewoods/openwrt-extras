#
# Copyright (C) 2026 James Buren
#
# This is free software, licensed under the GNU General Public License v2.
# See /COPYING for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=datto-dna
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_MAINTAINER:=James Buren <braewoods+ow@braewoods.net>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/board-datto-dna
  SECTION:=base
  CATEGORY:=Base system
  TITLE:=Board support files for Datto DNA
  DEPENDS:=+kmod-datto-dna +kmod-i2c-i801 +kmod-hwmon-jc42 +kmod-eeprom-at24 +kmod-hwmon-nct6775
endef

define Package/board-datto-dna/description
  This package contains additional files to support the Datto DNA routers.
  Furthermore, it also depends on extra kernel modules for any hardware
  that is integrated into the board so as to make it easier to create
  custom images.

  Please only include this package in your image if you are targeting a
  Datto DNA router. The scripts will clobber the configuration for any
  other router because specific targeting cannot be implemented reliably.
endef

define Package/board-datto-dna/install
	$(CP) ./files/* $(1)/
endef

include $(INCLUDE_DIR)/kernel.mk

define KernelPackage/datto-dna
  SUBMENU:=Other modules
  TITLE:=Datto DNA Platform Driver
  DEPENDS:=+kmod-gpio-nuvoton +kmod-leds-gpio +kmod-input-gpio-keys-polled
  FILES:=$(PKG_BUILD_DIR)/datto-dna.ko
  AUTOLOAD:=$(call AutoLoad,60,datto-dna,1)
  KCONFIG:=
endef

define KernelPackage/datto-dna/description
  Kernel module for the Datto DNA Board

  It maps the GPIOs to key inputs and LED outputs.
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call BuildPackage,board-datto-dna))
$(eval $(call KernelPackage,datto-dna))
