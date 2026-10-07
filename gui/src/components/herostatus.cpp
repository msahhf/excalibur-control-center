#include "herostatus.h"

#include "animatednumber.h"
#include "statuspill.h"
#include "theme/theme.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QVBoxLayout>

// A tiny colored state dot (band colour). Kept internal to the hero; it is a
// single, deliberate accent rather than another label.
class StateDot : public QWidget
{
public:
    explicit StateDot(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(8, 8); }

    void setColor(const QColor &c)
    {
        if (m_color == c)
            return;
        m_color = c;
        update();
    }

    QSize sizeHint() const override { return QSize(8, 8); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(m_color);
        p.drawEllipse(QRectF(rect()));
    }

private:
    QColor m_color = Theme::textMuted();
};

namespace {

// One inline headline temperature: a small band-coloured dot, a caption and a
// medium-weight value with its unit. Deliberately lighter than the CPU/GPU cards
// below so primary values there stay dominant.
struct Headline {
    QWidget *root = nullptr;
    StateDot *dot = nullptr;
    QLabel *caption = nullptr;
    AnimatedNumber *value = nullptr;
    QLabel *unit = nullptr;
};

Headline makeHeadline(const QString &caption)
{
    Headline h;
    h.root = new QWidget;
    auto *row = new QHBoxLayout(h.root);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(Theme::Space::S);

    h.dot = new StateDot;

    auto *text = new QWidget;
    auto *tv = new QVBoxLayout(text);
    tv->setContentsMargins(0, 0, 0, 0);
    tv->setSpacing(0);

    h.caption = new QLabel(caption);
    h.caption->setFont(Theme::labelFont(9, QFont::DemiBold, 140));
    {
        QPalette p = h.caption->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        h.caption->setPalette(p);
    }
    tv->addWidget(h.caption);

    auto *valueRow = new QHBoxLayout;
    valueRow->setContentsMargins(0, 0, 0, 0);
    valueRow->setSpacing(3);
    h.value = new AnimatedNumber;
    h.value->setFont(Theme::font(17, QFont::DemiBold));
    h.value->setInvalidText(QStringLiteral("\u2014"));
    {
        QPalette p = h.value->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        h.value->setPalette(p);
    }
    h.unit = new QLabel(QStringLiteral("\u00B0C"));
    h.unit->setFont(Theme::font(10, QFont::Medium));
    {
        QPalette p = h.unit->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        h.unit->setPalette(p);
    }
    valueRow->addWidget(h.value, 0, Qt::AlignBottom);
    valueRow->addWidget(h.unit, 0, Qt::AlignBottom);
    valueRow->addStretch(1);
    tv->addLayout(valueRow);

    row->addWidget(h.dot, 0, Qt::AlignTop);
    row->addWidget(text, 0, Qt::AlignTop);
    row->addStretch(1);
    return h;
}

} // namespace

HeroStatus::HeroStatus(QWidget *parent)
    : SurfacePanel(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    // Content lives in an inner widget so a state change can cross-fade the
    // text/pill while the panel surface and left state bar stay put.
    m_content = new QWidget;
    auto *cv = new QVBoxLayout(m_content);
    cv->setContentsMargins(Theme::Space::XL + Theme::Space::S, Theme::Space::L,
                           Theme::Space::XL, Theme::Space::L);
    cv->setSpacing(Theme::Space::S);
    v->addWidget(m_content);

    // Row 1: section label + cooling pill, so the pill no longer adds a row.
    m_label = new QLabel(QStringLiteral("SYSTEM STATUS"));
    m_label->setFont(Theme::labelFont(9, QFont::DemiBold, 155));
    {
        QPalette p = m_label->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_label->setPalette(p);
    }
    m_pill = new StatusPill;

    auto *row1 = new QHBoxLayout;
    row1->setContentsMargins(0, 0, 0, 0);
    row1->setSpacing(Theme::Space::M);
    row1->addWidget(m_label, 0, Qt::AlignVCenter);
    row1->addStretch(1);
    row1->addWidget(m_pill, 0, Qt::AlignVCenter);
    cv->addLayout(row1);

    // Row 2: the state headline + the two headline temperatures inline.
    m_title = new QLabel(QStringLiteral("System Normal"));
    m_title->setFont(Theme::font(22, QFont::Bold));
    {
        QPalette p = m_title->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        m_title->setPalette(p);
    }

    const Headline cpu = makeHeadline(QStringLiteral("CPU"));
    const Headline gpu = makeHeadline(QStringLiteral("GPU"));
    m_cpuDot = cpu.dot;
    m_gpuDot = gpu.dot;
    m_cpuCaption = cpu.caption;
    m_gpuCaption = gpu.caption;
    m_cpuTemp = cpu.value;
    m_gpuTemp = gpu.value;
    m_cpuUnit = cpu.unit;
    m_gpuUnit = gpu.unit;

    auto *headline = new QWidget;
    auto *hl = new QHBoxLayout(headline);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(Theme::Space::XXL);
    hl->addWidget(cpu.root);
    hl->addWidget(gpu.root);

    auto *row2 = new QHBoxLayout;
    row2->setContentsMargins(0, 0, 0, 0);
    row2->setSpacing(Theme::Space::L);
    row2->addWidget(m_title, 0, Qt::AlignBottom);
    row2->addStretch(1);
    row2->addWidget(headline, 0, Qt::AlignBottom);
    cv->addLayout(row2);

    // Row 3: plain-language detail.
    m_subtitle = new QLabel(QStringLiteral("Temperatures and cooling are operating normally."));
    m_subtitle->setFont(Theme::font(11, QFont::Normal));
    m_subtitle->setWordWrap(true);
    {
        QPalette p = m_subtitle->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        m_subtitle->setPalette(p);
    }
    cv->addWidget(m_subtitle);

    m_effect = new QGraphicsOpacityEffect(m_content);
    m_effect->setOpacity(1.0);
    m_content->setGraphicsEffect(m_effect);
    m_fade = new QPropertyAnimation(m_effect, "opacity", this);
    m_fade->setDuration(180);
    m_fade->setEasingCurve(QEasingCurve::OutCubic);
}

