// English is the source of truth for the translation shape: `Dict` is derived
// from it, so any missing key in tr.ts / de.ts is a compile error.

export const en = {
  meta: {
    title: "EXCALIBUR Control Center — read-only telemetry for EXCALIBUR laptops",
    description:
      "A native Qt6, read-only telemetry app for CASPER EXCALIBUR laptops on Linux: CPU/GPU temperature, fan RPM, system information and an exportable diagnostics report. No root, no hardware control.",
    ogTitle: "EXCALIBUR Control Center — read-only hardware telemetry",
    ogDescription:
      "Read-only thermal and cooling telemetry for CASPER EXCALIBUR laptops on Linux.",
  },

  nav: {
    telemetry: "Telemetry",
    readonly: "Read-only",
    architecture: "Architecture",
    download: "Download",
    compatibility: "Compatibility",
    github: "GitHub",
    get: "Get",
    language: "Language",
    openMenu: "Open menu",
    closeMenu: "Close menu",
    home: "EXCALIBUR Control Center — home",
    repoLink: "GitHub repository",
  },

  hero: {
    kicker: "Read-only hardware telemetry for Linux",
    titleA: "Your hardware.",
    titleB: "Clearly understood.",
    lead:
      "A native Qt6 app that reads the temperature, fan and system telemetry your CASPER EXCALIBUR laptop already exposes on Linux — CPU and GPU temperatures, fan RPM, hardware and kernel details, and an exportable diagnostics report.",
    ctaExplore: "Explore the telemetry",
    ctaDownload: "Download",
    facts: ["CPU / GPU temperature", "Fan RPM", "60-second history", "Diagnostics export", "No root required"],
    shotTitle: "Dashboard",
    shotAlt:
      "Dashboard of the app: CPU and GPU temperatures with a 60-second history and fan speed in RPM",
  },

  story: {
    heading: "Telemetry you can see",
    lead:
      "Each scene is the real application reading real hardware. Move through the four screens — the copy and the screenshot change together.",
    scenesLabel: "Telemetry scenes",
    sceneLabel: "Scene",
    prev: "Previous scene",
    next: "Next scene",
    of: "of",
    scenes: {
      dashboard: {
        title: "Know your thermals",
        body:
          "CPU and GPU temperature lead the dashboard, with a rolling 60-second history so a trend is visible at a glance.",
        alt: "Dashboard screen showing CPU and GPU temperature with history sparklines and fan RPM",
      },
      cooling: {
        title: "See cooling telemetry",
        body:
          "Each channel pairs its temperature with fan speed in RPM and traces both over time, so you can see how cooling responds to load.",
        alt: "Cooling screen showing per-channel temperature and fan-speed history",
      },
      device: {
        title: "Understand your system",
        body:
          "Model, board, BIOS and kernel come from the system itself, alongside the live telemetry interface the driver exposes.",
        alt: "Device screen listing model, board, BIOS, kernel and the telemetry driver",
      },
      diagnostics: {
        title: "Diagnostics when you need them",
        body:
          "One export gathers the app, OS, kernel, telemetry state, sensor map and recent history into portable JSON.",
        alt: "Diagnostics screen with application, system and telemetry sections and an export button",
      },
    },
  },

  readonly: {
    heading: "Read, don't control.",
    lead:
      "The app reads telemetry. It does not change fan speeds, power limits or lighting — that capability is not built in, and no screen pretends otherwise.",
    messages: [
      {
        title: "Read, don't control",
        body: "Every value comes from a read-only source. Nothing is written back to the hardware.",
      },
      {
        title: "One telemetry source",
        body: "The interface reads a single read-only hwmon/sysfs device exposed by the driver.",
      },
      {
        title: "No hardware tuning",
        body: "Fan speed, power limits and RGB are out of scope by design.",
      },
    ],
    diagram: {
      caption: "Telemetry flows one way — from the sensors to the screen.",
      boundary: "READ-ONLY BOUNDARY",
      direction: "one-way read path",
      sensors: "Hardware sensors",
      sensorsSub: "EC / thermal",
      driver: "excalibur-wmi",
      driverSub: "WMI · kernel",
      hwmon: "hwmon / sysfs",
      hwmonSub: "read-only",
      app: "Qt6 application",
      appSub: "single read boundary",
    },
  },

  arch: {
    heading: "How it works",
    lead: "Four stages, matching the project's source code and README.",
    stages: {
      driver: {
        title: "EXCALIBUR WMI kernel driver",
        body:
          "A small read-only kernel module, excalibur-wmi, speaks the laptop's WMI interface and registers a standard hwmon device.",
        code: "excalibur_wmi · WMI GET",
      },
      hwmon: {
        title: "Read-only hwmon / sysfs",
        body:
          "The kernel exposes the readings as ordinary sysfs files under a device named excalibur_g870 — the single, unprivileged interface the app reads.",
        code: "temp1_input · fan1_input",
      },
      app: {
        title: "Qt6 desktop application",
        body:
          "One read boundary discovers the device by name and reads it on a timer. Every page is fed from a single telemetry state.",
        code: "HwmonClient → TelemetryModel",
      },
      output: {
        title: "Monitoring and diagnostics",
        body:
          "The views render the live snapshot; Diagnostics turns the same state into a portable JSON report for support.",
        code: "Dashboard · Cooling · Device · Diagnostics",
      },
    },
  },

  diag: {
    heading: "When something looks off, you have the receipts.",
    lead:
      "Diagnostics gathers everything the app knows into one view and exports it as JSON, so a report carries evidence instead of guesswork.",
    points: [
      "System and kernel identity — OS, kernel, architecture, desktop and session type.",
      "Hardware telemetry that is actually reachable — temperature and fan channels, or an honest “unavailable”.",
      "Recent history, windowed to the last sixty seconds.",
      "One export you can attach to a report — no environment variables, no credentials.",
    ],
    schemaNote: "Representative schema — real field names from the app; values are placeholders.",
    shotTitle: "Diagnostics",
    shotAlt: "Diagnostics screen showing application, system and telemetry sections and an Export button",
  },

  downloads: {
    heading: "Download",
    lead:
      "Every file below is published in the v{version} release. Debian and Ubuntu packages are built against different Qt versions, so pick the one for your distribution.",
    selectorLabel: "Choose your platform",
    download: "Download",
    checksums: "Checksums",
    files: "Files",
    releasePage: "Release page",
    sourceArchive: "Source archive",
    all: "All downloads",
    packages: {
      arch: {
        title: "Arch Linux / CachyOS",
        desc: "One native package with the Qt6 GUI and the integrated excalibur-wmi read-only driver as DKMS.",
        note: "pacman installs this only where the system Qt is at least as new as the build's.",
      },
      debian: {
        title: "Debian 13 (trixie)",
        desc: "A GUI package plus a separate DKMS source package, built against Debian 13's own Qt. Install both for telemetry.",
        note: "Not interchangeable with the Ubuntu package.",
      },
      ubuntu: {
        title: "Ubuntu 24.04 LTS (noble)",
        desc: "Separately built for Ubuntu 24.04's own Qt. Install both packages to get telemetry.",
        note: "Not interchangeable with the Debian package.",
      },
      appimage: {
        title: "Portable AppImage",
        desc: "A single-file Qt6 GUI for any glibc ≥ 2.35 desktop. No root, nothing written to the system — but it is GUI only.",
        note: "The AppImage does not install the excalibur-wmi DKMS driver. Live telemetry needs a native driver package.",
      },
    },
  },

  compat: {
    heading: "Compatibility",
    lead: "What has been tested is marked as verified; the rest is stated plainly.",
    rows: {
      hardware: { label: "CASPER EXCALIBUR G870 / NLXB 001", tag: "Primary target" },
      primaryDistro: { label: "CachyOS / Arch Linux", tag: "Verified" },
      publishedPackages: { label: "Debian 13 · Ubuntu 24.04", tag: "Published" },
      appimage: { label: "AppImage (GUI only)", tag: "Available" },
      kernel72: { label: "Kernel 7.2.x (WMI wmidev_* API)", tag: "Verified" },
      kernel618: { label: "Kernel 6.18.x LTS", tag: "Not supported" },
      fedora: { label: "Fedora RPM", tag: "Not published" },
    },
    note:
      "The read-only driver needs the kernel WMI API available in the 7.2.x series: it does not build on 6.18.x LTS, and a Fedora RPM is not published. The AppImage ships the GUI only.",
  },

  final: {
    heading: "See your system more clearly.",
    lead:
      "Read-only telemetry for CASPER EXCALIBUR laptops on Linux — native, unprivileged, and honest about what it can do.",
    primary: "Download",
    secondary: "Explore the source",
  },

  footer: {
    tagline: "A read-only telemetry control center for CASPER EXCALIBUR laptops on Linux.",
    project: "Project",
    downloadCol: "Download",
    legalCol: "Legal",
    repo: "GitHub repository",
    releases: "Releases",
    readme: "README",
    issues: "Report an issue",
    packages: "Packages & AppImage",
    compatibility: "Compatibility",
    architecture: "Architecture",
    license: "License (GPL-2.0-only)",
    diagnostics: "Diagnostics & support",
    notAffiliated: "Not affiliated with or endorsed by CASPER.",
    noTracking: "Static showcase — no tracking, no cookies.",
    copyright: "© {year} EXCALIBUR Control Center · GPL-2.0-only",
    credit: "Made by",
  },

  a11y: {
    skip: "Skip to content",
    enlarge: "Enlarge screenshot",
    close: "Close",
    lightbox: "Enlarged screenshot",
  },
};

export type Dict = typeof en;
