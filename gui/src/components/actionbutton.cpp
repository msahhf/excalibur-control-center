#include "actionbutton.h"

#include "theme/theme.h"

#include <QEnterEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>

namespace {

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    const int r = qRound(a.red() + (b.red() - a.red()) * t);
    const int g = qRound(a.green() + (b.green() - a.green()) * t);
    const int bl = qRound(a.blue() + (b.blue() - a.blue()) * t);
    const int al = qRound(a.alpha() + (b.alpha() - a.alpha()) * t);
    return QColor(r, g, bl, al);
}

} // namespace

ActionButton::ActionButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFont(Theme::font(11, QFont::Medium));
    setFlat(true);
    setAttribute(Qt::WA_Hover, true);

    m_anim = new QPropertyAnimation(this, "hoverProgress", this);
    m_anim->setDuration(130);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
}

void ActionButton::setHoverProgress(qreal value)
{
    m_hover = value;
    update();
}

QSize ActionButton::sizeHint() const
{
    const QFontMetrics fm(font());
    return QSize(fm.horizontalAdvance(text()) + 34, qMax(36, fm.height() + 18));
}

void ActionButton::animateTo(qreal target)
{
    m_anim->stop();
    m_anim->setStartValue(m_hover);
    m_anim->setEndValue(target);
    m_anim->start();
}

void ActionButton::enterEvent(QEnterEvent *) { animateTo(1.0); }
void ActionButton::leaveEvent(QEvent *) { animateTo(0.0); }

void ActionButton::focusInEvent(QFocusEvent *e)
{
    m_focusVisible = (e->reason() == Qt::TabFocusReason
                      || e->reason() == Qt::BacktabFocusReason
                      || e->reason() == Qt::ShortcutFocusReason);
    QPushButton::focusInEvent(e);
    update();
}

void ActionButton::focusOutEvent(QFocusEvent *e)
{
    m_focusVisible = false;
    QPushButton::focusOutEvent(e);
    update();
}

void ActionButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const QColor bg = mix(Theme::surfaceElevated(), Theme::surfaceHover(), m_hover);

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

    // Border brightens on hover; focus ring is the accent colour and thicker.
    QColor border = Theme::border();
    border.setAlpha(180);
    QPen pen(border, 1);
    if (m_focusVisible) {
        pen = QPen(Theme::accent(), 2);
    } else if (m_hover > 0.0) {
        pen = QPen(mix(border, Theme::textMuted(), m_hover), 1);
    }
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, Theme::RadiusSmall, Theme::RadiusSmall);

    p.setPen(Theme::textPrimary());
    p.setFont(font());
    p.drawText(rect(), Qt::AlignCenter, text());
}
