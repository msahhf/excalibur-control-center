#!/bin/sh
# Build a GUI-only AppImage for EXCALIBUR Control Center.
#
# IMPORTANT: the AppImage bundles the Qt6 GUI ONLY. It does NOT install the
# excalibur-wmi DKMS kernel driver, and it never modifies the system. The driver
# must be installed separately (native package) for live telemetry; without it
# the GUI correctly shows an Unavailable/Degraded state.
#
# Requires: linuxdeploy, linuxdeploy-plugin-qt, appimagetool (download from
# their upstream releases). Run in a container with Qt6 available.
set -eu
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
APPDIR="$(mktemp -d)/AppDir"
mkdir -p "$APPDIR/usr/bin"
cmake -S "$REPO/gui" -B "$REPO/gui/build-appimage" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$REPO/gui/build-appimage"
DESTDIR="$APPDIR" cmake --install "$REPO/gui/build-appimage"
# Bundle Qt and produce the AppImage.
export QMAKE="${QMAKE:-qmake6}"
linuxdeploy --appdir "$APPDIR" --plugin qt \
    --desktop-file "$APPDIR/usr/share/applications/excalibur-control-center.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/excalibur-control-center.png" \
    --output appimage
echo "AppImage built (GUI only). The excalibur-wmi DKMS driver is NOT included."
