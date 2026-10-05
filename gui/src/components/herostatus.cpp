#include "herostatus.h"

#include "statuspill.h"
#include "theme/theme.h"

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QVBoxLayout>

HeroStatus::HeroStatus(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    // Content lives in an inner widget so a state change can cross-fade the
    // text/pill while the panel surface and left state bar stay put.
    m_content = new QWidget;
    auto *cv = new QVBoxLayout(m_content);
    cv->setContentsMargins(Theme::Space::XL + Theme::Space::S, Theme::Space::XL,
                           Theme::Space::XL, Theme::Space::XL);
    cv->setSpacing(0);
    v->addWidget(m_content);

    m_label = new QLabel(QStringLiteral("SYSTEM STATUS"));
    m_label->setFont(Theme::labelFont(9, QFont::DemiBold, 155));
    {
        QPalette p = m_label->palette();
        p.setColor(QPalette::WindowText, Theme::textMuted());
        m_label->setPalette(p);
    }
    cv->addWidget(m_label);
    cv->addSpacing(Theme::Space::S);

    m_title = new QLabel(QStringLiteral("System Normal"));
    m_title->setFont(Theme::font(25, QFont::Bold));
    {
        QPalette p = m_title->palette();
        p.setColor(QPalette::WindowText, Theme::textPrimary());
        m_title->setPalette(p);
    }
    cv->addWidget(m_title);
    cv->addSpacing(Theme::Space::S);

    m_subtitle = new QLabel(QStringLiteral("Temperatures and cooling are operating normally."));
    m_subtitle->setFont(Theme::font(11, QFont::Normal));
    m_subtitle->setWordWrap(true);
    {
        QPalette p = m_subtitle->palette();
        p.setColor(QPalette::WindowText, Theme::textSecondary());
        m_subtitle->setPalette(p);
    }
    cv->addWidget(m_subtitle);
    cv->addSpacing(Theme::Space::L);

    m_pill = new StatusPill;
    cv->addWidget(m_pill, 0, Qt::AlignLeft);

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
        m_pill->setText(QStringLiteral("Cooling system offline"));
        m_pill->setTone(StatusPill::Tone::Neutral);
        break;
    case AppState::Status::Degraded:
        m_title->setText(QStringLiteral("Limited Telemetry"));
        m_subtitle->setText(QStringLiteral(
            "Some sensor readings are currently unavailable."));
        m_pill->setText(QStringLiteral("Partial cooling data"));
        m_pill->setTone(StatusPill::Tone::Warning);
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
            m_title->setText(QStringLiteral("System Hot"));
            m_subtitle->setText(QStringLiteral(
                "Thermal load is currently elevated."));
            break;
        }
        m_pill->setText(QStringLiteral("Cooling system active"));
        m_pill->setTone(StatusPill::Tone::Success);
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

void HeroStatus::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor border = Theme::border();
    border.setAlpha(170);

    p.setPen(QPen(border, 1));
    p.setBrush(Theme::surface());
    p.drawRoundedRect(r, Theme::Radius, Theme::Radius);

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
