// Non-textual product facts and real release data. All user-facing copy lives in
// src/i18n/ (per locale); this module only holds identifiers, real asset names,
// URLs and image paths that are the same in every language.

export const RELEASE_VERSION = "0.5.8";
export const REPO = "https://github.com/msahhf/excalibur-control-center";
export const RELEASES = `${REPO}/releases`;
export const RELEASE_PAGE = `${REPO}/releases/tag/v${RELEASE_VERSION}`;
export const README = `${REPO}/blob/main/README.md`;
export const LICENSE_URL = `${REPO}/blob/main/LICENSE`;
export const ISSUES = `${REPO}/issues`;
export const AUTHOR = "https://github.com/msahhf";

const ASSET_BASE = `${REPO}/releases/download/v${RELEASE_VERSION}`;

export type PackageId = "arch" | "debian" | "ubuntu" | "appimage";

export type PackageFile = {
  name: string;
  url: string;
  size?: number;
  sha256?: string;
  distro?: string;
  label?: "sourceBundle";
};

export type FileGroupKey = "main" | "ui" | "driver";
export type FileGroup = { key: FileGroupKey; files: PackageFile[] };

const SUMS: PackageFile = {
  name: "SHA256SUMS",
  url: `${ASSET_BASE}/SHA256SUMS`,
  size: 946,
};

// Real files published in the v0.5.8 release (verified against the GitHub
// release API and by downloading every asset and hashing it; sizes are the
// published asset byte counts and sha256 values match the published
// SHA256SUMS of the same release).
//
// The one-command installer is published as a release asset by
// .github/workflows/release.yml (`install.sh`, a copy of scripts/install.sh).
const INSTALL_SCRIPT: PackageFile = {
  name: "install.sh",
  url: `${ASSET_BASE}/install.sh`,
  size: 37338,
  sha256: "d27a0333d9d85a89a9459eb0fc0053081b43aa6684ddb393ab4a1bc07826fdec",
};

const PKG_ZST: PackageFile = {
  name: `excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
  url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
  size: 184940,
  sha256: "d8ffc721bf0408b865f9055e2b86f3e671d2f677b5990bf6892e0272ff39ead6",
};

const DEB13_GUI: PackageFile = {
  name: `excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
  url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
  size: 141184,
  sha256: "9a36d79b1ef9941987378669a814313ea190880caeae4d132e9a517e3d158b02",
};

const DEB13_DKMS: PackageFile = {
  name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
  url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
  size: 12972,
  sha256: "a8ae28238058a89ebfeb97afcd66768025e598f38f6fa8bf954fe247c5775f0c",
};

const UBU_GUI: PackageFile = {
  name: `excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
  url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
  size: 148428,
  sha256: "1ab80cfd26caed9c6a63c830cf131250712176e8443bacd0c246c408756fa295",
};

const UBU_DKMS: PackageFile = {
  name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
  url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
  size: 13304,
  sha256: "c734ea604cf1889401321fcb21fec371594580a91f9e81cffbf173785cf56017",
};

const APPIMAGE: PackageFile = {
  name: "EXCALIBUR_Control_Center-x86_64.AppImage",
  url: `${ASSET_BASE}/EXCALIBUR_Control_Center-x86_64.AppImage`,
  size: 26667512,
  sha256: "f4fdf101154804e899ac5fb5d60df026c31128769dd573a71d262e78a2620887",
};

const TARGZ: PackageFile = {
  name: `excalibur-control-center-${RELEASE_VERSION}.tar.gz`,
  url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}.tar.gz`,
  size: 308248,
  sha256: "61742bb71538df3f5e267eb355c8692b97865b827af27ba5020f609c5e8cfcd1",
  label: "sourceBundle",
};

// The published files, grouped per platform. AppImage ships the GUI alone, so
// its driver source bundle is a separate group alongside the UI.
export const packageFileGroups: Record<PackageId, FileGroup[]> = {
  arch: [{ key: "main", files: [PKG_ZST, SUMS] }],
  debian: [{ key: "main", files: [DEB13_GUI, DEB13_DKMS, SUMS] }],
  ubuntu: [{ key: "main", files: [UBU_GUI, UBU_DKMS, SUMS] }],
  appimage: [
    { key: "ui", files: [APPIMAGE, SUMS] },
    { key: "driver", files: [TARGZ] },
  ],
};

export const packageMeta: Record<PackageId, { format: string; arch: string }> = {
  arch: { format: "pkg.tar.zst", arch: "x86_64" },
  debian: { format: ".deb", arch: "amd64 + all" },
  ubuntu: { format: ".deb", arch: "amd64 + all" },
  appimage: { format: "AppImage", arch: "x86_64" },
};

export const SOURCE_ARCHIVE = `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}.tar.gz`;

// The one-command installer, published next to the packages in the same
// release (see .github/workflows/release.yml). `scripts/install.sh` in the
// repository is the source of this asset.
export const INSTALL_SCRIPT_ASSET: PackageFile = INSTALL_SCRIPT;
export const INSTALL_SUMS_URL = SUMS.url;

// The exact terminal command shown on the site. Three lines on purpose: the
// script never calls sudo itself, so the site shows download → review → run
// instead of piping a remote file straight into a root shell.
export const INSTALL_COMMAND = [
  `curl -fsSLO ${INSTALL_SCRIPT.url}`,
  "less install.sh",
  "sudo bash install.sh",
].join("\n");

// Distributions the shipped installer actually detects (scripts/install.sh,
// detect_distro) — nothing else is claimed.
export const INSTALL_DISTROS = ["Arch Linux", "CachyOS", "Debian 13", "Ubuntu 24.04"] as const;

export const packageOrder: PackageId[] = ["arch", "debian", "ubuntu", "appimage"];

// Proper nouns, identical in every language — kept out of the translation dict.
export const pickerLabels: Record<PackageId, string> = {
  arch: "Arch",
  debian: "Debian",
  ubuntu: "Ubuntu",
  appimage: "AppImage",
};

// Story scenes: assets only (copy is per-locale, keyed by `id`).
export type StoryId = "dashboard" | "cooling" | "device" | "diagnostics";
export const storyOrder: StoryId[] = ["dashboard", "cooling", "device", "diagnostics"];
export const storyImages: Record<StoryId, string> = {
  dashboard: "./images/dashboard-dark.png",
  cooling: "./images/cooling-dark.png",
  device: "./images/device-dark.png",
  diagnostics: "./images/diagnostics-dark.png",
};

export const APPIMAGE_NODRIVER_IMAGE = "./images/dashboard-nodriver-dark.png";

export const COMPAT_ROWS = [
  "hardware",
  "primaryDistro",
  "publishedPackages",
  "appimage",
  "kernel72",
  "kernel618",
  "fedora",
] as const;
export type CompatRowId = (typeof COMPAT_ROWS)[number];

// In-page navigation targets (labels are localized).
export const NAV_IDS = ["telemetry", "readonly", "architecture", "download", "compatibility"] as const;
export type NavId = (typeof NAV_IDS)[number];
