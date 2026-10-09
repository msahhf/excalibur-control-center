#include "coolingpage.h"

#include "app/usersettings.h"
#include "components/animatednumber.h"
#include "components/emptystate.h"
#include "components/fanstatus.h"
#include "components/sparkline.h"
#include "components/surfacepanel.h"
#include "pageutils.h"
#include "util/units.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QVector<SparkPoint> series(const QVector<HistorySample> &h, bool cpu, bool temp,
                           Units::TemperatureUnit unit)
{
    QVector<SparkPoint> pts;
    pts.reserve(h.size());
    for (const HistorySample &s : h) {
        SparkPoint p;
        p.timestampMs = s.timestampMs;
        if (cpu) {
            p.value = temp ? Units::fromCelsius(s.cpuTempC, unit) : static_cast<double>(s.cpuFanRpm);
            p.valid = temp ? s.cpuTempValid : s.cpuFanValid;
        } else {
            p.value = temp ? Units::fromCelsius(s.gpuTempC, unit) : static_cast<double>(s.gpuFanRpm);
            p.valid = temp ? s.gpuTempValid : s.gpuFanValid;
        }
        pts.append(p);
    }
    return pts;
}

// Conservative, plain-language read on how fan and temperature moved together
// over the visible window. Never over-claims what the telemetry supports.
QString relationText(const QVector<HistorySample> &h, bool cpu)
{
    bool haveT = false;
    bool haveF = false;
    double firstT = 0.0;
    double lastT = 0.0;
    double firstF = 0.0;
    double lastF = 0.0;
    int validT = 0;
    int validF = 0;

    for (const HistorySample &s : h) {
        const bool tv = cpu ? s.cpuTempValid : s.gpuTempValid;
        const bool fv = cpu ? s.cpuFanValid : s.gpuFanValid;
        if (tv) {
            const double t = cpu ? s.cpuTempC : s.gpuTempC;
            if (!haveT) {
                firstT = t;
                haveT = true;
            }
            lastT = t;
            ++validT;
        }
        if (fv) {
            const double f = cpu ? s.cpuFanRpm : s.gpuFanRpm;
            if (!haveF) {
                firstF = f;
                haveF = true;
            }
            lastF = f;
            ++validF;
        }
    }

    if (validT < 3)
        return QStringLiteral("Collecting 60-second history\u2026");
    if (!haveF || validF < 3)
        return QStringLiteral("Fan data unavailable");

    const double dt = lastT - firstT;
    const double df = lastF - firstF;

    if (df > 100.0 && dt > 1.5)
        return QStringLiteral("Fan rising with temperature");
    if (df > 100.0)
        return QStringLiteral("Fan active, temperature steady");
    if (df < -100.0 && dt < -1.5)
        return QStringLiteral("Fan easing as temperature falls");
    if (df < -100.0)
        return QStringLiteral("Fan easing, temperature steady");
    if (dt > 1.5)
        return QStringLiteral("Temperature rising, fan steady");
    if (dt < -1.5)
        return QStringLiteral("Temperature falling, fan steady");
    return QStringLiteral("Temperature and fan steady");
}

} // namespace

CoolingPage::CoolingPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = PageUtils::makeCenteredColumn(this);

    root->addWidget(PageUtils::pageTitle(QStringLiteral("Cooling")));
    root->addWidget(PageUtils::pageSubtitle(
        QStringLiteral("How the cooling system responds to thermal load.")));

    m_stack = new QStackedWidget;

    m_content = new QWidget;
    auto *v = new QVBoxLayout(m_content);
    v->setContentsMargins(0, Theme::Space::M, 0, 0);
    v->setSpacing(Theme::Space::L);

    m_cpu = makeSection(QStringLiteral("CPU"));
    m_gpu = makeSection(QStringLiteral("GPU"));
    v->addWidget(m_cpu.panel);
    v->addWidget(m_gpu.panel);
    v->addStretch(1);

    m_empty = new EmptyState;
    m_empty->setMessage(
        QStringLiteral("Cooling data unavailable"),
        QStringLiteral("The EXCALIBUR hardware interface is not currently available. "
                       "History appears once the excalibur_wmi driver is loaded."));

    m_stack->addWidget(m_content);
    m_stack->addWidget(m_empty);
    root->addWidget(m_stack, 1);
}

