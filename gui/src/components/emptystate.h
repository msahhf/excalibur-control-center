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
    // Defaults are assigned in the constructor (tr() cannot be used in a
    // member default initializer, plan section 4.1).
    QString m_title;
    QString m_body;
};
