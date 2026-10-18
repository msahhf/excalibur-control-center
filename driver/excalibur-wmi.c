// SPDX-License-Identifier: GPL-2.0
/*
 * excalibur-wmi.c — hwmon (salt-okuma) + RGB LED kontrol surucusu
 *                   CASPER EXCALIBUR G870 (NLXB 001)
 *
 * ============================================================================
 * 1) TELEMETRI (salt-okuma hwmon)
 * ============================================================================
 * Tek telemetri kaynagi: ACPI-WMI RW_GMWMI data block'u uzerinden GET 0x200.
 *   GUID     : 644C5791-B7B0-4123-A90B-E93876E0DAAD
 *   instance : 0
 *   istek    : a0 = 0xFA00 (GET), a1 = 0x0200   (32 byte)
 *
 * DSDT kaniti (WSAA, GWF0==0xFA00 && GWF1==0x0200) bu dalda YALNIZCA okuma yapar:
 *   DAT2 = ECRD(RTMP)  (CPU sicaklik)
 *   DAT3 = ECRD(VGAT)  (GPU sicaklik)
 *   DAT4 = (FS1H<<8)|FS1L  (CPU fan tach)
 *   DAT5 = (FS2H<<8)|FS2L  (GPU fan tach)
 * Hicbir ECWT / hardware write yoktur.
 *
 * wmidev_set_block() WMI adiyla "set block" olsa da, 0xFA00/0x0200 icin firmware
 * yalnizca OKUMA dalini calistirir (istegi BUFF'e yazip EC'den okur); baska komut
 * (herhangi bir SET) gonderilmez. Bu yuzden salt-okuma icin guvenlidir.
 *
 * hwmon eslemesi (read-only):
 *   temp1_input = a2 * 1000 (CPU, milidegree)   fan1_input = a4 (CPU fan, RPM)
 *   temp2_input = a3 * 1000 (GPU, milidegree)   fan2_input = a5 (GPU fan, RPM)
 *   a6 (keyboard state) hwmon'a EXPOSE EDILMEZ.
 *
 * NOT (fan birimi): a4/a5 firmware'de fan hizi (FS1/FS2) olarak uretilir ve gozlenen
 * degerler makul RPM araligindadir; ancak bagimsiz fiziksel dogrulama YAPILMAMISTIR
 * -> "LIKELY RPM". Saha dogrulamasi sonrasi gerekirse olcek ayarlanir.
 *
 * ============================================================================
 * 2) RGB LED KONTROLU (yazma yolu — VARSAYILAN OLARAK DEVRE DISI)
 * ============================================================================
 * Dogrulanmis LED yazma protokolu (tek WMI driver, AYNI GUID, AYNI instance):
 *   a0 = 0xFB00 (LED SET), a1 = 0x0100, a2 = zone, a3 = LEDData32
 *   -> wmidev_set_block(wdev, 0, 32B); ardindan wmidev_query_block(wdev, 0, 32B)
 *      cagrilir. DIKKAT: bu ikinci cagri bir "RGB geri okuma" (read-back) DEGILDIR;
 *      ayni WMI data block'unu yeniden queryler, donus degeri ve out tamponu
 *      KULLANILMAZ / YOKSAYILIR. Cagri yalnizca firmware akisina sadakat icin
 *      korunur (SET sonrasi ek WMI transaction; firmware davranisi dogrulanmadi).
 *
 *   Istek buffer yerlesimi (mevcut GET 0xFA00/0x0200 paketlemesiyle AYNI konvansiyon):
 *     [0..1] = a0 (LE u16, 0xFB00)     [2..3] = a1 (LE u16, 0x0100)
 *     [4..7] = a2 (LE u32) = zone      [8..11] = a3 (LE u32) = LEDData32
 *   (DSDT yanit bloklari a2..a6 alanlarini 4-byte araliklarla (4,8,12,16,20)
 *    yerlestirir; istek de ayni hizalamayi kullanir.)
 *
 *   LEDData32 (LE u32): [mode:4 bit28-31][brightness:4 bit24-27]
 *                       [R:8 bit16-23][G:8 bit8-15][B:8 bit0-7]
 *
 *   Zone (a2) whitelist     : All=0, ZoneC=3, ZoneB=4, ZoneA=5, Keyboard=6, Trunk=7, Logo=8
 *   Efekt (mode) whitelist  : Off=0, Static=1, Breathing=3, ColorfulCycle=6, Ambilight=7
 *   brightness = 0..15 ; RGB = 0..255
 *
 * GUVENLIK (enable_write):
 *   Yazma yolu VARSAYILAN OLARAK KAPALIDIR (enable_write=0). Bu durumda TUM LED
 *   store yollari -EPERM doner ve HICBIR wmidev_set_block() cagrilmaz. Kullanicinin
 *   acikca "modprobe ... enable_write=1" demesi gerekir. Amac: surucu yuklendiginde
 *   (ya da otomatik olarak) donanima istenmeyen RGB komutu gitmemesidir.
 *
 * KILIT TASARIMI (tek kilit):
 *   wmidev_set_block + wmidev_query_block atomik DEGILDIR. Bu yuzden hwmon okumasi,
 *   LED store yollari ve cache guncellemesi AYNI mevcut d->lock ile serilestirilir.
 *   Yon TEK YONLUDUR: led_access (LED core) -> d->lock. d->lock tutulurken ASLA
 *   led_* cagrisi yapilmaz (orn. led_mc_calc_color_components() kilitten ONCE
 *   cagrilir), dolayisiyla dongusel kilitlenme (deadlock) yoktur. Ikinci kilit YOKTUR.
 *
 * KAYIT SIRASI ve KISMI HATA:
 *   probe() once devm alloc + mutex + hwmon'u kaydeder, SONRA LED'leri kaydeder.
 *   probe() HICBIR wmidev_* cagirmaz -> yukleme aninda RGB otomatik etkinlesmez.
 *   Bir LED kaydi basarisiz olursa o ana kadar kaydedilen TUM LED'ler (ve her LED'in
 *   devm-yonetimli 'effect' sysfs grubu) ters sirada TAM olarak geri alinir; ardindan
 *   probe() 0 doner. Boylece devm hwmon (excalibur_g870) AYAKTA kalir ve telemetri
 *   calismaya devam eder (probe hata dondurmedigi icin devres_release_all() hwmon'u
 *   birakmaz); RGB ise kullanilamaz durumdadir ve kullaniciya dev_err/dev_warn ile
 *   acikca bildirilir.
 *
 * TEARDOWN (devm action — F1/F2 FIX):
 *   Probe, TUM LED kayitlarindan SONRA bir devm action kaydeder:
 *     ret = devm_add_action_or_reset(&wdev->dev, exc_leds_disable_action, d);
 *   Bu action driver unbind/remove aninda calisir (devres_release_all() zinciri
 *   icinde kobject_del DEN ONCE). exc_leds_disable_action() -> exc_leds_disable(d)
 *   d->lock alir, d->leds_disabled = true yapar, kilidi birakir.
 *   Bu sayede driver'un clear yolu (devm hwmon + LED class device unregister)
 *   varken LED core resume/son-yazma path'leri -EPERM alir ve HICBIR WMI SET
 *   komutu gondermez.
 *   Suspend/resume hook'u YOKTUR (kastli); enable_write=0 kapisi yine de gecerli kalir.
 *
 * NOT (dogrulanmamis): LED durum GERI OKUMA (a0=0xFA00, a1=0x0100) protokolu
 *   DOGRULANMAMISTIR -> bu surucude KULLANILMAZ; durum yalnizca yazma yolunda izlenir.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/leds.h>
#include <linux/led-class-multicolor.h>
#include <linux/wmi.h>

#define EXC_GUID	"644C5791-B7B0-4123-A90B-E93876E0DAAD"
#define EXC_INSTANCE	0
#define EXC_TTL_MS	1000	/* cache freshness window */
#define EXC_RESP_LEN	32

