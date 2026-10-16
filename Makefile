TARGET := iphone:clang:latest:16.0
ARCHS = arm64 arm64e
THEOS_PACKAGE_SCHEME = rootless
INSTALL_TARGET_PROCESSES = SpringBoard

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = SidebarLauncher

SidebarLauncher_FILES = Tweak.xm SidebarConfigManager.m
SidebarLauncher_FRAMEWORKS = UIKit Foundation
SidebarLauncher_CFLAGS = -fobjc-arc
SidebarLauncher_CFLAGS += -Wno-error=deprecated-declarations
SidebarLauncher_CODESIGN_FLAGS = -Sentitlements.plist

include $(THEOS_MAKE_PATH)/tweak.mk

before-package::
	@python3 tests/validate_config.py layout/var/jb/Library/SidebarLauncher/config.plist
