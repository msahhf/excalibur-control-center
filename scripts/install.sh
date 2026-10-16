#!/usr/bin/env bash
#
# excalibur-control-center one-command installer
#
# Installs the GUI package *and* the "excalibur-wmi" DKMS driver (via the
# distribution's native package manager) on a supported distribution.
#
# This script NEVER runs sudo, dkms, modprobe, pacman/apt, or any other
# privileged command on its own. It prints exactly what you should run and,
# when it is about to use the package manager, it requires that *you* already
# be root. It never modifies Secure Boot state.
#
# Supported distributions (detected from /etc/os-release):
#   * Arch-based (Arch Linux, CachyOS, ... ; ID=arch / ID_LIKE=arch)
#   * Debian 13 (trixie)      -> needs a 7.2+ kernel + headers
#   * Ubuntu 24.04 LTS (noble)
#
# Minimum kernel: 7.2 (see driver/dkms.conf: BUILD_EXCLUSIVE_KERNEL_MIN="7.2").
#
set -euo pipefail

# --------------------------------------------------------------------------
# Constants
# --------------------------------------------------------------------------
readonly PROJECT_SLUG="msahhf/excalibur-control-center"
readonly API_BASE="https://api.github.com/repos/${PROJECT_SLUG}"
readonly DL_BASE="https://github.com/${PROJECT_SLUG}/releases/download"
readonly GUI_PKG="excalibur-control-center"
readonly DKMS_PKG="excalibur-control-center-dkms"
readonly MIN_KERNEL_MAJOR=7
readonly MIN_KERNEL_MINOR=2

# --------------------------------------------------------------------------
# Logging helpers (INFO / WARN / ERROR), colorized when stdout is a TTY
# --------------------------------------------------------------------------
if [ -t 1 ]; then
    C_RESET=$'\033[0m'; C_INFO=$'\033[1;34m'; C_WARN=$'\033[1;33m'; C_ERR=$'\033[1;31m'
else
    C_RESET=""; C_INFO=""; C_WARN=""; C_ERR=""
fi

info()  { printf '%s[INFO]%s  %s\n'  "$C_INFO" "$C_RESET" "$*"; }
warn()  { printf '%s[WARN]%s  %s\n'  "$C_WARN" "$C_RESET" "$*" >&2; }
error() { printf '%s[ERROR]%s %s\n'  "$C_ERR"  "$C_RESET" "$*" >&2; }
die()   { error "$*"; exit 1; }

usage() {
    cat <<'EOF'
excalibur-control-center one-command installer

Usage:
  ./install.sh [OPTIONS]

Options:
  --check           Do everything except download and install: detect the
                    distribution, verify prerequisites (kernel, headers, dkms)
                    and report which release files WOULD be downloaded.
                    Does NOT require root.
  --version X       Install release X instead of the latest GitHub release
                    (the leading "v" is optional).
  --uninstall       Remove the installed packages with the native package
                    manager. Requires root.
  --help            Show this help and exit.

Notes:
  * This script never calls sudo itself. To install or uninstall you must run
    it as root, e.g.  sudo ./install.sh
  * Requires a running kernel >= 7.2 and, on Debian/Ubuntu, the matching
    kernel headers, because the excalibur-wmi DKMS module is built locally.
EOF
}

# --------------------------------------------------------------------------
# Global state
# --------------------------------------------------------------------------
OPT_CHECK=0
OPT_UNINSTALL=0
OPT_VERSION=""
DISTRO=""            # arch | debian | ubuntu
VERSION=""           # resolved release version, without leading "v"
TMPDIR_INSTALL=""

cleanup() {
    if [ -n "$TMPDIR_INSTALL" ] && [ -d "$TMPDIR_INSTALL" ]; then
        rm -rf "$TMPDIR_INSTALL"
    fi
}
trap cleanup EXIT

