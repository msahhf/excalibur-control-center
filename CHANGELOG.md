# Changelog

## v0.5.6

Release-automation fix. Identical package contents to v0.5.5.

### Fixed

- **Release checksum verifiability:** the release host rewrites `~` in asset
  filenames (so `…_0.5.5-1~deb13_amd64.deb` was published as
  `…_0.5.5-1.deb13_amd64.deb`), which made the published `SHA256SUMS` list names
  that did not match the actual assets. The release job now applies the same
  normalisation **before** computing the checksums, so the asset names and
  `SHA256SUMS` agree and `sha256sum -c SHA256SUMS` verifies. (Debian package
  filenames therefore use a `.` where the internal Debian version keeps `~`.)

## v0.5.5

Packaging and distribution release. No new application capabilities; still strictly
read-only telemetry.

### Added

- **Debian 13 (`trixie`) and Ubuntu 24.04 LTS (`.deb`)** packages: a GUI package
  (`excalibur-control-center`) and a separate DKMS source package
  (`excalibur-control-center-dkms`), each built against its distribution's own Qt
  version (Debian 13: Qt 6.8; Ubuntu 24.04: Qt 6.4) — they are **not**
  interchangeable. Both are built, installed, launched and removed by the release
  CI in isolated containers, and published as release assets.
- **Unit tests** (`gui/tests/`, `-DEXCALIBUR_BUILD_TESTS=ON`) covering temperature
  conversion/formatting and settings parsing/validation, run in CI.

### Fixed

- **Debian/Ubuntu DKMS hooks:** `postinst`/`prerm` no longer hard-code the module
  version (previously `0.5.3`, which made them silently no-op for any other
  release). The version is derived from the installed `/usr/src/excalibur-wmi-*`
  directory.
- **Debian packaging:** switched to `3.0 (quilt)` with a per-distribution Debian
  revision (`~deb13` / `~ubuntu24.04`) so the two `.deb` sets have distinct,
  deterministic filenames; the GUI package no longer hard-depends on `dkms`
  (it is a `Recommends`, and the GUI works without the driver); no auto-generated
  `-dbgsym` packages.
- **RPM spec/script:** the source-tarball version is now derived from the spec
  (previously `build-rpm.sh` hard-coded `0.5.3` and disagreed with the spec).
- **Dev settings hooks:** combined `--set-*` / `--reset-settings` flags in one
  invocation now all apply (previously only the first was honoured).

### Changed

- **Release CI** now also builds+tests the Debian 13 and Ubuntu 24.04 `.deb`
  packages; the `release` job requires the AppImage **and** both `.deb` jobs, and
  publishes all of them, their source archive and a combined `SHA256SUMS`.

### Known limitations

- **Fedora:** an RPM recipe exists but is **not published/supported** — its
  lifecycle was only exercised in a container, never in a verified Fedora VM.
- **AppImage Wayland:** verified under an isolated headless compositor (now
  PASS with the bundled shell/graphics integration); a real KDE/Wayland desktop
  run of the *AppImage* was not performed (the native packages were tested on a
  real KDE/Wayland session).
- **Real hardware / kernel API:** live CASPER telemetry is verified on the
  maintainer's real EXCALIBUR G870 / 7.2.x kernel; CI/containers have no WMI/EC
  hardware. The driver does **not** build against **6.18.x-lts**.
- **Debian/Ubuntu GUI on their real desktops:** install/launch/remove are verified
  in isolated containers; the full GUI/telemetry path is verified on the real
  Arch host, not inside a Debian/Ubuntu VM.

## v0.5.4

Packaging and compatibility patch. No new application capabilities; still strictly
read-only telemetry.

### Added

- **AppImage (GUI only)** distribution: a single-file AppImage built on Debian 12
  (glibc >= 2.35), now produced and tested by the release CI (offscreen + Xvfb)
  and published alongside the Arch package. It bundles the Qt6 GUI **only** — it
  does not install the DKMS driver, does not require root and does not touch the
  system.