/* --- LED komut sabitleri (dogrulanmis protokol) --- */
#define EXC_LED_CMD_HI		0xFB00	/* a0 : LED SET */
#define EXC_LED_CMD_LO		0x0100	/* a1 */
#define EXC_BRIGHT_MAX		15	/* 4 bit */
#define EXC_RGB_MAX		255	/* 8 bit */
#define EXC_RGB_COLORS		3	/* R,G,B */

/* Zone (a2) whitelist */
#define EXC_ZONE_ALL		0
#define EXC_ZONE_ZONE_C		3
#define EXC_ZONE_ZONE_B		4
#define EXC_ZONE_ZONE_A		5
#define EXC_ZONE_KEYBOARD	6
#define EXC_ZONE_TRUNK		7
#define EXC_ZONE_LOGO		8

/* Efekt (mode) whitelist */
#define EXC_MODE_OFF		0
#define EXC_MODE_STATIC		1
#define EXC_MODE_BREATHING	3
#define EXC_MODE_COLORFUL	6
#define EXC_MODE_AMBILIGHT	7

struct excalibur_data;

/*
 * Zone basina tek bir cok-renkli LED class device.
 * mc (led_classdev_mc) ilk uye olmali: container_of(cdev, struct exc_led, mc.led_cdev)
 * ile geri donulur. 'subled' sabit boyutlu, mc.subled_info buna isaret eder.
 */
