#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "app/appstate.h"
#include "app/systeminfo.h"
#include "app/telemetrymodel.h"

// Runtime facts that only the window knows (page, tray, single-instance).
struct DiagnosticsContext {
    QString currentPage;
    bool trayAvailable = false;
    bool trayVisible = false;
    bool singleInstance = true;
};

struct DiagnosticsHistorySample {
    QString timestamp; // ISO-8601 (wall clock, derived from the sample window)
    bool cpuTempValid = false;
    double cpuTempC = 0.0;
    bool gpuTempValid = false;
    double gpuTempC = 0.0;
    bool cpuFanValid = false;
    int cpuFanRpm = 0;
    bool gpuFanValid = false;
    int gpuFanRpm = 0;
};

// One consistent diagnostics snapshot: built once from the model + settings +
// system info, then rendered in the UI and serialised to JSON. Telemetry values
// are canonical Celsius; the JSON uses explicit *_c field names, and the user's
// preference is recorded separately under settings/runtime.
struct DiagnosticsSnapshot {
    // application
    QString productName;
    QString version;
    QString buildType;
    QString qtVersion;
    qint64 uptimeSeconds = 0;

    // system
    QString os;
    QString kernel;
    QString architecture;
    QString desktopSession;
    QString sessionType;

    // telemetry
    QString state; // Connected / Degraded / Unavailable
    bool cpuTempValid = false;
    double cpuTempC = 0.0;
    bool gpuTempValid = false;
    double gpuTempC = 0.0;
    bool cpuFanValid = false;
    int cpuFanRpm = 0;
    bool gpuFanValid = false;
    int gpuFanRpm = 0;
    int refreshMs = 0;
    QString snapshotTimestamp;
    int historyWindowSeconds = 60;

    // runtime
    QString currentPage;
    bool trayAvailable = false;
    bool trayVisible = false;
    bool singleInstance = true;

    // settings (user preference)
    QString theme;
    QString temperatureUnit;

    // sensors (read-only backend)
    bool sensorsAvailable = false;
    QString hwmonPath;
    QString deviceName;
    QString cpuLabel;
    QString gpuLabel;
    QString cpuFanLabel;
    QString gpuFanLabel;

    // 60 s history (canonical Celsius)
    QVector<DiagnosticsHistorySample> history;

    QJsonObject toJson() const;
};

// Builds a single consistent snapshot. `history` is the model's 60 s window.
DiagnosticsSnapshot buildDiagnostics(const TelemetrySnapshot &telemetry,
                                     const QVector<HistorySample> &history,
                                     const SystemInfo &systemInfo,
                                     const DiagnosticsContext &context);

// Pretty-printed JSON for the snapshot (Qt JSON API, never manual string concat).
QString diagnosticsJson(const DiagnosticsSnapshot &snapshot);

// Writes the pretty JSON to `path`. On failure returns false and sets *error.
bool saveDiagnostics(const DiagnosticsSnapshot &snapshot, const QString &path, QString *error);

// "excalibur-control-center-diagnostics-YYYYMMDD-HHMMSS.json"
QString diagnosticsFileName();
