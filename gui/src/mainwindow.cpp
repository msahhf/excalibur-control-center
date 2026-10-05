#include "mainwindow.h"

#include "navigation/pagecontainer.h"
#include "navigation/sidebar.h"
#include "pages/aboutpage.h"
#include "pages/coolingpage.h"
#include "pages/dashboardpage.h"
#include "pages/devicepage.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("EXCALIBUR Control Center"));
    setMinimumSize(1000, 650);
    resize(1100, 700);

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

    connect(m_sidebar, &Sidebar::pageSelected, this, [this](int index) {
        m_pages->setCurrentIndex(index);
        m_sidebar->setCurrentIndex(index);
    });

    m_pages->setCurrentIndex(0);
    m_sidebar->setCurrentIndex(0);
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);
    m_sidebar->setCurrentIndex(index);
}
