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
# UI: a numbered, staged terminal interface with status indicators. On a real
# terminal it uses colors and box-drawing characters and offers an optional,
# non-blocking "log panel" (toggle with 'l', force with -v/--verbose, hide
# with -q/--quiet). When output is redirected or piped it degrades to plain
# ASCII text with no panels and never waits for input.
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
readonly UI_WIDTH=64          # width of the stage rule line
readonly PANEL_INNER=66       # inner content width of the log panel (frame = +4)

# --------------------------------------------------------------------------
# Global state
# --------------------------------------------------------------------------
OPT_CHECK=0
OPT_UNINSTALL=0
OPT_VERSION=""
OPT_VERBOSE=0
OPT_QUIET=0
OPT_NO_COLOR=0

DISTRO=""            # arch | debian | ubuntu
VERSION=""           # resolved release version, without leading "v"
TMPDIR_INSTALL=""

MODE="Install"       # Install | Check | Uninstall
TOTAL_STAGES=0
STAGE_IDX=0
BANNER_VERSION=""

# UI state
UI_TTY=0             # stdout is a terminal
UI_FANCY=0           # colors + unicode + panel allowed
PANEL_ON=0           # log panel currently visible
LOG_FILE=""          # buffered verbose detail lines (temp file, TTY only)

# Styles / symbols (filled in by refresh_ui)
C_RESET=""; C_HDR=""; C_OK=""; C_ERR=""; C_WARN=""; C_INFO=""; C_DIM=""
S_OK=""; S_ERR=""; S_WARN=""; S_INFO=""
BOX_TL=""; BOX_TR=""; BOX_BL=""; BOX_BR=""; BOX_H=""; BOX_V=""

# --------------------------------------------------------------------------
# UI setup
# --------------------------------------------------------------------------
refresh_ui() {
    if [ -t 1 ]; then UI_TTY=1; else UI_TTY=0; fi

    if [ "$UI_TTY" -eq 1 ] && [ "$OPT_NO_COLOR" -eq 0 ] && [ -z "${NO_COLOR:-}" ]; then
        UI_FANCY=1
    else
        UI_FANCY=0
    fi

    if [ "$UI_FANCY" -eq 1 ]; then
        C_RESET=$'\033[0m'
        C_HDR=$'\033[1;36m'; C_OK=$'\033[1;32m'; C_ERR=$'\033[1;31m'
        C_WARN=$'\033[1;33m'; C_INFO=$'\033[1;34m'; C_DIM=$'\033[2m'
        S_OK='✔'; S_ERR='✖'; S_WARN='⚠'; S_INFO='▸'
        BOX_TL='╭'; BOX_TR='╮'; BOX_BL='╰'; BOX_BR='╯'
        BOX_H='─'; BOX_V='│'
    else
        C_RESET=""; C_HDR=""; C_OK=""; C_ERR=""; C_WARN=""; C_INFO=""; C_DIM=""
        S_OK='[ok]'; S_ERR='[!!]'; S_WARN='[warn]'; S_INFO='[..]'
        BOX_TL='+'; BOX_TR='+'; BOX_BL='+'; BOX_BR='+'
        BOX_H='-'; BOX_V='|'
    fi
}

# repeat_char <count> <char> — no external tools (works with multi-byte chars)
repeat_char() {
    local n="$1" c="$2" out="" i
    for ((i = 0; i < n; i++)); do out+="$c"; done
    printf '%s' "$out"
}

# --------------------------------------------------------------------------
# Logging helpers: INFO / OK / WARN / ERROR
#   Fancy (TTY):   ✔ ok    ✖ error   ⚠ warn   ▸ info
#   Plain  (pipe): [ok]    [!!]      [warn]   [..]
# --------------------------------------------------------------------------
info()  { [ "$OPT_QUIET" -eq 1 ] && return 0; printf '%s%s%s %s\n' "$C_INFO" "$S_INFO" "$C_RESET" "$*"; }
ok()    { printf '%s%s%s %s\n' "$C_OK"   "$S_OK"   "$C_RESET" "$*"; }
warn()  { printf '%s%s%s %s\n' "$C_WARN" "$S_WARN" "$C_RESET" "$*" >&2; }
error() { printf '%s%s%s %s\n' "$C_ERR"  "$S_ERR"  "$C_RESET" "$*" >&2; }
die()   { error "$*"; exit 1; }

