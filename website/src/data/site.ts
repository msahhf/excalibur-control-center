// Non-textual product facts and real release data. All user-facing copy lives in
// src/i18n/ (per locale); this module only holds identifiers, real asset names,
// URLs and image paths that are the same in every language.

export const RELEASE_VERSION = "0.5.6";
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
  size: 869,
};

// Real files published in the v0.5.6 release (verified against the GitHub
// release API, never guessed); sizes are the published asset byte counts and
// sha256 values are the published digests from the same release.
const PKG_ZST: PackageFile = {
  name: `excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
  url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
  size: 174157,
  sha256: "85d430b5251c7d8ac87640c19575d246f62b92efb82a1517d0cf5ec44339be8a",
};

const DEB13_GUI: PackageFile = {
  name: `excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
  url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
  size: 130768,
  sha256: "cb4c1349c463368388ae7a810de2c2433173b756f8ba42c0569e0e17388b4029",
};

const DEB13_DKMS: PackageFile = {
  name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
  url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
  size: 12596,
  sha256: "73a9c101a2780b1879e887975539d9fb8379cf483c4dd594a3a20199300e5a98",
};

const UBU_GUI: PackageFile = {
  name: `excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
  url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
  size: 138170,
  sha256: "0cfb4cdafd9fcbfdc3c1fef657346fbc1fc937a600ce9ec4dc3fad3f0ad5d03d",
};

const UBU_DKMS: PackageFile = {
  name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
  url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
  size: 12876,
  sha256: "50e16f421c068a4b7bb8b454725a53cb76515d3a900f649b352016b9ba3e62c2",
};

const APPIMAGE: PackageFile = {
  name: "EXCALIBUR_Control_Center-x86_64.AppImage",
  url: `${ASSET_BASE}/EXCALIBUR_Control_Center-x86_64.AppImage`,
  size: 26655224,
  sha256: "8186b8a74d85e1df02fb35948bc7943b40358ff473bb0e9b95f30fa3091190d8",
};

const TARGZ: PackageFile = {
  name: `excalibur-control-center-${RELEASE_VERSION}.tar.gz`,
  url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}.tar.gz`,
  size: 394003,
  sha256: "f0acf8b7908cdf643684a57deeb024fe25340a94f22a822b1d165630fdcca877",
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
