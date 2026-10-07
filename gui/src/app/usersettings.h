#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVector>

#include "util/units.h"

// Central, single-source user preferences for the whole application. Persisted
// with QSettings under the product identity (organisation + application) and
// validated on load so a corrupt/unknown stored value always falls back to a
// safe default instead of crashing.
//
// Pages never keep their own copy of a preference; they read this one source and
// react to its change signals.
class UserSettings : public QObject
{
    Q_OBJECT

public:
    enum class AppTheme { System, Light, Dark };

    static constexpr int kDefaultRefreshMs = 2000;

    static UserSettings &instance();

    AppTheme theme() const { return m_theme; }
    Units::TemperatureUnit tempUnit() const { return m_unit; }
    int refreshMs() const { return m_refreshMs; }

    // persist == false is used for session overrides (e.g. the --light/--dark
    // developer flags) so they do not overwrite the stored preference.
    void setTheme(AppTheme theme, bool persist = true);
    void setTempUnit(Units::TemperatureUnit unit, bool persist = true);
    void setRefreshMs(int ms, bool persist = true);

    void resetToDefaults();

    // Parsing / validation, shared by the QSettings loader and the dev hooks.
    static AppTheme themeFromString(const QString &s);
    static Units::TemperatureUnit tempUnitFromString(const QString &s);
    static bool isRefreshValid(int ms);
    static QVector<int> refreshOptions();
    static QString themeToString(AppTheme theme);
    static QString tempUnitToString(Units::TemperatureUnit unit);

signals:
    void themeChanged();
    void tempUnitChanged();
    void refreshMsChanged();

private:
    explicit UserSettings(QObject *parent = nullptr);

    void load();

    QSettings m_settings;
    AppTheme m_theme = AppTheme::System;
    Units::TemperatureUnit m_unit = Units::TemperatureUnit::Celsius;
    int m_refreshMs = kDefaultRefreshMs;
};