struct exc_led {
	struct led_classdev_mc mc;
	struct mc_subled subled[EXC_RGB_COLORS];
	struct excalibur_data *d;	/* geri-isaretci (probe'da atanir) */
	u8 zone;			/* a2 : whitelist'ten gelir */
	u8 mode;			/* efekt : whitelist'ten gelir */
	bool has_last;			/* cache/dedup gecerli mi */
	u32 last_data;			/* zone basina son LEDData32 */
};

struct excalibur_data {
	struct wmi_device *wdev;
	struct mutex lock;	/* serilestirir: hwmon cache + WMI set + query + parse */
	unsigned long last_update;	/* jiffies */
	bool valid;
	bool leds_disabled;	/* teardown devm action tarafindan set edilir */
	u16 cpu_temp;		/* a2 : CPU sicaklik (C, EC RTMP)  */
	u16 gpu_temp;		/* a3 : GPU sicaklik (C, EC VGAT)  */
	u16 cpu_fan;		/* a4 : CPU fan tach (LIKELY RPM)  */
	u16 gpu_fan;		/* a5 : GPU fan tach (LIKELY RPM)  */
	struct exc_led leds[];	/* esnek dizi: struct_size(d, leds, N) */
};

/*
 * enable_write: VARSAYILAN 0. Yazma yolu tamamen kapidir; 0 iken hicbir
 * wmidev_set_block() / wmidev_query_block() cagrilmaz ve tum LED store yollari
 * -EPERM doner. Guvenli (salt-okuma) davranis varsayilandir.
 */
static bool enable_write;
module_param(enable_write, bool, 0644);
MODULE_PARM_DESC(enable_write,
	"RGB LED donanim yazmasini etkinlestir (varsayilan 0 = salt-okuma). 0 iken tum LED store yollari -EPERM doner ve hicbir WMI SET komutu gonderilmez.");

/* LED tanimlari: LED sysfs adlari tam olarak 'excalibur::<function>' olur
 * (devicename="excalibur" + ':' + default_label=":<function>"). */
struct exc_led_def {
	const char *label;	/* default_label: ":<function>" */
	u8 zone;
};

static const struct exc_led_def exc_led_defs[] = {
	{ ":zone-a",	EXC_ZONE_ZONE_A },	/* -> excalibur::zone-a   (5) */
	{ ":zone-b",	EXC_ZONE_ZONE_B },	/* -> excalibur::zone-b   (4) */
	{ ":zone-c",	EXC_ZONE_ZONE_C },	/* -> excalibur::zone-c   (3) */
	{ ":keyboard",	EXC_ZONE_KEYBOARD },	/* -> excalibur::keyboard (6) */
	{ ":trunk",	EXC_ZONE_TRUNK },	/* -> excalibur::trunk    (7) */
	{ ":logo",	EXC_ZONE_LOGO },	/* -> excalibur::logo     (8) */
	{ ":all",	EXC_ZONE_ALL },		/* -> excalibur::all      (0) */
};
#define EXC_NUM_LEDS	ARRAY_SIZE(exc_led_defs)

/* Efekt token -> mode esleme (sysfs 'effect' niteligi icin). */
struct exc_mode_def {
	const char *token;
	u8 mode;
};

static const struct exc_mode_def exc_modes[] = {
	{ "off",		EXC_MODE_OFF },
	{ "static",		EXC_MODE_STATIC },
	{ "breathing",		EXC_MODE_BREATHING },
	{ "colorful-cycle",	EXC_MODE_COLORFUL },
	{ "ambilight",		EXC_MODE_AMBILIGHT },
};

static inline u16 exc_le16(const u8 *p)
{
	return (u16)p[0] | ((u16)p[1] << 8);
}

/* ==========================================================================
 * TEARDOWN (devm action — F1/F2 FIX)
 * ========================================================================== */

/*
 * exc_leds_disable: teardown aninda cagrilir (devm action zinciri icinde).
 * d->lock alir, leds_disabled = true yapar, kilidi birakir.
 * Bu sayede tum ileriki LED store yollari -EPERM doner ve HICBIR WMI SET
 * komutu gonderilmez (enable_write=1 olsa bile). enable_write kapisi yine de
 * gecerli kalir; bu ikinci kapidir.
 */
static void exc_leds_disable(struct excalibur_data *d)
{
	mutex_lock(&d->lock);
	d->leds_disabled = true;
	mutex_unlock(&d->lock);
}

/*
 * exc_leds_disable_action: devm action callback.
 * devres_release_all() -> action calistirilir -> exc_leds_disable(d).
 */
