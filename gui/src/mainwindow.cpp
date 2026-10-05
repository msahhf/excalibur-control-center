#include "mainwindow.h"

#include "app/telemetrymodel.h"
#include "navigation/pagecontainer.h"
#include "navigation/sidebar.h"
#include "pages/aboutpage.h"
#include "pages/coolingpage.h"
#include "pages/dashboardpage.h"
#include "pages/devicepage.h"
#include "theme/theme.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    setupTray();
    m_model->start();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("EXCALIBUR Control Center"));
    setMinimumSize(1000, 650);
    resize(1100, 700);

    // The window owns initial focus so child widgets do not show a keyboard
    // focus ring until the user actually navigates with the keyboard.
    setFocusPolicy(Qt::StrongFocus);

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
    m_about = new AboutPage;

    m_pages->addPage(m_dashboard);
    m_pages->addPage(m_cooling);
    m_pages->addPage(m_device);
    m_pages->addPage(m_about);

    root->addWidget(m_sidebar);
    root->addWidget(m_pages, 1);

    setCentralWidget(central);

    m_sidebar->addItem(QStringLiteral("Dashboard"), Sidebar::Icon::Dashboard);
    m_sidebar->addItem(QStringLiteral("Cooling"), Sidebar::Icon::Cooling);
    m_sidebar->addItem(QStringLiteral("Device"), Sidebar::Icon::Device);
    m_sidebar->addBottomItem(QStringLiteral("About"), Sidebar::Icon::About);

    connect(m_sidebar, &Sidebar::pageSelected, this, &MainWindow::gotoPage);

    m_pages->setCurrentIndex(0);
    m_sidebar->setCurrentIndex(0);

    // Telemetry: single source of truth; pages receive snapshots, never sysfs.
    m_model = new TelemetryModel(this);
    connect(m_model, &TelemetryModel::updated, m_dashboard, &DashboardPage::setSnapshot);
    connect(m_model, &TelemetryModel::updated, m_cooling, &CoolingPage::setTelemetry);
    connect(m_model, &TelemetryModel::updated, m_device, &DevicePage::setSnapshot);
}

void MainWindow::gotoPage(int index)
{
    m_pages->setCurrentIndex(index);
    m_sidebar->setCurrentIndex(index);
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
        && event->key() <= Qt::Key_4) {
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
    setFocus(Qt::ActiveWindowFocusReason);
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

    // Sidebar slides in slightly from the left and settles.
    const QPoint sidebarEnd = m_sidebar->pos();
    const QPoint sidebarStart = sidebarEnd - QPoint(16, 0);
    m_sidebar->move(sidebarStart);
    auto *slide = new QPropertyAnimation(m_sidebar, "pos", this);
    slide->setDuration(260);
    slide->setStartValue(sidebarStart);
    slide->setEndValue(sidebarEnd);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->start(QAbstractAnimation::DeleteWhenStopped);

    // Dashboard hero and hardware panel settle in with a short stagger.
    m_dashboard->playIntro();
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::accent());
        p.drawRoundedRect(QRectF(4, 4, 56, 56), 16, 16);
        QPen pen(Theme::background(), 5);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.drawLine(QPointF(22, 22), QPointF(22, 42));
        p.drawLine(QPointF(22, 22), QPointF(42, 22));
        p.drawLine(QPointF(22, 32), QPointF(37, 32));
        p.drawLine(QPointF(22, 42), QPointF(42, 42));
    }

    m_tray = new QSystemTrayIcon(QIcon(pm), this);
    m_tray->setToolTip(QStringLiteral("EXCALIBUR Control Center"));

    auto *menu = new QMenu(this);
    auto *openAction = menu->addAction(QStringLiteral("Open Control Center"));
    connect(openAction, &QAction::triggered, this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    menu->addSeparator();
    auto *quitAction = menu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    showNormal();
                    raise();
                    activateWindow();
                }
            });
    m_tray->show();
}
