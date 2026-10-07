# DKMS build for the excalibur-wmi read-only hwmon driver.
#
# DKMS installs the sources under /usr/src/<PACKAGE_NAME>-<PACKAGE_VERSION>/ and
# builds them for each installed kernel by invoking `make` with KERNELRELEASE set
# to the target kernel version (never the running kernel), so this Makefile must
# derive the kernel build directory from KERNELRELEASE, not `uname -r`.
#
# The module source file is excalibur-wmi.c (copied next to this Makefile in the
# DKMS tree). It is the single source of truth; the driver is not duplicated in
# the repository.

KVER ?= $(KERNELRELEASE)
KDIR ?= /lib/modules/$(KVER)/build

obj-m := excalibur-wmi.o

# Build with the same compiler family the target kernel was configured with
# (CachyOS kernels are clang-built; other kernels may be gcc-built). Detected
# from the generated kernel config so no toolchain is hardcoded.
LLVM_ARG := $(shell grep -qs '^CONFIG_CC_IS_CLANG=y' $(KDIR)/include/config/auto.conf && echo LLVM=1)

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(LLVM_ARG) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(LLVM_ARG) clean

.PHONY: all clean
