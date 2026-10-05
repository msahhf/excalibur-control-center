#include "mainwindow.h"

#include "hardwarecard.h"
#include "theme.h"
#include "statuspanel.h"

#include <QFrame>
#include <QtGlobal>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QLabel *makeLabel(const QString &text, int deltaPt, bool bold,
                  const QColor &color = QColor(), double tracking = 100.0)
{
    auto *l = new QLabel(text);
    QFont f = l->font();
    if (deltaPt != 0)
        f.setPointSize(f.pointSize() + deltaPt);
    f.setBold(bold);
    if (tracking != 100.0)
        f.setLetterSpacing(QFont::PercentageSpacing, tracking);
    l->setFont(f);
    if (color.isValid()) {
        QPalette p = l->palette();
        p.setColor(QPalette::WindowText, color);
        l->setPalette(p);
    }
    return l;
}

QLabel *sectionHeading(const QString &text, const QWidget *ref)
{
    auto *l = makeLabel(text, 0, true, Theme::dim(ref), 140.0);
    return l;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_client(qEnvironmentVariable("EXCALIBUR_HWMON_ROOT", QStringLiteral("/sys/class/hwmon")))
{
    buildUi();

    auto *timer = new QTimer(this);
    timer->setInterval(1000); // 1 s refresh (driver already caches ~1 s)
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh);
    timer->start();

    refresh();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("EXCALIBUR Control Center"));
    setMinimumSize(960, 620);
    resize(1100, 700);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *column = buildColumn();
    column->setMaximumWidth(1180);
    column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    root->addStretch(1);
    root->addWidget(column);
    root->addStretch(1);

    setCentralWidget(central);
}

QWidget *MainWindow::buildColumn()
{
    auto *column = new QWidget;
    auto *v = new QVBoxLayout(column);
    v->setContentsMargins(36, 28, 36, 22);
    v->setSpacing(22);

    v->addWidget(buildHeader());

    auto *accentLine = new QFrame;
    accentLine->setFixedHeight(2);
    accentLine->setStyleSheet(
        QStringLiteral("background-color:%1;").arg(QLatin1String(Theme::Accent)));
    v->addWidget(accentLine);

    v->addWidget(buildCenter(), 1);
    v->addWidget(buildStatusSection());
    v->addWidget(buildFooter());
    return column;
}

QWidget *MainWindow::buildHeader()
{
    auto *header = new QWidget;
    auto *row = new QHBoxLayout(header);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(16);

    // Left: wordmark + product line
    auto *left = new QVBoxLayout;
    left->setSpacing(1);
    auto *word1 = makeLabel(QStringLiteral("EXCALIBUR"), 16, true);
    auto *word2 = makeLabel(QStringLiteral("CONTROL CENTER"), 4, true, QColor(), 145.0);
    auto *subtitle = makeLabel(QStringLiteral("CASPER EXCALIBUR G870"), 0, false, Theme::dim(this));
    left->addWidget(word1);
    left->addWidget(word2);
    left->addSpacing(3);
    left->addWidget(subtitle);

    // Right: system state
    auto *right = new QVBoxLayout;
    right->setSpacing(3);
    right->setAlignment(Qt::AlignTop | Qt::AlignRight);

    auto *stateRow = new QHBoxLayout;
    stateRow->setSpacing(8);
    m_statusDot = new QLabel;
    m_statusDot->setFixedSize(11, 11);
    m_statusText = makeLabel(QStringLiteral("—"), 2, true, QColor(), 135.0);
    stateRow->addStretch(1);
    stateRow->addWidget(m_statusDot, 0, Qt::AlignVCenter);
    stateRow->addWidget(m_statusText, 0, Qt::AlignVCenter);

    m_headerMeta = makeLabel(QString(), -1, false, Theme::dim(this));
    m_headerMeta->setAlignment(Qt::AlignRight);

    right->addLayout(stateRow);
    right->addWidget(m_headerMeta);

    row->addLayout(left);
    row->addStretch(1);
    row->addLayout(right);
    return header;
}

QWidget *MainWindow::buildCenter()
{
    m_center = new QStackedWidget;

    // Page 0: hardware telemetry
    auto *live = new QWidget;
    auto *v = new QVBoxLayout(live);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);
    v->addWidget(sectionHeading(QStringLiteral("HARDWARE"), this));

    m_cpuCard = new HardwareCard(QStringLiteral("PROCESSOR"), QStringLiteral("CPU"),
                                 QStringLiteral("CPU TEMPERATURE"), QStringLiteral("CPU FAN"), this);
    m_gpuCard = new HardwareCard(QStringLiteral("GRAPHICS"), QStringLiteral("NVIDIA GPU"),
                                 QStringLiteral("GPU TEMPERATURE"), QStringLiteral("GPU FAN"), this);

    auto *cards = new QHBoxLayout;
    cards->setSpacing(18);
    cards->addWidget(m_cpuCard, 1);
    cards->addWidget(m_gpuCard, 1);
    v->addLayout(cards, 1);

    // Page 1: empty state (driver not loaded)
    m_center->addWidget(live);
    m_center->addWidget(buildEmptyState());
    return m_center;
}