# --------------------------------------------------------------------------
# Argument parsing
# --------------------------------------------------------------------------
parse_args() {
    while [ $# -gt 0 ]; do
        case "$1" in
            --check)     OPT_CHECK=1 ;;
            --uninstall) OPT_UNINSTALL=1 ;;
            --help|-h)   usage; exit 0 ;;
            --version)
                [ $# -ge 2 ] || die "--version requires an argument, e.g. --version 0.5.6"
                OPT_VERSION="${2#v}"
                shift
                ;;
            --version=*)
                OPT_VERSION="${1#--version=}"
                OPT_VERSION="${OPT_VERSION#v}"
                ;;
            *)
                error "Unknown option: $1"
                echo
                usage
                exit 1
                ;;
        esac
        shift
    done

    if [ "$OPT_CHECK" -eq 1 ] && [ "$OPT_UNINSTALL" -eq 1 ]; then
        die "--check and --uninstall cannot be combined."
    fi
}

# --------------------------------------------------------------------------
# Distribution detection from /etc/os-release
# --------------------------------------------------------------------------
detect_distro() {
    [ -r /etc/os-release ] || die "/etc/os-release not found; cannot detect the distribution."
    # shellcheck disable=SC1091
    . /etc/os-release

    local id="${ID:-}" like="${ID_LIKE:-}"

    if [ "$id" = "arch" ] || [ "$id" = "cachyos" ] || [[ " $like " == *" arch "* ]]; then
        DISTRO="arch"
        info "Detected distribution: ${PRETTY_NAME:-$id} (Arch-based)"
    elif [ "$id" = "debian" ] && [ "${VERSION_ID:-}" = "13" ]; then
        DISTRO="debian"
        info "Detected distribution: ${PRETTY_NAME:-$id} (Debian 13)"
    elif [ "$id" = "ubuntu" ] && [ "${VERSION_ID:-}" = "24.04" ]; then
        DISTRO="ubuntu"
        info "Detected distribution: ${PRETTY_NAME:-$id} (Ubuntu 24.04)"
    else
        error "Unsupported distribution: ${PRETTY_NAME:-${id:-unknown}} (ID=$id, ID_LIKE=$like)."
        error "This installer supports only: Arch-based (incl. CachyOS), Debian 13 and Ubuntu 24.04."
        error "On other systems please use the AppImage or the source archive instead"
        error "and follow the 'Manual installation' instructions in the README:"
        error "  https://github.com/${PROJECT_SLUG}#installation"
        exit 1
    fi
}

# --------------------------------------------------------------------------
# Prerequisite checks
# --------------------------------------------------------------------------
check_kernel() {
    local rel major minor rest
    rel="$(uname -r)"
    major="${rel%%.*}"
    rest="${rel#*.}"
    minor="${rest%%.*}"

    if ! [[ "$major" =~ ^[0-9]+$ ]] || ! [[ "$minor" =~ ^[0-9]+$ ]]; then
        die "Could not parse the running kernel version from '$rel'."
    fi

    if [ "$major" -lt "$MIN_KERNEL_MAJOR" ] \
       || { [ "$major" -eq "$MIN_KERNEL_MAJOR" ] && [ "$minor" -lt "$MIN_KERNEL_MINOR" ]; }; then
        error "Running kernel $rel is too old."
        error "The excalibur-wmi driver needs kernel ${MIN_KERNEL_MAJOR}.${MIN_KERNEL_MINOR}+ "
        error "(it uses the wmidev_set_block()/wmidev_query_block() API)."
        error "DKMS would refuse to build the module on this kernel. Please boot a"
        error "${MIN_KERNEL_MAJOR}.${MIN_KERNEL_MINOR}+ kernel and re-run the installer."
        exit 1
    fi
    info "Running kernel: $rel (OK, >= ${MIN_KERNEL_MAJOR}.${MIN_KERNEL_MINOR})"
}

