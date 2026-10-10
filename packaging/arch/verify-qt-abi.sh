#!/usr/bin/env bash
#
# Verify that a package's declared minimum Qt (runtime dependency) is at least
# the Qt symbol version the shipped binary actually requires.
#
# Qt uses symbol versioning (Qt_6.N): a binary linked against Qt 6.N refuses to
# load on a libQt6Core that does not provide Qt_6.N. The package's runtime
# dependency `depend = qt6-base>=D` must therefore satisfy D >= R, where R is
# the highest Qt_6.N the binary requires.
#
#   * D >= R : accepted. Declaring a HIGHER minimum than needed is safe (only
#              more restrictive for the user), not an incorrect ABI claim.
#   * D <  R : rejected. A system whose Qt is between D and R would install the
#              package but the binary could not start (false "successful" install).
#
# This script performs no package installation and needs no root.
#
# Usage: verify-qt-abi.sh <binary> <declared_qt_min>
# Exit:  0 = ok, 1 = declared < required (incorrect ABI claim),
#        2 = could not determine the requirement (bad input).

set -euo pipefail

bin="${1:?usage: verify-qt-abi.sh <binary> <declared_qt_min>}"
declared="${2:?usage: verify-qt-abi.sh <binary> <declared_qt_min>}"

[[ -f "$bin" ]] || { echo "verify-qt-abi: no such binary: $bin" >&2; exit 2; }

# Highest Qt_6.N symbol version the binary requires (e.g. "6.12").
required="$(readelf --version-info "$bin" 2>/dev/null \
            | grep -oE 'Qt_6\.[0-9]+' | sed 's/^Qt_//' | sort -uV | tail -n1 || true)"

if [[ -z "$required" ]]; then
  echo "verify-qt-abi: no Qt_6.x symbol requirement found in $bin" >&2
  exit 2
fi

echo "verify-qt-abi: binary requires Qt >= ${required}; package declares Qt >= ${declared}"

if [[ "$(vercmp "$declared" "$required")" -lt 0 ]]; then
  echo "verify-qt-abi: ERROR: declared minimum (${declared}) is lower than the required ${required}" >&2
  exit 1
fi

echo "verify-qt-abi: OK"
exit 0
