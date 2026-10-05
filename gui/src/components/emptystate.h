#pragma once

#include <QWidget>
#include <QString>

// A designed "no telemetry" screen (Section 10): icon, title, guidance. Not a
// debug panel and offers no fake "load driver" button — the GUI stays
// unprivileged.
class EmptyState : public QWidget
{
    Q_OBJECT

public:
    explicit EmptyState(QWidget *parent = nullptr);

    void setMessage(const QString &title, const QString &body);

private:
    QString m_title = QStringLiteral("Telemetry Unavailable");
    QString m_body = QStringLiteral(
        "Load the EXCALIBUR hardware driver to enable hardware telemetry.");
};
