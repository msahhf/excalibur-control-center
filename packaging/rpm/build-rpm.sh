#!/bin/sh
# Build the RPM prototype from a source tarball. Run inside a Fedora container.
set -eu
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION=0.5.3
TOPDIR="$(mktemp -d)"
mkdir -p "$TOPDIR"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
tar -c --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    --transform "s,^,excalibur-control-center-$VERSION/," \
    -C "$REPO" gui driver analysis/phase3/excalibur-wmi.c LICENSE \
  | gzip -n > "$TOPDIR/SOURCES/excalibur-control-center-$VERSION.tar.gz"
cp "$REPO/packaging/rpm/excalibur-control-center.spec" "$TOPDIR/SPECS/"
rpmbuild --define "_topdir $TOPDIR" -ba "$TOPDIR/SPECS/excalibur-control-center.spec"
echo "--- built RPMs ---"
find "$TOPDIR/RPMS" -name '*.rpm' -exec ls -la {} \;
echo "--- copy ---"
[ -d /out ] && cp "$TOPDIR"/RPMS/*/*.rpm /out/ 2>/dev/null || true
