#pragma once

#include <QRectF>
#include <QWidget>

class QPainter;

// The standard EXCALIBUR panel surface: a rounded surface with a subtle 1px
// border, drawn with QPainter (never a stylesheet) so it looks identical across
// light/dark and any host theme. Hero, thermal panel, cooling sections and device
// groups all used to repeat this drawing; they now share it. Subclasses add their
// own details (accent bar, divider) by overriding paintOverlay().
class SurfacePanel : public QWidget
{
    Q_OBJECT

public:
    explicit SurfacePanel(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

    // Draw details on top of the surface. `surface` is the border-aligned rect
    // already used for the rounded surface, so overlays line up exactly.
    virtual void paintOverlay(QPainter &p, const QRectF &surface);
};
