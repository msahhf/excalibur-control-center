import type { Dict } from "./en";

export const de: Dict = {
  meta: {
    title: "EXCALIBUR Control Center — Nur-Lesen-Telemetrie für EXCALIBUR-Laptops",
    description:
      "Eine native Qt6-App zur Nur-Lesen-Telemetrie für CASPER EXCALIBUR Laptops unter Linux: CPU-/GPU-Temperatur, Lüfterdrehzahl, Systeminformationen und ein exportierbarer Diagnosebericht. Kein Root, keine Hardware-Steuerung.",
    ogTitle: "EXCALIBUR Control Center — Nur-Lesen-Hardware-Telemetrie",
    ogDescription: "Nur-Lesen-Thermal- und Kühlungstelemetrie für CASPER EXCALIBUR Laptops unter Linux.",
  },

  nav: {
    telemetry: "Telemetrie",
    readonly: "Nur Lesen",
    architecture: "Architektur",
    download: "Download",
    compatibility: "Kompatibilität",
    github: "GitHub",
    get: "Holen",
    language: "Sprache",
    openMenu: "Menü öffnen",
    closeMenu: "Menü schließen",
    home: "EXCALIBUR Control Center — Startseite",
    repoLink: "GitHub-Repository",
    primary: "Hauptnavigation",
  },

  hero: {
    kicker: "Nur-Lesen-Hardware-Telemetrie für Linux",
    titleA: "Ihre Hardware.",
    titleB: "Klar verstanden.",
    lead:
      "Eine native Qt6-App, die die Temperatur-, Lüfter- und Systemtelemetrie liest, die Ihr CASPER EXCALIBUR Laptop unter Linux bereits bereitstellt — CPU- und GPU-Temperatur, Lüfterdrehzahl, Hardware- und Kernel-Details sowie ein exportierbarer Diagnosebericht.",
    ctaExplore: "Telemetrie ansehen",
    ctaDownload: "Herunterladen",
    scroll: "Zur Telemetrie",
    metaLabel: "Auf einen Blick",
    facts: [
      "CPU-/GPU-Temperatur",
      "Lüfterdrehzahl",
      "60-Sekunden-Verlauf",
      "Diagnose-Export",
      "Kein Root nötig",
    ],
    shotTitle: "Übersicht",
    shotAlt:
      "Übersicht der App: CPU- und GPU-Temperatur mit 60-Sekunden-Verlauf und Lüfterdrehzahl in U/min",
  },

  story: {
    heading: "Telemetrie, die man sieht",
    lead:
      "Jede Szene ist die echte Anwendung beim Lesen echter Hardware. Bewegen Sie sich durch die vier Bildschirme — Text und Screenshot ändern sich gemeinsam.",
    scenesLabel: "Telemetrie-Szenen",
    sceneLabel: "Szene",
    prev: "Vorherige Szene",
    next: "Nächste Szene",
    of: "von",
    scenes: {
      dashboard: {
        title: "Temperaturen im Blick",
        body:
          "CPU- und GPU-Temperatur führen die Übersicht an, mit einem gleitenden 60-Sekunden-Verlauf, sodass ein Trend sofort sichtbar ist.",
        alt: "Übersicht mit CPU- und GPU-Temperatur, Verlaufsgraphen und Lüfterdrehzahl",
      },
      cooling: {
        title: "Kühlungstelemetrie sehen",
        body:
          "Jeder Kanal verbindet seine Temperatur mit der Lüfterdrehzahl in U/min und zeichnet beides über die Zeit auf, sodass die Reaktion auf Last sichtbar wird.",
        alt: "Kühlungsansicht mit Temperatur und Lüfterdrehzahl je Kanal über die Zeit",
      },
      device: {
        title: "Ihr System verstehen",
        body:
          "Modell, Mainboard, BIOS und Kernel stammen aus dem System selbst, neben der vom Treiber bereitgestellten Live-Telemetrieschnittstelle.",
        alt: "Geräteansicht mit Modell, Mainboard, BIOS, Kernel und dem Telemetrietreiber",
      },
      diagnostics: {
        title: "Diagnose, wenn nötig",
        body:
          "Ein Export bündelt App, Betriebssystem, Kernel, Telemetrie-Status, Sensor-Zuordnung und den jüngsten Verlauf in portablem JSON.",
        alt: "Diagnoseansicht mit den Bereichen Anwendung, System und Telemetrie sowie einer Export-Schaltfläche",
      },
    },
  },

  readonly: {
    heading: "Lesen, nicht steuern.",
    lead:
      "Die App liest Telemetrie. Sie ändert weder Lüfterdrehzahl noch Leistungsgrenzen oder Beleuchtung — diese Fähigkeit ist nicht enthalten, und kein Bildschirm täuscht etwas anderes vor.",
    messages: [
      {
        title: "Lesen, nicht steuern",
        body: "Jeder Wert stammt aus einer Nur-Lesen-Quelle. Nichts wird an die Hardware zurückgeschrieben.",
      },
      {
        title: "Eine Telemetriequelle",
        body: "Die Oberfläche liest ein einzelnes Nur-Lesen-hwmon/sysfs-Gerät, das der Treiber bereitstellt.",
      },
      {
        title: "Keine Hardware-Abstimmung",
        body: "Lüfterdrehzahl, Leistungsgrenzen und RGB liegen bewusst außerhalb des Umfangs.",
      },
    ],
    diagram: {
      caption: "Telemetrie fließt in eine Richtung — von den Sensoren zum Bildschirm.",
      boundary: "NUR-LESEN-GRENZE",
      direction: "einseitiger Lesepfad",
      sensors: "Hardware-Sensoren",
      sensorsSub: "EC / thermisch",
      driver: "excalibur-wmi",
      driverSub: "WMI · Kernel",
      hwmon: "hwmon / sysfs",
      hwmonSub: "nur Lesen",
      app: "Qt6-Anwendung",
      appSub: "eine Lesegrenze",
    },
  },

  arch: {
    heading: "So funktioniert es",
    lead: "Vier Stufen, übereinstimmend mit Quellcode und README des Projekts.",
    stages: {
      driver: {
        title: "EXCALIBUR WMI Kernel-Treiber",
        body:
          "Ein kleines Nur-Lesen-Kernelmodul, excalibur-wmi, spricht die WMI-Schnittstelle des Laptops an und registriert ein Standard-hwmon-Gerät.",
        code: "excalibur_wmi · WMI GET",
      },
      hwmon: {
        title: "Nur-Lesen-hwmon / sysfs",
        body:
          "Der Kernel stellt die Messwerte als gewöhnliche sysfs-Dateien unter einem Gerät namens excalibur_g870 bereit — die einzige, unprivilegierte Schnittstelle, die die App liest.",
        code: "temp1_input · fan1_input",
      },
      app: {
        title: "Qt6-Desktop-Anwendung",
        body:
          "Eine Lesegrenze findet das Gerät über seinen Namen und liest es zeitgesteuert. Jede Seite wird aus einem einzigen Telemetriezustand gespeist.",
        code: "HwmonClient → TelemetryModel",
      },
      output: {
        title: "Überwachung und Diagnose",
        body:
          "Die Ansichten zeigen die Live-Momentaufnahme; die Diagnose macht daraus einen portablen JSON-Bericht für den Support.",
        code: "Übersicht · Kühlung · Gerät · Diagnose",
      },
    },
  },

  diag: {
    heading: "Wenn etwas seltsam aussieht, haben Sie Belege.",
    lead:
      "Die Diagnose bündelt alles, was die App weiß, in einer Ansicht und exportiert es als JSON, damit ein Bericht Belege statt Vermutungen enthält.",
    points: [
      "System- und Kernel-Identität — Betriebssystem, Kernel, Architektur, Desktop und Sitzungstyp.",
      "Tatsächlich erreichbare Hardware-Telemetrie — Temperatur- und Lüfterkanäle oder ein ehrliches „nicht verfügbar“.",
      "Der jüngste Verlauf, auf die letzten sechzig Sekunden begrenzt.",
      "Ein Export für Ihren Bericht — keine Umgebungsvariablen, keine Zugangsdaten.",
    ],
    schemaNote: "Repräsentatives Schema — echte Feldnamen der App; Werte sind Platzhalter.",
    shotTitle: "Diagnose",
    shotAlt: "Diagnoseansicht mit den Bereichen Anwendung, System und Telemetrie sowie einer Export-Schaltfläche",
  },

  downloads: {
    heading: "Download",
    lead:
      "Jede Datei unten ist im Release v{version} veröffentlicht. Debian- und Ubuntu-Pakete sind gegen unterschiedliche Qt-Versionen gebaut — wählen Sie die für Ihre Distribution.",
    selectorLabel: "Plattform wählen",
    download: "Herunterladen",
    checksums: "Prüfsummen",
    files: "Dateien",
    fileCount: "{count} Dateien",
    releasePage: "Release-Seite",
    sourceArchive: "Quellarchiv (GUI + Treiberquelle, Build erforderlich)",
    otherSource: "Sonstiges / Quelle",
    appimageNoDriver: "So sieht die App aus, wenn der Treiber nicht installiert ist.",
    all: "Alle Downloads",
    shaLabel: "SHA-256 (zum Kopieren klicken)",
    groups: {
      ui: "Oberfläche",
      driver: "Treiber (für Live-Telemetrie erforderlich)",
    },
    driverArchNote: "Das Arch-Paket enthält den Treiber bereits.",
    driverPrereq: "Der Treiber braucht Kernel 7.2+, dkms und Kernel-Header.",
    packages: {
      arch: {
        title: "Arch Linux / CachyOS",
        desc:
          "Ein natives Paket mit der Qt6-GUI und dem integrierten excalibur-wmi-Nur-Lesen-Treiber als DKMS.",
        note: "Der excalibur-wmi-Treiber ist in diesem Paket enthalten.",
      },
      debian: {
        title: "Debian 13 (trixie)",
        desc:
          "Ein GUI-Paket plus ein separates DKMS-Quellpaket, gebaut gegen das Qt von Debian 13. Für Telemetrie beide installieren.",
        note: "Diese Pakete sind nur für Debian 13. Für Ubuntu 24.04 die Pakete im Ubuntu-Tab verwenden.",
      },
      ubuntu: {
        title: "Ubuntu 24.04 LTS (noble)",
        desc: "Separat für das Qt von Ubuntu 24.04 gebaut. Für Telemetrie beide Pakete installieren.",
        note: "Diese Pakete sind nur für Ubuntu 24.04. Für Debian 13 die Pakete im Debian-Tab verwenden.",
      },
      appimage: {
        title: "Portables AppImage",
        desc:
          "Eine einteilige Qt6-GUI für jeden Desktop mit glibc ≥ 2.35. Kein Root, nichts wird ins System geschrieben — aber nur die GUI.",
        note: "",
      },
    },
  },

  compat: {
    heading: "Kompatibilität",
    lead: "Getestetes ist als verifiziert gekennzeichnet; der Rest wird offen benannt.",
    rows: {
      hardware: { label: "CASPER EXCALIBUR G870 / NLXB 001", tag: "Hauptziel" },
      primaryDistro: { label: "CachyOS / Arch Linux", tag: "Verifiziert" },
      publishedPackages: { label: "Debian 13 · Ubuntu 24.04", tag: "Veröffentlicht" },
      appimage: { label: "AppImage (nur GUI)", tag: "Verfügbar" },
      kernel72: { label: "Kernel 7.2.x (WMI wmidev_* API)", tag: "Verifiziert" },
      kernel618: { label: "Kernel 6.18.x LTS", tag: "Nicht unterstützt" },
      fedora: { label: "Fedora RPM", tag: "Nicht veröffentlicht" },
    },
    note:
      "Der Nur-Lesen-Treiber benötigt die Kernel-WMI-API der 7.2.x-Serie: Er lässt sich unter 6.18.x LTS nicht bauen, und ein Fedora-RPM ist nicht veröffentlicht. Das AppImage enthält nur die GUI.",
  },

  final: {
    heading: "Sehen Sie Ihr System klarer.",
    lead:
      "Nur-Lesen-Telemetrie für CASPER EXCALIBUR Laptops unter Linux — nativ, unprivilegiert und ehrlich darüber, was sie kann.",
    primary: "Herunterladen",
    secondary: "Quellcode ansehen",
  },

  footer: {
    tagline: "Ein Nur-Lesen-Telemetrie-Kontrollzentrum für CASPER EXCALIBUR Laptops unter Linux.",
    project: "Projekt",
    downloadCol: "Download",
    legalCol: "Rechtliches",
    repo: "GitHub-Repository",
    releases: "Releases",
    readme: "README",
    issues: "Problem melden",
    packages: "Pakete & AppImage",
    compatibility: "Kompatibilität",
    architecture: "Architektur",
    license: "Lizenz (GPL-2.0-only)",
    diagnostics: "Diagnose & Support",
    notAffiliated: "Nicht mit CASPER verbunden oder von CASPER unterstützt.",
    noTracking: "Statische Showcase-Seite — kein Tracking, keine Cookies.",
    copyright: "© {year} EXCALIBUR Control Center · GPL-2.0-only",
    credit: "Made by",
  },

  a11y: {
    skip: "Zum Inhalt springen",
    enlarge: "Screenshot vergrößern",
    close: "Schließen",
    lightbox: "Vergrößerter Screenshot",
  },
};
