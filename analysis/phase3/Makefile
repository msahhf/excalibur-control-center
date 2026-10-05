# Phase III — read-only hwmon prototype (EXCALIBUR G870)
# Build ONLY for the running kernel; no modules_install, no persistent install.
obj-m += excalibur-wmi.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Kernel is clang-built on this system -> invoke `make LLVM=1`.
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
