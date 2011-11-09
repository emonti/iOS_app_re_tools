ROOT := $(realpath $(shell dirname $(lastword $(MAKEFILE_LIST))))

# Create some symlinks in the CWD to your toolchain and sdk sysroot
# or just point these directly to their respective paths.
#
# TOOLCHAIN would be /Developer/Platforms/iPhoneOS.platform/Developer 
# if you were using the iPhone sdk.
#
# SYSROOT would be the location of your iPhoneOS*.sdk directory. Using
# the sdk, that would be under SDKs in the toolchain location.
TOOLCHAIN_DIR=$(ROOT)/toolchain
SYSROOT = $(ROOT)/sdk

BIN=$(TOOLCHAIN_DIR)/usr/bin
GCC_BIN = $(BIN)/gcc


# You may want a thin binary if you plan to use ldid for debugging
# entitlements.
CFLAGS= -arch armv6 -arch armv7

CC = $(GCC_BIN) -Os -Wimplicit -isysroot $(SYSROOT)

