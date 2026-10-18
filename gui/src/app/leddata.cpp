#include "app/leddata.h"

namespace Lighting {

std::uint32_t pack(const LedData &data)
{
    const auto effectNibble =
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(data.effect) & 0x0Fu);
    return (effectNibble << 28)
         | (static_cast<std::uint32_t>(data.brightness & 0x0Fu) << 24)
         | (static_cast<std::uint32_t>(data.red) << 16)
         | (static_cast<std::uint32_t>(data.green) << 8)
         | static_cast<std::uint32_t>(data.blue);
}

LedData unpack(std::uint32_t word)
{
    LedData data;
    data.effect = static_cast<Effect>((word >> 28) & 0x0Fu);
    data.brightness = static_cast<std::uint8_t>((word >> 24) & 0x0Fu);
    data.red = static_cast<std::uint8_t>((word >> 16) & 0xFFu);
    data.green = static_cast<std::uint8_t>((word >> 8) & 0xFFu);
    data.blue = static_cast<std::uint8_t>(word & 0xFFu);
    return data;
}

bool isValidEffect(int effect)
{
    switch (static_cast<Effect>(effect)) {
    case Effect::Off:
    case Effect::Static:
    case Effect::Breathing:
    case Effect::ColorfulCycle:
    case Effect::Ambilight:
        return true;
    }
    return false;
}

bool isValidZone(int zone)
{
    switch (static_cast<Zone>(zone)) {
    case Zone::All:
    case Zone::ZoneC:
    case Zone::ZoneB:
    case Zone::ZoneA:
    case Zone::Keyboard:
    case Zone::Trunk:
    case Zone::Logo:
        return true;
    }
    return false;
}

bool isValidBrightness(int brightness)
{
    return brightness >= kBrightnessMin && brightness <= kBrightnessMax;
}

bool isValidColorComponent(int component)
{
    return component >= 0 && component <= 255;
}

bool isValid(const LedData &data)
{
    // Every field is checked BEFORE narrowing matters: the enum tags and the
    // brightness are widened back to int so an out-of-range value (e.g. a
    // brightness of 200 that pack() would silently mask to 8) is caught.
    return isValidEffect(static_cast<int>(data.effect))
        && isValidBrightness(static_cast<int>(data.brightness))
        && isValidColorComponent(static_cast<int>(data.red))
        && isValidColorComponent(static_cast<int>(data.green))
        && isValidColorComponent(static_cast<int>(data.blue));
}

std::optional<LedData> makeLedData(int effect, int brightness, int red, int green, int blue)
{
    // Validate every raw, not-yet-narrowed value first. Nothing is masked or
    // truncated: an out-of-range component simply fails the whole conversion.
    if (!isValidEffect(effect) || !isValidBrightness(brightness)
        || !isValidColorComponent(red) || !isValidColorComponent(green)
        || !isValidColorComponent(blue))
        return std::nullopt;

    LedData data;
    // Safe now: every value is proven to fit its field, so these casts cannot
    // wrap (brightness <= 15, colours <= 255, and every whitelisted effect tag
    // is <= 7).
    data.effect = static_cast<Effect>(effect);
    data.brightness = static_cast<std::uint8_t>(brightness);
    data.red = static_cast<std::uint8_t>(red);
    data.green = static_cast<std::uint8_t>(green);
    data.blue = static_cast<std::uint8_t>(blue);
    return data;
}

bool isValidLedData(std::uint32_t word)
{
    const LedData data = unpack(word);
    return isValidEffect(static_cast<int>(data.effect))
        && isValidBrightness(static_cast<int>(data.brightness));
}

QString effectToString(Effect effect)
{
    switch (effect) {
    case Effect::Off:
        return QStringLiteral("off");
    case Effect::Static:
        return QStringLiteral("static");
    case Effect::Breathing:
        return QStringLiteral("breathing");
    case Effect::ColorfulCycle:
        return QStringLiteral("colorful-cycle");
    case Effect::Ambilight:
        return QStringLiteral("ambilight");
    }
    // Unknown enum value: safe, non-empty default rather than an empty token.
    return QStringLiteral("off");
}

Effect effectFromString(const QString &token, bool *ok)
{
    const QString key = token.trimmed().toLower();
    if (key == QLatin1String("off")) {
        if (ok) *ok = true;
        return Effect::Off;
    }
    if (key == QLatin1String("static")) {
        if (ok) *ok = true;
        return Effect::Static;
    }
    if (key == QLatin1String("breathing")) {
        if (ok) *ok = true;
        return Effect::Breathing;
    }
    // "colorful_cycle" is accepted as a legacy alias, but the canonical output
    // token is always the dashed "colorful-cycle".
    if (key == QLatin1String("colorful-cycle") || key == QLatin1String("colorful_cycle")) {
        if (ok) *ok = true;
        return Effect::ColorfulCycle;
    }
    if (key == QLatin1String("ambilight")) {
        if (ok) *ok = true;
        return Effect::Ambilight;
    }
    if (ok) *ok = false;
    return Effect::Off;
}

QString zoneToString(Zone zone)
{
    switch (zone) {
    case Zone::All:
        return QStringLiteral("all");
    case Zone::ZoneC:
        return QStringLiteral("zone-c");
    case Zone::ZoneB:
        return QStringLiteral("zone-b");
    case Zone::ZoneA:
        return QStringLiteral("zone-a");
    case Zone::Keyboard:
        return QStringLiteral("keyboard");
    case Zone::Trunk:
        return QStringLiteral("trunk");
    case Zone::Logo:
        return QStringLiteral("logo");
    }
    // Unknown enum value: return NOTHING, never a real token. Returning "all"
    // here (the old behaviour) let a stray value such as (Zone)1 masquerade as
    // the real All zone once a caller prefixed it with "excalibur::". An empty
    // token cannot name a directory, so the mistake stays visible; callers must
    // reject the value with isValidZone() before building a path.
    return QString();
}

Zone zoneFromString(const QString &token, bool *ok)
{
    const QString key = token.trimmed().toLower();
    if (key == QLatin1String("all")) {
        if (ok) *ok = true;
        return Zone::All;
    }
    if (key == QLatin1String("zone-a")) {
        if (ok) *ok = true;
        return Zone::ZoneA;
    }
    if (key == QLatin1String("zone-b")) {
        if (ok) *ok = true;
        return Zone::ZoneB;
    }
    if (key == QLatin1String("zone-c")) {
        if (ok) *ok = true;
        return Zone::ZoneC;
    }
    if (key == QLatin1String("keyboard")) {
        if (ok) *ok = true;
        return Zone::Keyboard;
    }
    if (key == QLatin1String("trunk")) {
        if (ok) *ok = true;
        return Zone::Trunk;
    }
    if (key == QLatin1String("logo")) {
        if (ok) *ok = true;
        return Zone::Logo;
    }
    if (ok) *ok = false;
    return Zone::All;
}

} // namespace Lighting
