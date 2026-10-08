# EXCALIBUR Control Center

A native Linux desktop application that shows the **read-only** thermal and cooling
telemetry of CASPER EXCALIBUR laptops (EXCALIBUR G870 / NLXB 001), plus an integrated
read-only hwmon kernel driver delivered via DKMS.

```
                  EXCALIBUR Control Center
                           │
             ┌─────────────┴─────────────┐
             │ GUI (Qt6)                 │ excalibur-wmi (DKMS driver)
             └──────────── same package ─┘
                           │
                     one AUR package
```

## What it does

- CPU and GPU temperature
- CPU and GPU fan RPM telemetry
- 60-second temperature / fan history
- Hardware, firmware and kernel information
- Diagnostics view with JSON export (for support)
- Settings: theme (System/Light/Dark), Celsius/Fahrenheit, refresh interval
- System tray with background operation and single-instance enforcement

Everything is **unprivileged and read-only**. The application reads
`/sys/class/hwmon/*` through its own `HwmonClient`; it never writes to the hardware,
never touches WMI/ACPI/EC, and never needs root at runtime.

## What it is NOT

This is **not** a control tool. There is **no** fan control, **no** RGB control,
**no** power/performance modes, and **no** overclocking. The only declared kernel
feature is read-only telemetry. Claims of fan/RGB/power control are intentionally
absent.

## Install (Arch Linux / CachyOS)

> **AUR publication is pending registration availability.** The package is not on
> the AUR yet. Until then, build/install from the GitHub release or from source.

The single `excalibur-control-center` package provides the GUI **and** the
`excalibur-wmi` DKMS driver. `dkms` (a runtime dependency) and its pacman hooks
build/install the module for every installed kernel and rebuild it on kernel updates.

### Control Center (GUI)

Runs as a normal, unprivileged user.

### Driver

Provided as a DKMS kernel module (`excalibur_wmi`); the kernel module is built during
package installation (and rebuilt by the `dkms` hooks on kernel updates).

### Privileges

The GUI runtime needs **no root**. Package installation and the kernel-module
lifecycle (DKMS) may use root.

### Install methods

AUR (once published):

```bash
yay -S excalibur-control-center        # or: paru -S excalibur-control-center
```

From the GitHub release (a built package is attached to the `v0.5.1` release):

```bash
sudo pacman -U excalibur-control-center-0.5.1-1-x86_64.pkg.tar.zst
```

From source: see below.

## Build from source

Requirements: Qt6 Widgets, CMake ≥ 3.16, Ninja, a C++17 compiler.

```bash
cmake -S gui -B gui/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build gui/build
./gui/build/excalibur-control-center
```

The app works without the driver (the Dashboard shows `Telemetry Unavailable`) and
switches to live telemetry automatically once the module is loaded.

## Driver (DKMS)

The driver source is `analysis/phase3/excalibur-wmi.c` (single source of truth) and
is packaged as a DKMS module:

```
/usr/src/excalibur-wmi-<version>/{dkms.conf, Makefile, excalibur-wmi.c}
```

Build/install is automatic via the distro `dkms` pacman hooks. Manual commands:

```bash
dkms status
sudo modprobe excalibur_wmi
cat /sys/class/hwmon/hwmon*/name     # excalibur_g870
```

### Kernel compatibility (known limitation)

The driver uses the `wmidev_set_block` / `wmidev_query_block` kernel API:

- **7.2.x — verified** (builds and loads on 7.2.9-1-cachyos).
- **6.18.x-lts — known incompatibility** (`wmidev_*` API absent → build fails).

Driver compatibility therefore depends on the kernel API. No header package is
hardcoded and no artificial compatibility range is declared.

## Telemetry fidelity

The GUI displays the raw hwmon value unmodified. It does **not** smooth, average,
median-filter, clamp or otherwise alter readings. If the firmware/EC emits an
anomalous sample (e.g. a sudden CPU-temperature step), it is shown as-is; that is a
backend/EC investigation, not something the GUI hides.

## Diagnostics

The Diagnostics page shows the application's real runtime state and can export it as
pretty JSON (no secrets, no environment dump; canonical temperatures are Celsius,
with the user's unit recorded separately).

## License

GPL-2.0-only. See [`LICENSE`](LICENSE).
