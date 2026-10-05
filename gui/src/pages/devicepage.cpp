#include "devicepage.h"

#include "pageutils.h"
#include "theme/theme.h"

#include <QClipboard>
#include <QDateTime>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

class GroupSurface : public QWidget
{
public:
    explicit GroupSurface(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QColor border = Theme::border();
        border.setAlpha(170);
        p.setPen(QPen(border, 1));
        p.setBrush(Theme::surface());
        p.drawRoundedRect(r, Theme::Radius, Theme::Radius);
    }
};

QString notAvailable(const QString &v)
{
    return v.isEmpty() ? QStringLiteral("Not available") : v;
}

const QString kSensors = QStringLiteral("temp1_input \u00B7 temp2_input \u00B7 fan1_input \u00B7 fan2_input");

QLabel *captionLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::font(11, QFont::Normal));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    l->setMinimumHeight(32);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textSecondary());
    l->setPalette(p);
    return l;
}

QLabel *valueLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::font(11, QFont::Medium));
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->setMinimumHeight(32);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textPrimary());
    l->setPalette(p);
    return l;
}

QWidget *makeGroup(const QString &title, QGridLayout *&gridOut, QWidget *parent)
{
    auto *group = new QWidget(parent);
    auto *v = new QVBoxLayout(group);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(Theme::Space::S);

    auto *titleLabel = new QLabel(title);
    titleLabel->setFont(Theme::labelFont(9, QFont::DemiBold, 150));
    {
        QPalette p = titleLabel->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        titleLabel->setPalette(p);
    }

    auto *surface = new GroupSurface;
    gridOut = new QGridLayout(surface);
    gridOut->setContentsMargins(Theme::Space::XL, Theme::Space::S, Theme::Space::XL, Theme::Space::S);
    gridOut->setHorizontalSpacing(Theme::Space::XL);
    gridOut->setVerticalSpacing(0);
    gridOut->setColumnStretch(1, 1);

    v->addWidget(titleLabel);
    v->addWidget(surface);
    return group;
}

void addRow(QGridLayout *grid, int row, QLabel *caption, QLabel *value)
{
    grid->addWidget(caption, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(value, row, 1, Qt::AlignRight | Qt::AlignVCenter);
}

} // namespace

DevicePage::DevicePage(QWidget *parent)
    : QWidget(parent)
{
    m_info = readSystemInfo();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Space::XXL, Theme::Space::XL, Theme::Space::XXL, Theme::Space::XL);
    root->setSpacing(Theme::Space::L);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Device")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("Hardware and software interface details for this machine.")));

    // Product identity line.
    auto *product = new QLabel(m_info.model());
    product->setFont(Theme::font(17, QFont::Bold));
    {
        QPalette p = product->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        product->setPalette(p);
    }
    auto *maker = new QLabel(notAvailable(m_info.manufacturer));
    maker->setFont(Theme::font(11, QFont::Normal));
    {
        QPalette p = maker->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        maker->setPalette(p);
    }
    auto *identity = new QWidget;
    auto *idLayout = new QVBoxLayout(identity);
    idLayout->setContentsMargins(0, Theme::Space::S, 0, 0);
    idLayout->setSpacing(2);
    idLayout->addWidget(product);
    idLayout->addWidget(maker);
    root->addWidget(identity);

    // SYSTEM group.
    QGridLayout *sysGrid = nullptr;
    QWidget *sysGroup = makeGroup(QStringLiteral("SYSTEM"), sysGrid, this);
    m_model = valueLabel(m_info.model());
    m_board = valueLabel(notAvailable(m_info.boardName));
    m_bios = valueLabel(m_info.bios());
    const QString kernel = m_info.kernelVersion.isEmpty()
                               ? QString()
                               : QStringLiteral("%1 %2 (%3)")
                                     .arg(m_info.kernelType, m_info.kernelVersion, m_info.architecture);
    m_kernel = valueLabel(notAvailable(kernel));
    int r = 0;
    addRow(sysGrid, r++, captionLabel(QStringLiteral("Model")), m_model);
    addRow(sysGrid, r++, captionLabel(QStringLiteral("Board")), m_board);
    addRow(sysGrid, r++, captionLabel(QStringLiteral("BIOS")), m_bios);
    addRow(sysGrid, r++, captionLabel(QStringLiteral("Kernel")), m_kernel);
    root->addWidget(sysGroup);

    // TELEMETRY group.
    QGridLayout *telGrid = nullptr;
    QWidget *telGroup = makeGroup(QStringLiteral("TELEMETRY"), telGrid, this);
    m_driver = valueLabel(QString());
    m_interface = valueLabel(QStringLiteral("WMI \u2192 hwmon"));
    m_sensorStatus = valueLabel(QString());
    m_hwmon = valueLabel(QString());
    m_sensors = valueLabel(kSensors);
    m_state = valueLabel(QString());
    r = 0;
    addRow(telGrid, r++, captionLabel(QStringLiteral("Telemetry Driver")), m_driver);
    addRow(telGrid, r++, captionLabel(QStringLiteral("Interface")), m_interface);
    addRow(telGrid, r++, captionLabel(QStringLiteral("Sensor Status")), m_sensorStatus);
    addRow(telGrid, r++, captionLabel(QStringLiteral("Device")), m_hwmon);
    addRow(telGrid, r++, captionLabel(QStringLiteral("Sensors")), m_sensors);
    addRow(telGrid, r++, captionLabel(QStringLiteral("Internal State")), m_state);
    root->addWidget(telGroup);

    // Copy diagnostics action.
    m_copy = new QPushButton(QStringLiteral("Copy diagnostics"));
    m_copy->setCursor(Qt::PointingHandCursor);
    m_copy->setFont(Theme::font(11, QFont::Medium));
    m_copy->setStyleSheet(QStringLiteral(
                              "QPushButton{background:%1;color:%2;border:1px solid %3;"
                              "border-radius:8px;padding:8px 16px;}"
                              "QPushButton:hover{background:%4;}"
                              "QPushButton:pressed{background:%1;}")
                              .arg(Theme::surfaceElevated().name(),
                                   Theme::textPrimary().name(),
                                   Theme::border().name(),
                                   Theme::surfaceHover().name()));
    connect(m_copy, &QPushButton::clicked, this, &DevicePage::copyDiagnostics);
    root->addWidget(m_copy, 0, Qt::AlignLeft);
    root->addStretch(1);

    refreshStatus();
}