static void exc_leds_disable_action(void *data)
{
	exc_leds_disable(data);
}

/* ==========================================================================
 * TELEMETRI (salt-okuma) — davranis DEGISMEDI
 * ========================================================================== */

/*
 * Tek GET 0x200: okuma dalini tetikler. out.data kernel tarafindan AYRILIR
 * (caller-alloc degil); bu yuzden out.data uzerinden okunur ve her yolda kfree edilir.
 */
static int excalibur_get_telemetry(struct wmi_device *wdev, struct excalibur_data *d)
{
	u8 req[EXC_RESP_LEN] = { 0 };
	struct wmi_buffer in = { .length = sizeof(req), .data = req };
	struct wmi_buffer out = { 0 };
	int ret;
	const u8 *p;

	/* a0 = 0xFA00 (GET) ; a1 = 0x0200 ; kalan 0 (little-endian) */
	req[0] = 0x00;  req[1] = 0xFA;
	req[2] = 0x00;  req[3] = 0x02;

	ret = wmidev_set_block(wdev, EXC_INSTANCE, &in);
	if (ret < 0) {
		dev_err_ratelimited(&wdev->dev, "set_block(GET 0x200) failed: %d\n", ret);
		return ret;
	}

	ret = wmidev_query_block(wdev, EXC_INSTANCE, &out, EXC_RESP_LEN);
	if (ret < 0) {
		dev_err_ratelimited(&wdev->dev, "query_block failed: %d\n", ret);
		return ret;
	}

	if (!out.data || out.length < EXC_RESP_LEN) {
		dev_err_ratelimited(&wdev->dev,
				    "invalid response (data=%p len=%zu)\n",
				    out.data, out.length);
		kfree(out.data);
		return -ENODATA;
	}

	p = out.data;
	d->cpu_temp = exc_le16(p + 4);		/* a2 (u32 alan; deger < 65536) */
	d->gpu_temp = exc_le16(p + 8);		/* a3 */
	d->cpu_fan  = exc_le16(p + 12);		/* a4 */
	d->gpu_fan  = exc_le16(p + 16);		/* a5 */
	/* a6 (keyboard state) okunabilir ama hwmon'a expose edilmez. */

	kfree(out.data);
	return 0;
}

/* Cagri sirasinda d->lock tutulmalidir. */
static int excalibur_update(struct excalibur_data *d)
{
	int ret;

	lockdep_assert_held(&d->lock);

	if (d->valid &&
	    time_before(jiffies, d->last_update + msecs_to_jiffies(EXC_TTL_MS)))
		return 0;			/* cache taze */

	ret = excalibur_get_telemetry(d->wdev, d);
	if (ret) {
		/* Refresh basarisiz: onceki gecerli veri varsa son iyi degeri kullan
		 * (stale-on-error). Ilk okumada gecerli veri yoksa hatayi dondur. */
		return d->valid ? 0 : ret;
	}

	d->last_update = jiffies;
	d->valid = true;
	return 0;
}

static umode_t excalibur_is_visible(const void *drvdata,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel)
{
	/* Yalnizca sicaklik ve fan; hepsi SALT-OKUMA. */
	if (type == hwmon_temp || type == hwmon_fan)
		return 0444;
	return 0;
}

static int excalibur_read(struct device *dev, enum hwmon_sensor_types type,
			  u32 attr, int channel, long *val)
{
	struct excalibur_data *d = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&d->lock);

	ret = excalibur_update(d);
	if (!ret) {
		if (type == hwmon_temp && attr == hwmon_temp_input) {
			*val = (long)(channel == 0 ? d->cpu_temp : d->gpu_temp) * 1000;
		} else if (type == hwmon_fan && attr == hwmon_fan_input) {
			*val = channel == 0 ? d->cpu_fan : d->gpu_fan;
		} else {
			ret = -EOPNOTSUPP;
		}
	}

	mutex_unlock(&d->lock);
	return ret;
}

static int excalibur_read_string(struct device *dev, enum hwmon_sensor_types type,
				 u32 attr, int channel, const char **str)
{
	if (type == hwmon_temp && attr == hwmon_temp_label) {
		*str = channel == 0 ? "CPU" : "GPU";
		return 0;
	}
	if (type == hwmon_fan && attr == hwmon_fan_label) {
		*str = channel == 0 ? "CPU Fan" : "GPU Fan";
		return 0;
	}
	return -EOPNOTSUPP;
}

static const struct hwmon_ops excalibur_hwmon_ops = {
	.is_visible = excalibur_is_visible,
	.read = excalibur_read,
	.read_string = excalibur_read_string,
	/* .write = NULL : SALT-OKUMA (writable attribute yok) */
};

