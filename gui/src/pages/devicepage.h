#pragma once

#include <QVector>
#include <QWidget>

#include "app/systeminfo.h"
#include "app/telemetrymodel.h"

class QLabel;
class QPushButton;

// Device page: read-only machine and interface facts, grouped and quiet. This is
// where technical identifiers live (hwmon path, sensor file names, internal
// state). Identity comes from DMI + kernel; anything unreadable shows
// "Not available". Includes a real clipboard "Copy diagnostics" action.
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
    QLabel *m_state = nullptr;

    QPushButton *m_copy = nullptr;
};
