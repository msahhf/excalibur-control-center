#include "devicepage.h"

#include "app/usersettings.h"
#include "components/actionbutton.h"
#include "components/surfacepanel.h"
#include "pageutils.h"
#include "theme/theme.h"
#include "util/units.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// Raw hwmon channel names — technical, shown only in the copied diagnostics.
const QString kSensorsRaw =
    QStringLiteral("temp1_input \u00B7 temp2_input \u00B7 fan1_input \u00B7 fan2_input");

QString notAvailable(const QString &v)
{
    // Not a QObject member: tr() does not compile here (plan section 4.1).
    return v.isEmpty() ? QCoreApplication::translate("DevicePage", "Not available") : v;
}

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

    auto *surface = new SurfacePanel;
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

    auto *root = PageUtils::makeCenteredColumn(this);

    root->addWidget(PageUtils::pageTitle(tr("Device")));
    root->addWidget(PageUtils::pageSubtitle(
        tr("Hardware and software interface details for this machine.")));

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
    QWidget *sysGroup = makeGroup(tr("SYSTEM"), sysGrid, this);
    m_model = valueLabel(m_info.model());
    m_board = valueLabel(notAvailable(m_info.boardName));
    m_bios = valueLabel(m_info.bios());
    const QString kernel = m_info.kernelVersion.isEmpty()
                               ? QString()
                               : QStringLiteral("%1 %2 (%3)")
                                     .arg(m_info.kernelType, m_info.kernelVersion, m_info.architecture);
    m_kernel = valueLabel(notAvailable(kernel));
    int r = 0;
    addRow(sysGrid, r++, captionLabel(tr("Model")), m_model);
    addRow(sysGrid, r++, captionLabel(tr("Board")), m_board);
    addRow(sysGrid, r++, captionLabel(tr("BIOS")), m_bios);
    addRow(sysGrid, r++, captionLabel(tr("Kernel")), m_kernel);
    root->addWidget(sysGroup);

    // TELEMETRY group.
    QGridLayout *telGrid = nullptr;
    QWidget *telGroup = makeGroup(tr("TELEMETRY"), telGrid, this);
    m_driver = valueLabel(QString());
    m_interface = valueLabel(QStringLiteral("WMI \u2192 hwmon"));
    m_sensorStatus = valueLabel(QString());
    m_hwmon = valueLabel(QString());
    m_sensors = valueLabel(tr("CPU temp \u00B7 GPU temp \u00B7 CPU fan \u00B7 GPU fan"));
    r = 0;
    addRow(telGrid, r++, captionLabel(tr("Telemetry Driver")), m_driver);
    addRow(telGrid, r++, captionLabel(tr("Interface")), m_interface);
    addRow(telGrid, r++, captionLabel(tr("Sensor Status")), m_sensorStatus);
    addRow(telGrid, r++, captionLabel(tr("Device")), m_hwmon);
    addRow(telGrid, r++, captionLabel(tr("Sensors")), m_sensors);
    root->addWidget(telGroup);

    // Copy diagnostics action.
    m_copy = new ActionButton(tr("Copy diagnostics"));
    connect(m_copy, &ActionButton::clicked, this, &DevicePage::copyDiagnostics);
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

    const QString friendly = connected ? tr("Connected")
                             : degraded  ? tr("Limited")
                                         : tr("Unavailable");
    const QString raw = connected ? QStringLiteral("CONNECTED")
                        : degraded  ? QStringLiteral("DEGRADED")
                                    : QStringLiteral("DISCONNECTED");

    m_driver->setText(m_snapshot.deviceFound ? QStringLiteral("excalibur_wmi")
                                             : tr("Not available"));
    m_sensorStatus->setText(friendly);
    m_stateText = raw;
    m_hwmon->setText(disconnected ? tr("Not available") : notAvailable(m_snapshot.hwmonPath));

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
        return QStringLiteral("%1: %2\n").arg(label, valid ? value : tr("Not available"));
    };
    const Units::TemperatureUnit unit = UserSettings::instance().tempUnit();
    const QString unitSym = Units::symbol(unit);
    const QString cpuTemp = QStringLiteral("%1 %2")
                                .arg(QString::number(Units::fromCelsius(m_snapshot.cpuTempC, unit), 'f', 1), unitSym);
    const QString gpuTemp = QStringLiteral("%1 %2")
                                .arg(QString::number(Units::fromCelsius(m_snapshot.gpuTempC, unit), 'f', 1), unitSym);
    const QString cpuFan = QStringLiteral("%1 RPM").arg(m_snapshot.cpuFanRpm);
    const QString gpuFan = QStringLiteral("%1 RPM").arg(m_snapshot.gpuFanRpm);
    const QString kernel = m_info.kernelVersion.isEmpty()
                               ? QString()
                               : QStringLiteral("%1 %2 (%3)")
                                     .arg(m_info.kernelType, m_info.kernelVersion, m_info.architecture);

    QString t;
    t += QStringLiteral("EXCALIBUR Control Center \u2014 Diagnostics\n");
    t += tr("Generated: %1\n\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    t += tr("Model: %1\n").arg(m_info.model());
    t += tr("Manufacturer: %1\n").arg(notAvailable(m_info.manufacturer));
    t += tr("Board: %1\n").arg(notAvailable(m_info.boardName));
    t += tr("BIOS: %1\n").arg(m_info.bios());
    t += tr("Kernel: %1\n").arg(notAvailable(kernel));
    t += tr("Driver: %1\n").arg(notAvailable(m_snapshot.deviceFound ? QStringLiteral("excalibur_wmi") : QString()));
    t += QStringLiteral("Interface: WMI \u2192 hwmon\n");
    t += tr("Sensor status: %1\n").arg(m_sensorStatus->text());
    t += tr("Internal state: %1\n").arg(m_stateText);
    t += tr("hwmon: %1\n").arg(notAvailable(m_snapshot.hwmonPath));
    t += tr("Sensors: %1\n\n").arg(kSensorsRaw);
    t += tr("Current readings\n");
    t += reading(tr("CPU temperature"), m_snapshot.cpuTempValid, cpuTemp);
    t += reading(tr("GPU temperature"), m_snapshot.gpuTempValid, gpuTemp);
    t += reading(tr("CPU fan"), m_snapshot.cpuFanValid, cpuFan);
    t += reading(tr("GPU fan"), m_snapshot.gpuFanValid, gpuFan);
    return t;
}

void DevicePage::copyDiagnostics()
{
    QGuiApplication::clipboard()->setText(diagnosticsText());
    m_copy->setText(tr("Copied"));
    QTimer::singleShot(1500, this, [this]() {
        m_copy->setText(tr("Copy diagnostics"));
    });
}