static const struct hwmon_channel_info * const excalibur_hwmon_info[] = {
	HWMON_CHANNEL_INFO(temp,
			   HWMON_T_INPUT | HWMON_T_LABEL,	/* temp1 = CPU */
			   HWMON_T_INPUT | HWMON_T_LABEL),	/* temp2 = GPU */
	HWMON_CHANNEL_INFO(fan,
			   HWMON_F_INPUT | HWMON_F_LABEL,	/* fan1 = CPU fan */
			   HWMON_F_INPUT | HWMON_F_LABEL),	/* fan2 = GPU fan */
	NULL
};

static const struct hwmon_chip_info excalibur_chip_info = {
	.ops = &excalibur_hwmon_ops,
	.info = excalibur_hwmon_info,
};

/* ==========================================================================
 * RGB LED — whitelist / protokol yardimcilari
 * ========================================================================== */

static bool exc_zone_valid(u8 zone)
{
	switch (zone) {
	case EXC_ZONE_ALL:
	case EXC_ZONE_ZONE_C:
	case EXC_ZONE_ZONE_B:
	case EXC_ZONE_ZONE_A:
	case EXC_ZONE_KEYBOARD:
	case EXC_ZONE_TRUNK:
	case EXC_ZONE_LOGO:
		return true;
	default:
		return false;
	}
}

static bool exc_mode_valid(u8 mode)
{
	switch (mode) {
	case EXC_MODE_OFF:
	case EXC_MODE_STATIC:
	case EXC_MODE_BREATHING:
	case EXC_MODE_COLORFUL:
	case EXC_MODE_AMBILIGHT:
		return true;
	default:
		return false;
	}
}

/* sysfs 'effect' token -> mode (yeniden dogrulama). Gecersizse -EINVAL. */
static int exc_parse_mode(const char *buf, u8 *mode_out)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(exc_modes); i++) {
		if (sysfs_streq(buf, exc_modes[i].token)) {
			*mode_out = exc_modes[i].mode;
			return 0;
		}
	}
	return -EINVAL;
}

/*
 * LEDData32 paketle: [mode:4 bit28-31][brightness:4 bit24-27]
 *                    [R:8 bit16-23][G:8 bit8-15][B:8 bit0-7]
 */
static u32 exc_led_data32(u8 mode, unsigned int brightness,
			  unsigned int r, unsigned int g, unsigned int b)
{
	return ((u32)(mode & 0xf) << 28) |
	       ((u32)(brightness & 0xf) << 24) |
	       ((u32)(r & 0xff) << 16) |
	       ((u32)(g & 0xff) << 8) |
	       ((u32)(b & 0xff));
}

/*
 * Tek LED SET cagrisi. Cagri sirasinda d->lock TUTULMALIDIR.
 * enable_write kapisi cagiran tarafta da uygulanir; burada savunma amacli tekrar
 * kontrol edilir (hicbir wmidev_* cagrisindan once).
 */
static int exc_led_send_locked(struct exc_led *led, u32 data)
{
	struct excalibur_data *d = led->d;
	u8 req[EXC_RESP_LEN] = { 0 };
	struct wmi_buffer in = { .length = sizeof(req), .data = req };
	struct wmi_buffer out = { 0 };
	int ret;

	lockdep_assert_held(&d->lock);

	if (!enable_write)
		return -EPERM;

	if (d->leds_disabled)
		return -EPERM;

	/* a0 = 0xFB00 (LED SET) ; a1 = 0x0100 (LE u16) */
	req[0] = EXC_LED_CMD_HI & 0xff;  req[1] = (EXC_LED_CMD_HI >> 8) & 0xff;
	req[2] = EXC_LED_CMD_LO & 0xff;  req[3] = (EXC_LED_CMD_LO >> 8) & 0xff;
	/* a2 = zone (LE u32 alan) */
	req[4] = led->zone;
	/* a3 = LEDData32 (LE u32) */
	req[8]  = data & 0xff;
	req[9]  = (data >> 8) & 0xff;
	req[10] = (data >> 16) & 0xff;
	req[11] = (data >> 24) & 0xff;

	ret = wmidev_set_block(d->wdev, EXC_INSTANCE, &in);
	if (ret < 0) {
		dev_err_ratelimited(&d->wdev->dev,
				    "set_block(LED zone %u) failed: %d\n",
				    led->zone, ret);
		return ret;
	}

	/*
	 * SET sonrasi ek WMI transaction: ayni data block'u yeniden queryler.
	 * BU BIR "RGB GERI OKUMA" (read-back) DEGILDIR ve LED durumunu okumaz;
	 * donus degeri ve out tamponu KULLANILMAZ (yalnizca SET'in basarisi
	 * onemlidir). Cagri firmware akisina sadakat icin korunur (firmware
	 * davranisi dogrulanmadigindan KALDIRILMAZ). out.data caller tarafindan
	 * serbest birakilir (out sifir-initialize -> kfree(NULL) guvenli).
	 */
	wmidev_query_block(d->wdev, EXC_INSTANCE, &out, EXC_RESP_LEN);
	kfree(out.data);

	return 0;
}

