// Single source of truth for the site's product facts and external links.
// Values are verified against the repository and the published v0.5.6 release.

export const RELEASE_VERSION = "0.5.6";
export const REPO = "https://github.com/msahhf/excalibur-control-center";
export const RELEASES = `${REPO}/releases`;
export const RELEASE_PAGE = `${REPO}/releases/tag/v${RELEASE_VERSION}`;
export const README = `${REPO}/blob/main/README.md`;
export const LICENSE_URL = `${REPO}/blob/main/LICENSE`;
export const ISSUES = `${REPO}/issues`;

const ASSET_BASE = `${REPO}/releases/download/v${RELEASE_VERSION}`;

export type PackageFile = { name: string; url: string };

export type Package = {
  id: string;
  label: string;
  meta: string;
  title: string;
  desc: string;
  files: PackageFile[];
  note?: string;
};

export const packages: Package[] = [
  {
    id: "arch",
    label: "Arch / CachyOS",
    meta: "pkg.tar.zst · x86_64",
    title: "Arch Linux / CachyOS",
    desc: "One native package that ships the Qt6 GUI and the integrated excalibur-wmi read-only hwmon driver as DKMS. This is the primary, real-hardware verified target.",
    files: [
      {
        name: `excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
        url: `${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}-1-x86_64.pkg.tar.zst`,
      },
    ],
    note: "pacman refuses to install on a Qt older than the ABI the binary was built against.",
  },
  {
    id: "debian",
    label: "Debian 13",
    meta: ".deb · amd64 + all",
    title: "Debian 13 (trixie)",
    desc: "A GUI package plus a separate DKMS source package, built against Debian 13's own Qt (6.8). Install both to get telemetry.",
    files: [
      {
        name: `excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
        url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.deb13_amd64.deb`,
      },
      {
        name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
        url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.deb13_all.deb`,
      },
    ],
    note: "Built against Debian 13's Qt 6.8 — not interchangeable with the Ubuntu package.",
  },
  {
    id: "ubuntu",
    label: "Ubuntu 24.04",
    meta: ".deb · amd64 + all",
    title: "Ubuntu 24.04 LTS (noble)",
    desc: "Separately built for Ubuntu 24.04's own Qt (6.4). Use these packages on Ubuntu; the Debian build will not run there.",
    files: [
      {
        name: `excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
        url: `${ASSET_BASE}/excalibur-control-center_${RELEASE_VERSION}-1.ubuntu24.04_amd64.deb`,
      },
      {
        name: `excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
        url: `${ASSET_BASE}/excalibur-control-center-dkms_${RELEASE_VERSION}-1.ubuntu24.04_all.deb`,
      },
    ],
    note: "Built against Ubuntu 24.04's Qt 6.4 — not interchangeable with the Debian package.",
  },
  {
    id: "appimage",
    label: "AppImage",
    meta: "portable · x86_64",
    title: "Portable AppImage",
    desc: "A single-file Qt6 GUI for any glibc ≥ 2.35 desktop. It needs no root and never touches the system — but it is GUI only.",
    files: [
      {
        name: "EXCALIBUR_Control_Center-x86_64.AppImage",
        url: `${ASSET_BASE}/EXCALIBUR_Control_Center-x86_64.AppImage`,
      },
    ],
    note: "GUI only: the AppImage does NOT install the excalibur-wmi DKMS driver. Live telemetry needs a native driver package.",
  },
];

export type Step = {
  no: string;
  id: string;
  title: string;
  body: string;
  image: string;
  alt: string;
};

export const steps: Step[] = [
  {
    no: "01",
    id: "thermals",
    title: "Know your thermals",
    body: "The dashboard leads with CPU and GPU temperature and a rolling 60-second history, so a trend is visible at a glance — and thresholds are classified into honest Cool/Normal/Warm/High bands.",
    image: "./images/dashboard-dark.png",
    alt: "Dashboard page showing CPU and GPU temperature with history sparklines and fan RPM",
  },
  {
    no: "02",
    id: "cooling",
    title: "See cooling telemetry",
    body: "The Cooling view pairs each channel's temperature with its fan speed (RPM) and traces both over time, describing how the cooling system responds to load.",
    image: "./images/cooling-dark.png",
    alt: "Cooling page showing per-channel temperature and fan-speed history",
  },
  {
    no: "03",
    id: "device",
    title: "Understand your system",
    body: "Model, board, BIOS and kernel are read from the system itself, alongside the live telemetry interface — the WMI-to-hwmon path the driver exposes.",
    image: "./images/device-dark.png",
    alt: "Device page listing model, board, BIOS, kernel and the telemetry driver",
  },
  {
    no: "04",
    id: "diagnostics",
    title: "Diagnostics when you need them",
    body: "A single export gathers the app, OS, kernel, telemetry state, sensor map and recent history into portable JSON you can attach to a support thread.",
    image: "./images/diagnostics-dark.png",
    alt: "Diagnostics page with application, system and telemetry sections and an export button",
  },
];

export const compat = [
  { label: "CASPER EXCALIBUR G870 / NLXB 001", tag: "Primary target", tone: "ok" as const },
  { label: "CachyOS / Arch Linux", tag: "Verified · real hardware", tone: "ok" as const },
  { label: "Debian 13 · Ubuntu 24.04 packages", tag: "Published", tone: "ok" as const },
  { label: "AppImage (GUI only, no driver)", tag: "Available", tone: "muted" as const },
  { label: "Kernel 7.2.x (WMI wmidev_* API)", tag: "Verified", tone: "ok" as const },
  { label: "Kernel 6.18.x LTS", tag: "Not supported", tone: "warn" as const },
  { label: "Fedora RPM", tag: "Not published", tone: "muted" as const },
];

export const nav = [
  { href: "#telemetry", label: "Telemetry" },
  { href: "#readonly", label: "Read-only" },
  { href: "#architecture", label: "Architecture" },
  { href: "#downloads", label: "Download" },
  { href: "#compatibility", label: "Compatibility" },
];
