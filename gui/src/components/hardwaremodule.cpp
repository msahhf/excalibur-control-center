#include "hardwaremodule.h"

#include "animatednumber.h"
#include "fanstatus.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

namespace {
constexpr int kModulePadding = 18;
}

// Thin, band-colored thermal bar. Internal to the module (no separate file).
class HardwareModule::ThermalBar : public QWidget
{
public:
    explicit ThermalBar(QWidget *parent = nullptr) : QWidget(parent) { setFixedHeight(5); }

    void setValue(double celsius, bool valid, AppState::Band band)
    {
        m_celsius = celsius;
        m_valid = valid;
        m_band = band;
        update();
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

        if (!m_valid)
            return;

        const double frac = qBound(0.0, m_celsius / 100.0, 1.0);
        QRectF fill = r;
        fill.setWidth(r.width() * frac);
        if (fill.width() <= 0.0)
            return;
        p.setBrush(AppState::bandColor(m_band));
        p.drawRoundedRect(fill, r.height() / 2.0, r.height() / 2.0);
    }

private:
    double m_celsius = 0.0;
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
    m_spark->setFixedHeight(38);
    m_spark->setMinimumSpan(4.0);
    m_spark->setFillAlpha(60);
    v->addWidget(m_spark);
    v->addStretch(1);

    m_fan = new FanStatus;
    v->addWidget(m_fan);

    setTemperature(0.0, false, AppState::Band::Normal);
}

void HardwareModule::setTemperature(double celsius, bool valid, AppState::Band band)
{
    m_temp->setNumeric(celsius, valid);
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
    m_spark->setPoints(points);
}

void HardwareModule::setFan(int rpm, bool valid)
{
    m_fan->setFan(rpm, valid);
}

void HardwareModule::setFanCaption(const QString &caption)
{
    m_fan->setCaption(caption);
}
