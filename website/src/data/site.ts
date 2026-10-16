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

export type PackageFile = { name: string; url: string; size?: number };

const SUMS: PackageFile = {
  name: "SHA256SUMS",
  url: `${ASSET_BASE}/SHA256SUMS`,
  size: 869,
};

// Real files published in the v0.5.6 release (verified against the GitHub
// release API, never guessed); sizes are the published asset byte counts.
export const packageFiles: Record<PackageId, PackageFile[]> = {
  arch: [
    {
      name: `excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
      url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
      size: 174157,
    },
    SUMS,
  ],
  debian: [
    {
      name: `excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
      url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
      size: 130768,
    },
    {
      name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
      url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
      size: 12596,
    },
    SUMS,
  ],
  ubuntu: [
    {
      name: `excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
      url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
      size: 138170,
    },
    {
      name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
      url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
      size: 12876,
    },
    SUMS,
  ],
  appimage: [
    {
      name: "EXCALIBUR_Control_Center-x86_64.AppImage",
      url: `${ASSET_BASE}/EXCALIBUR_Control_Center-x86_64.AppImage`,
      size: 26655224,
    },
    SUMS,
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
