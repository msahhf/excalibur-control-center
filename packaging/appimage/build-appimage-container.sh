#!/bin/bash
# Reproducible AppImage build, verified on debian:12 (Debian 12, Qt 6.4.2,
# glibc 2.36) — the OLDEST practical build host, which lowers the AppImage's
# glibc floor (measured: GLIBC_2.35 required, vs 2.38 when built on Debian 13).
# Run INSIDE an isolated container; it never touches the host:
#
#   mkdir -p out
#   docker run --rm -v "$PWD":/repo:ro -v "$PWD/packaging/appimage/build-appimage-container.sh":/b.sh:ro \
#     -v "$PWD/out":/out debian:12 bash /b.sh
#
# GUI ONLY: the AppImage never installs the excalibur-wmi DKMS driver, never asks
# for root and never writes outside itself.
set -e
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1
apt-get install -y -qq --no-install-recommends \
  cmake ninja-build g++ qt6-base-dev qt6-base-dev-tools qt6-qpa-plugins qt6-wayland qt6-svg-dev \
  libgl1-mesa-dev libxkbcommon-dev file wget ca-certificates zlib1g-dev >/dev/null 2>&1

mkdir -p /tools && cd /tools
b=https://github.com
wget -q "$b/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy
wget -q "$b/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O linuxdeploy-plugin-qt
wget -q "$b/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" -O appimagetool
chmod +x linuxdeploy linuxdeploy-plugin-qt appimagetool
export PATH=/tools:$PATH ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 QMAKE=qmake6

rm -rf /build /AppDir
cmake -S /repo/gui -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build /build
mkdir -p /AppDir
DESTDIR=/AppDir cmake --install /build

# --custom-apprun: linuxdeploy's default AppRun is a symlink that relies on the
# binary's RUNPATH; RUNPATH is not honoured when Qt dlopen()s platform plugins,
# so bundled plugins (e.g. libqxcb) cannot find libQt6XcbQpa. The wrapper sets
# LD_LIBRARY_PATH and QT_PLUGIN_PATH explicitly.
./linuxdeploy --appdir /AppDir --plugin qt --custom-apprun /repo/packaging/appimage/AppRun \
  --desktop-file /AppDir/usr/share/applications/excalibur-control-center.desktop \
  --icon-file /AppDir/usr/share/icons/hicolor/256x256/apps/excalibur-control-center.png

# Provision the platform plugins that actually exist on the build host.
QP=/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms
mkdir -p /AppDir/usr/plugins/platforms
for p in libqoffscreen.so libqxcb.so libqwayland-generic.so libqwayland-egl.so libqwayland.so; do
  [ -f "$QP/$p" ] && cp "$QP/$p" /AppDir/usr/plugins/platforms/
done
# The Wayland platform plugins need the Qt Wayland client library and the
# libwayland client stack; linuxdeploy does not pull them for a non-Wayland app.
for l in libwayland-client.so.0 libwayland-cursor.so.0 libwayland-egl.so.1 \
         libQt6WaylandClient.so.6 libQt6WaylandEglClientHwIntegration.so.6; do
  [ -f "/usr/lib/x86_64-linux-gnu/$l" ] && cp "/usr/lib/x86_64-linux-gnu/$l" /AppDir/usr/lib/ 2>/dev/null
done
# The wayland platform plugin still needs a shell-integration plugin (xdg-shell)
# and its graphics integration; without them the platform loads but cannot create
# a window ("No shell integration named xdg-shell found").
for d in wayland-shell-integration wayland-graphics-integration-client; do
  src="/usr/lib/x86_64-linux-gnu/qt6/plugins/$d"
  if [ -d "$src" ]; then mkdir -p "/AppDir/usr/plugins/$d"; cp "$src"/*.so "/AppDir/usr/plugins/$d/" 2>/dev/null; fi
done
echo "platform plugins: $(ls /AppDir/usr/plugins/platforms)"
echo "shell integration: $(ls /AppDir/usr/plugins/wayland-shell-integration 2>/dev/null)"

ARCH=x86_64 ./appimagetool /AppDir /EXCALIBUR_Control_Center-x86_64.AppImage
[ -d /out ] && cp /EXCALIBUR_Control_Center-x86_64.AppImage /out/ && echo "AppImage -> /out/"
echo "NOTE: GUI only. The excalibur-wmi DKMS driver must be installed separately."
