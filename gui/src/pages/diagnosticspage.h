#pragma once

#include <QHash>
#include <QWidget>

#include "app/diagnostics.h"

class QLabel;
class ActionButton;

// Diagnostics viewer + exporter. Shows one consistent DiagnosticsSnapshot
// (application / system / telemetry / runtime / settings / sensors) built by the
// window, and exports that same snapshot as pretty JSON. Read-only: it never
// reads sysfs itself and never writes to the backend.
class DiagnosticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticsPage(QWidget *parent = nullptr);

public slots:
    void setSnapshot(const DiagnosticsSnapshot &snapshot);

private:
    void addRow(class QGridLayout *grid, int &row, const QString &caption, const QString &key);
    void exportDiagnostics();

    DiagnosticsSnapshot m_snapshot;
    QHash<QString, QLabel *> m_rows;
    ActionButton *m_export = nullptr;
    QLabel *m_exportStatus = nullptr;
};
