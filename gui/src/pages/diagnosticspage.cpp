#include "diagnosticspage.h"

#include "app/usersettings.h"
#include "components/actionbutton.h"
#include "components/surfacepanel.h"
#include "pageutils.h"
#include "theme/theme.h"
#include "util/units.h"

#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const QString kDash = QStringLiteral("\u2014");

QLabel *captionLabel(const QString &text)
{
    auto *l = new QLabel(text);
    l->setFont(Theme::font(11, QFont::Normal));
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    l->setMinimumHeight(30);
    QPalette p = l->palette();
    p.setColor(QPalette::WindowText, Theme::textSecondary());
    l->setPalette(p);
    return l;
}

QLabel *valueLabel()
{
    auto *l = new QLabel(kDash);
    l->setFont(Theme::font(11, QFont::Medium));
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->setMinimumHeight(30);
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

QString fmtUptime(qint64 seconds)
{
    const qint64 h = seconds / 3600;
    const qint64 m = (seconds % 3600) / 60;
    const qint64 s = seconds % 60;
    if (h > 0)
        return QStringLiteral("%1h %2m").arg(h).arg(m);
    if (m > 0)
        return QStringLiteral("%1m %2s").arg(m).arg(s);
    return QStringLiteral("%1s").arg(s);
}

QString fmtTemp(bool valid, double celsius)
{
    if (!valid)
        return kDash;
    const Units::TemperatureUnit u = UserSettings::instance().tempUnit();
    return QStringLiteral("%1 %2")
        .arg(QString::number(Units::fromCelsius(celsius, u), 'f', 1), Units::symbol(u));
}

QString fmtRpm(bool valid, int rpm)
{
    if (!valid)
        return kDash;
    QString s = QString::number(rpm);
    for (int i = s.size() - 3; i > 0; i -= 3)
        s.insert(i, QLatin1Char(','));
    return s + QStringLiteral(" RPM");
}

} // namespace

