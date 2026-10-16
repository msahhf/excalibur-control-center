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

    // UI language preference. Not a tr()-able value anywhere: the enum is the
    // in-memory form, languageToString() the on-disk form ("system"|"en"|"tr")
    // and the combo shows endonyms (English / Türkçe) that never get translated.
    enum class Language { System, English, Turkish };

    static constexpr int kDefaultRefreshMs = 2000;

    static UserSettings &instance();

    AppTheme theme() const { return m_theme; }
    Units::TemperatureUnit tempUnit() const { return m_unit; }
    int refreshMs() const { return m_refreshMs; }
    Language language() const { return m_language; }

    // persist == false is used for session overrides (e.g. the --light/--dark
    // developer flags) so they do not overwrite the stored preference.
    void setTheme(AppTheme theme, bool persist = true);
    void setTempUnit(Units::TemperatureUnit unit, bool persist = true);
    void setRefreshMs(int ms, bool persist = true);

    // The language change is persisted immediately but only takes visual effect
    // on the next start (the UI is built once, see SettingsPage's hint).
    void setLanguage(Language language, bool persist = true);

    void resetToDefaults();

    // Parsing / validation, shared by the QSettings loader and the dev hooks.
    static AppTheme themeFromString(const QString &s);
    static Units::TemperatureUnit tempUnitFromString(const QString &s);
    static bool isRefreshValid(int ms);
    static QVector<int> refreshOptions();
    static QString themeToString(AppTheme theme);
    static QString tempUnitToString(Units::TemperatureUnit unit);
    static Language languageFromString(const QString &s);
    static QString languageToString(Language language);

signals:
    void themeChanged();
    void tempUnitChanged();
    void refreshMsChanged();
    void languageChanged();

private:
    explicit UserSettings(QObject *parent = nullptr);

    void load();

    QSettings m_settings;
    AppTheme m_theme = AppTheme::System;
    Units::TemperatureUnit m_unit = Units::TemperatureUnit::Celsius;
    int m_refreshMs = kDefaultRefreshMs;
    Language m_language = Language::System;
};