CoolingPage::Section CoolingPage::makeSection(const QString &identity)
{
    Section s;

    auto *panel = new SurfacePanel;
    s.panel = panel;

    auto *v = new QVBoxLayout(panel);
    v->setContentsMargins(Theme::Space::L + 2, Theme::Space::L, Theme::Space::L + 2, Theme::Space::L);
    v->setSpacing(0);

    // Header: identity + fan RPM.
    s.identity = new QLabel(identity);
    s.identity->setFont(Theme::font(12, QFont::Bold, 108));
    {
        QPalette p = s.identity->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        s.identity->setPalette(p);
    }
    s.fan = new FanStatus;

    auto *header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(Theme::Space::S);
    header->addWidget(s.identity, 0, Qt::AlignVCenter);
    header->addStretch(1);
    header->addWidget(s.fan, 0, Qt::AlignVCenter);
    v->addLayout(header);
    v->addSpacing(Theme::Space::S);

    // Current temperature + band label.
    s.temp = new AnimatedNumber;
    s.temp->setFont(Theme::font(30, QFont::DemiBold));
    s.temp->setInvalidText(QStringLiteral("\u2014"));
    {
        QPalette p = s.temp->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        s.temp->setPalette(p);
    }
    s.unit = new QLabel(QStringLiteral("\u00B0C"));
    s.unit->setFont(Theme::font(12, QFont::Medium));
    {
        QPalette p = s.unit->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        s.unit->setPalette(p);
    }
    s.band = new QLabel(QStringLiteral("\u2014"));
    s.band->setFont(Theme::font(11, QFont::DemiBold));

    auto *tempRow = new QHBoxLayout;
    tempRow->setContentsMargins(0, 0, 0, 0);
    tempRow->setSpacing(Theme::Space::S);
    tempRow->addWidget(s.temp, 0, Qt::AlignBottom);
    tempRow->addWidget(s.unit, 0, Qt::AlignBottom);
    tempRow->addSpacing(Theme::Space::S);
    tempRow->addWidget(s.band, 0, Qt::AlignBottom);
    tempRow->addStretch(1);
    v->addLayout(tempRow);
    v->addSpacing(Theme::Space::M);

    const auto makeSparkBlock = [&](const QString &caption, Sparkline *&out, QLabel *&agoOut) {
        auto *block = new QVBoxLayout;
        block->setContentsMargins(0, 0, 0, 0);
        block->setSpacing(2);

        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(Theme::Space::M);
        auto *l = new QLabel(caption);
        l->setFont(Theme::labelFont(9, QFont::Medium, 120));
        l->setFixedWidth(92);
        l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        {
            QPalette p = l->palette();
            p.setColor(QPalette::WindowText, Theme::textMuted());
            l->setPalette(p);
        }
        out = new Sparkline;
        out->setFixedHeight(34);
        row->addWidget(l, 0);
        row->addWidget(out, 1);
        block->addLayout(row);

        // Quiet time labels: oldest sample at the left, "now" at the right.
        auto *timeRow = new QHBoxLayout;
        timeRow->setContentsMargins(92 + Theme::Space::M, 0, 0, 0);
        timeRow->setSpacing(0);
        agoOut = new QLabel;
        agoOut->setFont(Theme::font(9, QFont::Normal));
        auto *now = new QLabel(QStringLiteral("now"));
        now->setFont(Theme::font(9, QFont::Normal));
        for (QLabel *tl : {agoOut, now}) {
            QPalette p = tl->palette();
            p.setColor(QPalette::WindowText, Theme::textMuted());
            tl->setPalette(p);
        }
        timeRow->addWidget(agoOut, 0, Qt::AlignLeft);
        timeRow->addStretch(1);
        timeRow->addWidget(now, 0, Qt::AlignRight);
        block->addLayout(timeRow);
        return block;
    };

    v->addLayout(makeSparkBlock(QStringLiteral("Temperature"), s.tempSpark, s.tempAgo));
    v->addSpacing(Theme::Space::S);
    v->addLayout(makeSparkBlock(QStringLiteral("Fan speed"), s.fanSpark, s.fanAgo));
    v->addSpacing(Theme::Space::S);

    s.tempSpark->setFillAlpha(55);
    s.tempSpark->setMinimumSpan(6.0);
    s.tempSpark->setValueSuffix(QStringLiteral(" \u00B0C"));
    s.fanSpark->setLineColor(Theme::cool());
    s.fanSpark->setFillAlpha(40);
    s.fanSpark->setMinimumSpan(400.0);
    s.fanSpark->setValueSuffix(QStringLiteral(" RPM"));

    s.relation = new QLabel;
    s.relation->setFont(Theme::font(10, QFont::Medium));
    {
        QPalette p = s.relation->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        s.relation->setPalette(p);
    }
    v->addWidget(s.relation);

    return s;
}

