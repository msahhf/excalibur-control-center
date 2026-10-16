#pragma once

#include "app/usersettings.h"

#include <QWidget>

class QComboBox;
class QLabel;
class ActionButton;

// Settings page. Four persisted preferences (language, theme, temperature unit,
// refresh interval) plus Reset to Defaults. Reads/writes the single
// UserSettings source and reuses the shared SurfacePanel so it matches the
// rest of the product.
//
// The language row is the only one whose change is not visible immediately:
// the UI is built once, so the new catalog is picked up on the next start and
// an inline hint under the grid says so while a change is pending.
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

private:
    void syncFromSettings();
    void onThemeIndex(int index);
    void onUnitIndex(int index);
    void onRefreshIndex(int index);
    void onLanguageIndex(int index);

    QComboBox *m_language = nullptr;
    QComboBox *m_theme = nullptr;
    QComboBox *m_unit = nullptr;
    QComboBox *m_refresh = nullptr;
    QLabel *m_languageHint = nullptr;
    ActionButton *m_reset = nullptr;

    // Language at page construction: the hint is visible only while the stored
    // preference differs from it (a pending change that still needs a restart).
    UserSettings::Language m_startLanguage = UserSettings::Language::System;
};
