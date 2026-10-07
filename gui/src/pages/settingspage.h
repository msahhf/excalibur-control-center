#pragma once

#include <QWidget>

class QComboBox;
class ActionButton;

// Settings page. Three persisted preferences (theme, temperature unit, refresh
// interval) plus Reset to Defaults. Reads/writes the single UserSettings source
// and reuses the shared SurfacePanel so it matches the rest of the product.
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

    QComboBox *m_theme = nullptr;
    QComboBox *m_unit = nullptr;
    QComboBox *m_refresh = nullptr;
    ActionButton *m_reset = nullptr;
};
