#pragma once

#include <QWidget>

class QVBoxLayout;

// Device page: model / BIOS / kernel / driver / interface facts and a
// copy-diagnostics action. Filled in Stage 4.
class DevicePage : public QWidget
{
    Q_OBJECT

public:
    explicit DevicePage(QWidget *parent = nullptr);

    QVBoxLayout *body() const { return m_body; }

private:
    QVBoxLayout *m_body = nullptr;
};