/*
 * Yeniden dogrula + dedup + gonder. Cagri sirasinda d->lock TUTULMALIDIR.
 * Gecersiz zone/mode/brightness/RGB -> -EINVAL; tele'ye gecersiz cerceve gitmez.
 */
static int exc_led_commit_locked(struct exc_led *led, u8 mode,
				 unsigned int brightness,
				 unsigned int r, unsigned int g, unsigned int b)
{
	u32 data;
	int ret;

	lockdep_assert_held(&led->d->lock);

	if (!exc_zone_valid(led->zone) || !exc_mode_valid(mode))
		return -EINVAL;
	if (brightness > EXC_BRIGHT_MAX ||
	    r > EXC_RGB_MAX || g > EXC_RGB_MAX || b > EXC_RGB_MAX)
		return -EINVAL;

	if (!enable_write)
		return -EPERM;

	data = exc_led_data32(mode, brightness, r, g, b);

	/* Cache/dedup: ayni LEDData32 ise tekrar gonderme (uygulama m_cache davranisi). */
	if (led->has_last && led->last_data == data)
		return 0;

	ret = exc_led_send_locked(led, data);
	if (ret)
		return ret;

	led->last_data = data;
	led->has_last = true;
	return 0;
}

/* led_classdev -> exc_led geri donusu. */
static inline struct exc_led *exc_from_cdev(struct led_classdev *cdev)
{
	return container_of(cdev, struct exc_led, mc.led_cdev);
}

/*
 * Global brightness degisti: RGB = kullanicinin multi_intensity degerleri
 * (subled_info[].intensity, 0..255), parlaklik nibble'i = global brightness (0..15).
 *
 * SEMANTIK FARK (led-class-multicolor sozlesmesi): Linux LED core'da global
 * 'brightness' bir OLCEKTIR; led_mc_calc_color_components() her kanali
 * brightness * intensity / max_brightness seklinde kisarak uygular (brightness=0
 * tum renkleri 0 yapar). Bu cihazda ise protokolde brightness AYRI bir 4-bit OEM
 * alanidir (0..15 nibble) ve RGB kanallarini OLCEKLEMEZ. Bu nedenle
 * led_mc_calc_color_components() BILEREK cagrilmaz; aksi halde parlaklik hem
 * RGB'ye hem de donanim nibble'ina uygulanip iki kez kisilirdi. Sonuc: bu surucude
 * 'brightness' yazmak RGB'yi karartmaz, yalnizca OEM 0..15 alanini degistirir.
 */
static int exc_led_brightness_set(struct led_classdev *cdev,
				  enum led_brightness brightness)
{
	struct exc_led *led = exc_from_cdev(cdev);
	struct excalibur_data *d = led->d;
	unsigned int bright = brightness;
	int ret;

	if (!enable_write)
		return -EPERM;

	if (bright > EXC_BRIGHT_MAX)
		bright = EXC_BRIGHT_MAX;

	/* d->lock tutulurken led_* cagrilmaz; sadece donanim yazmasi yapilir. */
	mutex_lock(&d->lock);
	ret = exc_led_commit_locked(led, led->mode, bright,
				    led->mc.subled_info[0].intensity,
				    led->mc.subled_info[1].intensity,
				    led->mc.subled_info[2].intensity);
	mutex_unlock(&d->lock);

	return ret;
}

/* ---- 'effect' sysfs niteligi (ek, per-zone) ---- */

static ssize_t effect_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct led_classdev *cdev = dev_get_drvdata(dev);
	struct exc_led *led = exc_from_cdev(cdev);
	int i;

	for (i = 0; i < ARRAY_SIZE(exc_modes); i++)
		if (exc_modes[i].mode == led->mode)
			return sysfs_emit(buf, "%s\n", exc_modes[i].token);
	return sysfs_emit(buf, "off\n");
}

