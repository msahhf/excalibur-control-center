#pragma once

#include <QMainWindow>

#include "hwmonclient.h"

class HardwareCard;
class StatusPanel;
class QLabel;
class QStackedWidget;

// Premium, read-only EXCALIBUR G870 control-center UI (Qt6 Widgets).
// Presentation only; telemetry comes exclusively from HwmonClient (hwmon/sysfs).
// No root, no WMI/ACPI/EC access, no hardware writes.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    void buildUi();
    QWidget *buildColumn();
    QWidget *buildHeader();
    QWidget *buildCenter();
    QWidget *buildStatusSection();
    QWidget *buildFooter();
    QWidget *buildEmptyState();

    HwmonClient m_client;

    // Header
    QLabel *m_statusDot = nullptr;
    QLabel *m_statusText = nullptr;
    QLabel *m_headerMeta = nullptr;

    // Telemetry
    HardwareCard *m_cpuCard = nullptr;
    HardwareCard *m_gpuCard = nullptr;
    QStackedWidget *m_center = nullptr;

    // Status
    StatusPanel *m_statusPanel = nullptr;
};
