# Changelog

## v0.5.1

First public release of **EXCALIBUR Control Center** — a native Linux desktop
application plus an integrated read-only hwmon driver for CASPER EXCALIBUR laptops
(EXCALIBUR G870 / NLXB 001).

### Added

- **Product identity** — application name/icon/desktop entry, AppStream metadata,
  hicolor icon set (16–256 px + SVG), and a `.desktop` launcher entry.
- **Dashboard** — multi-factor system-state hero (CPU/GPU temperature, cooling state)
  with a compact summary, plus CPU/GPU cards (temperature, thermal band, 60 s
  sparkline, fan RPM and 60 s range).
- **Cooling** — per-channel current temperature + fan, 60 s temperature and fan
  history charts, hover readout with real sample ages, and a conservative
  temperature/fan relationship caption.
- **Device** — hardware, firmware and kernel identity (unprivileged).
- **Diagnostics** — a dedicated page showing real application/system/telemetry/
  runtime/settings/sensors state, with **JSON export** (pretty-printed, no secrets;
  canonical temperatures in Celsius).
- **Settings** — theme (System / Light / Dark), temperature unit (Celsius /
  Fahrenheit), refresh interval (1 / 2 / 5 / 10 s) and **Reset to Defaults**, all
  persisted via `QSettings`.
- **System tray & background runtime** — native Qt tray (Open / Show-Hide / Quit),
  instant hide/show, background telemetry, single-instance enforcement (QLockFile).
- **Read-only hwmon driver** (`excalibur_wmi`), delivered via **integrated DKMS**
  inside the single `excalibur-control-center` package. No prebuilt `.ko` is shipped;
  DKMS builds the module per kernel.
- **Instant navigation** — pages are created once and swapped instantly; last-known
  telemetry and the 60 s history are preserved across page changes.

### Packaging

- Single Arch/CachyOS package `excalibur-control-center` (GUI + DKMS driver).
- `depends=('qt6-base' 'dkms')`; no hardcoded kernel-header dependency.
- Standard install layout (`/usr/bin`, `/usr/share/{applications,icons,metainfo,
  licenses}`, `/usr/src/excalibur-wmi-<ver>`).
- Reproducible local source tarball with a real SHA-256 checksum; `.SRCINFO` generated.

### Known limitations

- **Kernel API compatibility:** the driver is verified against the **7.2.x** kernel
  API. It does **not** build against **6.18.x-lts** because `wmidev_set_block` /
  `wmidev_query_block` are absent there. Driver compatibility depends on the kernel
  API.
- **Read-only:** there is **no** fan control, RGB control, power/performance mode, or
  overclocking. The product only reports telemetry.
- **Telemetry fidelity:** the GUI shows raw hwmon values without smoothing or
  filtering. Anomalous firmware/EC samples are displayed as-is.
- **Secure Boot:** the package does not sign kernel modules; on Secure Boot systems
  the user must handle module signing.
- The AUR entry is **not yet published**.

### Not supported (explicitly)

Fan control · RGB/keyboard lighting · power profiles · performance modes ·
overclocking · manual fan curves. These are out of scope for this release.