- **Debian (`.deb`) and Fedora (`.rpm`) packaging prototypes** under `packaging/`
  (not published; see the repository for build scripts).

### Changed

- **Qt 6.4 support:** the GUI now builds on Qt 6.4 (Ubuntu 24.04, Debian 12) by
  guarding `QStyleHints::colorScheme` (Qt >= 6.5). Behaviour on Qt >= 6.5 is
  unchanged.
- **DKMS:** `BUILD_EXCLUSIVE_KERNEL_MIN="7.2"` so DKMS skips kernels older than
  7.2 (e.g. 6.18.x-lts) **before compiling**, with a clear message, instead of a
  misleading build failure.

### Known limitations

- **Fedora:** the RPM/VM path is **NOT TESTED** (deferred; VirtualBox unattended
  install is not compatible with the Fedora Workstation Live ISO).
- **AppImage Wayland:** the platform plugin loads under an isolated headless
  compositor, but the EGL client buffer integration is not guaranteed →
  **PARTIAL**.
- **Real hardware / kernel API:** live CASPER telemetry and the `wmidev_*` kernel
  API are **NOT TESTED** here (verified only on the maintainer's 7.2.x device).
- **Kernel API compatibility:** the driver builds against the **7.2.x** API and
  does **not** build against **6.18.x-lts**.

## v0.5.3

Packaging/release hardening patch. No new capabilities; still strictly read-only
telemetry.

### Fixed

- **Prebuilt package could install on an incompatible Qt.** The release binary is
  linked against a specific Qt and needs that symbol version at runtime
  (`Qt_6.N`), but the package declared only an unversioned `qt6-base` dependency,
  so it installed on older Qt and then failed to start. The release package now
  declares the exact minimum Qt it was built against (derived from the build
  environment and verified against the binary), so pacman refuses to install it
  where it could not run. Source/AUR builds stay unconstrained and compile against
  the system Qt.
- **Self-referential source checksum.** The PKGBUILD records the SHA-256 of the
  release source archive, but that archive contained the PKGBUILD itself, making
  the checksum impossible to satisfy for the tagged commit. `packaging/` and
  `.github/` are now excluded from source archives (`.gitattributes`
  `export-ignore`), so the checksum is a stable function of the build inputs. CI
  verifies the PKGBUILD checksum against the published tag archive.

### Changed

- Removed the deprecated `CLEAN` directive from `driver/dkms.conf` (DKMS >= 3
  ignores it and warned about it).

### Notes

- Kernel API incompatibility is unchanged: the driver builds against the
  **7.2.x** API and does **not** build against **6.18.x-lts** (`wmidev_*` absent).
- No fan/RGB/power/performance control; no hardware writes.

## v0.5.2

Patch release from the V1.0 hardening pass. Two real defects found on real
hardware were fixed; no new capabilities were added. Still strictly read-only.

### Fixed

- **Duplicate diagnostics connection on theme change.** The telemetry → diagnostics
  connection was re-established on every theme rebuild (the receiver is the main
  window, which survives the rebuild), so slots accumulated with each theme switch
  and diagnostics work grew with the number of switches. The connection is now made
  once. (Reproduced: 20 s of theme switching produced 3546 diagnostics rebuilds;
  after the fix, 354.)
- **History window semantics.** History kept a fixed 60 **samples**, which at the
  default 2 s refresh produced ~120 s of history while the UI and diagnostics export
  reported "60 s" (up to ~600 s at a 10 s refresh). History is now a true rolling
  **60-second** window based on real sample timestamps at every refresh rate; the
  Cooling "…s ago" label is derived from the same timestamps.

### Changed

- Release workflow regenerates the source checksum from the published tag archive
  before building (the PKGBUILD source is the tag's own archive, so its SHA256 cannot
  be embedded in the tagged commit).

### Notes

- Known kernel API incompatibility is unchanged: the driver builds against the
  **7.2.x** API and does **not** build against **6.18.x-lts** (`wmidev_*` absent).
- No fan/RGB/power/performance control; no hardware writes.

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
