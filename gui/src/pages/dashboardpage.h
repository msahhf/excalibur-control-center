#pragma once

#include <QVector>
#include <QWidget>

#include "app/appstate.h"
#include "app/telemetrymodel.h"

class HeroStatus;
class ThermalPanel;
class HardwareModule;
class EmptyState;
class QStackedWidget;
class QVBoxLayout;

// Hero page. Reads a TelemetrySnapshot + history (never sysfs) and renders:
// system state first, then a single thermal panel holding the CPU/GPU modules.
// Falls back to a designed unavailable screen when the driver is absent.
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

public slots:
    void setSnapshot(const TelemetrySnapshot &snapshot, const QVector<HistorySample> &history);

private:
    QVBoxLayout *m_body = nullptr;

    HeroStatus *m_hero = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_content = nullptr;
    ThermalPanel *m_panel = nullptr;
    HardwareModule *m_cpu = nullptr;
    HardwareModule *m_gpu = nullptr;
    EmptyState *m_empty = nullptr;

    // Hysteresis state (Section 6.1).
    bool m_haveBands = false;
    AppState::Band m_cpuBand = AppState::Band::Normal;
    AppState::Band m_gpuBand = AppState::Band::Normal;
};
