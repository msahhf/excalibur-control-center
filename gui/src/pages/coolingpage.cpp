#include "coolingpage.h"

#include "components/animatednumber.h"
#include "components/emptystate.h"
#include "components/fanstatus.h"
#include "components/sparkline.h"
#include "pageutils.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

// Local rounded surface for a cooling section.
class SectionSurface : public QWidget
{
public:
    explicit SectionSurface(QWidget *parent = nullptr) : QWidget(parent) {}

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

QVector<SparkPoint> series(const QVector<HistorySample> &h, bool cpu, bool temp)
{
    QVector<SparkPoint> pts;
    pts.reserve(h.size());
    for (const HistorySample &s : h) {
        SparkPoint p;
        if (cpu) {
            p.value = temp ? s.cpuTempC : static_cast<double>(s.cpuFanRpm);
            p.valid = temp ? s.cpuTempValid : s.cpuFanValid;
        } else {
            p.value = temp ? s.gpuTempC : static_cast<double>(s.gpuFanRpm);
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

    auto *panel = new SectionSurface;
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
        m_haveBands = false;
        m_stack->setCurrentWidget(m_empty);
        return;
    }
    m_stack->setCurrentWidget(m_content);

    const auto cpuTh = AppState::cpuThresholds();
    const auto gpuTh = AppState::gpuThresholds();
    if (!m_haveBands) {
        if (s.cpuTempValid)
            m_cpuBand = AppState::classify(s.cpuTempC, cpuTh);
        if (s.gpuTempValid)
            m_gpuBand = AppState::classify(s.gpuTempC, gpuTh);
        m_haveBands = true;
    } else {
        if (s.cpuTempValid)
            m_cpuBand = AppState::classifyHysteresis(s.cpuTempC, cpuTh, m_cpuBand);
        if (s.gpuTempValid)
            m_gpuBand = AppState::classifyHysteresis(s.gpuTempC, gpuTh, m_gpuBand);
    }

    const auto apply = [](Section &sec, double temp, bool tempValid, int fan, bool fanValid,
                          AppState::Band band, const QVector<HistorySample> &h, bool cpu) {
        sec.temp->setNumeric(temp, tempValid);
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
        sec.tempSpark->setPoints(series(h, cpu, true));
        sec.fanSpark->setPoints(series(h, cpu, false));
        sec.relation->setText(relationText(h, cpu));

        const int span = qBound(0, h.size(), TelemetryModel::kHistorySize);
        const QString ago = (span <= 1) ? QStringLiteral("now")
                                        : QStringLiteral("%1s ago").arg(span);
        sec.tempAgo->setText(ago);
        sec.fanAgo->setText(ago);
    };

    apply(m_cpu, s.cpuTempC, s.cpuTempValid, s.cpuFanRpm, s.cpuFanValid, m_cpuBand, history, true);
    apply(m_gpu, s.gpuTempC, s.gpuTempValid, s.gpuFanRpm, s.gpuFanValid, m_gpuBand, history, false);
}