check_headers() {
    local build_dir="/lib/modules/$(uname -r)/build"
    if [ -d "$build_dir" ]; then
        info "Kernel headers present: $build_dir"
        return 0
    fi

    error "Kernel headers for the running kernel are missing ($build_dir)."
    error "The excalibur-wmi DKMS module cannot be built without them."
    error "Install the headers package for your running kernel, then re-run this installer:"
    case "$DISTRO" in
        arch)
            error "  sudo pacman -S linux-headers"
            error "  (or the headers package matching your running kernel, e.g. linux-lts-headers)"
            ;;
        debian|ubuntu)
            error "  sudo apt install linux-headers-amd64"
            error "  (or: sudo apt install linux-headers-generic)"
            ;;
    esac
    exit 1
}

check_dkms() {
    if command -v dkms >/dev/null 2>&1; then
        info "dkms found: $(command -v dkms)"
        return 0
    fi
    warn "The 'dkms' command was not found on this system."
    warn "This is usually fine: the package manager will pull in 'dkms' as a"
    warn "dependency of the driver package (the Arch package depends on 'dkms';"
    warn "the Debian/Ubuntu driver .deb depends on 'dkms'). Proceeding."
}

check_secure_boot() {
    command -v mokutil >/dev/null 2>&1 || return 0
    local state
    state="$(mokutil --sb-state 2>/dev/null || true)"
    [ -n "$state" ] || return 0
    info "Secure Boot state: $state"
    if printf '%s' "$state" | grep -qi 'SecureBoot enabled'; then
        warn "Secure Boot is ENABLED. The locally built excalibur-wmi module is NOT"
        warn "signed, so it may refuse to load until you enroll a Machine Owner Key"
        warn "(MOK) yourself. This installer will not touch Secure Boot; see your"
        warn "distribution's documentation if the module fails to load."
    fi
}

# --------------------------------------------------------------------------
# Release / version resolution and asset selection
# --------------------------------------------------------------------------
fetch_release_json() {
    local url="$1" path out

    # Prefer gh when available, fall back to curl.
    if command -v gh >/dev/null 2>&1; then
        path="${url#https://api.github.com/}"
        if out="$(gh api "$path" 2>/dev/null)"; then
            printf '%s' "$out"
            return 0
        fi
        warn "'gh api' failed; falling back to curl."
    fi

    command -v curl >/dev/null 2>&1 \
        || die "Neither 'gh' nor 'curl' is available; cannot query the GitHub API."

    curl -fsSL -H 'Accept: application/vnd.github+json' "$url"
}

json_tag() {
    printf '%s' "$1" \
        | grep -o '"tag_name"[[:space:]]*:[[:space:]]*"[^"]*"' \
        | head -n1 \
        | sed -E 's/.*"([^"]*)"$/\1/'
}

json_asset_names() {
    printf '%s' "$1" \
        | grep -o '"name"[[:space:]]*:[[:space:]]*"[^"]*"' \
        | sed -E 's/.*"([^"]*)"$/\1/'
}

resolve_version() {
    if [ -n "$OPT_VERSION" ]; then
        VERSION="$OPT_VERSION"
        info "Using requested version: $VERSION"
        return 0
    fi

    info "Querying the latest release from the GitHub API..."
    local json tag
    json="$(fetch_release_json "${API_BASE}/releases/latest")" \
        || die "Failed to query the latest release from the GitHub API."
    tag="$(json_tag "$json")"
    [ -n "$tag" ] || die "Could not parse the release tag from the GitHub API response."
    VERSION="${tag#v}"
    info "Latest release: $tag (version $VERSION)"
}

# ARR_ASSETS holds the real asset names of the release (one per line),
# RESULT_ASSETS the ones selected for this distribution.
ARR_ASSETS=""
RESULT_ASSETS=()

