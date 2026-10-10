#!/bin/bash
# Reproducible AppImage build, tested on debian:stable (Debian 13, Qt 6.8.2).
# Intended to run INSIDE an isolated container; it never touches the host.
#
#   docker run --rm -v "$PWD":/repo:ro -v "$PWD/packaging/appimage":/b.sh:ro \
#     -v "$PWD/out":/out debian:stable bash /b.sh
set -e
REPO=/repo
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1
apt-get install -y -qq --no-install-recommends \
  cmake ninja-build g++ qt6-base-dev qt6-base-dev-tools qt6-svg-dev \
  libgl1-mesa-dev libxkbcommon-dev file wget ca-certificates zlib1g-dev >/dev/null 2>&1

mkdir -p /tools && cd /tools
b=https://github.com
wget -q "$b/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy
wget -q "$b/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O linuxdeploy-plugin-qt
wget -q "$b/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" -O appimagetool
chmod +x linuxdeploy linuxdeploy-plugin-qt appimagetool
export PATH=/tools:$PATH ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 QMAKE=qmake6

rm -rf /build /AppDir
cmake -S "$REPO/gui" -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build /build
mkdir -p /AppDir
DESTDIR=/AppDir cmake --install /build

./linuxdeploy --appdir /AppDir --plugin qt \
  --desktop-file /AppDir/usr/share/applications/excalibur-control-center.desktop \
  --icon-file /AppDir/usr/share/icons/hicolor/256x256/apps/excalibur-control-center.png \
  --output appimage

[ -d /out ] && cp ./*.AppImage /out/ && echo "AppImage -> /out/"
echo "NOTE: GUI only. The excalibur-wmi DKMS driver must be installed separately."
