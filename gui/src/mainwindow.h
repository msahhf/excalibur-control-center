#pragma once

#include <QMainWindow>

#include "app/systeminfo.h"

class Sidebar;
class PageContainer;
class DashboardPage;
class CoolingPage;
class DevicePage;
class DiagnosticsPage;
class SettingsPage;
class AboutPage;
class TelemetryModel;
class QSystemTrayIcon;

// EXCALIBUR Control Center application shell: a persistent navigation rail and
// a content area that swaps product pages instantly. Owns the single telemetry
// model and reacts to the central UserSettings (theme / unit / refresh).
//
// Presentation only. Telemetry is supplied by the read-only hwmon backend; this
// window never talks to WMI/ACPI/EC and performs no privileged access.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Test hook: select a page headlessly (dev builds).
    void showPage(int index);

    // One-shot startup animation.
    void playIntro();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    // Builds/replaces the whole central surface (sidebar + pages) against the
    // current theme. Replacing it is how a runtime theme switch refreshes every
    // explicitly-coloured label; the telemetry model is untouched.
    void installCentral();
    void rebuildCentral();
    void applyThemeTokens();

    void setupTray();
    void gotoPage(int index);
    void pushCurrentTelemetry(int index);
    void pushAllTelemetry();
    void refreshDiagnostics();
    QString pageName(int index) const;

    // Tray / background lifecycle. Hide/show is an instant swap (hide()/show()),
    // never an animation, and never recreates the window, model or pages.
    void showWindow();
    void hideWindow();
    void toggleVisibility();

    void onThemeChanged();
    void onTempUnitChanged();
    void onRefreshChanged();

    Sidebar *m_sidebar = nullptr;
    PageContainer *m_pages = nullptr;

    TelemetryModel *m_model = nullptr;

    DashboardPage *m_dashboard = nullptr;
    CoolingPage *m_cooling = nullptr;
    DevicePage *m_device = nullptr;
    DiagnosticsPage *m_diagnostics = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    AboutPage *m_about = nullptr;

    SystemInfo m_sysInfo;

    QSystemTrayIcon *m_tray = nullptr;
    Qt::WindowStates m_stateBeforeHide = Qt::WindowNoState;
};
