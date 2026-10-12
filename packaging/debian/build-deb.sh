#!/bin/sh
# Assemble a dpkg-buildpackage source tree from the repository and build the
# .deb binaries. Intended to run inside a Debian/Ubuntu build container.
#
# Layout: the upstream sources (gui/, driver/, analysis/phase3/, LICENSE) are the
# orig tarball; packaging/debian is the debian/ directory (3.0 quilt). A
# per-distribution Debian revision can be supplied so the Debian 13 and Ubuntu
# 24.04 artifacts get distinct, deterministic filenames and never overwrite each
# other in a release:
#
#   DEB_VERSION="0.5.6-1~deb13"  OUT=/out  sh packaging/debian/build-deb.sh
#
# With no DEB_VERSION the version from debian/changelog is used.
set -eu
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$REPO/packaging/debian/dist}"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# Upstream version (before the Debian revision) drives the source dir + orig tar.
UPSTREAM="$(sed -n '1s/^[^(]*(\([^)]*\)).*/\1/p' "$REPO/packaging/debian/changelog" | cut -d- -f1)"
[ -n "$UPSTREAM" ] || { echo "cannot read version from debian/changelog" >&2; exit 1; }

SRC="$WORK/excalibur-control-center-$UPSTREAM"
mkdir -p "$SRC"
cp -a "$REPO/gui" "$SRC/gui"
rm -rf "$SRC/gui/build"
cp -a "$REPO/driver" "$SRC/driver"
mkdir -p "$SRC/analysis"
cp -a "$REPO/analysis/phase3" "$SRC/analysis/phase3"
cp -a "$REPO/LICENSE" "$SRC/LICENSE"
cp -a "$REPO/packaging/debian" "$SRC/debian"

# Optional per-distro revision override (distinct filenames per distribution).
if [ -n "${DEB_VERSION:-}" ]; then
  sed -i "1s/^\([^ ]*\) ([^)]*)/\1 ($DEB_VERSION)/" "$SRC/debian/changelog"
fi

# 3.0 (quilt): the upstream tarball must not contain a debian/ directory.
tar --exclude='./debian' -czf "$WORK/excalibur-control-center_$UPSTREAM.orig.tar.gz" \
    -C "$WORK" "excalibur-control-center-$UPSTREAM"

cd "$SRC"
dpkg-buildpackage -us -uc -b

mkdir -p "$OUT"
cp -v "$WORK"/*.deb "$OUT"/
echo "--- built packages ($OUT) ---"
ls -la "$OUT"
