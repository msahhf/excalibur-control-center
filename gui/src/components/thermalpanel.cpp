#include "thermalpanel.h"

#include "hardwaremodule.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QPainter>

#include <cmath>

ThermalPanel::ThermalPanel(QWidget *parent)
    : SurfacePanel(parent)
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_cpu = new HardwareModule(QStringLiteral("CPU"), this);
    m_gpu = new HardwareModule(QStringLiteral("GPU"), this);
    row->addWidget(m_cpu, 1);
    row->addWidget(m_gpu, 1);

    // A generous but bounded height: on taller windows the panel grows with the
    // space below the hero instead of leaving a blank area, but it never becomes
    // a giant empty card. The modules keep their internal hierarchy.
    setMinimumHeight(230);
    setMaximumHeight(380);
}

void ThermalPanel::paintOverlay(QPainter &p, const QRectF &r)
{
    // Subtle center divider (inset so it does not touch the rounded corners).
    QColor divider = Theme::border();
    divider.setAlpha(150);
    p.setPen(QPen(divider, 1));
    const qreal x = std::round(r.center().x()) + 0.5;
    p.drawLine(QPointF(x, r.top() + Theme::Space::XL), QPointF(x, r.bottom() - Theme::Space::XL));
}
