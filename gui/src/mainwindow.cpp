#include "mainwindow.h"

#include "app/identity.h"
#include "app/diagnostics.h"
#include "app/telemetrymodel.h"
#include "app/usersettings.h"
#include "navigation/pagecontainer.h"
#include "navigation/sidebar.h"
#include "pages/aboutpage.h"
#include "pages/coolingpage.h"
#include "pages/dashboardpage.h"
#include "pages/devicepage.h"
#include "pages/diagnosticspage.h"
#include "pages/settingspage.h"
#include "theme/theme.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QStyleHints>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(AppIdentity::displayName());
    setWindowIcon(QApplication::windowIcon());
    setMinimumSize(1000, 650);
    resize(1100, 700);

    // The window owns initial focus so child widgets do not show a keyboard
    // focus ring until the user actually navigates with the keyboard.
    setFocusPolicy(Qt::StrongFocus);

    m_model = new TelemetryModel(this);
    m_sysInfo = readSystemInfo();

    // The diagnostics viewer is rebuilt from the same snapshot (single source).
    // Connected ONCE here, not in installCentral(): the receiver is `this`
    // (MainWindow), which survives a theme rebuild, so re-connecting there would
    // accumulate duplicate slots on every theme change.
    connect(m_model, &TelemetryModel::updated, this, [this]() { refreshDiagnostics(); });

    // Apply the resolved theme before any widget is built, then build the shell.
    applyThemeTokens();
    installCentral();
    setupTray();
    refreshDiagnostics();

    UserSettings &us = UserSettings::instance();
    connect(&us, &UserSettings::themeChanged, this, &MainWindow::onThemeChanged);
    connect(&us, &UserSettings::tempUnitChanged, this, &MainWindow::onTempUnitChanged);
    connect(&us, &UserSettings::refreshMsChanged, this, &MainWindow::onRefreshChanged);

    // Follow the OS colour scheme while the preference is "System".
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        if (UserSettings::instance().theme() == UserSettings::AppTheme::System)
            onThemeChanged();
    });

    m_model->start(us.refreshMs());
}

void MainWindow::applyThemeTokens()
{
    const UserSettings::AppTheme pref = UserSettings::instance().theme();
    Theme::Mode mode;
    if (pref == UserSettings::AppTheme::Light) {
        mode = Theme::Mode::Light;
    } else if (pref == UserSettings::AppTheme::Dark) {
        mode = Theme::Mode::Dark;
    } else {
        const Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
        mode = (scheme == Qt::ColorScheme::Light) ? Theme::Mode::Light : Theme::Mode::Dark;
    }
    Theme::setMode(mode);
    Theme::applyApplicationPalette();
}

void MainWindow::installCentral()
{
    auto *central = new QWidget(this);
    QPalette p = central->palette();
    p.setColor(QPalette::Window, Theme::background());
    central->setPalette(p);
    central->setAutoFillBackground(true);

    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_sidebar = new Sidebar(central);
    m_pages = new PageContainer(central);

    m_dashboard = new DashboardPage;
    m_cooling = new CoolingPage;
    m_device = new DevicePage;
    m_diagnostics = new DiagnosticsPage;
    m_settingsPage = new SettingsPage;
    m_about = new AboutPage;

    m_pages->addPage(m_dashboard);    // 0
    m_pages->addPage(m_cooling);      // 1
    m_pages->addPage(m_device);       // 2
    m_pages->addPage(m_diagnostics);  // 3
    m_pages->addPage(m_settingsPage); // 4
    m_pages->addPage(m_about);        // 5

    root->addWidget(m_sidebar);
    root->addWidget(m_pages, 1);

    setCentralWidget(central); // takes ownership; deletes any previous central

    m_sidebar->addItem(QStringLiteral("Dashboard"), Sidebar::Icon::Dashboard);
    m_sidebar->addItem(QStringLiteral("Cooling"), Sidebar::Icon::Cooling);
    m_sidebar->addItem(QStringLiteral("Device"), Sidebar::Icon::Device);
    m_sidebar->addItem(QStringLiteral("Diagnostics"), Sidebar::Icon::Diagnostics);
    m_sidebar->addItem(QStringLiteral("Settings"), Sidebar::Icon::Settings);
    m_sidebar->addBottomItem(QStringLiteral("About"), Sidebar::Icon::About);

    connect(m_sidebar, &Sidebar::pageSelected, this, &MainWindow::gotoPage);

    // Telemetry: single source of truth; pages receive snapshots, never sysfs.
    connect(m_model, &TelemetryModel::updated, m_dashboard, &DashboardPage::setSnapshot);
    connect(m_model, &TelemetryModel::updated, m_cooling, &CoolingPage::setTelemetry);
    connect(m_model, &TelemetryModel::updated, m_device, &DevicePage::setSnapshot);

    m_pages->setCurrentIndex(0);
    m_sidebar->setCurrentIndex(0);
}