select_assets() {
    local json
    if [ -n "$OPT_VERSION" ]; then
        # Tag query for a pinned version; tolerate a "v" prefix or not.
        json="$(fetch_release_json "${API_BASE}/releases/tags/v${VERSION}" 2>/dev/null || true)"
        if [ -z "$json" ] || [ -z "$(json_tag "$json")" ]; then
            json="$(fetch_release_json "${API_BASE}/releases/tags/${VERSION}")" \
                || die "Release v${VERSION} was not found."
        fi
    else
        json="$(fetch_release_json "${API_BASE}/releases/latest")" \
            || die "Failed to query the latest release from the GitHub API."
    fi

    ARR_ASSETS="$(json_asset_names "$json")"
    [ -n "$ARR_ASSETS" ] || die "The release has no assets (or the API response could not be parsed)."

    # Pattern-match against the REAL asset list — never guess an exact name.
    local asset
    case "$DISTRO" in
        arch)
            asset="$(pick_asset '-1-x86_64.pkg.tar.zst')"
            require_asset "$asset" '-1-x86_64.pkg.tar.zst'
            RESULT_ASSETS+=("$asset")
            ;;
        debian)
            asset="$(pick_asset 'deb13_amd64.deb')"
            require_asset "$asset" 'deb13_amd64.deb'
            RESULT_ASSETS+=("$asset")
            asset="$(pick_asset 'deb13_all.deb')"
            require_asset "$asset" 'deb13_all.deb'
            RESULT_ASSETS+=("$asset")
            ;;
        ubuntu)
            asset="$(pick_asset 'ubuntu24.04_amd64.deb')"
            require_asset "$asset" 'ubuntu24.04_amd64.deb'
            RESULT_ASSETS+=("$asset")
            asset="$(pick_asset 'ubuntu24.04_all.deb')"
            require_asset "$asset" 'ubuntu24.04_all.deb'
            RESULT_ASSETS+=("$asset")
            ;;
    esac

    local sums
    sums="$(pick_asset 'SHA256SUMS')"
    if [ "$sums" != "SHA256SUMS" ]; then
        die "SHA256SUMS asset not found in release ${VERSION}; cannot verify downloads."
    fi
    RESULT_ASSETS+=("SHA256SUMS")
}

# pick_asset <literal-substring> -> prints the first matching asset name
pick_asset() {
    printf '%s\n' "$ARR_ASSETS" | grep -F -m1 -- "$1" || true
}

require_asset() {
    local name="$1" pattern="$2"
    if [ -z "$name" ]; then
        error "Release ${VERSION} does not contain an asset matching '${pattern}'."
        error "Available assets in this release:"
        printf '%s\n' "$ARR_ASSETS" | sed 's/^/    /' >&2
        die "Aborting: refusing to guess an asset name."
    fi
}

asset_url() {
    printf '%s/v%s/%s' "$DL_BASE" "$VERSION" "$1"
}

# --------------------------------------------------------------------------
# Installed-state helpers
# --------------------------------------------------------------------------
is_installed() {
    case "$DISTRO" in
        arch)
            pacman -Qi "$GUI_PKG" >/dev/null 2>&1
            ;;
        debian|ubuntu)
            dpkg-query -W -f='${Status}' "$GUI_PKG" 2>/dev/null \
                | grep -q 'install ok installed'
            ;;
    esac
}

require_root() {
    if [ "$(id -u)" -ne 0 ]; then
        error "Root privileges are required for this operation, but this script does"
        error "not call sudo on its own."
        error "Please run it with sudo:"
        error "    sudo ./install.sh $*"
        exit 1
    fi
}

# --------------------------------------------------------------------------
# Download + checksum verification
# --------------------------------------------------------------------------
download_assets() {
    command -v curl >/dev/null 2>&1 || die "'curl' is required to download the release files."
    command -v sha256sum >/dev/null 2>&1 || die "'sha256sum' is required to verify the downloads."

    TMPDIR_INSTALL="$(mktemp -d)"
    info "Downloading release files into a temporary directory..."
    local a
    for a in "${RESULT_ASSETS[@]}"; do
        info "  - $a"
        curl -fsSL -o "${TMPDIR_INSTALL}/${a}" "$(asset_url "$a")" \
            || die "Failed to download ${a}."
    done

    info "Verifying SHA256 checksums..."
    if ! ( cd "$TMPDIR_INSTALL" && sha256sum -c --ignore-missing SHA256SUMS ); then
        die "Checksum verification FAILED. The downloaded files are corrupt or have been tampered with. Aborting."
    fi
    info "All downloads verified against SHA256SUMS."
}

