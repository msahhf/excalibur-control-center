#pragma once

#include <QWidget>

class QVBoxLayout;

// Cooling page: per-module temperature + fan, plus 60 s history sparklines.
// Filled in Stage 3.
class CoolingPage : public QWidget
{
    Q_OBJECT

public:
    explicit CoolingPage(QWidget *parent = nullptr);

    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body = nullptr;
};