void MainWindow::rebuildCentral()
{
    const int current = m_pages ? m_pages->currentIndex() : 0;
    installCentral();
    m_pages->setCurrentIndex(current);
    m_sidebar->setCurrentIndex(current);
    // All pages were just recreated, so hydrate them from the last snapshot
    // immediately (a steady sensor may not emit again for a while).
    pushAllTelemetry();
}

void MainWindow::gotoPage(int index)
{
    m_pages->setCurrentIndex(index);
    m_sidebar->setCurrentIndex(index);
    pushCurrentTelemetry(index);
    refreshDiagnostics(); // reflect the new current page
}

void MainWindow::pushCurrentTelemetry(int index)
{
    const TelemetrySnapshot &s = m_model->snapshot();
    const QVector<HistorySample> &h = m_model->history();

    switch (index) {
    case 0:
        m_dashboard->setSnapshot(s, h);
        break;
    case 1:
        m_cooling->setTelemetry(s, h);
        break;
    case 2:
        m_device->setSnapshot(s, h);
        break;
    default:
        break; // Diagnostics / Settings / About get their own updates
    }
}

void MainWindow::pushAllTelemetry()
{
    const TelemetrySnapshot &s = m_model->snapshot();
    const QVector<HistorySample> &h = m_model->history();
    m_dashboard->setSnapshot(s, h);
    m_cooling->setTelemetry(s, h);
    m_device->setSnapshot(s, h);
    refreshDiagnostics();
}

QString MainWindow::pageName(int index) const
{
    switch (index) {
    case 0: return QStringLiteral("Dashboard");
    case 1: return QStringLiteral("Cooling");
    case 2: return QStringLiteral("Device");
    case 3: return QStringLiteral("Diagnostics");
    case 4: return QStringLiteral("Settings");
    case 5: return QStringLiteral("About");
    default: break;
    }
    return QString();
}

void MainWindow::refreshDiagnostics()
{
    if (!m_diagnostics || !m_model || !m_pages)
        return;

    DiagnosticsContext ctx;
    ctx.currentPage = pageName(m_pages->currentIndex());
    ctx.trayAvailable = m_tray != nullptr;
    ctx.trayVisible = m_tray && m_tray->isVisible();
    ctx.singleInstance = true; // this process holds the lock

    const DiagnosticsSnapshot d =
        buildDiagnostics(m_model->snapshot(), m_model->history(), m_sysInfo, ctx);
    m_diagnostics->setSnapshot(d);
}

void MainWindow::onThemeChanged()
{
    // Defer: a theme change is usually triggered from a Settings combo, and
    // rebuilding the shell (which destroys that combo) must not run inside its
    // own signal. One event-loop turn later is still instant to the user.
    QTimer::singleShot(0, this, [this]() {
        applyThemeTokens();
        rebuildCentral();
    });
}

void MainWindow::onTempUnitChanged()
{
    // Re-render the existing snapshot in the new unit; no firmware read.
    pushAllTelemetry();
}

