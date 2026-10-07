#pragma once

#include <QWidget>

class AnimatedNumber;
class QLabel;

// A compact fan readout: a quiet "Fan" caption and a prominent RPM value.
// Missing fan data shows an em dash, never 0.
class FanStatus : public QWidget
{
    Q_OBJECT

public:
    explicit FanStatus(QWidget *parent = nullptr);

    void setFan(int rpm, bool valid);

private:
    QLabel *m_caption = nullptr;
    AnimatedNumber *m_rpm = nullptr;
    QLabel *m_unit = nullptr;
};