void CoolingPage::setTelemetry(const TelemetrySnapshot &s, const QVector<HistorySample> &history)
{
    if (s.status == AppState::Status::Disconnected) {
        m_bands.reset();
        m_stack->setCurrentWidget(m_empty);
        return;
    }
    m_stack->setCurrentWidget(m_content);

    m_bands.update(s.cpuTempValid, s.cpuTempC, s.gpuTempValid, s.gpuTempC);

    m_cpu.identity->setText(s.cpuLabel);
    m_gpu.identity->setText(s.gpuLabel);

    const auto apply = [](Section &sec, double temp, bool tempValid, int fan, bool fanValid,
                          AppState::Band band, const QVector<HistorySample> &h, bool cpu,
                          Units::TemperatureUnit unit) {
        const int dec = Units::decimals(unit);
        const QString sym = Units::symbol(unit);
        sec.temp->setDecimals(dec);
        sec.unit->setText(sym);
        sec.temp->setNumeric(Units::fromCelsius(temp, unit), tempValid);
        if (tempValid) {
            sec.band->setText(AppState::bandLabel(band));
            QPalette p = sec.band->palette();
            p.setColor(QPalette::WindowText, AppState::bandColor(band));
            sec.band->setPalette(p);
            sec.tempSpark->setLineColor(AppState::bandColor(band));
        } else {
            sec.band->setText(QStringLiteral("\u2014"));
            QPalette p = sec.band->palette();
            p.setColor(QPalette::WindowText, Theme::textMuted());
            sec.band->setPalette(p);
            sec.tempSpark->setLineColor(Theme::textMuted());
        }
        sec.fan->setFan(fan, fanValid);
        sec.tempSpark->setDecimals(dec);
        sec.tempSpark->setValueSuffix(QStringLiteral(" ") + sym);
        sec.tempSpark->setMinimumSpan(6.0 * Units::spanFactor(unit));
        sec.tempSpark->setPoints(series(h, cpu, true, unit));
        sec.fanSpark->setPoints(series(h, cpu, false, unit));
        sec.relation->setText(relationText(h, cpu));

        const qint64 spanMs = (h.size() >= 2)
                                  ? (h.last().timestampMs - h.first().timestampMs)
                                  : 0;
        const int spanSec = int(spanMs / 1000);
        const QString ago = (spanSec <= 0) ? QStringLiteral("now")
                                           : QStringLiteral("%1s ago").arg(spanSec);
        sec.tempAgo->setText(ago);
        sec.fanAgo->setText(ago);
    };

    const Units::TemperatureUnit unit = UserSettings::instance().tempUnit();
    apply(m_cpu, s.cpuTempC, s.cpuTempValid, s.cpuFanRpm, s.cpuFanValid, m_bands.cpu, history, true, unit);
    apply(m_gpu, s.gpuTempC, s.gpuTempValid, s.gpuFanRpm, s.gpuFanValid, m_bands.gpu, history, false, unit);
}
