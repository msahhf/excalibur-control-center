#pragma once

#include <QWidget>

#include "app/appstate.h"

class AnimatedNumber;
class FanStatus;
class QLabel;

// A hardware module panel (CPU or GPU). The temperature is the visually
// dominant element; a contextual band label and a thin band-colored thermal bar
// give the number meaning. Missing values show an em dash.
class HardwareModule : public QWidget
{
    Q_OBJECT

public:
    explicit HardwareModule(const QString &identity, QWidget *parent = nullptr);

    void setTemperature(double celsius, bool valid, AppState::Band band);
    void setFan(int rpm, bool valid);
    void setFanCaption(const QString &caption);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    class ThermalBar;

    QLabel *m_identity = nullptr;
    AnimatedNumber *m_temp = nullptr;
    QLabel *m_unit = nullptr;
    QLabel *m_band = nullptr;
    ThermalBar *m_bar = nullptr;
    FanStatus *m_fan = nullptr;
};