# --------------------------------------------------------------------------
# Optional log panel (verbose details)
# --------------------------------------------------------------------------
# log_detail <line...> — record a detailed line. On a terminal it is appended
# to a buffer file (a file, not an array, so that lines emitted from inside a
# command-substitution subshell still reach the panel); off a terminal it is
# printed inline to stderr in verbose mode only.
log_detail() {
    if [ "$UI_TTY" -eq 1 ]; then
        [ -n "$LOG_FILE" ] || return 0
        printf '%s\n' "$*" >> "$LOG_FILE"
        if [ "$(wc -l < "$LOG_FILE")" -gt 400 ]; then
            tail -n 300 "$LOG_FILE" > "${LOG_FILE}.new"
            mv "${LOG_FILE}.new" "$LOG_FILE"
        fi
    elif [ "$OPT_VERBOSE" -eq 1 ] && [ "$OPT_QUIET" -eq 0 ]; then
        # stderr, so callers capturing stdout (e.g. the GitHub API JSON) do
        # not swallow the detail line.
        printf '    %s\n' "$*" >&2
    fi
}

# Non-blocking key poll, run between stages. Only on an interactive terminal.
# 'l' toggles the log panel. Never blocks, never runs when non-interactive.
poll_key() {
    [ "$UI_TTY" -eq 1 ] || return 0
    [ -r /dev/tty ] || return 0
    local key=""
    if IFS= read -r -s -n 1 -t 0 key < /dev/tty 2>/dev/null; then
        case "$key" in
            l | L)
                if [ "$PANEL_ON" -eq 1 ]; then
                    PANEL_ON=0
                    printf '  %s log panel: hidden\n' "$S_INFO"
                else
                    PANEL_ON=1
                    printf '  %s log panel: shown\n' "$S_INFO"
                fi
                ;;
        esac
    fi
    return 0
}

