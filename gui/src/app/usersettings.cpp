#include "usersettings.h"

#include "app/identity.h"

namespace {
constexpr char kKeyTheme[] = "theme";
constexpr char kKeyUnit[] = "temperatureUnit";
constexpr char kKeyRefresh[] = "refreshIntervalMs";
} // namespace

UserSettings &UserSettings::instance()
{
    static UserSettings s;
    return s;
}

UserSettings::UserSettings(QObject *parent)
    : QObject(parent),
      m_settings(AppIdentity::organization(), AppIdentity::desktopId())
{
    load();
}

void UserSettings::load()
{
    // Validate everything read from disk; unknown/corrupt values fall back.
    m_theme = themeFromString(m_settings.value(kKeyTheme, themeToString(AppTheme::System)).toString());
    m_unit = tempUnitFromString(m_settings.value(kKeyUnit, tempUnitToString(Units::TemperatureUnit::Celsius)).toString());

    bool ok = false;
    const int refresh = m_settings.value(kKeyRefresh, kDefaultRefreshMs).toInt(&ok);
    m_refreshMs = (ok && isRefreshValid(refresh)) ? refresh : kDefaultRefreshMs;
}

void UserSettings::setTheme(AppTheme theme, bool persist)
{
    if (m_theme == theme)
        return;
    m_theme = theme;
    if (persist) {
        m_settings.setValue(kKeyTheme, themeToString(theme));
        m_settings.sync();
    }
    emit themeChanged();
}

void UserSettings::setTempUnit(Units::TemperatureUnit unit, bool persist)
{
    if (m_unit == unit)
        return;
    m_unit = unit;
    if (persist) {
        m_settings.setValue(kKeyUnit, tempUnitToString(unit));
        m_settings.sync();
    }
    emit tempUnitChanged();
}

void UserSettings::setRefreshMs(int ms, bool persist)
{
    if (!isRefreshValid(ms) || m_refreshMs == ms)
        return;
    m_refreshMs = ms;
    if (persist) {
        m_settings.setValue(kKeyRefresh, ms);
        m_settings.sync();
    }
    emit refreshMsChanged();
}

void UserSettings::resetToDefaults()
{
    setTheme(AppTheme::System);
    setTempUnit(Units::TemperatureUnit::Celsius);
    setRefreshMs(kDefaultRefreshMs);
}

UserSettings::AppTheme UserSettings::themeFromString(const QString &s)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("light"))
        return AppTheme::Light;
    if (v == QLatin1String("dark"))
        return AppTheme::Dark;
    return AppTheme::System; // default / invalid
}

Units::TemperatureUnit UserSettings::tempUnitFromString(const QString &s)
{
    return s.trimmed().toLower() == QLatin1String("fahrenheit") ? Units::TemperatureUnit::Fahrenheit
                                                                : Units::TemperatureUnit::Celsius;
}

QString UserSettings::themeToString(AppTheme theme)
{
    switch (theme) {
    case AppTheme::Light: return QStringLiteral("light");
    case AppTheme::Dark: return QStringLiteral("dark");
    case AppTheme::System: break;
    }
    return QStringLiteral("system");
}

QString UserSettings::tempUnitToString(Units::TemperatureUnit unit)
{
    return Units::isFahrenheit(unit) ? QStringLiteral("fahrenheit") : QStringLiteral("celsius");
}

QVector<int> UserSettings::refreshOptions()
{
    return {1000, 2000, 5000, 10000};
}

bool UserSettings::isRefreshValid(int ms)
{
    return refreshOptions().contains(ms);
}
