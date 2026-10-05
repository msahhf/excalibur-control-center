#include "thermalpanel.h"

#include "hardwaremodule.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>

#include <cmath>

ThermalPanel::ThermalPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_cpu = new HardwareModule(QStringLiteral("CPU"), this);
    m_gpu = new HardwareModule(QStringLiteral("GPU"), this);
    row->addWidget(m_cpu, 1);
    row->addWidget(m_gpu, 1);

    // Cap the panel height so tall windows do not open up large empty areas
    // inside the modules; leftover space stays outside the panel.
    setMaximumHeight(300);
}

void ThermalPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor border = Theme::border();
    border.setAlpha(170);

    p.setPen(QPen(border, 1));
    p.setBrush(Theme::surface());
    p.drawRoundedRect(r, Theme::Radius, Theme::Radius);

    // Subtle center divider (inset so it does not touch the rounded corners).
    QColor divider = Theme::border();
    divider.setAlpha(150);
    p.setPen(QPen(divider, 1));
    const qreal x = std::round(r.center().x()) + 0.5;
    p.drawLine(QPointF(x, r.top() + Theme::Space::XL), QPointF(x, r.bottom() - Theme::Space::XL));
}
