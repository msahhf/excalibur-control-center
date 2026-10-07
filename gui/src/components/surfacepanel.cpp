#include "surfacepanel.h"

#include "theme/theme.h"

#include <QPainter>
#include <QPaintEvent>

SurfacePanel::SurfacePanel(QWidget *parent)
    : QWidget(parent)
{
}

void SurfacePanel::paintOverlay(QPainter &, const QRectF &)
{
}

void SurfacePanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor border = Theme::border();
    border.setAlpha(170);

    p.setPen(QPen(border, 1));
    p.setBrush(Theme::surface());
    p.drawRoundedRect(r, Theme::Radius, Theme::Radius);

    paintOverlay(p, r);
}
