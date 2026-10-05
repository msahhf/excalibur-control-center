// SPDX-License-Identifier: GPL-2.0
/*
 * excalibur-wmi.c — read-only hwmon driver for CASPER EXCALIBUR G870 (NLXB 001)
 *
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
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/hwmon.h>
#include <linux/wmi.h>

#define EXC_GUID	"644C5791-B7B0-4123-A90B-E93876E0DAAD"
#define EXC_INSTANCE	0
#define EXC_TTL_MS	1000	/* cache freshness window */
#define EXC_RESP_LEN	32

struct excalibur_data {
	struct wmi_device *wdev;
	struct mutex lock;	/* serilestirir: cache + WMI set + query + parse + cache */
	unsigned long last_update;	/* jiffies */
	bool valid;
	u16 cpu_temp;		/* a2 : CPU sicaklik (C, EC RTMP)  */
	u16 gpu_temp;		/* a3 : GPU sicaklik (C, EC VGAT)  */
	u16 cpu_fan;		/* a4 : CPU fan tach (LIKELY RPM)  */
	u16 gpu_fan;		/* a5 : GPU fan tach (LIKELY RPM)  */
};

static inline u16 exc_le16(const u8 *p)
{
	return (u16)p[0] | ((u16)p[1] << 8);
}

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

static int excalibur_probe(struct wmi_device *wdev, const void *context)
{
	struct excalibur_data *d;
	struct device *hwmon;

	d = devm_kzalloc(&wdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	mutex_init(&d->lock);
	d->wdev = wdev;
	d->valid = false;

	dev_set_drvdata(&wdev->dev, d);

	hwmon = devm_hwmon_device_register_with_info(&wdev->dev, "excalibur_g870",
						     d, &excalibur_chip_info, NULL);
	if (IS_ERR(hwmon)) {
		dev_err(&wdev->dev, "hwmon register failed: %ld\n", PTR_ERR(hwmon));
		return PTR_ERR(hwmon);
	}

	dev_info(&wdev->dev, "probe ok; read-only hwmon 'excalibur_g870' registered\n");
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
	/* devm_* kaynaklari otomatik serbest birakilir; manuel cleanup gerekmez */
};
module_wmi_driver(excalibur_wmi_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read-only hwmon driver for CASPER EXCALIBUR G870 (WMI GET 0x200)");
MODULE_AUTHOR("controlcenter analysis");