QWidget *MainWindow::buildEmptyState()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);
    v->addStretch(2);

    auto *title = makeLabel(QStringLiteral("Telemetry unavailable"), 8, true);
    title->setAlignment(Qt::AlignCenter);
    auto *hint = makeLabel(QStringLiteral(
                               "Load the EXCALIBUR kernel driver to enable hardware telemetry."),
                           0, false, Theme::dim(this));
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    v->addWidget(title);
    v->addWidget(hint);
    v->addStretch(3);
    return page;
}

QWidget *MainWindow::buildStatusSection()
{
    auto *section = new QWidget;
    auto *v = new QVBoxLayout(section);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(12);
    v->addWidget(sectionHeading(QStringLiteral("SYSTEM STATUS"), this));

    m_statusPanel = new StatusPanel(this);
    v->addWidget(m_statusPanel);
    return section;
}

QWidget *MainWindow::buildFooter()
{
    auto *footer = new QWidget;
    auto *row = new QHBoxLayout(footer);
    row->setContentsMargins(0, 6, 0, 0);

    auto *left = makeLabel(QStringLiteral("READ-ONLY TELEMETRY"), -1, false, Theme::dim(this), 120.0);
    auto *right = makeLabel(QStringLiteral("v0.1.0"), -1, false, Theme::dim(this));

    row->addWidget(left);
    row->addStretch(1);
    row->addWidget(right);
    return footer;
}

void MainWindow::refresh()
{
    // Re-discover every tick so the UI reacts to the driver being loaded/removed.
    m_client.discover();
    const ExcTelemetry t = m_client.read();
    const ExcStatus st = m_client.status(t);

    m_cpuCard->setTemperature(t.cpuTempC, t.cpuTempValid);
    m_cpuCard->setFan(t.cpuFanRpm, t.cpuFanValid);
    m_gpuCard->setTemperature(t.gpuTempC, t.gpuTempValid);
    m_gpuCard->setFan(t.gpuFanRpm, t.gpuFanValid);

    QString statusText;
    QString statusColor;
    bool hollow = false;

    StatusPanel::Info info;
    info.hardware = QStringLiteral("EXCALIBUR G870");

    switch (st) {
    case ExcStatus::Connected:
        statusText = QStringLiteral("SYSTEM ONLINE");
        statusColor = QLatin1String(Theme::Ok);
        info.telemetry = QStringLiteral("Live");
        info.driver = QStringLiteral("Loaded");
        info.interfaceName = QStringLiteral("hwmon");
        info.secondary = QStringLiteral("hwmon interface · detected");
        m_center->setCurrentIndex(0);
        m_headerMeta->setText(QStringLiteral("G870 · hwmon"));
        break;
    case ExcStatus::Degraded:
        statusText = QStringLiteral("TELEMETRY DEGRADED");
        statusColor = QLatin1String(Theme::Warn);
        info.telemetry = QStringLiteral("Degraded");
        info.driver = QStringLiteral("Loaded");
        info.interfaceName = QStringLiteral("hwmon");
        info.secondary = QStringLiteral("some telemetry values unavailable");
        m_center->setCurrentIndex(0);
        m_headerMeta->setText(QStringLiteral("G870 · hwmon"));
        break;
    case ExcStatus::Disconnected:
        statusText = QStringLiteral("DRIVER OFFLINE");
        statusColor = QLatin1String(Theme::Error);
        hollow = true;
        info.telemetry = QStringLiteral("Unavailable");
        info.driver = QStringLiteral("Not loaded");
        info.interfaceName = QStringLiteral("—");
        info.secondary = QStringLiteral("driver not detected");
        m_center->setCurrentIndex(1);
        m_headerMeta->setText(QStringLiteral("G870"));
        break;
    }

    m_statusText->setText(statusText);
    m_statusDot->setStyleSheet(
        hollow ? QStringLiteral("border:2px solid %1; border-radius:5px; background:transparent;")
                     .arg(statusColor)
               : QStringLiteral("background-color:%1; border-radius:5px;").arg(statusColor));

    m_statusPanel->setInfo(info);
}
