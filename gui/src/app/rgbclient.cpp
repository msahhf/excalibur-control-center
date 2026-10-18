#include "app/rgbclient.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QStringList>

namespace {

// "<prefix><zone-token>", e.g. "excalibur::zone-a".
QString zoneLedName(Lighting::Zone zone)
{
    return QString::fromLatin1(RgbClient::kPrefix) + Lighting::zoneToString(zone);
}

// Every zone the driver/UI can legitimately present, in whitelist order.
const Lighting::Zone kAllZones[] = {
    Lighting::Zone::All,
    Lighting::Zone::ZoneC,
    Lighting::Zone::ZoneB,
    Lighting::Zone::ZoneA,
    Lighting::Zone::Keyboard,
    Lighting::Zone::Trunk,
    Lighting::Zone::Logo,
};

// Open `path` write-only (truncating) and write `value`. Returns false on any
// failure so the caller can report an incomplete write rather than a partial
// one.
bool writeAttribute(const QString &path, const QString &value)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray bytes = value.toUtf8();
    return file.write(bytes) == bytes.size();
}

// Read a single attribute fully and return it trimmed. Empty optional on I/O
// failure.
std::optional<QString> readAttribute(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    const QString text = QString::fromUtf8(file.readAll()).trimmed();
    return text;
}

} // namespace

RgbClient::RgbClient(const QString &ledBasePath)
    : m_basePath(ledBasePath)
{
}

bool RgbClient::discover()
{
    m_zones.clear();

    QDir dir(m_basePath);
    if (!dir.exists())
        return false;

    const QStringList entries =
        dir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &name : entries) {
        if (!name.startsWith(QString::fromLatin1(kPrefix)))
            continue;
        // Only keep names whose suffix maps to a whitelisted zone, so a stray
        // entry can never reach read()/write().
        bool ok = false;
        Lighting::zoneFromString(name.mid(QString::fromLatin1(kPrefix).size()), &ok);
        if (ok)
            m_zones.append(name);
    }

    m_zones.sort();
    return !m_zones.isEmpty();
}

bool RgbClient::available() const
{
    return !m_zones.isEmpty();
}

QVector<Lighting::Zone> RgbClient::availableZones() const
{
    QVector<Lighting::Zone> zones;
    for (Lighting::Zone zone : kAllZones) {
        // Defensive: only ever name a whitelisted zone, so a future edit to the
        // table can never leak an unknown id into a path.
        if (!Lighting::isValidZone(static_cast<int>(zone)))
            continue;
        if (m_zones.contains(zoneLedName(zone)))
            zones.append(zone);
    }
    return zones;
}

QString RgbClient::zonePath(Lighting::Zone zone) const
{
    // An unknown enum tag has no on-disk name (zoneToString() is intentionally
    // empty for it), so return an empty path rather than "<base>/excalibur::".
    if (!Lighting::isValidZone(static_cast<int>(zone)))
        return QString();
    return m_basePath + QLatin1Char('/') + zoneLedName(zone);
}

QString RgbClient::multiIntensityLine(const Lighting::LedData &data)
{
    return QStringLiteral("%1 %2 %3")
        .arg(data.red)
        .arg(data.green)
        .arg(data.blue);
}

std::optional<Lighting::LedData> RgbClient::read(Lighting::Zone zone) const
{
    // Reject a zone that is not on the whitelist BEFORE it is mapped to a
    // directory name. zoneToString() cannot name it (it returns an empty
    // token), and an unknown tag must never fall back onto Zone::All
    // ("excalibur::all"). No file is opened either way.
    if (!Lighting::isValidZone(static_cast<int>(zone)))
        return std::nullopt;
    // Unknown/absent zone: not discovered on this machine.
    if (!m_zones.contains(zoneLedName(zone)))
        return std::nullopt;

    const QString base = zonePath(zone);

    const auto intensityText = readAttribute(base + QLatin1String("/multi_intensity"));
    if (!intensityText)
        return std::nullopt;
    const QStringList parts =
        intensityText->simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() != 3)
        return std::nullopt;

    bool okR = false, okG = false, okB = false;
    const int red = parts.at(0).toInt(&okR);
    const int green = parts.at(1).toInt(&okG);
    const int blue = parts.at(2).toInt(&okB);
    if (!okR || !okG || !okB)
        return std::nullopt;
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
        return std::nullopt;

    const auto brightnessText = readAttribute(base + QLatin1String("/brightness"));
    if (!brightnessText)
        return std::nullopt;
    bool okBrightness = false;
    const int brightness = brightnessText->toInt(&okBrightness);
    if (!okBrightness || !Lighting::isValidBrightness(brightness))
        return std::nullopt;

    const auto effectText = readAttribute(base + QLatin1String("/effect"));
    if (!effectText)
        return std::nullopt;
    bool okEffect = false;
    const Lighting::Effect effect = Lighting::effectFromString(*effectText, &okEffect);
    if (!okEffect)
        return std::nullopt;

    Lighting::LedData data;
    data.effect = effect;
    data.brightness = static_cast<std::uint8_t>(brightness);
    data.red = static_cast<std::uint8_t>(red);
    data.green = static_cast<std::uint8_t>(green);
    data.blue = static_cast<std::uint8_t>(blue);
    return data;
}

bool RgbClient::write(Lighting::Zone zone, const Lighting::LedData &data)
{
    // (a) Disabled: never open a file, not even for a probe.
    if (!m_writeEnabled)
        return false;
    // (b) Reject a zone that is not on the whitelist BEFORE mapping it to a
    //     name. An unknown enum tag must never be silently rewritten into
    //     Zone::All (zoneToString's safe default), which would otherwise let
    //     `write((Zone)1, ...)` land on the "all" LED directory.
    if (!Lighting::isValidZone(static_cast<int>(zone)))
        return false;
    // (c) Unknown/absent zone: not discovered on this machine.
    if (!m_zones.contains(zoneLedName(zone)))
        return false;
    // (d) Raw field validation, independent of pack()'s nibble masking. This
    //     sees the un-narrowed values, so e.g. brightness 200 is rejected here
    //     instead of being masked to 8 and written verbatim to sysfs.
    if (!Lighting::isValid(data))
        return false;

    const QString base = zonePath(zone);
    if (!writeAttribute(base + QLatin1String("/multi_intensity"),
                        multiIntensityLine(data) + QLatin1Char('\n')))
        return false;
    if (!writeAttribute(base + QLatin1String("/brightness"),
                        QString::number(data.brightness) + QLatin1Char('\n')))
        return false;
    if (!writeAttribute(base + QLatin1String("/effect"),
                        Lighting::effectToString(data.effect) + QLatin1Char('\n')))
        return false;
    return true;
}