void HeroStatus::setSystemState(AppState::Status status, AppState::Band band)
{
    const bool changed = m_hasState && (status != m_status || band != m_band);
    m_status = status;
    m_band = band;
    m_hasState = true;

    switch (status) {
    case AppState::Status::Disconnected:
        m_title->setText(QStringLiteral("Telemetry Unavailable"));
        m_subtitle->setText(QStringLiteral(
            "The EXCALIBUR hardware interface is not currently available."));
        break;
    case AppState::Status::Degraded:
        m_title->setText(QStringLiteral("Limited Telemetry"));
        m_subtitle->setText(QStringLiteral(
            "Some sensor readings are currently unavailable."));
        break;
    case AppState::Status::Connected:
        switch (band) {
        case AppState::Band::Cool:
        case AppState::Band::Normal:
            m_title->setText(QStringLiteral("System Normal"));
            m_subtitle->setText(QStringLiteral(
                "Temperatures and cooling are operating normally."));
            break;
        case AppState::Band::Warm:
            m_title->setText(QStringLiteral("System Warm"));
            m_subtitle->setText(QStringLiteral(
                "The system is under increased thermal load."));
            break;
        case AppState::Band::High:
            m_title->setText(QStringLiteral("Thermal Load Elevated"));
            m_subtitle->setText(QStringLiteral(
                "Temperatures are high. Cooling is working to keep up."));
            break;
        }
        break;
    }

    if (changed) {
        m_fade->stop();
        m_fade->setStartValue(0.0);
        m_fade->setEndValue(1.0);
        m_fade->start();
    }
    update();
}

void HeroStatus::setCoolingState(AppState::CoolingState cooling)
{
    m_pill->setText(AppState::coolingStateLabel(cooling));
    switch (cooling) {
    case AppState::CoolingState::Unavailable:
        m_pill->setTone(StatusPill::Tone::Neutral);
        break;
    case AppState::CoolingState::Idle:
        m_pill->setTone(StatusPill::Tone::Info);
        break;
    case AppState::CoolingState::Active:
        m_pill->setTone(StatusPill::Tone::Success);
        break;
    case AppState::CoolingState::Elevated:
        m_pill->setTone(StatusPill::Tone::Warning);
        break;
    }
}

void HeroStatus::setSensorLabels(const QString &cpu, const QString &gpu)
{
    m_cpuCaption->setText(cpu);
    m_gpuCaption->setText(gpu);
}

void HeroStatus::setTemperatureUnit(Units::TemperatureUnit unit)
{
    m_tempUnit = unit;
    const QString sym = Units::symbol(unit);
    const int dec = Units::decimals(unit);
    m_cpuUnit->setText(sym);
    m_gpuUnit->setText(sym);
    m_cpuTemp->setDecimals(dec);
    m_gpuTemp->setDecimals(dec);
}

void HeroStatus::setHeadlineTemps(bool cpuValid, double cpuC, AppState::Band cpuBand,
                                  bool gpuValid, double gpuC, AppState::Band gpuBand)
{
    m_cpuTemp->setNumeric(Units::fromCelsius(cpuC, m_tempUnit), cpuValid);
    m_gpuTemp->setNumeric(Units::fromCelsius(gpuC, m_tempUnit), gpuValid);
    m_cpuUnit->setVisible(cpuValid);
    m_gpuUnit->setVisible(gpuValid);
    m_cpuDot->setColor(cpuValid ? AppState::bandColor(cpuBand) : Theme::textMuted());
    m_gpuDot->setColor(gpuValid ? AppState::bandColor(gpuBand) : Theme::textMuted());
}

void HeroStatus::paintOverlay(QPainter &p, const QRectF &r)
{
    // Left state bar: color encodes the current system state without shouting.
    QColor accent = Theme::textMuted();
    if (m_status == AppState::Status::Connected) {
        switch (m_band) {
        case AppState::Band::Warm: accent = Theme::warning(); break;
        case AppState::Band::High: accent = Theme::critical(); break;
        case AppState::Band::Cool:
        case AppState::Band::Normal: accent = Theme::success(); break;
        }
    } else if (m_status == AppState::Status::Degraded) {
        accent = Theme::warning();
    }
    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawRoundedRect(QRectF(r.left() + 0.0, r.top() + 1.0, 4.0, r.height() - 2.0),
                      Theme::Radius, Theme::Radius);
}
