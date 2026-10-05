#include "hwmonclient.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>

HwmonClient::HwmonClient(const QString &basePath)
    : m_basePath(basePath)
{
}

QString HwmonClient::readString(const QString &file) const
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

bool HwmonClient::readLong(const QString &file, long *out) const
{
    const QString s = readString(file);
    if (s.isEmpty())
        return false;
    bool ok = false;
    const long v = s.toLong(&ok);
    if (!ok)
        return false;
    *out = v;
    return true;
}

bool HwmonClient::discover()
{
    m_devicePath.clear();

    const QDir dir(m_basePath);
    const QFileInfoList list =
        dir.entryInfoList(QStringList() << QStringLiteral("hwmon*"),
                          QDir::NoDotAndDotDot | QDir::AllEntries, QDir::Name);

    for (const QFileInfo &fi : list) {
        if (!fi.isDir()) // follows symlinks: /sys/class/hwmon/hwmonN -> device dir
            continue;
        const QString path = fi.absoluteFilePath();
        if (readString(path + QStringLiteral("/name")) == QLatin1String(kDeviceName)) {
            m_devicePath = path;
            return true;
        }
    }
    return false;
}

ExcTelemetry HwmonClient::read() const
{
    ExcTelemetry t;
    if (m_devicePath.isEmpty())
        return t; // deviceFound stays false

    t.deviceFound = true;
    const QString p = m_devicePath + QLatin1Char('/');

    long v = 0;

    // Temperature: millidegree Celsius -> Celsius.
    if (readLong(p + QStringLiteral("temp1_input"), &v)) {
        t.cpuTempC = static_cast<double>(v) / 1000.0;
        t.cpuTempValid = true;
    }
    if (readLong(p + QStringLiteral("temp2_input"), &v)) {
        t.gpuTempC = static_cast<double>(v) / 1000.0;
        t.gpuTempValid = true;
    }
    // Fan: RPM (integer passthrough).
    if (readLong(p + QStringLiteral("fan1_input"), &v)) {
        t.cpuFanRpm = v;
        t.cpuFanValid = true;
    }
    if (readLong(p + QStringLiteral("fan2_input"), &v)) {
        t.gpuFanRpm = v;
        t.gpuFanValid = true;
    }

    // Labels (fall back to sensible defaults if absent).
    t.cpuTempLabel = readString(p + QStringLiteral("temp1_label"));
    if (t.cpuTempLabel.isEmpty())
        t.cpuTempLabel = QStringLiteral("CPU");
    t.gpuTempLabel = readString(p + QStringLiteral("temp2_label"));
    if (t.gpuTempLabel.isEmpty())
        t.gpuTempLabel = QStringLiteral("GPU");
    t.cpuFanLabel = readString(p + QStringLiteral("fan1_label"));
    if (t.cpuFanLabel.isEmpty())
        t.cpuFanLabel = QStringLiteral("CPU Fan");
    t.gpuFanLabel = readString(p + QStringLiteral("fan2_label"));
    if (t.gpuFanLabel.isEmpty())
        t.gpuFanLabel = QStringLiteral("GPU Fan");

    return t;
}

ExcStatus HwmonClient::status(const ExcTelemetry &t) const
{
    if (!t.deviceFound)
        return ExcStatus::Disconnected;
    if (t.cpuTempValid && t.gpuTempValid && t.cpuFanValid && t.gpuFanValid)
        return ExcStatus::Connected;
    return ExcStatus::Degraded;
}
