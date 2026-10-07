#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "appstate.h"
#include "hwmonclient.h"

class QTimer;

// A single, complete view of the system at one instant. A missing value is
// represented with an explicit `...Valid` flag (never a fake 0 shown as real).
struct TelemetrySnapshot {
    bool deviceFound = false;
    AppState::Status status = AppState::Status::Disconnected;

    bool cpuTempValid = false;
    bool gpuTempValid = false;
    bool cpuFanValid = false;
    bool gpuFanValid = false;

    double cpuTempC = 0.0;
    double gpuTempC = 0.0;
    int cpuFanRpm = 0;
    int gpuFanRpm = 0;

    QString driverName; // e.g. "excalibur_wmi" when present
    QString hwmonPath;  // technical path, shown only on the Device page

    // Sanitised sensor labels from the hwmon backend (fallback CPU/GPU/…).
    QString cpuLabel;
    QString gpuLabel;
    QString cpuFanLabel;
    QString gpuFanLabel;
};

// One history tick (1 sample/second). Kept in memory only; session only.
struct HistorySample {
    qint64 timestampMs = 0; // monotonic ms (QElapsedTimer reference) when sampled
    bool cpuTempValid = false;
    bool gpuTempValid = false;
    bool cpuFanValid = false;
    bool gpuFanValid = false;
    double cpuTempC = 0.0;
    double gpuTempC = 0.0;
    int cpuFanRpm = 0;
    int gpuFanRpm = 0;
};

// Owns telemetry polling. Wraps the unchanged HwmonClient, discovers by name on
// every tick (so it adapts to load/unload and to a changed hwmon index) and keeps
// a bounded 60-sample history. This is the ONLY object that reads sysfs.
class TelemetryModel : public QObject
{
    Q_OBJECT

public:
    static constexpr int kHistorySize = 60; // 60 samples at 1 Hz = 60 s

    explicit TelemetryModel(QObject *parent = nullptr);

    void start(int intervalMs = 1000);
    void pollNow();
    // Change the polling interval without recreating the timer or forcing an
    // immediate read (used by the refresh-interval setting).
    void setInterval(int intervalMs);

    const TelemetrySnapshot &snapshot() const { return m_snapshot; }
    const QVector<HistorySample> &history() const { return m_history; }

signals:
    void updated(const TelemetrySnapshot &snapshot, const QVector<HistorySample> &history);

private:
    void poll();

    HwmonClient m_client;
    TelemetrySnapshot m_snapshot;
    QVector<HistorySample> m_history;
    QTimer *m_timer = nullptr;
    bool m_hasEmitted = false;
};