static ssize_t effect_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct led_classdev *cdev = dev_get_drvdata(dev);
	struct exc_led *led = exc_from_cdev(cdev);
	struct excalibur_data *d = led->d;
	unsigned int bright;
	u8 mode;
	int ret;

	if (!enable_write)
		return -EPERM;

	if (exc_parse_mode(buf, &mode))
		return -EINVAL;

	bright = cdev->brightness;
	if (bright > EXC_BRIGHT_MAX)
		bright = EXC_BRIGHT_MAX;

	mutex_lock(&d->lock);
	led->mode = mode;
	ret = exc_led_commit_locked(led, mode, bright,
				    led->mc.subled_info[0].intensity,
				    led->mc.subled_info[1].intensity,
				    led->mc.subled_info[2].intensity);
	mutex_unlock(&d->lock);

	return ret ? ret : (ssize_t)count;
}
static DEVICE_ATTR_RW(effect);

/*
 * 'effect' niteligini tasiyan attribute_group. mc.led_cdev.groups, multicolor
 * kaydi tarafindan led_multicolor_groups ile EZILDIGI icin 'effect' grup yoluyla
 * kayittan ONCE eklenemez. Bu yuzden kayittan SONRA, LED class device'in kendi
 * cihazina (cdev->dev) devm_device_add_group() ile eklenir. Grup, cihaz
 * device_unregister() ile kaldirilirken device_del() -> devres_release_all()
 * tarafindan OTOMATIK ve TAM olarak kaldirilir (kobject_del'den ONCE); hata/geri-
 * alma ve normal sokulme yollarinda sizinti, cift-serbest veya use-after-free
 * olmaz.
 */
static struct attribute *exc_effect_attrs[] = {
	&dev_attr_effect.attr,
	NULL,
};

static const struct attribute_group exc_effect_group = {
	.attrs = exc_effect_attrs,
};

/*
 * Tek LED'i kaydet: once multicolor LED class device, sonra devm-yonetimli
 * 'effect' sysfs grubu. Basarisizlikta kayit kendi kendini geri alir; cagiran
 * taraf (probe) kismi kayitlari temizler (bkz. excalibur_unregister_led).
 */
static int excalibur_register_led(struct excalibur_data *d, int idx)
{
	struct exc_led *led = &d->leds[idx];
	struct led_classdev *cdev = &led->mc.led_cdev;
	struct led_init_data init = { };
	int ret, k;

	led->d = d;
	led->zone = exc_led_defs[idx].zone;
	led->mode = EXC_MODE_STATIC;
	led->has_last = false;
	led->last_data = 0;

	led->subled[0].color_index = LED_COLOR_ID_RED;
	led->subled[1].color_index = LED_COLOR_ID_GREEN;
	led->subled[2].color_index = LED_COLOR_ID_BLUE;
	for (k = 0; k < EXC_RGB_COLORS; k++) {
		led->subled[k].max_intensity = EXC_RGB_MAX;
		led->subled[k].channel = k;
	}

	led->mc.num_colors = EXC_RGB_COLORS;
	led->mc.subled_info = led->subled;

	cdev->max_brightness = EXC_BRIGHT_MAX;		/* 0..15 */
	cdev->brightness_set_blocking = exc_led_brightness_set;

	/*
	 * LED sysfs adi: devicename="excalibur", default_label=":<function>"
	 * -> led_compose_name() "%s:%s" => "excalibur::zone-a" vb. (tam eslesme).
	 * fwnode YOK -> LED core default_label yolunu kullanir.
	 */
	init.devicename = "excalibur";
	init.default_label = exc_led_defs[idx].label;

	ret = devm_led_classdev_multicolor_register_ext(&d->wdev->dev, &led->mc,
							&init);
	if (ret)
		return ret;

	/*
	 * 'effect' grubunu LED cihazina devm ile bagla. Cihaz kaldirilinca
	 * device_del() -> devres_release_all() grubu ve sysfs dosyasini otomatik
	 * olarak kaldirir -> elle device_remove_file gerekmez ve cift-serbest
	 * riski olusmaz.
	 */
	ret = devm_device_add_group(cdev->dev, &exc_effect_group);
	if (ret) {
		devm_led_classdev_multicolor_unregister(&d->wdev->dev, &led->mc);
		return ret;
	}

	return 0;
}

/*
 * Kayitli tek bir LED'i TAM olarak geri al: multicolor LED class device'i devm
 * kaydindan dusurur ve unregister eder. 'effect' grubu LED cihazina devm ile
 * bagli oldugundan, device_unregister() -> device_del() -> devres_release_all()
 * zinciri grubu ve sysfs dosyasini da otomatik kaldirir (kobject_del'den once).
 * Boylece kismi kayit kalmaz; elle sysfs temizligi gerekmez.
 */
static void excalibur_unregister_led(struct excalibur_data *d, int idx)
{
	devm_led_classdev_multicolor_unregister(&d->wdev->dev, &d->leds[idx].mc);
}