# Print the buffered detail lines inside a framed panel (only when visible).
flush_log_panel() {
    [ "$PANEL_ON" -eq 1 ] || return 0
    [ -n "$LOG_FILE" ] || return 0
    [ -s "$LOG_FILE" ] || return 0

    local -a lines=()
    mapfile -t lines < "$LOG_FILE"
    : > "$LOG_FILE"

    local inner="$PANEL_INNER"
    local top_fill=$((inner - 4))
    [ "$top_fill" -lt 1 ] && top_fill=1

    printf '%s%s Log %s%s\n' \
        "$C_DIM$BOX_TL" "$BOX_H" "$(repeat_char "$top_fill" "$BOX_H")" "$BOX_TR$C_RESET"

    local line text n pad
    for line in "${lines[@]}"; do
        # Strip ANSI escapes and carriage returns, then truncate to fit.
        text="$(printf '%s' "$line" | sed -E 's/\x1b\[[0-9;]*[A-Za-z]//g; s/\r//g')"
        n=${#text}
        if [ "$n" -gt "$inner" ]; then
            text="${text:0:$((inner - 3))}..."
            n=$inner
        fi
        pad=$((inner - n))
        printf '%s%s %s%s %s%s\n' "$C_DIM" "$BOX_V" "$text" \
            "$(repeat_char "$pad" ' ')" "$BOX_V" "$C_RESET"
    done

    printf '%s%s%s\n\n' "$C_DIM$BOX_BL" "$(repeat_char "$((inner + 2))" "$BOX_H")" "$BOX_BR$C_RESET"
}

# --------------------------------------------------------------------------
# Banner and stage headers
# --------------------------------------------------------------------------
print_banner() {
    if [ "$UI_FANCY" -eq 0 ]; then
        printf '\nexcalibur-control-center - one-command installer\n'
        printf 'Mode: %s   Version: %s\n\n' "$MODE" "$BANNER_VERSION"
        return 0
    fi

    local title="excalibur-control-center"
    local meta="Mode: ${MODE}   ·   Version: ${BANNER_VERSION}"
    local w=${#title}
    [ "${#meta}" -gt "$w" ] && w=${#meta}
    w=$((w + 2))

    local rule
    rule="$(repeat_char "$w" "$BOX_H")"
    printf '\n%s%s%s%s%s\n' "$C_HDR" "$BOX_TL" "$rule" "$BOX_TR" "$C_RESET"
    printf '%s%s%s %-*s %s%s\n' "$C_HDR" "$BOX_V" "$C_RESET" "$((w - 2))" "$title" "$C_HDR" "$BOX_V$C_RESET"
    printf '%s%s%s %-*s %s%s\n' "$C_HDR" "$BOX_V" "$C_RESET" "$((w - 2))" "$meta" "$C_HDR" "$BOX_V$C_RESET"
    printf '%s%s%s%s%s\n' "$C_HDR" "$BOX_BL" "$rule" "$BOX_BR" "$C_RESET"
    printf '\n'
}

# stage <label> — flush details, poll the panel key, then print a numbered,
# aligned stage header.
stage() {
    STAGE_IDX=$((STAGE_IDX + 1))
    poll_key
    flush_log_panel

    local label="$1"
    local ascii_pre="-- [${STAGE_IDX}/${TOTAL_STAGES}] ${label} "
    local pad=$((UI_WIDTH - ${#ascii_pre}))
    [ "$pad" -lt 3 ] && pad=3

    printf '\n%s%s %s %s%s\n' "$C_HDR" \
        "$(repeat_char 2 "$BOX_H")" \
        "[${STAGE_IDX}/${TOTAL_STAGES}] ${label}" \
        "$(repeat_char "$pad" "$BOX_H")" "$C_RESET"
}

# --------------------------------------------------------------------------
# Usage
# --------------------------------------------------------------------------
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
  -v, --verbose     Show the detailed log panel (commands and outputs).
                    Only used on an interactive terminal.
  -q, --quiet       Hide the log panel and the informational messages.
  --no-color        Disable colored output and box-drawing characters
                    (also honored via the NO_COLOR environment variable).
  --help, -h        Show this help and exit.

Notes:
  * This script never calls sudo itself. To install or uninstall you must run
    it as root, e.g.  sudo ./install.sh
  * Requires a running kernel >= 7.2 and, on Debian/Ubuntu, the matching
    kernel headers, because the excalibur-wmi DKMS module is built locally.
  * On an interactive terminal, press 'l' between stages to toggle the log
    panel. When output is piped or redirected, no panel is shown and the
    script never waits for input.
EOF
}

# --------------------------------------------------------------------------
# Temporary directory cleanup
# --------------------------------------------------------------------------
cleanup() {
    if [ -n "$TMPDIR_INSTALL" ] && [ -d "$TMPDIR_INSTALL" ]; then
        rm -rf "$TMPDIR_INSTALL"
    fi
    if [ -n "$LOG_FILE" ] && [ -f "$LOG_FILE" ]; then
        rm -f "$LOG_FILE" "${LOG_FILE}.new"
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
            --verbose|-v) OPT_VERBOSE=1 ;;
            --quiet|-q)  OPT_QUIET=1 ;;
            --no-color)  OPT_NO_COLOR=1 ;;
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
    ok "Running kernel: $rel (>= ${MIN_KERNEL_MAJOR}.${MIN_KERNEL_MINOR})"
}

check_headers() {
    local build_dir="/lib/modules/$(uname -r)/build"
    if [ -d "$build_dir" ]; then
        ok "Kernel headers present: $build_dir"
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
        ok "dkms found: $(command -v dkms)"
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
        log_detail "gh api ${path}"
        if out="$(gh api "$path" 2>/dev/null)"; then
            printf '%s' "$out"
            return 0
        fi
        warn "'gh api' failed; falling back to curl."
    fi

    command -v curl >/dev/null 2>&1 \
        || die "Neither 'gh' nor 'curl' is available; cannot query the GitHub API."

    log_detail "curl -fsSL -H 'Accept: application/vnd.github+json' ${url}"
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
    ok "Latest release: $tag (version $VERSION)"
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
        log_detail "curl -fsSL -o ${TMPDIR_INSTALL}/${a} $(asset_url "$a")"
        curl -fsSL -o "${TMPDIR_INSTALL}/${a}" "$(asset_url "$a")" \
            || die "Failed to download ${a}."
    done

    info "Verifying SHA256 checksums..."
    log_detail "sha256sum -c --ignore-missing SHA256SUMS (in ${TMPDIR_INSTALL})"
    if ! ( cd "$TMPDIR_INSTALL" && sha256sum -c --ignore-missing SHA256SUMS ); then
        die "Checksum verification FAILED. The downloaded files are corrupt or have been tampered with. Aborting."
    fi
    ok "All downloads verified against SHA256SUMS."
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
            log_detail "pacman -U ${pkg}"
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
            log_detail "apt install ${gui} ${dkms_deb}"
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
            log_detail "pacman -Rns ${GUI_PKG}"
            if ! pacman -Rns "$GUI_PKG"; then
                error "pacman failed to remove ${GUI_PKG}. Output above is unchanged."
                exit 1
            fi
            ;;
        debian|ubuntu)
            info "Uninstalling: apt remove ${GUI_PKG} ${DKMS_PKG}"
            log_detail "apt remove ${GUI_PKG} ${DKMS_PKG}"
            if ! apt remove "$GUI_PKG" "$DKMS_PKG"; then
                error "apt failed to remove the packages. Output above is unchanged."
                exit 1
            fi
            ;;
    esac
    ok "Uninstall complete."
}

post_install_hints() {
    echo
    ok "Installation finished."
    info "Suggested next steps (run these yourself; this installer did not run them):"
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
    ok "Check complete. No files were downloaded or installed."
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
    refresh_ui
    parse_args "$@"
    refresh_ui

    # Determine the mode (drives the banner and the stage numbering).
    if [ "$OPT_UNINSTALL" -eq 1 ]; then
        MODE="Uninstall"; TOTAL_STAGES=3
    elif [ "$OPT_CHECK" -eq 1 ]; then
        MODE="Check";     TOTAL_STAGES=6
    else
        MODE="Install";   TOTAL_STAGES=8
    fi

    if [ "$MODE" = "Uninstall" ]; then
        BANNER_VERSION="n/a"
    elif [ -n "$OPT_VERSION" ]; then
        BANNER_VERSION="v${OPT_VERSION}"
    else
        BANNER_VERSION="latest"
    fi

    # The log panel is only ever used on a real terminal, and never in quiet
    # mode. Off a terminal there is no panel and no waiting, ever.
    if [ "$UI_TTY" -eq 1 ] && [ "$OPT_VERBOSE" -eq 1 ] && [ "$OPT_QUIET" -eq 0 ]; then
        PANEL_ON=1
    fi

    # Detail buffer is only needed on a terminal (the panel is TTY-only).
    if [ "$UI_TTY" -eq 1 ]; then
        LOG_FILE="$(mktemp)"
    fi

    print_banner

    # --uninstall only needs the distribution (to pick the right package
    # manager) and root. It must NOT require kernel checks, kernel headers,
    # network access, version resolution or asset selection.
    if [ "$OPT_UNINSTALL" -eq 1 ]; then
        stage "Distribution detection"
        detect_distro
        stage "Root check"
        require_root "--uninstall"
        stage "Uninstall packages"
        uninstall_packages
        flush_log_panel
        exit 0
    fi

    if [ "$OPT_CHECK" -eq 1 ]; then
        stage "Distribution detection"; detect_distro
        stage "Kernel check";           check_kernel
        stage "Kernel headers";         check_headers
        stage "DKMS availability";      check_dkms
        stage "Secure Boot";            check_secure_boot
        stage "Release & assets";       resolve_version; select_assets
        flush_log_panel
        report_check
        exit 0
    fi

    stage "Distribution detection"; detect_distro
    stage "Kernel check";           check_kernel
    stage "Kernel headers";         check_headers
    stage "DKMS & Secure Boot";     check_dkms; check_secure_boot
    stage "Release & assets";       resolve_version; select_assets
    stage "Root & installed state"
    require_root ""
    if is_installed; then
        ok "${GUI_PKG} is already installed. Nothing to do."
        flush_log_panel
        exit 0
    fi
    stage "Download & verify";      download_assets
    stage "Install packages";       install_packages
    flush_log_panel
    post_install_hints
}

main "$@"
