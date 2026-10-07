# Packaging — EXCALIBUR Control Center (Arch Linux / CachyOS)

Single user-facing product package: **`excalibur-control-center`**, which provides
**both** the Qt6 GUI and the `excalibur-wmi` read-only hwmon driver, integrated as
DKMS. There is intentionally **no** separate `excalibur-wmi-dkms` package and no
meta package.

## Install (end-user)

```bash
yay -S excalibur-control-center        # or: paru -S excalibur-control-center
```

This installs the GUI and the DKMS driver sources. Arch's `dkms` pacman hooks build
and install the kernel module for every installed kernel. Root is only needed for
the package/module installation; the **GUI itself never needs root**. Loading the
module (`modprobe`) is left to the user / the boot process — the app does not do it.

After install:
```bash
dkms status                       # excalibur-wmi/0.5.1, <kernel>: installed
sudo modprobe excalibur_wmi
cat /sys/class/hwmon/hwmon*/name  # excalibur_g870
```

## Driver / kernel compatibility

The driver source uses the `wmidev_set_block` / `wmidev_query_block` kernel API:

```
Supported/tested kernel API range: 7.2.x   (verified on 7.2.9-1-cachyos)
6.18.x: known source/API incompatibility   (build fails: wmidev_* absent)
```

Driver compatibility depends on the kernel API; no header package is hardcoded and
no artificial `BUILD_EXCLUSIVE_KERNEL` range is declared. The driver source is
protected and not modified here.

## Build the package (local development source)

`makepkg` expects a **reproducible** source tarball named
`excalibur-control-center-<pkgver>.tar.gz` next to the `PKGBUILD`, containing
`gui/`, `driver/`, `analysis/phase3/excalibur-wmi.c` and `LICENSE`.

```bash
VERSION=$(sed -n 's/^[[:space:]]*VERSION[[:space:]]\+\([0-9][0-9.]*\).*/\1/p' gui/CMakeLists.txt | head -n1)
tar -c --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    --exclude='gui/build' \
    --transform "s,^,excalibur-control-center-$VERSION/," \
    -C . gui driver analysis/phase3/excalibur-wmi.c LICENSE \
  | gzip -n > packaging/arch/excalibur-control-center-$VERSION.tar.gz

cd packaging/arch
makepkg --cleanbuild --clean -f      # clean, reproducible build
makepkg --printsrcinfo > .SRCINFO    # regenerate when metadata changes
```

The tarball is reproducible (fixed mtimes/owner, sorted, `gzip -n`); its sha256 is
recorded in `PKGBUILD`/`.SRCINFO`.

## Final AUR source model

The AUR repository should ship **`PKGBUILD` + `.SRCINFO`** and pull the source from
the upstream release archive, not from a committed tarball:

```
source=("<upstream-release-url>/excalibur-control-center-$pkgver.tar.gz")
sha256sums=('<checksum of the release archive>')   # regenerate with updpkgsums
```

The current `source=("$pkgname-$pkgver.tar.gz")` + real checksum is the local
development input; swapping it for the upstream URL is the only change needed once a
public release URL exists.

## What the package installs

```
/usr/bin/excalibur-control-center
/usr/share/applications/excalibur-control-center.desktop
/usr/share/icons/hicolor/<size>x<size>/apps/excalibur-control-center.png   (16…256)
/usr/share/icons/hicolor/scalable/apps/excalibur-control-center.svg
/usr/share/metainfo/excalibur-control-center.metainfo.xml
/usr/share/licenses/excalibur-control-center/LICENSE
/usr/src/excalibur-wmi-0.5.1/{dkms.conf,Makefile,excalibur-wmi.c,LICENSE}
```

The prebuilt `analysis/phase3/excalibur-wmi.ko` is **not** packaged.

## DKMS naming / lifecycle (Arch)

- DKMS `PACKAGE_NAME` = `excalibur-wmi`, version = `0.5.1` → source dir
  `/usr/src/excalibur-wmi-0.5.1/` (DKMS identity may differ from the package name;
  cf. `virtualbox-host-dkms` → `vboxhost`). The Arch `dkms` hook derives the module
  id from that directory name.
- Kernel module `excalibur_wmi` (built `excalibur-wmi.ko`).
- Lifecycle is automatic via `/usr/share/libalpm/hooks/{70-dkms-install,
  71-dkms-remove,70-dkms-upgrade}.hook`; the package ships **no** `.install` script
  and never calls `dkms`.
- The DKMS `Makefile` uses `KERNELRELEASE` (target kernel, not `uname -r`) and builds
  with the target kernel's compiler (clang detected from `include/config/auto.conf`).

### AUR naming note

The Arch DKMS guideline names DKMS packages with a `-dkms` suffix. This is a
**combined product package** (GUI + driver) whose primary content is the GUI, so the
name is `excalibur-control-center` without the suffix. This deviation is intentional
and documented for AUR reviewers.

## User data

The package owns only `/usr/...`. User runtime state
(`~/.config/EXCALIBUR/excalibur-control-center.conf`) is created by the app, is not
package-owned, and survives uninstall.

## CI build commands (for P3.3)

```bash
# GUI
cmake -S gui -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build                      # expect 0 warning / 0 error (-Wall -Wextra)

# Package
cd packaging/arch && makepkg -C -c -f --noconfirm

# Validations
desktop-file-validate pkg/*/usr/share/applications/excalibur-control-center.desktop
appstreamcli validate --no-net pkg/*/usr/share/metainfo/excalibur-control-center.metainfo.xml

# DKMS source build (non-root; DKMS install itself needs root)
make -C /usr/src/excalibur-wmi-0.5.1 KERNELRELEASE=$(uname -r)   # → excalibur-wmi.ko

# Package contents
bsdtar -tf excalibur-control-center-0.5.1-1-x86_64.pkg.tar.zst | sort
```

## Security

No `.install`/post-install script, no root helper, no `sudo`/`pkexec`/`polkit`, no
network during build, no hardcoded home path, no writable-path executable. The GUI
runs entirely as the user. Secure Boot: the package neither generates nor stores keys
and does not sign modules; the user handles module signing (`mokutil`) themselves.

## License

GPL-2.0-only. Installed at `/usr/share/licenses/excalibur-control-center/LICENSE` and
inside the DKMS source tree; consistent with the driver's `SPDX-License-Identifier:
GPL-2.0` (protected source, unchanged).
