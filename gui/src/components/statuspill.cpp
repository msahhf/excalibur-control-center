#include "statuspill.h"

#include "theme/theme.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>

StatusPill::StatusPill(QWidget *parent)
    : QWidget(parent)
{
    setFont(Theme::font(10, QFont::Medium));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void StatusPill::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    updateGeometry();
    update();
}

void StatusPill::setTone(Tone tone)
{
    if (m_tone == tone)
        return;
    m_tone = tone;
    update();
}

QColor StatusPill::toneColor() const
{
    switch (m_tone) {
    case Tone::Success: return Theme::success();
    case Tone::Warning: return Theme::warning();
    case Tone::Critical: return Theme::critical();
    case Tone::Info: return Theme::cool();
    case Tone::Neutral: break;
    }
    return Theme::textMuted();
}

QSize StatusPill::sizeHint() const
{
    const QFontMetrics fm(font());
    const int w = fm.horizontalAdvance(m_text) + 42; // dot + paint insets + padding
    return QSize(w, 28);
}

void StatusPill::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(Qt::NoPen);
    QColor bg = Theme::surfaceElevated();
    p.setBrush(bg);
    p.drawRoundedRect(r, r.height() / 2.0, r.height() / 2.0);

    const qreal cy = r.center().y();
    const qreal dotR = 3.5;
    p.setBrush(toneColor());
    p.drawEllipse(QPointF(r.left() + 13.0, cy), dotR, dotR);

    p.setPen(Theme::textSecondary());
    p.setFont(font());
    p.drawText(r.adjusted(25.0, 0.0, -12.0, 0.0), Qt::AlignLeft | Qt::AlignVCenter, m_text);
}
