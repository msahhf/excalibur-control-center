#pragma once

#include "components/surfacepanel.h"

class HardwareModule;
class QPainter;

// A single wide surface holding the CPU and GPU modules, separated by a subtle
// vertical divider. This reads as one instrument rather than two floating cards.
class ThermalPanel : public SurfacePanel
{
    Q_OBJECT

public:
    explicit ThermalPanel(QWidget *parent = nullptr);

    HardwareModule *cpu() const { return m_cpu; }
    HardwareModule *gpu() const { return m_gpu; }

protected:
    void paintOverlay(QPainter &p, const QRectF &surface) override;

private:
    HardwareModule *m_cpu = nullptr;
    HardwareModule *m_gpu = nullptr;
};
