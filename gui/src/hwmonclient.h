#pragma once

#include <QString>

// Read-only telemetry snapshot discovered from a Linux hwmon device.
struct ExcTelemetry
{
    bool deviceFound = false; // an hwmon device named "excalibur_g870" exists

    bool cpuTempValid = false;
    bool gpuTempValid = false;
    bool cpuFanValid  = false;
    bool gpuFanValid  = false;

    double cpuTempC = 0.0; // degrees Celsius
    double gpuTempC = 0.0;
    long   cpuFanRpm = 0;  // LIKELY RPM (hwmon fan*_input semantics)
    long   gpuFanRpm = 0;

    QString cpuTempLabel;
    QString gpuTempLabel;
    QString cpuFanLabel;
    QString gpuFanLabel;
};

enum class ExcStatus
{
    Connected,    // device found + all telemetry readable
    Degraded,     // device found but some telemetry missing/unreadable
    Disconnected  // device not found
};

// Discovers the EXCALIBUR hwmon device by NAME (never by a fixed hwmonN index)
// and reads its telemetry files. Pure userspace, read-only sysfs access.
class HwmonClient
{
public:
    static constexpr const char *kDeviceName = "excalibur_g870";

    // basePath defaults to /sys/class/hwmon; overridable for testing only.
    explicit HwmonClient(const QString &basePath = QStringLiteral("/sys/class/hwmon"));

    // (Re)discover the device whose "name" == kDeviceName. Safe to call repeatedly
    // so the GUI can pick up a module loaded/removed after startup.
    bool discover();

    bool deviceFound() const { return !m_devicePath.isEmpty(); }
    QString devicePath() const { return m_devicePath; }

    // Read telemetry. Never throws; missing/unreadable files yield invalid fields.
    ExcTelemetry read() const;

    ExcStatus status(const ExcTelemetry &t) const;

private:
    QString readString(const QString &file) const;
    bool readLong(const QString &file, long *out) const;

    QString m_basePath;
    QString m_devicePath; // discovered at runtime; NOT a hardcoded hwmonN
};
