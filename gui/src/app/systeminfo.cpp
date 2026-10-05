#include "systeminfo.h"

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

constexpr const char *kNotAvailable = "Not available";

} // namespace

QString SystemInfo::model() const
{
    return productName.isEmpty() ? QString::fromLatin1(kNotAvailable) : productName;
}

QString SystemInfo::bios() const
{
    if (biosVersion.isEmpty() && biosDate.isEmpty())
        return QString::fromLatin1(kNotAvailable);
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
