#pragma once

#include <QWidget>

class QVBoxLayout;

// Hero page: system state + CPU/GPU hardware modules. Content is filled in
// Stage 2; this file owns the page layout root.
class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body = nullptr;
};
