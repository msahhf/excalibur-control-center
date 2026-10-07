#pragma once

#include <QVector>
#include <QWidget>

#include "app/appstate.h"
#include "app/telemetrymodel.h"

class AnimatedNumber;
class EmptyState;
class FanStatus;
class QLabel;
class Sparkline;
class QStackedWidget;

// Cooling page: current temperature + fan for CPU and GPU, plus 60 s history
// sparklines and a conservative plain-language read on how fan and temperature
// moved together. Reads snapshots/history only (never sysfs).
class CoolingPage : public QWidget
{
    Q_OBJECT

public:
    explicit CoolingPage(QWidget *parent = nullptr);

public slots:
    void setTelemetry(const TelemetrySnapshot &snapshot, const QVector<HistorySample> &history);

private:
    struct Section {
        QWidget *panel = nullptr;
        QLabel *identity = nullptr;
        AnimatedNumber *temp = nullptr;
        QLabel *unit = nullptr;
        QLabel *band = nullptr;
        FanStatus *fan = nullptr;
        Sparkline *tempSpark = nullptr;
        QLabel *tempAgo = nullptr;
        Sparkline *fanSpark = nullptr;
        QLabel *fanAgo = nullptr;
        QLabel *relation = nullptr;
    };

    Section makeSection(const QString &identity);

    Section m_cpu;
    Section m_gpu;

    QStackedWidget *m_stack = nullptr;
    QWidget *m_content = nullptr;
    EmptyState *m_empty = nullptr;

    // Hysteresis-aware CPU/GPU band tracking (shared helper, Section 15).
    AppState::BandTracker m_bands;
};
