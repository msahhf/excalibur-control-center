#include "telemetrymodel.h"

#include <QElapsedTimer>
#include <QTimer>

namespace {

// Monotonic "now" in milliseconds, from a process-wide timer. Monotonic so that
// sample ages are unaffected by wall-clock/NTP jumps.
qint64 monotonicMs()
{
    static QElapsedTimer timer = []() {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer.elapsed();
}

// Two snapshots are "equal" when every value the UI can display is unchanged.
// Used to avoid repainting the whole UI every second while telemetry is steady.
bool sameSnapshot(const TelemetrySnapshot &a, const TelemetrySnapshot &b)
{
    return a.deviceFound == b.deviceFound && a.status == b.status
           && a.cpuTempValid == b.cpuTempValid && a.gpuTempValid == b.gpuTempValid
           && a.cpuFanValid == b.cpuFanValid && a.gpuFanValid == b.gpuFanValid
           && a.cpuTempC == b.cpuTempC && a.gpuTempC == b.gpuTempC
           && a.cpuFanRpm == b.cpuFanRpm && a.gpuFanRpm == b.gpuFanRpm
           && a.driverName == b.driverName && a.hwmonPath == b.hwmonPath
           && a.cpuLabel == b.cpuLabel && a.gpuLabel == b.gpuLabel
           && a.cpuFanLabel == b.cpuFanLabel && a.gpuFanLabel == b.gpuFanLabel;
}

} // namespace

TelemetryModel::TelemetryModel(QObject *parent)
    : QObject(parent),
      m_client(qEnvironmentVariable("EXCALIBUR_HWMON_ROOT", QStringLiteral("/sys/class/hwmon")))
{
}

void TelemetryModel::start(int intervalMs)
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &TelemetryModel::poll);
    }
    m_timer->start(intervalMs);
    poll(); // populate immediately so the first frame is real
}

void TelemetryModel::pollNow()
{
    poll();
}

void TelemetryModel::setInterval(int intervalMs)
{
    if (m_timer)
        m_timer->start(intervalMs);
    else
        start(intervalMs);
}

void TelemetryModel::poll()
{
    // Re-discover every tick so the UI reacts to the driver being loaded/removed
    // and to a hwmon index change, without a restart.
    m_client.discover();
    const ExcTelemetry t = m_client.read();
    const ExcStatus st = m_client.status(t);

    TelemetrySnapshot s;
    s.deviceFound = t.deviceFound;
    s.status = (st == ExcStatus::Connected)    ? AppState::Status::Connected
               : (st == ExcStatus::Degraded)   ? AppState::Status::Degraded
                                               : AppState::Status::Disconnected;
    s.cpuTempValid = t.cpuTempValid;
    s.gpuTempValid = t.gpuTempValid;
    s.cpuFanValid = t.cpuFanValid;
    s.gpuFanValid = t.gpuFanValid;
    s.cpuTempC = t.cpuTempC;
    s.gpuTempC = t.gpuTempC;
    s.cpuFanRpm = static_cast<int>(t.cpuFanRpm);
    s.gpuFanRpm = static_cast<int>(t.gpuFanRpm);
    s.hwmonPath = m_client.devicePath();
    if (t.deviceFound)
        s.driverName = QStringLiteral("excalibur_wmi");

    // Backend labels, sanitised once here (fallback keeps the product language).
    s.cpuLabel = AppState::displaySensorLabel(t.cpuTempLabel, QStringLiteral("CPU"));
    s.gpuLabel = AppState::displaySensorLabel(t.gpuTempLabel, QStringLiteral("GPU"));
    s.cpuFanLabel = AppState::displaySensorLabel(t.cpuFanLabel, QStringLiteral("CPU Fan"));
    s.gpuFanLabel = AppState::displaySensorLabel(t.gpuFanLabel, QStringLiteral("GPU Fan"));

    HistorySample h;
    h.timestampMs = monotonicMs();
    h.cpuTempValid = s.cpuTempValid;
    h.gpuTempValid = s.gpuTempValid;
    h.cpuFanValid = s.cpuFanValid;
    h.gpuFanValid = s.gpuFanValid;
    h.cpuTempC = s.cpuTempC;
    h.gpuTempC = s.gpuTempC;
    h.cpuFanRpm = s.cpuFanRpm;
    h.gpuFanRpm = s.gpuFanRpm;
    m_history.append(h);
    // Rolling 60-second window: trim by the real sample timestamp so the window
    // stays 60 s at every refresh interval (the poll timer also drives sampling),
    // with kHistorySize as a hard cap for the fastest 1 s refresh.
    const qint64 cutoff = h.timestampMs - static_cast<qint64>(kHistorySeconds) * 1000;
    while (!m_history.isEmpty() && m_history.first().timestampMs < cutoff)
        m_history.removeFirst();
    while (m_history.size() > kHistorySize)
        m_history.removeFirst();

    // Emit while the history window is still filling (so the charts build up),
    // then only when a displayed value/state actually changed. Polling and
    // history sampling stay at 1 Hz; this only avoids redundant repaints.
    const bool historyFilling = m_history.size() < kHistorySize;
    const bool changed = !m_hasEmitted || !sameSnapshot(s, m_snapshot);
    m_snapshot = s;
    if (!changed && !historyFilling)
        return;
    m_hasEmitted = true;
    emit updated(m_snapshot, m_history);
}
