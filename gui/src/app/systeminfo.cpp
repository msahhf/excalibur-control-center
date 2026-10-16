#include "systeminfo.h"

#include <QCoreApplication>
#include <QFile>
#include <QSysInfo>

namespace {

QString readFirstLine(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

// Shown to the user on the Device page (Model / BIOS rows), so it goes through
// the translator. Context "SystemInfo" (struct name, free function -> no tr()).
QString notAvailable()
{
    return QCoreApplication::translate("SystemInfo", "Not available");
}

} // namespace

QString SystemInfo::model() const
{
    return productName.isEmpty() ? notAvailable() : productName;
}

QString SystemInfo::bios() const
{
    if (biosVersion.isEmpty() && biosDate.isEmpty())
        return notAvailable();
    if (biosDate.isEmpty())
        return biosVersion;
    if (biosVersion.isEmpty())
        return biosDate;
    return QStringLiteral("%1 (%2)").arg(biosVersion, biosDate);
}

SystemInfo readSystemInfo()
{
    const QString base = QStringLiteral("/sys/class/dmi/id/");
    SystemInfo s;
    s.manufacturer = readFirstLine(base + QStringLiteral("sys_vendor"));
    s.productName = readFirstLine(base + QStringLiteral("product_name"));
    s.boardName = readFirstLine(base + QStringLiteral("board_name"));
    s.biosVersion = readFirstLine(base + QStringLiteral("bios_version"));
    s.biosDate = readFirstLine(base + QStringLiteral("bios_date"));

    // uname equivalent; unprivileged and always available.
    s.kernelVersion = QSysInfo::kernelVersion();
    s.kernelType = QSysInfo::kernelType();
    s.architecture = QSysInfo::currentCpuArchitecture();
    return s;
}
