#pragma once

#include <QString>
#include <QWidget>

class QLabel;

// Compact, product-styled system status panel. Values are supplied by the owner
// (MainWindow) as plain strings to keep this widget decoupled from the backend.
class StatusPanel : public QWidget
{
    Q_OBJECT

public:
    struct Info {
        QString hardware;
        QString telemetry;
        QString driver;
        QString interfaceName;
        QString secondary; // small muted metadata line under the rows
    };

    explicit StatusPanel(QWidget *parent = nullptr);

    void setInfo(const Info &info);

private:
    QLabel *m_hardware = nullptr;
    QLabel *m_telemetry = nullptr;
    QLabel *m_driver = nullptr;
    QLabel *m_interface = nullptr;
    QLabel *m_secondary = nullptr;
};
