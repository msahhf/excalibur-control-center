#pragma once

#include <QVector>
#include <QWidget>

#include "app/systeminfo.h"
#include "app/telemetrymodel.h"

class QLabel;
class ActionButton;

// Device page: read-only machine and interface facts, grouped and quiet. This is
// where technical identifiers live (hwmon path, sensor channel names). Identity
// comes from DMI + kernel; anything unreadable shows "Not available". Includes a
// real clipboard "Copy diagnostics" action (which carries the raw file names and
// the raw internal connection state).
class DevicePage : public QWidget
{
    Q_OBJECT

public:
    explicit DevicePage(QWidget *parent = nullptr);

public slots:
    void setSnapshot(const TelemetrySnapshot &snapshot, const QVector<HistorySample> &);

private:
    void refreshStatus();
    QString diagnosticsText() const;
    void copyDiagnostics();

    SystemInfo m_info;
    TelemetrySnapshot m_snapshot;

    QLabel *m_model = nullptr;
    QLabel *m_board = nullptr;
    QLabel *m_bios = nullptr;
    QLabel *m_kernel = nullptr;

    QLabel *m_driver = nullptr;
    QLabel *m_interface = nullptr;
    QLabel *m_sensorStatus = nullptr;
    QLabel *m_hwmon = nullptr;
    QLabel *m_sensors = nullptr;
    QString m_stateText; // raw CONNECTED/DEGRADED/DISCONNECTED, diagnostics only

    ActionButton *m_copy = nullptr;
};
