#include "hardwaremodule.h"

#include "animatednumber.h"
#include "fanstatus.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QVariantAnimation>

namespace {
constexpr int kModulePadding = 18;
}

// Thin, band-colored thermal bar. Internal to the module (no separate file).
// The fill animates smoothly (no hard jump every second).
class HardwareModule::ThermalBar : public QWidget
{
public:
    explicit ThermalBar(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(5);
        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(380);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_frac = v.toReal();
            update();
        });
    }

    void setValue(double celsius, bool valid, AppState::Band band)
    {
        m_valid = valid;
        m_band = band;

        const double target = valid ? qBound(0.0, celsius / 100.0, 1.0) : 0.0;
        if (!m_has) { // first sample: no animation
            m_has = true;
            m_frac = target;
            update();
            return;
        }
        if (qFuzzyCompare(m_frac, target))
            return;
        m_anim->stop();
        m_anim->setStartValue(m_frac);
        m_anim->setEndValue(target);
        m_anim->start();
    }

    QSize sizeHint() const override { return QSize(180, 5); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = QRectF(rect()).adjusted(0.0, 0.5, 0.0, -0.5);
        QColor track = Theme::border();
        track.setAlpha(150);

        p.setPen(Qt::NoPen);
        p.setBrush(track);
        p.drawRoundedRect(r, r.height() / 2.0, r.height() / 2.0);

        if (!m_valid && m_frac <= 0.0)
            return;

        QRectF fill = r;
        fill.setWidth(r.width() * qBound(0.0, m_frac, 1.0));
        if (fill.width() <= 0.0)
            return;
        p.setBrush(AppState::bandColor(m_band));
        p.drawRoundedRect(fill, r.height() / 2.0, r.height() / 2.0);
    }

private:
    QVariantAnimation *m_anim = nullptr;
    double m_frac = 0.0;
    bool m_has = false;
    bool m_valid = false;
    AppState::Band m_band = AppState::Band::Normal;
};

HardwareModule::HardwareModule(const QString &identity, QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(196);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(kModulePadding, kModulePadding, kModulePadding, kModulePadding);
    v->setSpacing(0);

    m_identity = new QLabel(identity);
    m_identity->setFont(Theme::font(12, QFont::Bold, 108));
    {
        QPalette p = m_identity->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        m_identity->setPalette(p);
    }
    v->addWidget(m_identity);
    v->addSpacing(Theme::Space::S);

    // Big temperature + unit, baseline aligned. Temperature dominates.
    m_temp = new AnimatedNumber;
    m_temp->setFont(Theme::font(40, QFont::DemiBold));
    m_temp->setInvalidText(QStringLiteral("\u2014"));
    {
        QPalette p = m_temp->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        m_temp->setPalette(p);
    }

    m_unit = new QLabel(QStringLiteral("\u00B0C"));
    m_unit->setFont(Theme::font(13, QFont::Medium));
    {
        QPalette p = m_unit->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        m_unit->setPalette(p);
    }

    auto *tempRow = new QHBoxLayout;
    tempRow->setContentsMargins(0, 0, 0, 0);
    tempRow->setSpacing(Theme::Space::S);
    tempRow->addWidget(m_temp, 0, Qt::AlignBottom);
    tempRow->addWidget(m_unit, 0, Qt::AlignBottom);
    tempRow->addStretch(1);
    v->addLayout(tempRow);
    v->addSpacing(Theme::Space::XS);

    m_band = new QLabel;
    m_band->setFont(Theme::font(11, QFont::DemiBold));
    v->addWidget(m_band);
    v->addSpacing(Theme::Space::S);

    m_bar = new ThermalBar;
    v->addWidget(m_bar);
    v->addSpacing(Theme::Space::M);

    m_spark = new Sparkline;
    // Bounded growth: on taller windows the history chart (not empty space) uses
    // the extra height, so the panel fills without a large internal gap.
    m_spark->setMinimumHeight(38);
    m_spark->setMaximumHeight(96);
    m_spark->setMinimumSpan(4.0);
    m_spark->setFillAlpha(60);
    v->addWidget(m_spark, 1);
    v->addStretch(1);

    m_fan = new FanStatus;
    m_range = new QLabel;
    m_range->setFont(Theme::font(9, QFont::Normal));
    m_range->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    {
        QPalette p = m_range->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_range->setPalette(p);
    }
    m_range->setVisible(false);

    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, 0, 0, 0);
    footer->setSpacing(Theme::Space::S);
    footer->addWidget(m_fan, 0, Qt::AlignVCenter);
    footer->addStretch(1);
    footer->addWidget(m_range, 0, Qt::AlignVCenter);
    v->addLayout(footer);

    setTemperature(0.0, false, AppState::Band::Normal);
    setTemperatureUnit(Units::TemperatureUnit::Celsius);
}

void HardwareModule::setIdentity(const QString &identity)
{
    m_identity->setText(identity);
}

void HardwareModule::setTemperatureUnit(Units::TemperatureUnit unit)
{
    m_tempUnit = unit;
    const QString sym = Units::symbol(unit);
    const int dec = Units::decimals(unit);
    m_unit->setText(sym);
    m_temp->setDecimals(dec);
    m_spark->setDecimals(dec);
    m_spark->setValueSuffix(QStringLiteral(" ") + sym);
    m_spark->setMinimumSpan(4.0 * Units::spanFactor(unit));
    // Reformat the secondary range in the new unit.
    setTemperatureRange(m_rangeValid, m_rMin, m_rMax);
}

void HardwareModule::setTemperature(double celsius, bool valid, AppState::Band band)
{
    // The bar keeps the raw Celsius scale (0..100 °C); only the label converts.
    m_temp->setNumeric(Units::fromCelsius(celsius, m_tempUnit), valid);
    if (valid) {
        m_band->setText(AppState::bandLabel(band));
        QPalette p = m_band->palette();
        p.setColor(QPalette::WindowText, AppState::bandColor(band));
        m_band->setPalette(p);
        m_spark->setLineColor(AppState::bandColor(band));
    } else {
        m_band->setText(QStringLiteral("\u2014"));
        QPalette p = m_band->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_band->setPalette(p);
        m_spark->setLineColor(Theme::textMuted());
    }
    m_bar->setValue(celsius, valid, band);
}

void HardwareModule::setTemperatureHistory(const QVector<SparkPoint> &points)
{
    QVector<SparkPoint> converted = points;
    for (SparkPoint &p : converted)
        p.value = Units::fromCelsius(p.value, m_tempUnit);
    m_spark->setPoints(converted);
}

void HardwareModule::setTemperatureRange(bool valid, double minValue, double maxValue)
{
    m_rangeValid = valid;
    m_rMin = minValue;
    m_rMax = maxValue;
    m_range->setVisible(valid);
    if (!valid)
        return;
    const int dec = Units::decimals(m_tempUnit);
    const QString sym = Units::symbol(m_tempUnit);
    const QString lo = QString::number(Units::fromCelsius(minValue, m_tempUnit), 'f', dec);
    const QString hi = QString::number(Units::fromCelsius(maxValue, m_tempUnit), 'f', dec);
    if (lo == hi)
        m_range->setText(QStringLiteral("60s %1 %2").arg(lo, sym));
    else
        m_range->setText(QStringLiteral("60s %1\u2013%2 %3").arg(lo, hi, sym));
}

void HardwareModule::setFan(int rpm, bool valid)
{
    m_fan->setFan(rpm, valid);
}