void DevicePage::setSnapshot(const TelemetrySnapshot &snapshot, const QVector<HistorySample> &)
{
    m_snapshot = snapshot;
    refreshStatus();
}

void DevicePage::refreshStatus()
{
    const bool connected = m_snapshot.status == AppState::Status::Connected;
    const bool degraded = m_snapshot.status == AppState::Status::Degraded;
    const bool disconnected = m_snapshot.status == AppState::Status::Disconnected;

    const QString friendly = connected ? QStringLiteral("Connected")
                             : degraded  ? QStringLiteral("Limited")
                                         : QStringLiteral("Unavailable");
    const QString raw = connected ? QStringLiteral("CONNECTED")
                        : degraded  ? QStringLiteral("DEGRADED")
                                    : QStringLiteral("DISCONNECTED");

    m_driver->setText(m_snapshot.deviceFound ? QStringLiteral("excalibur_wmi")
                                             : QStringLiteral("Not available"));
    m_sensorStatus->setText(friendly);
    m_state->setText(raw);
    m_hwmon->setText(disconnected ? QStringLiteral("Not available") : notAvailable(m_snapshot.hwmonPath));

    QColor statusColor = Theme::textMuted();
    if (connected)
        statusColor = Theme::success();
    else if (degraded)
        statusColor = Theme::warning();
    QPalette p = m_sensorStatus->palette();
    p.setColor(QPalette::WindowText, statusColor);
    m_sensorStatus->setPalette(p);
}

QString DevicePage::diagnosticsText() const
{
    const auto reading = [](const QString &label, bool valid, const QString &value) {
        return QStringLiteral("%1: %2\n").arg(label, valid ? value : QStringLiteral("Not available"));
    };
    const QString cpuTemp = QStringLiteral("%1 \u00B0C").arg(QString::number(m_snapshot.cpuTempC, 'f', 1));
    const QString gpuTemp = QStringLiteral("%1 \u00B0C").arg(QString::number(m_snapshot.gpuTempC, 'f', 1));
    const QString cpuFan = QStringLiteral("%1 RPM").arg(m_snapshot.cpuFanRpm);
    const QString gpuFan = QStringLiteral("%1 RPM").arg(m_snapshot.gpuFanRpm);
    const QString kernel = m_info.kernelVersion.isEmpty()
                               ? QString()
                               : QStringLiteral("%1 %2 (%3)")
                                     .arg(m_info.kernelType, m_info.kernelVersion, m_info.architecture);

    QString t;
    t += QStringLiteral("EXCALIBUR Control Center \u2014 Diagnostics\n");
    t += QStringLiteral("Generated: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    t += QStringLiteral("Model: %1\n").arg(m_info.model());
    t += QStringLiteral("Manufacturer: %1\n").arg(notAvailable(m_info.manufacturer));
    t += QStringLiteral("Board: %1\n").arg(notAvailable(m_info.boardName));
    t += QStringLiteral("BIOS: %1\n").arg(m_info.bios());
    t += QStringLiteral("Kernel: %1\n").arg(notAvailable(kernel));
    t += QStringLiteral("Driver: %1\n").arg(notAvailable(m_snapshot.deviceFound ? QStringLiteral("excalibur_wmi") : QString()));
    t += QStringLiteral("Interface: WMI \u2192 hwmon\n");
    t += QStringLiteral("Sensor status: %1\n").arg(m_sensorStatus->text());
    t += QStringLiteral("Internal state: %1\n").arg(m_state->text());
    t += QStringLiteral("hwmon: %1\n").arg(notAvailable(m_snapshot.hwmonPath));
    t += QStringLiteral("Sensors: %1\n\n").arg(kSensors);
    t += QStringLiteral("Current readings\n");
    t += reading(QStringLiteral("CPU temperature"), m_snapshot.cpuTempValid, cpuTemp);
    t += reading(QStringLiteral("GPU temperature"), m_snapshot.gpuTempValid, gpuTemp);
    t += reading(QStringLiteral("CPU fan"), m_snapshot.cpuFanValid, cpuFan);
    t += reading(QStringLiteral("GPU fan"), m_snapshot.gpuFanValid, gpuFan);
    return t;
}

void DevicePage::copyDiagnostics()
{
    QGuiApplication::clipboard()->setText(diagnosticsText());
    m_copy->setText(QStringLiteral("Copied"));
    QTimer::singleShot(1500, this, [this]() {
        m_copy->setText(QStringLiteral("Copy diagnostics"));
    });
}
