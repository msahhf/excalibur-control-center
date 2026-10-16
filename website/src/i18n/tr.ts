import type { Dict } from "./en";

export const tr: Dict = {
  meta: {
    title: "EXCALIBUR Control Center — EXCALIBUR dizüstüler için salt okunur telemetri",
    description:
      "CASPER EXCALIBUR dizüstüleri için Linux'ta çalışan, Qt6 ile yazılmış salt okunur telemetri uygulaması: CPU/GPU sıcaklığı, fan devri, sistem bilgileri ve dışa aktarılabilir tanılama raporu. Root yok, donanım kontrolü yok.",
    ogTitle: "EXCALIBUR Control Center — salt okunur donanım telemetrisi",
    ogDescription: "CASPER EXCALIBUR dizüstüleri için Linux'ta salt okunur sıcaklık ve soğutma telemetrisi.",
  },

  nav: {
    telemetry: "Telemetri",
    readonly: "Salt okunur",
    architecture: "Mimari",
    download: "İndir",
    compatibility: "Uyumluluk",
    github: "GitHub",
    get: "Al",
    language: "Dil",
    openMenu: "Menüyü aç",
    closeMenu: "Menüyü kapat",
    home: "EXCALIBUR Control Center — ana sayfa",
    repoLink: "GitHub deposu",
    primary: "Ana gezinme",
  },

  hero: {
    kicker: "Linux için salt okunur donanım telemetrisi",
    titleA: "Donanımınız.",
    titleB: "Net biçimde anlaşılır.",
    lead:
      "CASPER EXCALIBUR dizüstünüzün Linux'ta zaten sunduğu sıcaklık, fan ve sistem telemetrisini okuyan Qt6 tabanlı bir uygulama: CPU ve GPU sıcaklığı, fan devri, donanım ve çekirdek ayrıntıları ve dışa aktarılabilir tanılama raporu.",
    ctaExplore: "Telemetriyi keşfet",
    ctaDownload: "İndir",
    scroll: "Telemetriye kaydır",
    metaLabel: "Kısaca",
    facts: ["CPU / GPU sıcaklığı", "Fan devri", "60 saniyelik geçmiş", "Tanılama dışa aktarımı", "Root gerekmez"],
    shotTitle: "Gösterge Paneli",
    shotAlt: "Uygulamanın gösterge paneli: CPU ve GPU sıcaklıkları, 60 saniyelik geçmiş ve fan devri",
  },

  story: {
    heading: "Görülebilir telemetri",
    lead:
      "Her sahne, gerçek donanımı okuyan gerçek uygulamadır. Dört ekran arasında ilerleyin — metin ve ekran görüntüsü birlikte değişir.",
    scenesLabel: "Telemetri sahneleri",
    sceneLabel: "Sahne",
    prev: "Önceki sahne",
    next: "Sonraki sahne",
    of: "/",
    scenes: {
      dashboard: {
        title: "Sıcaklıklarınızı tanıyın",
        body:
          "Gösterge paneline CPU ve GPU sıcaklıkları yön verir; 60 saniyelik kayan geçmiş sayesinde eğilim tek bakışta görülür.",
        alt: "CPU ve GPU sıcaklıkları, geçmiş grafikleri ve fan devrini gösteren gösterge paneli",
      },
      cooling: {
        title: "Soğutma telemetrisini görün",
        body:
          "Her kanal sıcaklığını fan devriyle (RPM) eşleştirir ve ikisini zaman içinde izler; soğutmanın yüke nasıl tepki verdiği görülür.",
        alt: "Kanal başına sıcaklık ve fan devri geçmişini gösteren soğutma ekranı",
      },
      device: {
        title: "Sisteminizi tanıyın",
        body:
          "Model, anakart, BIOS ve çekirdek doğrudan sistemden okunur; sürücünün sunduğu canlı telemetri arayüzüyle birlikte gösterilir.",
        alt: "Model, anakart, BIOS, çekirdek ve telemetri sürücüsünü listeleyen cihaz ekranı",
      },
      diagnostics: {
        title: "Gerektiğinde tanılama",
        body:
          "Tek bir dışa aktarım; uygulama, işletim sistemi, çekirdek, telemetri durumu, sensör eşlemesi ve son geçmişi taşınabilir JSON'da toplar.",
        alt: "Uygulama, sistem ve telemetri bölümleri ile dışa aktarma düğmesini gösteren tanılama ekranı",
      },
    },
  },

  readonly: {
    heading: "Oku, kontrol etme.",
    lead:
      "Uygulama telemetri okur. Fan hızını, güç limitlerini veya aydınlatmayı değiştirmez — bu yetenek içinde yoktur ve hiçbir ekran aksini ima etmez.",
    messages: [
      {
        title: "Oku, kontrol etme",
        body: "Her değer salt okunur bir kaynaktan gelir. Donanıma hiçbir şey geri yazılmaz.",
      },
      {
        title: "Tek telemetri kaynağı",
        body: "Arayüz, sürücünün sunduğu tek bir salt okunur hwmon/sysfs aygıtını okur.",
      },
      {
        title: "Donanım ayarı yok",
        body: "Fan hızı, güç limitleri ve RGB tasarım gereği kapsam dışıdır.",
      },
    ],
    diagram: {
      caption: "Telemetri tek yönde akar — sensörlerden ekrana.",
      boundary: "SALT OKUNUR SINIR",
      direction: "tek yönlü okuma yolu",
      sensors: "Donanım sensörleri",
      sensorsSub: "EC / termal",
      driver: "excalibur-wmi",
      driverSub: "WMI · çekirdek",
      hwmon: "hwmon / sysfs",
      hwmonSub: "salt okunur",
      app: "Qt6 uygulaması",
      appSub: "tek okuma sınırı",
    },
  },

  arch: {
    heading: "Nasıl çalışır",
    lead: "Projenin kaynak kodu ve README'siyle örtüşen dört aşama.",
    stages: {
      driver: {
        title: "EXCALIBUR WMI çekirdek sürücüsü",
        body:
          "Küçük, salt okunur bir çekirdek modülü olan excalibur-wmi, dizüstünün WMI arayüzüyle konuşur ve standart bir hwmon aygıtı kaydeder.",
        code: "excalibur_wmi · WMI GET",
      },
      hwmon: {
        title: "Salt okunur hwmon / sysfs",
        body:
          "Çekirdek, okumaları excalibur_g870 adlı aygıt altında sıradan sysfs dosyaları olarak sunar — uygulamanın okuduğu tek, ayrıcalıksız arayüz.",
        code: "temp1_input · fan1_input",
      },
      app: {
        title: "Qt6 masaüstü uygulaması",
        body:
          "Tek bir okuma sınırı, aygıtı adından bulur ve bir zamanlayıcıyla okur. Her sayfa tek bir telemetri durumundan beslenir.",
        code: "HwmonClient → TelemetryModel",
      },
      output: {
        title: "İzleme ve tanılama",
        body:
          "Görünümler canlı anlık görüntüyü çizer; Tanılama aynı durumu destek için taşınabilir JSON raporuna dönüştürür.",
        code: "Gösterge Paneli · Soğutma · Cihaz · Tanılama",
      },
    },
  },

  diag: {
    heading: "Bir şey tuhaf göründüğünde elinizde kanıt olur.",
    lead:
      "Tanılama, uygulamanın bildiği her şeyi tek görünümde toplar ve JSON olarak dışa aktarır; böylece bir bildirim tahmin yerine kanıt taşır.",
    points: [
      "Sistem ve çekirdek kimliği — işletim sistemi, çekirdek, mimari, masaüstü ve oturum türü.",
      "Gerçekten erişilebilen donanım telemetrisi — sıcaklık ve fan kanalları ya da dürüst bir “kullanılamıyor”.",
      "Son altmış saniyeye pencereleyen güncel geçmiş.",
      "Bir bildirime ekleyebileceğiniz tek dışa aktarım — ortam değişkeni yok, kimlik bilgisi yok.",
    ],
    schemaNote: "Temsili şema — alan adları uygulamadan alınmıştır; değerler yer tutucudur.",
    shotTitle: "Tanılama",
    shotAlt: "Uygulama, sistem ve telemetri bölümleri ile Dışa Aktar düğmesini gösteren tanılama ekranı",
  },

  downloads: {
    heading: "İndir",
    lead:
      "Aşağıdaki her dosya v{version} sürümünde yayımlanmıştır. Debian ve Ubuntu paketleri farklı Qt sürümlerine göre derlenir; dağıtımınıza uygun olanı seçin.",
    selectorLabel: "Platformunuzu seçin",
    download: "İndir",
    checksums: "Sağlama toplamları",
    files: "Dosyalar",
    fileCount: "{count} dosya",
    releasePage: "Sürüm sayfası",
    sourceArchive: "Kaynak arşivi (arayüz + sürücü kaynağı, derleme gerekir)",
    otherSource: "Diğer / Kaynak",
    appimageNoDriver: "Sürücü kurulu değilken uygulama böyle görünür.",
    all: "Tüm indirmeler",
    shaLabel: "SHA-256 (kopyalamak için tıklayın)",
    groups: {
      ui: "Arayüz",
      driver: "Sürücü (canlı telemetri için gerekli)",
    },
    driverArchNote: "Arch paketi sürücüyü zaten içerir.",
    driverPrereq: "Sürücü için çekirdek 7.2+, dkms ve çekirdek başlıkları gerekir.",
    packages: {
      arch: {
        title: "Arch Linux / CachyOS",
        desc:
          "Qt6 arayüzünü ve DKMS olarak entegre excalibur-wmi salt okunur sürücüsünü içeren tek yerel paket.",
        note: "excalibur-wmi sürücüsü bu pakete dahildir.",
      },
      debian: {
        title: "Debian 13 (trixie)",
        desc:
          "Bir GUI paketi ve ayrı bir DKMS kaynak paketi; Debian 13'ün kendi Qt'sine göre derlenmiştir. Telemetri için ikisini de kurun.",
        note: "Bu paketler yalnızca Debian 13 içindir. Ubuntu 24.04 için Ubuntu sekmesindeki paketleri kullanın.",
      },
      ubuntu: {
        title: "Ubuntu 24.04 LTS (noble)",
        desc:
          "Ubuntu 24.04'ün kendi Qt'sine göre ayrıca derlenmiştir. Telemetri için iki paketi de kurun.",
        note: "Bu paketler yalnızca Ubuntu 24.04 içindir. Debian 13 için Debian sekmesindeki paketleri kullanın.",
      },
      appimage: {
        title: "Taşınabilir AppImage",
        desc:
          "glibc ≥ 2.35 olan her masaüstü için tek dosyalık Qt6 arayüzü. Root gerekmez, sisteme yazmaz — ancak yalnızca arayüzdür.",
        note: "",
      },
    },
  },

  compat: {
    heading: "Uyumluluk",
    lead: "Test edilenler doğrulanmış olarak işaretlenir; gerisi açıkça belirtilir.",
    rows: {
      hardware: { label: "CASPER EXCALIBUR G870 / NLXB 001", tag: "Ana hedef" },
      primaryDistro: { label: "CachyOS / Arch Linux", tag: "Doğrulandı" },
      publishedPackages: { label: "Debian 13 · Ubuntu 24.04", tag: "Yayımlandı" },
      appimage: { label: "AppImage (yalnızca arayüz)", tag: "Mevcut" },
      kernel72: { label: "Çekirdek 7.2.x (WMI wmidev_* API)", tag: "Doğrulandı" },
      kernel618: { label: "Çekirdek 6.18.x LTS", tag: "Desteklenmiyor" },
      fedora: { label: "Fedora RPM", tag: "Yayımlanmadı" },
    },
    note:
      "Salt okunur sürücü, 7.2.x serisindeki çekirdek WMI API'sine ihtiyaç duyar: 6.18.x LTS'te derlenmez ve Fedora RPM yayımlanmamıştır. AppImage yalnızca arayüzü içerir.",
  },

  final: {
    heading: "Sisteminizi daha net görün.",
    lead:
      "CASPER EXCALIBUR dizüstüleri için Linux'ta salt okunur telemetri — yerel, ayrıcalıksız ve ne yapabildiği konusunda dürüst.",
    primary: "İndir",
    secondary: "Kaynağı incele",
  },

  footer: {
    tagline: "CASPER EXCALIBUR dizüstüleri için Linux'ta salt okunur telemetri kontrol merkezi.",
    project: "Proje",
    downloadCol: "İndir",
    legalCol: "Yasal",
    repo: "GitHub deposu",
    releases: "Sürümler",
    readme: "README",
    issues: "Sorun bildir",
    packages: "Paketler ve AppImage",
    compatibility: "Uyumluluk",
    architecture: "Mimari",
    license: "Lisans (GPL-2.0-only)",
    diagnostics: "Tanılama ve destek",
    notAffiliated: "CASPER ile bağlantılı veya onun tarafından onaylanmış değildir.",
    noTracking: "Statik tanıtım — izleme yok, çerez yok.",
    copyright: "© {year} EXCALIBUR Control Center · GPL-2.0-only",
    credit: "Made by",
  },

  a11y: {
    skip: "İçeriğe geç",
    enlarge: "Ekran görüntüsünü büyüt",
    close: "Kapat",
    lightbox: "Büyütülmüş ekran görüntüsü",
  },
};
