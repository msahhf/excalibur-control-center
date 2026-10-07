# EXCALIBUR Control Center — GUI (v0.5.1)

Read-only telemetry dashboard for the **CASPER EXCALIBUR G870** (NLXB 001).

## v0.5.1 scope

- **Qt6 Widgets** (C++17), native KDE/Breeze look, Wayland-friendly.
- **Read-only telemetry** only. **No** writes, **no** power mode, **no** RGB, **no** fan control.
- Reads **Linux hwmon/sysfs** exclusively — the GUI never talks to WMI, ACPI, or the EC.
- **No root** required (plain sysfs reads, mode 0444).

## How it works

The GUI does **not** hardcode a `hwmonN` index (it can change across reboots / module reloads).
On every refresh it:

1. scans `/sys/class/hwmon/hwmon*/name`,
2. picks the device whose name is `excalibur_g870`,
3. reads its telemetry files,
4. converts and displays them.

A refresh runs every **1000 ms**. If the driver is (re)loaded or removed later, the GUI adapts
without a restart.

## Telemetry shown

| UI | hwmon file | Meaning |
|---|---|---|
| CPU temperature | `temp1_input` | m°C → °C |
| GPU temperature | `temp2_input` | m°C → °C |
| CPU fan | `fan1_input` | RPM *(likely; not independently verified)* |
| GPU fan | `fan2_input` | RPM *(likely; not independently verified)* |

Labels (`temp1_label`, …) are used when present.

## Status model

| Status | Condition | UI |
|---|---|---|
| `CONNECTED` | `excalibur_g870` found + all telemetry readable | Driver: Connected / Telemetry: Active |
| `DEGRADED` | device found but some files missing/unreadable | Driver: Connected / Telemetry: Degraded |
| `DISCONNECTED` | device not found | "Excalibur hardware driver bulunamadı." |

The GUI **never** loads the kernel module itself.

## Prerequisite

The read-only hwmon kernel module must be loaded (by the user, with appropriate privileges):

```
excalibur-wmi.ko   →   hwmon device "excalibur_g870"
```

The GUI works as a normal user; loading the module is outside the GUI's scope.

## Application identity & desktop integration

The application ships a real product identity, kept separate from the
executable/package name (`excalibur-control-center`, a machine identifier):

- **Display name:** `EXCALIBUR Control Center` (window title, About page, launcher).
- **Icon:** `assets/icons/` — a vector master plus PNGs at 16/24/32/48/64/128/256,
  installed under the freedesktop `share/icons/hicolor/<size>x<size>/apps/`
  hierarchy and also embedded in the binary (Qt resource) so the window/tray icon
  never depends on an installed theme or an absolute path.
- **Desktop entry:** `assets/excalibur-control-center.desktop` (installed to
  `share/applications/`), with `Categories=System;Monitor;`, `StartupWMClass`, and
  `Icon=excalibur-control-center`.
- **Version:** one source only — the CMake `project() VERSION`, surfaced through
  `app/version.h` (`EXCALIBUR_VERSION`).

Standard unprivileged install layout (`cmake --install`, `--prefix /usr`):

```
/usr/bin/excalibur-control-center
/usr/share/applications/excalibur-control-center.desktop
/usr/share/icons/hicolor/{16x16,…,256x256,scalable}/apps/excalibur-control-center.{png,svg}
```

## Cooling state

The Dashboard hero shows an honest cooling state derived from real telemetry only
(there is no programmatic OEM fan/performance mode):

| State | Condition |
|---|---|
| Cooling unavailable | no usable fan RPM reading |
| Cooling idle | fans readable but not turning |
| Cooling active | fans turning, system band Normal/Cool |
| Cooling elevated | fans turning with the system band Warm/High |

It never claims a specific OEM mode ("Turbo"/"Fan Boost"/"Gaming") from RPM alone.

## Build

```sh
cmake -S gui -B gui/build
cmake --build gui/build -j"$(nproc)"
```

Requires `qt6-base` (Qt6Widgets), CMake and a C++17 compiler.

## Run

```sh
./gui/build/excalibur-control-center
```

Headless / automated check (no display needed):

```sh
./gui/build/excalibur-control-center --once
```

`--once` prints `state=` plus the read values and exits.

## Testing hook

`EXCALIBUR_HWMON_ROOT` overrides the scan root (default `/sys/class/hwmon`). It exists **only**
to test parsing/status logic against a fake tree; it does not affect hardware.

## Not present in v0.5.1

Power mode, keyboard RGB, fan boost, manual fan control — intentionally absent.
(Manual fan control is unsupported by this firmware: `SET 0x205` is a no-op.)

## Dev-only hooks (not part of the shipped app)

`--once`, `--screenshot`, `--shot-delay`, `--page` and `--size` exist only for automated
validation. They are **off by default** — a normal `cmake` configure builds the app with no
dev hooks compiled in. To enable them for development or screenshots:

```sh
cmake -S gui -B gui/build -G Ninja -DEXCALIBUR_DEV_HOOKS=ON
cmake --build gui/build
```

With the default `EXCALIBUR_DEV_HOOKS=OFF` these flags are removed entirely and a normal
launch never runs them (verified: `--once` is ignored and the GUI starts). `--light`/`--dark`
remain because they are a real theme feature. No dev hook is visible in the UI.

Example (dev build):

```sh
./gui/build/excalibur-control-center --screenshot shot.png --page 1 --size 1000x650
```
