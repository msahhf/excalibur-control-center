#pragma once

#include <QWidget>

#include "app/appstate.h"
#include "app/telemetrymodel.h"

class HeroStatus;
class HardwareModule;
class EmptyState;
class QStackedWidget;
class QVBoxLayout;

// Hero page. Reads a TelemetrySnapshot (never sysfs) and renders: system state
// first, then CPU/GPU hardware modules. Falls back to a designed unavailable
// screen when the driver is absent.
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

public slots:
    void setSnapshot(const TelemetrySnapshot &snapshot);

private:
    QVBoxLayout *m_body = nullptr;

    HeroStatus *m_hero = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_content = nullptr;
    HardwareModule *m_cpu = nullptr;
    HardwareModule *m_gpu = nullptr;
    EmptyState *m_empty = nullptr;

    // Hysteresis state (Section 6.1).
    bool m_haveBands = false;
    AppState::Band m_cpuBand = AppState::Band::Normal;
    AppState::Band m_gpuBand = AppState::Band::Normal;
};
