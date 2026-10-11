#!/bin/sh
# Build the RPM from the repository sources. Run inside a Fedora container.
#
# The version is derived from the RPM spec (single source of truth) so the
# source tarball and the spec can never disagree about the version.
set -eu
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="$(sed -n 's/^Version:[[:space:]]*\([0-9][0-9.]*\).*/\1/p' \
    "$REPO/packaging/rpm/excalibur-control-center.spec" | head -n1)"
[ -n "$VERSION" ] || { echo "cannot read Version from the RPM spec" >&2; exit 1; }
echo "building RPM for version $VERSION"
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
