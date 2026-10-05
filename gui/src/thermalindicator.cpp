#include "thermalindicator.h"

#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

ThermalIndicator::ThermalIndicator(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void ThermalIndicator::setValue(double celsius, bool valid)
{
    if (m_celsius == celsius && m_valid == valid)
        return;
    m_celsius = celsius;
    m_valid = valid;
    update();
}

QSize ThermalIndicator::sizeHint() const
{
    return QSize(220, 22);
}

void ThermalIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int pad = 8;
    const int trackH = 4;
    const int cy = height() / 2;
    const QRectF track(pad, cy - trackH / 2.0, width() - 2.0 * pad, trackH);

    const QColor mid = Theme::border(this);
    QColor trackColor = mid;
    trackColor.setAlpha(140);

    // Tick marks (every 20 °C) — low contrast.
    QColor tick = mid;
    tick.setAlpha(90);
    p.setPen(QPen(tick, 1));
    for (int t = m_min; t <= m_max; t += 20) {
        const qreal x = track.left() + track.width() * (t - m_min) / double(m_max - m_min);
        p.drawLine(QPointF(x, cy - 6), QPointF(x, cy + 6));
    }

    // Track.
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(track, trackH / 2.0, trackH / 2.0);

    if (!m_valid)
        return;

    const qreal clamped = qBound<qreal>(m_min, m_celsius, m_max);
    const qreal frac = (clamped - m_min) / double(m_max - m_min);
    const qreal markerX = track.left() + track.width() * frac;

    // Semantic accent only when genuinely high.
    QColor accent(this->palette().color(QPalette::WindowText));
    if (m_celsius >= 90.0)
        accent = QColor(Theme::Error);
    else if (m_celsius >= 80.0)
        accent = QColor(Theme::Warn);

    // Filled segment up to the current value (subtle).
    QColor fill = accent;
    fill.setAlpha(150);
    QRectF filled(track.left(), track.top(), markerX - track.left(), track.height());
    if (filled.width() > 0.0) {
        p.setBrush(fill);
        p.drawRoundedRect(filled, trackH / 2.0, trackH / 2.0);
    }

    // Current-position indicator.
    p.setBrush(accent);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(markerX, cy), 5.0, 5.0);
}