# --------------------------------------------------------------------------
# Install / uninstall
# --------------------------------------------------------------------------
install_packages() {
    local gui="" dkms_deb=""

    info "Installing packages with the native package manager..."
    case "$DISTRO" in
        arch)
            local pkg=""
            local a
            for a in "${RESULT_ASSETS[@]}"; do
                case "$a" in *.pkg.tar.zst) pkg="${TMPDIR_INSTALL}/${a}" ;; esac
            done
            info "Running: pacman -U ${pkg}"
            if ! pacman -U "$pkg"; then
                error "The package manager (pacman) failed. The output above is shown"
                error "unchanged on purpose; please read it. This installer does not"
                error "attempt to work around package-manager errors."
                exit 1
            fi
            ;;
        debian|ubuntu)
            local a
            for a in "${RESULT_ASSETS[@]}"; do
                case "$a" in
                    *_amd64.deb) gui="${TMPDIR_INSTALL}/${a}" ;;
                    *_all.deb)   dkms_deb="${TMPDIR_INSTALL}/${a}" ;;
                esac
            done
            info "Running: apt install ${gui} ${dkms_deb}"
            if ! apt install "$gui" "$dkms_deb"; then
                error "The package manager (apt) failed. The output above is shown"
                error "unchanged on purpose; please read it. This installer does not"
                error "attempt to work around package-manager errors."
                exit 1
            fi
            ;;
    esac
}

uninstall_packages() {
    require_root "--uninstall"
    case "$DISTRO" in
        arch)
            info "Uninstalling: pacman -Rns ${GUI_PKG}"
            if ! pacman -Rns "$GUI_PKG"; then
                error "pacman failed to remove ${GUI_PKG}. Output above is unchanged."
                exit 1
            fi
            ;;
        debian|ubuntu)
            info "Uninstalling: apt remove ${GUI_PKG} ${DKMS_PKG}"
            if ! apt remove "$GUI_PKG" "$DKMS_PKG"; then
                error "apt failed to remove the packages. Output above is unchanged."
                exit 1
            fi
            ;;
    esac
    info "Uninstall complete."
}

post_install_hints() {
    echo
    info "Installation finished. Suggested next steps (run these yourself; this"
    info "installer did not run them):"
    echo
    echo "    sudo modprobe excalibur_wmi        # load the driver now"
    echo "    dkms status                        # check the DKMS module state"
    echo "    cat /sys/class/hwmon/hwmon*/name   # confirm the hwmon sensor appeared"
    echo
}

# --------------------------------------------------------------------------
# --check report
# --------------------------------------------------------------------------
report_check() {
    echo
    info "Check complete. No files were downloaded or installed."
    echo
    echo "  Distribution : $DISTRO"
    echo "  Release      : v${VERSION}"
    echo "  Kernel       : $(uname -r)"
    echo "  Headers      : /lib/modules/$(uname -r)/build"
    echo "  Would download and install:"
    local a
    for a in "${RESULT_ASSETS[@]}"; do
        echo "    - $(asset_url "$a")"
    done
    echo
}

# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------
main() {
    parse_args "$@"

    info "excalibur-control-center installer"
    detect_distro

    # Uninstall only needs the distribution (to pick the right package manager)
    # and root. It must NOT require kernel checks, kernel headers, network
    # access, version resolution or asset selection, so it runs right here —
    # before any of those.
    if [ "$OPT_UNINSTALL" -eq 1 ]; then
        require_root "--uninstall"
        uninstall_packages
        exit 0
    fi

    check_kernel
    check_headers
    check_dkms
    check_secure_boot

    resolve_version
    select_assets

    if [ "$OPT_CHECK" -eq 1 ]; then
        report_check
        exit 0
    fi

    require_root ""

    if is_installed; then
        info "${GUI_PKG} is already installed. Nothing to do."
        exit 0
    fi

    download_assets
    install_packages
    post_install_hints
}

main "$@"
