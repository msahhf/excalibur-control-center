#pragma once

#include <QMainWindow>

class Sidebar;
class PageContainer;
class DashboardPage;
class CoolingPage;
class DevicePage;
class AboutPage;
class TelemetryModel;
class QSystemTrayIcon;

// EXCALIBUR Control Center application shell: a persistent navigation rail and
// a content area that swaps product pages with a restrained transition.
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

    // One-shot startup animation (fade + sidebar slide + dashboard stagger).
    void playIntro();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    void setupTray();
    void gotoPage(int index);

    Sidebar *m_sidebar = nullptr;
    PageContainer *m_pages = nullptr;

    TelemetryModel *m_model = nullptr;

    DashboardPage *m_dashboard = nullptr;
    CoolingPage *m_cooling = nullptr;
    DevicePage *m_device = nullptr;
    AboutPage *m_about = nullptr;

    QSystemTrayIcon *m_tray = nullptr;
};
