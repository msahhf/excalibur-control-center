#!/bin/bash
# Test an AppImage artifact: type, bundle contents, glibc floor, and runtime under
# offscreen + Xvfb (release gate). Wayland (headless weston) is best-effort and
# reported as PASS/PARTIAL without failing the gate on its own.
#
# Usage: test-appimage.sh <AppImage>
# Exit: 0 = gate passed (offscreen + xcb OK), 1 = gate failed.
set -u
APPIMAGE="${1:?usage: test-appimage.sh <AppImage>}"
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1
apt-get install -y -qq --no-install-recommends \
  file binutils libgl1 libopengl0 libegl1 libxkbcommon0 libxkbcommon-x11-0 libfontconfig1 \
  libharfbuzz0b libfreetype6 libdbus-1-3 xvfb xauth weston libwayland-client0 libwayland-egl1 \
  libx11-6 libx11-xcb1 libxext6 libxrender1 libsm6 libice6 libxau6 libxdmcp6 libxcb1 \
  libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render0 libxcb-render-util0 \
  libxcb-shape0 libxcb-shm0 libxcb-sync1 libxcb-xfixes0 libxcb-util1 libxcb-xkb1 libxcb-cursor0 \
  libxcb-xinput0 libxcb-glx0 libxi6 >/dev/null 2>&1

echo "=== file type ==="; file "$APPIMAGE"
file -b "$APPIMAGE" | grep -q 'ELF 64-bit' || { echo "::error::not an ELF/AppImage"; exit 1; }

cp "$APPIMAGE" /tmp/a.AppImage; chmod +x /tmp/a.AppImage
cd /tmp
APPIMAGE_EXTRACT_AND_RUN=1 /tmp/a.AppImage --appimage-extract >/dev/null 2>&1 || { echo "::error::extract failed"; exit 1; }
S=/tmp/squashfs-root

echo "=== AppRun ==="; file "$S/AppRun"
{ [ -f "$S/AppRun" ] && [ ! -L "$S/AppRun" ]; } || { echo "::error::AppRun must be a regular file"; exit 1; }
echo "=== platform plugins ==="; ls "$S/usr/plugins/platforms" 2>/dev/null
for p in libqoffscreen.so libqxcb.so; do [ -f "$S/usr/plugins/platforms/$p" ] || { echo "::error::missing platform plugin $p"; exit 1; }; done
echo "=== wayland-shell-integration ==="; ls "$S/usr/plugins/wayland-shell-integration" 2>/dev/null || true
echo "=== wayland-graphics-integration-client ==="; ls "$S/usr/plugins/wayland-graphics-integration-client" 2>/dev/null || true

echo "=== glibc/GLIBCXX requirements ==="
GL=$( { readelf -V "$S/usr/bin/excalibur-control-center"; for l in "$S"/usr/lib/libQt6*.so.6; do readelf -V "$l" 2>/dev/null; done; } | grep -oE 'GLIBC_[0-9.]+' | sort -uV | tail -1)
CXX=$( { readelf -V "$S/usr/bin/excalibur-control-center"; for l in "$S"/usr/lib/libQt6*.so.6; do readelf -V "$l" 2>/dev/null; done; } | grep -oE 'GLIBCXX_[0-9.]+' | sort -uV | tail -1)
echo "max GLIBC=${GL}  max GLIBCXX=${CXX}"
# Portability guard: built on Debian 12 (glibc 2.36), the artifact must not require
# a newer glibc than the documented baseline.
if [ "$(printf '%s\n2.36\n' "${GL#GLIBC_}" | sort -V | tail -1)" != "2.36" ]; then
  echo "::error::AppImage requires newer glibc than the 2.36 baseline ($GL)"; exit 1
fi

fail=0
run() { # label, command...
  local label="$1"; shift; local log="/tmp/ta-$label.log"
  timeout 8 "$@" >"$log" 2>&1; local rc=$?
  echo "--- $label : rc=$rc ---"; tail -4 "$log"
  if [ "$rc" = 124 ] && ! grep -qiE 'Could not load the Qt platform plugin|no Qt platform plugin could be initialized|cannot open shared object|error while loading shared libraries' "$log"; then
    echo "$label: PASS"
  else
    echo "$label: FAIL"; fail=1
  fi
}
run offscreen env APPIMAGE_EXTRACT_AND_RUN=1 QT_QPA_PLATFORM=offscreen /tmp/a.AppImage
run xcb env APPIMAGE_EXTRACT_AND_RUN=1 xvfb-run -a /tmp/a.AppImage

# Wayland — best effort (never fails the gate on its own).
export XDG_RUNTIME_DIR=/tmp/wlrun; rm -rf "$XDG_RUNTIME_DIR"; mkdir -p "$XDG_RUNTIME_DIR"; chmod 700 "$XDG_RUNTIME_DIR"
if command -v weston >/dev/null 2>&1; then
  weston --backend=headless-backend.so --socket=wayland-1 --width=1280 --height=720 --idle-time=0 >/tmp/weston.log 2>&1 &
  WP=$!; sleep 4
  if kill -0 "$WP" 2>/dev/null; then
    WAYLAND_DISPLAY=wayland-1 QT_QPA_PLATFORM=wayland QT_LOGGING_RULES='qt.qpa.plugin=true' APPIMAGE_EXTRACT_AND_RUN=1 timeout 8 /tmp/a.AppImage >/tmp/wl.log 2>&1; rc=$?
    echo "--- wayland : rc=$rc ---"
    grep -iE 'Successfully loaded Qt platform plugin|Failed to load client buffer integration|no Qt platform plugin' /tmp/wl.log | head -3
    if grep -q 'Successfully loaded Qt platform plugin "wayland"' /tmp/wl.log; then
      if grep -q 'Failed to load client buffer integration' /tmp/wl.log; then
        echo "wayland: PARTIAL (platform loaded; EGL buffer integration unavailable)"
      else
        echo "wayland: PASS"
      fi
    else
      echo "wayland: PARTIAL (platform plugin not confirmed)"
    fi
    kill "$WP" 2>/dev/null
  else
    echo "wayland: NOT TESTED (weston failed to start)"
  fi
else
  echo "wayland: NOT TESTED (weston not available)"
fi

echo "=== RESULT: $( [ "$fail" = 0 ] && echo PASS || echo FAIL ) ==="
exit "$fail"
