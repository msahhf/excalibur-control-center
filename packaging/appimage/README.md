# AppImage (GUI only)

The AppImage packages **only the Qt6 GUI** and the Qt6 libraries it needs. It does
**not** install the `excalibur-wmi` DKMS kernel driver, never asks for root, and
never writes outside the AppImage. Live telemetry requires the driver to be
installed with a native package (Arch/Debian/RPM); without it the GUI shows the
`Unavailable` / `Degraded` state.

## Build

`build-appimage-container.sh` is the verified recipe (run in an isolated
container). Built on **Debian 12 (Qt 6.4.2, glibc 2.36)** — the oldest practical
host — which lowers the resulting glibc floor.

- **Qt >= 6.4** to build (the GUI uses `QStyleHints::colorScheme()`, guarded for
  Qt < 6.5). No longer requires Qt 6.5.
- Tools: `linuxdeploy` + `linuxdeploy-plugin-qt` + `appimagetool` (upstream
  "continuous"), Qt6 dev + `qmake6`, `qt6-qpa-plugins`, `qt6-wayland`.

## Measured result (Debian 12 build)

- `EXCALIBUR_Control_Center-x86_64.AppImage`, valid type-2 AppImage.
- **glibc floor: GLIBC_2.35**, GLIBCXX_3.4.30 (from the bundled Qt). Runs on
  Debian 12+, Ubuntu 22.04+ (glibc 2.35+). Building on Debian 13 instead would
  raise the floor to GLIBC_2.38 (Ubuntu 24.04+).
- Bundled platform plugins: **offscreen, xcb, wayland-egl, wayland**.
- Launcher: a **custom `AppRun`** (LD_LIBRARY_PATH + QT_PLUGIN_PATH) — required so
  Qt can `dlopen()` bundled plugins like `libqxcb.so` (a symlink AppRun that only
  relies on RUNPATH fails this).

## Isolated runtime tests (driver absent)

| Test | Result |
| --- | --- |
| `QT_QPA_PLATFORM=offscreen` (no X) | **PASS** (started, no plugin/lib error) |
| xcb under `xvfb-run` (virtual X) | **PASS** (started) |
| Wayland (compositor) | **NOT TESTED** (plugin bundled; no Wayland compositor in the container) |
| Real physical CASPER telemetry | **NOT TESTED** (no WMI/EC hardware in a container) |

Offscreen and Xvfb are **not** desktop tests. The AppImage still expects some
**host** libraries it deliberately does not bundle (OpenGL/EGL/GLX, freetype,
fontconfig, harfbuzz, X11/Wayland client stack, glibc, libstdc++) — normal on a
desktop, absent in a bare container.