void MainWindow::onRefreshChanged()
{
    m_model->setInterval(UserSettings::instance().refreshMs());
    refreshDiagnostics();
}

void MainWindow::showPage(int index)
{
    gotoPage(index);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int count = m_pages->count();
    if (event->modifiers().testFlag(Qt::ControlModifier) && event->key() == Qt::Key_PageDown) {
        gotoPage((m_pages->currentIndex() + 1) % count);
        event->accept();
        return;
    }
    if (event->modifiers().testFlag(Qt::ControlModifier) && event->key() == Qt::Key_PageUp) {
        gotoPage((m_pages->currentIndex() + count - 1) % count);
        event->accept();
        return;
    }
    if (event->modifiers().testFlag(Qt::AltModifier) && event->key() >= Qt::Key_1
        && event->key() <= Qt::Key_1 + count - 1) {
        const int index = event->key() - Qt::Key_1;
        if (index < count)
            gotoPage(index);
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // Deliberately do NOT force keyboard focus here: the window should not steal
    // focus from the desktop/session on startup. Keyboard focus rings only appear
    // once the user navigates with the keyboard (see NavButton::focusInEvent).
}

void MainWindow::playIntro()
{
    if (!qEnvironmentVariableIsEmpty("EXCALIBUR_NO_ANIM"))
        return;

    // Window fades in (~250 ms). Usability is never blocked: the window is
    // interactive immediately; opacity is purely visual.
    auto *effect = new QGraphicsOpacityEffect(centralWidget());
    effect->setOpacity(0.0);
    centralWidget()->setGraphicsEffect(effect);
    auto *fade = new QPropertyAnimation(effect, "opacity", this);
    fade->setDuration(250);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    connect(fade, &QPropertyAnimation::finished, this, [this]() {
        centralWidget()->setGraphicsEffect(nullptr);
    });
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    // Dashboard hero and hardware panel settle in with a short stagger.
    m_dashboard->playIntro();
}

void MainWindow::showWindow()
{
    // Restore the pre-hide window state (maximized/fullscreen/normal) instead of
    // forcing normal, and never recreate or animate anything.
    if (m_stateBeforeHide.testFlag(Qt::WindowFullScreen))
        showFullScreen();
    else if (m_stateBeforeHide.testFlag(Qt::WindowMaximized))
        showMaximized();
    else
        showNormal();
    raise();
    activateWindow();
    refreshDiagnostics(); // window/tray visibility changed
}

void MainWindow::hideWindow()
{
    m_stateBeforeHide = windowState();
    hide(); // instant: no fade, no effect, no offset
    refreshDiagnostics();
}

void MainWindow::toggleVisibility()
{
    if (isVisible())
        hideWindow();
    else
        showWindow();
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    // Native Qt tray with the real EXCALIBUR brand icon (embedded multi-res PNGs,
    // visible on both dark and light panel backgrounds). One icon, created once.
    m_tray = new QSystemTrayIcon(QApplication::windowIcon(), this);
    m_tray->setToolTip(AppIdentity::displayName());

    auto *menu = new QMenu(this);
    auto *openAction = menu->addAction(QStringLiteral("Open EXCALIBUR Control Center"));
    connect(openAction, &QAction::triggered, this, &MainWindow::showWindow);

    auto *toggleAction = menu->addAction(QStringLiteral("Show/Hide"));
    connect(toggleAction, &QAction::triggered, this, &MainWindow::toggleVisibility);

    menu->addSeparator();
    auto *quitAction = menu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(menu);

    // Single click toggles; double click raises. Instant, no animation.
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger)
                    toggleVisibility();
                else if (reason == QSystemTrayIcon::DoubleClick)
                    showWindow();
            });

    // Remove the tray icon cleanly on shutdown (no zombie tray entry).
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { m_tray->hide(); });

    m_tray->show();
}