DiagnosticsPage::DiagnosticsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget;
    auto *root = PageUtils::makeCenteredColumn(content, 780);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Diagnostics")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("The application's current knowledge, and an export for support.")));

    auto *actions = new QWidget;
    auto *av = new QVBoxLayout(actions);
    av->setContentsMargins(0, 0, 0, 0);
    av->setSpacing(Theme::Space::XS);
    m_export = new ActionButton(QStringLiteral("Export Diagnostics"));
    connect(m_export, &ActionButton::clicked, this, &DiagnosticsPage::exportDiagnostics);
    av->addWidget(m_export, 0, Qt::AlignLeft);
    m_exportStatus = new QLabel;
    m_exportStatus->setFont(Theme::font(10, QFont::Normal));
    {
        QPalette p = m_exportStatus->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_exportStatus->setPalette(p);
    }
    m_exportStatus->setWordWrap(true);
    av->addWidget(m_exportStatus);
    root->addWidget(actions);
    root->addSpacing(Theme::Space::S);

    QGridLayout *g = nullptr;
    int r = 0;

    root->addWidget(makeGroup(QStringLiteral("APPLICATION"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("Product"), QStringLiteral("app.product"));
    addRow(g, r, QStringLiteral("Version"), QStringLiteral("app.version"));
    addRow(g, r, QStringLiteral("Build"), QStringLiteral("app.build"));
    addRow(g, r, QStringLiteral("Qt version"), QStringLiteral("app.qt"));
    addRow(g, r, QStringLiteral("Uptime"), QStringLiteral("app.uptime"));

    root->addWidget(makeGroup(QStringLiteral("SYSTEM"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("OS"), QStringLiteral("sys.os"));
    addRow(g, r, QStringLiteral("Kernel"), QStringLiteral("sys.kernel"));
    addRow(g, r, QStringLiteral("Architecture"), QStringLiteral("sys.arch"));
    addRow(g, r, QStringLiteral("Desktop"), QStringLiteral("sys.desktop"));
    addRow(g, r, QStringLiteral("Session"), QStringLiteral("sys.session"));

    root->addWidget(makeGroup(QStringLiteral("TELEMETRY"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("State"), QStringLiteral("tel.state"));
    addRow(g, r, QStringLiteral("CPU temperature"), QStringLiteral("tel.cpu"));
    addRow(g, r, QStringLiteral("GPU temperature"), QStringLiteral("tel.gpu"));
    addRow(g, r, QStringLiteral("CPU fan"), QStringLiteral("tel.cpuFan"));
    addRow(g, r, QStringLiteral("GPU fan"), QStringLiteral("tel.gpuFan"));
    addRow(g, r, QStringLiteral("Refresh interval"), QStringLiteral("tel.refresh"));
    addRow(g, r, QStringLiteral("Snapshot"), QStringLiteral("tel.timestamp"));
    addRow(g, r, QStringLiteral("History"), QStringLiteral("tel.history"));

    root->addWidget(makeGroup(QStringLiteral("RUNTIME"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("Current page"), QStringLiteral("run.page"));
    addRow(g, r, QStringLiteral("System tray"), QStringLiteral("run.tray"));
    addRow(g, r, QStringLiteral("Single instance"), QStringLiteral("run.single"));
    addRow(g, r, QStringLiteral("Theme"), QStringLiteral("run.theme"));
    addRow(g, r, QStringLiteral("Temperature unit"), QStringLiteral("run.unit"));

    root->addWidget(makeGroup(QStringLiteral("SENSORS"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("hwmon device"), QStringLiteral("sen.device"));
    addRow(g, r, QStringLiteral("Backend"), QStringLiteral("sen.backend"));
    addRow(g, r, QStringLiteral("CPU"), QStringLiteral("sen.cpu"));
    addRow(g, r, QStringLiteral("GPU"), QStringLiteral("sen.gpu"));
    addRow(g, r, QStringLiteral("CPU fan"), QStringLiteral("sen.cpuFan"));
    addRow(g, r, QStringLiteral("GPU fan"), QStringLiteral("sen.gpuFan"));

    root->addWidget(makeGroup(QStringLiteral("SETTINGS"), g, this));
    r = 0;
    addRow(g, r, QStringLiteral("Theme"), QStringLiteral("set.theme"));
    addRow(g, r, QStringLiteral("Temperature unit"), QStringLiteral("set.unit"));
    addRow(g, r, QStringLiteral("Refresh interval"), QStringLiteral("set.refresh"));

    root->addStretch(1);

    scroll->setWidget(content);
    outer->addWidget(scroll);
}

void DiagnosticsPage::addRow(QGridLayout *grid, int &row, const QString &caption, const QString &key)
{
    QLabel *value = valueLabel();
    m_rows.insert(key, value);
    grid->addWidget(captionLabel(caption), row, 0, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(value, row, 1, Qt::AlignRight | Qt::AlignVCenter);
    ++row;
}

void DiagnosticsPage::setSnapshot(const DiagnosticsSnapshot &snapshot)
{
    m_snapshot = snapshot;
    const auto set = [this](const QString &key, const QString &text) {
        if (QLabel *l = m_rows.value(key))
            l->setText(text.isEmpty() ? kDash : text);
    };
    const DiagnosticsSnapshot &d = m_snapshot;

    set(QStringLiteral("app.product"), d.productName);
    set(QStringLiteral("app.version"), d.version);
    set(QStringLiteral("app.build"), d.buildType);
    set(QStringLiteral("app.qt"), d.qtVersion);
    set(QStringLiteral("app.uptime"), fmtUptime(d.uptimeSeconds));

    set(QStringLiteral("sys.os"), d.os);
    set(QStringLiteral("sys.kernel"), d.kernel);
    set(QStringLiteral("sys.arch"), d.architecture);
    set(QStringLiteral("sys.desktop"), d.desktopSession);
    set(QStringLiteral("sys.session"), d.sessionType);

    set(QStringLiteral("tel.state"), d.state);
    set(QStringLiteral("tel.cpu"), fmtTemp(d.cpuTempValid, d.cpuTempC));
    set(QStringLiteral("tel.gpu"), fmtTemp(d.gpuTempValid, d.gpuTempC));
    set(QStringLiteral("tel.cpuFan"), fmtRpm(d.cpuFanValid, d.cpuFanRpm));
    set(QStringLiteral("tel.gpuFan"), fmtRpm(d.gpuFanValid, d.gpuFanRpm));
    set(QStringLiteral("tel.refresh"), QStringLiteral("%1 s").arg(d.refreshMs / 1000));
    set(QStringLiteral("tel.timestamp"), d.snapshotTimestamp);
    set(QStringLiteral("tel.history"),
        QStringLiteral("%1 samples / %2 s").arg(d.history.size()).arg(d.historyWindowSeconds));

    set(QStringLiteral("run.page"), d.currentPage);
    set(QStringLiteral("run.tray"),
        !d.trayAvailable ? QStringLiteral("Unavailable")
                         : (d.trayVisible ? QStringLiteral("Available \u00B7 visible")
                                          : QStringLiteral("Available \u00B7 hidden")));
    set(QStringLiteral("run.single"),
        d.singleInstance ? QStringLiteral("Active (lock held)") : QStringLiteral("Unknown"));
    set(QStringLiteral("run.theme"), d.theme);
    set(QStringLiteral("run.unit"), d.temperatureUnit);

    set(QStringLiteral("sen.device"), d.sensorsAvailable ? d.hwmonPath : QStringLiteral("Unavailable"));
    set(QStringLiteral("sen.backend"), d.sensorsAvailable ? d.deviceName : QStringLiteral("Unavailable"));
    const auto chan = [](const QString &label, const QString &source) {
        return label.isEmpty() ? source : QStringLiteral("%1 \u00B7 %2").arg(label, source);
    };
    set(QStringLiteral("sen.cpu"), d.sensorsAvailable ? chan(d.cpuLabel, QStringLiteral("temp1_input")) : kDash);
    set(QStringLiteral("sen.gpu"), d.sensorsAvailable ? chan(d.gpuLabel, QStringLiteral("temp2_input")) : kDash);
    set(QStringLiteral("sen.cpuFan"), d.sensorsAvailable ? chan(d.cpuFanLabel, QStringLiteral("fan1_input")) : kDash);
    set(QStringLiteral("sen.gpuFan"), d.sensorsAvailable ? chan(d.gpuFanLabel, QStringLiteral("fan2_input")) : kDash);

    set(QStringLiteral("set.theme"), d.theme);
    set(QStringLiteral("set.unit"), d.temperatureUnit);
    set(QStringLiteral("set.refresh"), QStringLiteral("%1 s").arg(d.refreshMs / 1000));
}

void DiagnosticsPage::exportDiagnostics()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    const QString suggested = QDir(dir).filePath(diagnosticsFileName());

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Diagnostics"), suggested,
        QStringLiteral("JSON (*.json);;All files (*)"));
    if (path.isEmpty())
        return; // user cancelled

    QString error;
    if (saveDiagnostics(m_snapshot, path, &error)) {
        m_exportStatus->setText(QStringLiteral("Saved to %1").arg(path));
    } else {
        QMessageBox::warning(this, QStringLiteral("Export failed"),
                             QStringLiteral("Could not write the diagnostics file:\n%1").arg(error));
    }
}
