# AppImage (GUI only)

The AppImage packages **only the Qt6 GUI** and its Qt dependencies. It does
**not** install the `excalibur-wmi` DKMS kernel driver and never touches the
system. Live telemetry requires the driver to be installed with a native package
(Arch/Debian/RPM); without it the GUI shows `Unavailable`/`Degraded`.

Build: `packaging/appimage/build-appimage.sh` (needs linuxdeploy + qt plugin +
appimagetool). Not built in this phase.
