#include "telemetrymodel.h"

#include <QTimer>

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

    HistorySample h;
    h.cpuTempValid = s.cpuTempValid;
    h.gpuTempValid = s.gpuTempValid;
    h.cpuFanValid = s.cpuFanValid;
    h.gpuFanValid = s.gpuFanValid;
    h.cpuTempC = s.cpuTempC;
    h.gpuTempC = s.gpuTempC;
    h.cpuFanRpm = s.cpuFanRpm;
    h.gpuFanRpm = s.gpuFanRpm;
    m_history.append(h);
    while (m_history.size() > kHistorySize)
        m_history.removeFirst();

    m_snapshot = s;
    emit updated(m_snapshot);
}
