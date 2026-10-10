# AppImage (GUI only)

The AppImage packages **only the Qt6 GUI** and the Qt6 libraries it needs. It does
**not** install the `excalibur-wmi` DKMS kernel driver, never asks for root, and
never writes outside the AppImage. Live telemetry requires the driver to be
installed with a native package (Arch/Debian/RPM); without it the GUI shows the
`Unavailable` / `Degraded` state.

## Build requirements (verified)

- **Qt >= 6.5** to build: the code uses `QStyleHints::colorScheme()` /
  `Qt::ColorScheme`, added in Qt 6.5. Verified on **Debian 13 (Qt 6.8.2)**;
  **Ubuntu 24.04 LTS (Qt 6.4.2) cannot build it** — a newer Ubuntu/Debian or a
  newer Qt is required.
- `linuxdeploy` + `linuxdeploy-plugin-qt` + `appimagetool` (upstream "continuous"
  AppImages), Qt6 dev + `qmake6`.
- See `build-appimage-container.sh` for the exact, tested recipe (run inside an
  isolated container; it does not touch the host).

## Verified result (Debian 13, Qt 6.8.2)

- Produced: `EXCALIBUR_Control_Center-x86_64.AppImage` (~26.7 MB), valid type-2
  AppImage (`file` → ELF PIE).
- Bundled: `libQt6Core/Gui/Widgets/DBus/XcbQpa` + the `xcb` platform plugin +
  `.desktop` + icon.
- Driver-absent launch: opened under a virtual X display (Xvfb) with no Qt
  plugin / missing-library errors → the GUI starts without the driver.
- **Caveats (honest):** the bundle only includes the `xcb` platform plugin (no
  `offscreen`), and it relies on some **system** libraries that are not bundled
  (`libharfbuzz`, `libOpenGL`, the xcb/X11 stack). A real desktop provides these;
  a bare container must install them. Real physical CASPER telemetry is not
  tested (no WMI/EC hardware in a container).
