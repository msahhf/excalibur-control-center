#!/bin/sh
# Assemble a dpkg-buildpackage source tree from the repository and build the
# .deb binaries. Intended to run inside a Debian/Ubuntu build container.
set -eu
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE"
cp -a "$REPO/gui" "$STAGE/gui"
rm -rf "$STAGE/gui/build"
cp -a "$REPO/driver" "$STAGE/driver"
mkdir -p "$STAGE/analysis"
cp -a "$REPO/analysis/phase3" "$STAGE/analysis/phase3"
cp -a "$REPO/LICENSE" "$STAGE/LICENSE"
cp -a "$REPO/packaging/debian" "$STAGE/debian"
cd "$STAGE"
dpkg-buildpackage -us -uc -b
echo "--- built packages ---"
ls -la ../*.deb