/* ==========================================================================
 * PROBE — WMI cagrisi YOK
 * ========================================================================== */

static int excalibur_probe(struct wmi_device *wdev, const void *context)
{
	struct excalibur_data *d;
	struct device *hwmon;
	int i, ret;

	d = devm_kzalloc(&wdev->dev, struct_size(d, leds, EXC_NUM_LEDS),
			 GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	mutex_init(&d->lock);
	d->wdev = wdev;
	d->valid = false;

	dev_set_drvdata(&wdev->dev, d);

	/* 1) Telemetri ONCE: RGB kaydi kismi basarisiz olsa bile hwmon ayakta kalir. */
	hwmon = devm_hwmon_device_register_with_info(&wdev->dev, "excalibur_g870",
						     d, &excalibur_chip_info, NULL);
	if (IS_ERR(hwmon)) {
		dev_err(&wdev->dev, "hwmon register failed: %ld\n", PTR_ERR(hwmon));
		return PTR_ERR(hwmon);
	}

	/* 2) LED'ler: hicbir WMI cagrisi yok; yukleme aninda RGB etkinlesmez. */
	ret = 0;
	for (i = 0; i < EXC_NUM_LEDS; i++) {
		ret = excalibur_register_led(d, i);
		if (ret)
			break;
	}

	if (ret) {
		/*
		 * Kismi kayit olustu: o ana kadar kaydedilen TUM LED'leri (ve her
		 * LED'in devm-yonetimli 'effect' grubunu) TERS sirada TAM olarak
		 * geri al. Basarisiz LED'in kendi kaydi zaten
		 * excalibur_register_led() icinde geri alinmistir.
		 *
		 * F1/F2 FIX: leds_disabled = true set ETMEDEN ONCE unregister
		 * dongusune gir, bu sekilde teardown action'tan hemen once LED
		 * core resume/son-yazma path'leri -EPERM alir.
		 */
		d->leds_disabled = true;

		const char *failed = exc_led_defs[i].label;
		int failed_err = ret;

		for (i = i - 1; i >= 0; i--)
			excalibur_unregister_led(d, i);

		dev_err(&wdev->dev,
			"RGB LED '%s' register failed: %d; partial registrations rolled back\n",
			failed, failed_err);

		/*
		 * Telemetri ONCELIKLIDIR: RGB kaydi basarisiz olsa bile probe 0 doner.
		 * Boylece cekirdek hata yolu devres_release_all()'i cagirmaz; devm
		 * hwmon (excalibur_g870) AYAKTA kalir ve cihaz bagli kalir. Yalnizca
		 * RGB kontrolu kullanilamaz durumdadir.
		 */
		dev_warn(&wdev->dev,
			 "RGB LED control unavailable; read-only telemetry hwmon 'excalibur_g870' remains active\n");

		return 0;
	}

	/* 3) Devm action for teardown (F1/F2 fix): runs on driver unbind/remove,
	 *    sets leds_disabled under lock BEFORE LED unregister.
	 *    Order: hwmon -> LED1 -> ... -> LED7 -> teardown_action
	 *    LIFO cleanup: teardown_action -> LED7 -> ... -> LED1 -> hwmon
	 *    CORRECT: teardown_action runs FIRST, sets leds_disabled=true,
	 *    then LED unregister happens.
	 */
	ret = devm_add_action_or_reset(&wdev->dev, exc_leds_disable_action, d);
	if (ret)
		return ret;

	dev_info(&wdev->dev,
		 "probe ok; read-only hwmon 'excalibur_g870' + %zu RGB LEDs (write=%s)\n",
		 EXC_NUM_LEDS, enable_write ? "enabled" : "disabled (enable_write=0)");
	return 0;
}

static const struct wmi_device_id excalibur_wmi_ids[] = {
	{ .guid_string = EXC_GUID },
	{ }
};
MODULE_DEVICE_TABLE(wmi, excalibur_wmi_ids);

static struct wmi_driver excalibur_wmi_driver = {
	.driver = {
		.name = "excalibur-wmi",
	},
	.id_table = excalibur_wmi_ids,
	.probe = excalibur_probe,
	/* devm_* kaynaklari otomatik serbest birakilir; .remove EKLENMEZ. */
	/* suspend/resume hook'u KASITLI olarak yok (bkz. dosya basi notu). */
};
module_wmi_driver(excalibur_wmi_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hwmon (read-only) + RGB LED driver for CASPER EXCALIBUR G870 (WMI GET 0x200 / LED SET 0xFB00)");
MODULE_AUTHOR("controlcenter analysis");
